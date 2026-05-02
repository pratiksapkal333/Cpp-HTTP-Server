#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h> // close() is inside this lib
#include <cstring>
#include <string>
#include <thread>
#include <fcntl.h> //file control (used for unblocking)
#include <sys/epoll.h>

int main() {

	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8080);
	addr.sin_addr.s_addr = INADDR_ANY;
	
	bind(sock_fd, (struct sockaddr*)&addr, sizeof(addr));
	listen(sock_fd, 5);

	// make server unblocking
	int flags = fcntl(sock_fd, F_GETFL, 0);
				fcntl(sock_fd, F_SETFL, flags | O_NONBLOCK);

	// create epoll instance
	int epfd = epoll_create1(0);

	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLET;
	ev.data.fd = sock_fd;

	epoll_ctl(epfd, EPOLL_CTL_ADD, sock_fd, &ev);


	while(true){  // server keeps running forever

		//using epoll_wait()
		struct epoll_event events[10];

		int nfds = epoll_wait(epfd, events, 10, -1);

		for (int i = 0; i < nfds; i++){

			int fd = events[i].data.fd;
		
			// NEW CLIENT
			if (fd == sock_fd){

				int new_client = accept(sock_fd, NULL, NULL);

				std::cout << "New client: " << new_client << std::endl;

				//Makes client non-blocking
				int flags = fcntl(new_client, F_GETFL, 0);
                fcntl(new_client, F_SETFL, flags | O_NONBLOCK);

				// Add client to epoll
				struct epoll_event client_ev;
				client_ev.events = EPOLLIN | EPOLLET;
				client_ev.data.fd = new_client;

				epoll_ctl(epfd, EPOLL_CTL_ADD, new_client, &client_ev);

			}
			// EXISTING CLIENT
			else {
				std::cout << "Client ready: " << fd << std::endl;

				std::string request;
				char buffer[1024];
				
				while (true){
					int bytes = read(fd, buffer, sizeof(buffer));

					if (bytes > 0){
						request.append(buffer, bytes);

						//new condition
						if (request.find("\r\n\r\n") != std::string::npos){
							break; //full request received
						}
					}
					else if (bytes == -1){
						//no more data (non-blocking)
						break;
					}
					else {
						//client closed connections
						std::cout << "Client Disconnected: " << fd << std::endl;
						
						epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
						close(fd);
						break;
					
					}
					if (request.empty()) {
   						 continue;
					}
				
				}	
				//std::string request(buffer);

				size_t line_end = request.find("\r\n");
				std::string request_line = request.substr(0, line_end);
		
				if (!request_line.empty() && request_line.back() == '\r'){
				request_line.pop_back();
				}

				size_t pos1 = request_line.find(" ");
				size_t pos2 = request_line.find(" ", pos1 + 1); 

				std::string path = request_line.substr(pos1+1, pos2-pos1-1);//estracts - /

				std::string response_body; // stores the message part
				std::string status_line; //shows correctness of client request


				// if statement compares the requested path
				if (path == "/"){
					response_body = "Hello World\n";
					status_line = "HTTP/1.1 200 OK\r\n";
				} 
				else if (path == "/hello"){
					response_body = "Hello from /hello\n";
					status_line = "HTTP/1.1 200 OK\r\n";
				} 
				else {
					response_body = "404 Not Found\n";
					status_line = "HTTP/1.1 404 Not Found\r\n";
				}

				std::string response=
				status_line +
				"Content-Type: text/plain\r\n"
				"Content-Length: " + std::to_string(response_body.size()) + "\r\n"
				"\r\n" + 
				response_body;  //response_body.size() dynamically calc length 
				
				
				write(fd, response.c_str(), response.size());//c_str() converts string to C-style char* needed for write() // size() - total bytes to send
				std::cout << "server closing FD: " << fd << std::endl;
				epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
				close(fd); //closes connection with client , frees file decriptor , prevent resource leak

				}	
			}	
		}
return 0;
}