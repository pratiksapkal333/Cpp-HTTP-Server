#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h> // close() is inside this lib
#include <cstring>
#include <string>
#include <thread>

int main() {

	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8080);
	addr.sin_addr.s_addr = INADDR_ANY;
	
	bind(sock_fd, (struct sockaddr*)&addr, sizeof(addr));
	listen(sock_fd, 5);

	while(true){  // server keeps running forever

		// Connecting to client
		std::cout << "Waiting for client..." << std::endl;
		int client_fd = accept(sock_fd, NULL, NULL); // waits until client connects

		std::thread t([client_fd](){
				
			std::cout << "Handling client FD: " << client_fd << std::endl;


			char buffer[1024] = {0}; // Create memory to store the incoming data


			// reads and extracts the method and path from buffer data
			read(client_fd, buffer, 1024); //Reads data from client socket and stores it in buffer we just created
			// Data - GET / HTTP/1.1
			std::string request(buffer);
			//extract first line only

			size_t line_end = request.find("\r\n");
			std::string request_line = request.substr(0, line_end);

			if (!request_line.empty() && request_line.back() == '\r'){
				request_line.pop_back();
			} //removes extra characters like \r 
			
			size_t pos1 = request_line.find(" "); //finds first space
			size_t pos2 = request_line.find(" ", pos1 + 1); //finds second space

			std::string method = request_line.substr(0, pos1); // estracts - GET
			std::string path = request_line.substr(pos1+1, pos2-pos1-1);//estracts - /


			// prints the method and path and HTTP request (Was only for error solving)
			// std::cout << "Request Line: [" << request_line << "]" << std::endl;
			// std::cout << "Method: " << method << std::endl;
			// std::cout << "path" << path << std::endl;
			// std::cout << "Request received:\n" << buffer << std::endl; // prints the HTTP request


			// Response
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


			//Sends response to client
			std::cout << "Path: [" << path << "]" << std::endl;
			std::cout << "Length: " << path.length() << std::endl;

			std::string response=
				status_line +
				"Content-Type: text/plain\r\n"
				"Content-Length: " + std::to_string(response_body.size()) + "\r\n"
				"\r\n" + 
				response_body;  //response_body.size() dynamically calc length 

			sleep(10);
			write(client_fd, response.c_str(), response.size());//c_str() converts string to C-style char* needed for write() // size() - total bytes to send
			close(client_fd); //closes connection with client , frees file decriptor , prevent resource leak

		});
		t.detach();

		
		
	}

	return 0;
}
