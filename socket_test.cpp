#include <iostream>      // std::cout
#include <sys/socket.h>  // socket functions
#include <netinet/in.h>  // sockaddr_in
#include <unistd.h>      // read(), write(), close()
#include <cstring>
#include <string>
#include <thread>
#include <fcntl.h>       // fcntl(), O_NONBLOCK
#include <sys/epoll.h>   // epoll functions

int main() {

	// Create TCP socket
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	// Configure server address
	sockaddr_in addr;
	addr.sin_family = AF_INET;          // IPv4
	addr.sin_port = htons(8080);        // Port 8080
	addr.sin_addr.s_addr = INADDR_ANY;  // Accept from any IP
	
	// Bind socket to IP + port
	bind(sock_fd, (struct sockaddr*)&addr, sizeof(addr));

	// Start listening for connections
	listen(sock_fd, 5);

	// Make server socket non-blocking
	int flags = fcntl(sock_fd, F_GETFL, 0);
	fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK);

	// Create epoll instance
	int epfd = epoll_create1(0);

	// Event structure for server socket
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLET; // Watch readable events in edge-triggered mode
	ev.data.fd = sock_fd;          // Store server FD inside event

	// Add server socket to epoll
	epoll_ctl(epfd, EPOLL_CTL_ADD, sock_fd, &ev);

	while(true){

		// Array to store ready events
		struct epoll_event events[10];

		// Wait for events
		int nfds = epoll_wait(epfd, events, 10, -1);

		// Process all ready FDs
		for (int i = 0; i < nfds; i++){

			// Get ready FD
			int fd = events[i].data.fd;
		
			// Handle new client connection
			if (fd == sock_fd){

				// Accept new client
				int new_client = accept(sock_fd, NULL, NULL);

				std::cout << "New client: " << new_client << std::endl;

				// Make client socket non-blocking
				int flags = fcntl(new_client, F_GETFL, 0);
                fcntl(new_client, F_SETFL, flags | O_NONBLOCK);

				// Event structure for client
				struct epoll_event client_ev;

				// Watch client for readable data
				client_ev.events = EPOLLIN | EPOLLET;

				// Store client FD
				client_ev.data.fd = new_client;

				// Add client FD to epoll
				epoll_ctl(epfd, EPOLL_CTL_ADD, new_client, &client_ev);

			}

			// Handle existing client
			else {

				std::cout << "Client ready: " << fd << std::endl;

				// Store full request
				std::string request;

				// Temporary read buffer
				char buffer[1024];
				
				// Read until socket becomes empty
				while (true){

					// Read bytes from client
					int bytes = read(fd, buffer, sizeof(buffer));

					// Data received successfully
					if (bytes > 0){

						// Append received bytes to request
						request.append(buffer, bytes);

						// Stop if full HTTP headers received
						if (request.find("\r\n\r\n") != std::string::npos){
							break;
						}
					}

					// No more data available currently
					else if (bytes == -1){
						break;
					}

					// Client disconnected
					else {

						std::cout << "Client Disconnected: " << fd << std::endl;
						
						// Remove FD from epoll
						epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);

						// Close socket
						close(fd);

						break;
					
					}

					// Skip empty request
					if (request.empty()) {
   						 continue;
					}
				
				}	

				// Find end of first HTTP line
				size_t line_end = request.find("\r\n");

				// Extract request line
				std::string request_line = request.substr(0, line_end);
		
				// Remove extra carriage return
				if (!request_line.empty() && request_line.back() == '\r'){
					request_line.pop_back();
				}

				// Find spaces in request line
				size_t pos1 = request_line.find(" ");
				size_t pos2 = request_line.find(" ", pos1 + 1); 

				// Extract request path
				std::string path = request_line.substr(pos1+1, pos2-pos1-1);

				// Variables for HTTP response
				std::string response_body;
				std::string status_line;

				// Route: /
				if (path == "/"){
					response_body = "Hello World\n";
					status_line = "HTTP/1.1 200 OK\r\n";
				} 

				// Route: /hello
				else if (path == "/hello"){
					response_body = "Hello from /hello\n";
					status_line = "HTTP/1.1 200 OK\r\n";
				} 

				// Route: unknown path
				else {
					response_body = "404 Not Found\n";
					status_line = "HTTP/1.1 404 Not Found\r\n";
				}

				// Build HTTP response
				std::string response =
				status_line +
				"Content-Type: text/plain\r\n"
				"Content-Length: " + std::to_string(response_body.size()) + "\r\n"
				"\r\n" + 
				response_body;

				// Send response to client
				write(fd, response.c_str(), response.size());

				std::cout << "server closing FD: " << fd << std::endl;

				// Remove client FD from epoll
				epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);

				// Close client connection
				close(fd);

			}	
		}
	}

	return 0;
}