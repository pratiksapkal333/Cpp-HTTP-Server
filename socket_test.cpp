#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <thread>
#include <fcntl.h>
#include <sys/epoll.h>
#include <unordered_map>
#include <errno.h> // errno, EAGAIN, EWOULDBLOCK
#include <functional>
#include <fstream>

//applying fstream
std::string serve_file(const std::string& filename) {
	std::ifstream file(filename);
	if(!file.is_open()){
		return "404 File Not Found\n";
	}
	std::string content(
		(std::istreambuf_iterator<char>(file)),
		std::istreambuf_iterator<char>() 
	);
	return content;
}

std::string handle_css(const std::string&) {
	return serve_file("style.css");
}
std::string handle_index(const std::string&) {
	return serve_file("index.html");
}
std::string handle_home(const std::string&) {
	return "Hello World\n";
}
std::string handle_hello(const std::string&) {
	return "Hello from /hello\n";
}
std::string handle_login(const std::string& body) {
	return "POST received: " + body + "\n";
}

using Handler = std::function<std::string(const std::string&)>;

std::string get_content_type(const std::string& path) {
	if (path.find(".html") != std::string::npos){
		return "text/html";
	}
	if (path.find(".css") != std::string::npos){
		return "text/css";
	}
	if (path.find(".js") != std::string::npos) {
        return "application/javascript";
    }
    return "text/plain";
}	

int main() {

	// Create TCP socket
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	// Configure server address
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8080);
	addr.sin_addr.s_addr = INADDR_ANY;

	// Bind socket to IP + port
	bind(sock_fd, (struct sockaddr*)&addr, sizeof(addr));

	// Start listening
	listen(sock_fd, 5);

	// Make server socket non-blocking
	int flags = fcntl(sock_fd, F_GETFL, 0);
	fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK);

	// Create epoll instance
	int epfd = epoll_create1(0);

	// Stores request data for each client FD
	std::unordered_map<int, std::string> client_buffers;
	std::unordered_map<std::string, Handler> routes;

	routes["GET /"] = handle_home;
	routes["GET /hello"] = handle_hello;
	routes["POST /login"] = handle_login;
	routes["GET /index.html"] = handle_index;
	routes["GET /style.css"] = handle_css;

	// Configure server event
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = sock_fd;

	// Add server socket to epoll
	epoll_ctl(epfd, EPOLL_CTL_ADD, sock_fd, &ev);

	while (true) {

		// Stores triggered events
		struct epoll_event events[10];

		// Wait for events
		int nfds = epoll_wait(epfd, events, 10, -1);

		// Process all triggered events
		for (int i = 0; i < nfds; i++) {

			int fd = events[i].data.fd;

			// New incoming client connection
			if (fd == sock_fd) {

				// IMPORTANT:
				// Accept ALL pending clients in EPOLLET mode
				while (true) {

					int new_client = accept(sock_fd, NULL, NULL);

					// No more clients waiting
					if (new_client == -1) {

						if (errno == EAGAIN || errno == EWOULDBLOCK) {
							break;
						}

						perror("accept");
						break;
					}

					std::cout << "New client: " << new_client << std::endl;

					// Make client non-blocking
					int flags = fcntl(new_client, F_GETFL, 0);
					fcntl(new_client, F_SETFL, flags | O_NONBLOCK);

					// Configure client event
					struct epoll_event client_ev;
					client_ev.events = EPOLLIN | EPOLLET;
					client_ev.data.fd = new_client;

					// Add client to epoll
					epoll_ctl(epfd, EPOLL_CTL_ADD, new_client, &client_ev);
				}
			}

			// Existing client sent data
			else {

				std::cout << "Client ready: " << fd << std::endl;

				char buffer[1024];

				// Read ALL available socket data
				while (true) {

					int bytes = read(fd, buffer, sizeof(buffer));

					// Data received
					if (bytes > 0) {

						// Store request persistently
						client_buffers[fd].append(buffer, bytes);

						// Full HTTP headers received
						if (client_buffers[fd].find("\r\n\r\n") != std::string::npos) {
							break;
						}
					}

					// No more data currently available
					else if (bytes == -1) {

						if (errno == EAGAIN || errno == EWOULDBLOCK) {
							break;
						}

						// Real socket error
						perror("read");

						epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
						close(fd);
						client_buffers.erase(fd);

						break;
					}

					// Client disconnected
					else {

						std::cout << "Client Disconnected: " << fd << std::endl;

						epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
						close(fd);
						client_buffers.erase(fd);

						break;
					}
				}

				// Skip if no request data exists
				if (client_buffers.find(fd) == client_buffers.end()) {
					continue;
				}

				// Reference to this client's request buffer
				std::string& request = client_buffers[fd];
				std::cout << request << std::endl;
				size_t header_end = request.find("\r\n\r\n");
				// Skip incomplete requests
				if (header_end == std::string::npos) {
					continue;
				}
				size_t content_length = 0;
				size_t cl_pos = request.find("Content-Length:");
				
				//parse the number (read body according to content lenght)
				if (cl_pos != std::string::npos){
					size_t value_start = cl_pos + 16;
					size_t value_end = request.find("\r\n", value_start);
					std::string len_str = request.substr(value_start, value_end - value_start);
					content_length = std::stoi(len_str);
				}
				size_t total_expected = (header_end + 4) + content_length;
				if (request.size() < total_expected){
					continue;
				}
				//Extract body
				std::string body = request.substr(header_end + 4, content_length);
				std::cout << "BODY: " << body << std::endl;

				// Extract first HTTP line
				size_t line_end = request.find("\r\n");
				std::string request_line = request.substr(0, line_end);

				// Remove extra \r
				if (!request_line.empty() && request_line.back() == '\r') {
					request_line.pop_back();
				}

				// Extract method/path
				size_t pos1 = request_line.find(" ");
				size_t pos2 = request_line.find(" ", pos1 + 1);

				std::string method = request_line.substr(0, pos1);
				std::string path = request_line.substr(pos1 + 1, pos2 - pos1 - 1);

				std::string route_key = method + " " + path;
				std::cout << "METHOD: " << method << std::endl;
				// HTTP response variables
				std::string response_body;
				std::string status_line;
				std::string content_type = "text/plain";

				// Route handling

				if (method == "GET" &&
				path.find("/user/") == 0){
					std::string username = path.substr(6);
					response_body = "Hello " + username + "\n";
					status_line = "HTTP/1.1 200 OK\r\n";
					content_type = "text/plain";
				}

				else if (routes.find(route_key) != routes.end()){
					response_body = routes[route_key](body);
					status_line = "HTTP/1.1 200 OK\r\n";
					content_type = get_content_type(path);
				}
				else {
					response_body = "404 Not Found\n";
					status_line = "HTTP/1.1 404 Not Found\r\n";
				}
				// Build HTTP response
				std::string response =
					status_line +
					"Content-Type: " + content_type + "\r\n"
					"Content-Length: " + std::to_string(response_body.size()) + "\r\n"
					"\r\n" +
					response_body;

				// Send response
				write(fd, response.c_str(), response.size());

				std::cout << "Response sent to FD: " << fd << std::endl;

				// Clear processed request for keep-alive
				client_buffers[fd].clear();
			}
		}
	}

	return 0;
}