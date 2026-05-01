#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h> // close() is inside this lib
#include <cstring>
#include <string>
#include <thread>
#include <fcntl.h> //file control (used for unblocking)
#include <sys/select.h>
#include <vector>

int main() {

	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8080);
	addr.sin_addr.s_addr = INADDR_ANY;
	
	bind(sock_fd, (struct sockaddr*)&addr, sizeof(addr));
	listen(sock_fd, 5);

	std::vector<int> clients;

	while(true){  // server keeps running forever

		//add server socket
		fd_set readfds;
		FD_ZERO(&readfds);

		
		FD_SET(sock_fd, &readfds);
		int max_fd = sock_fd;

		//add all client sockets
		for (int fd : clients){
			FD_SET(fd, &readfds);
			if (fd > max_fd) max_fd = fd;
		}// watches all these sockets

		select(max_fd + 1, &readfds, NULL, NULL, NULL);

		if (FD_ISSET(sock_fd, &readfds)){
			int new_client = accept(sock_fd, NULL, NULL);

			std::cout << "New client: " << new_client << std::endl;

			clients.push_back(new_client);

			int flags = fcntl(new_client, F_GETFL, 0);
			fcntl(new_client, F_SETFL, flags | O_NONBLOCK);
		}
		
		for (int i = 0; i < clients.size(); i++) {

   			int fd = clients[i];

			if (FD_ISSET(fd, &readfds)) {
				std::cout << "Client ready: " << fd << std::endl;

				char buffer[1024] = {0}; 
				int bytes = read(fd, buffer, 1024);

				if (bytes <= 0) {
					close(fd);
					std::cout << "Client Disconnected: " << fd << std::endl;
					
					clients.erase(clients.begin() + i);
					i--;
					continue;
				}

				std::string request(buffer);

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
				close(fd); //closes connection with client , frees file decriptor , prevent resource leak

			}
		}
		
	}

return 0;
}
