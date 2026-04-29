#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h> // close() is inside this lib
#include <cstring>

int main() {
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8080);
	addr.sin_addr.s_addr = INADDR_ANY;
	
	bind(sock_fd, (struct sockaddr*)&addr, sizeof(addr));
	listen(sock_fd, 5);

	while(true){  // server keeps running forever
		std::cout << "Waiting for client..." << std::endl;

		int client_fd = accept(sock_fd, NULL, NULL); // waits until client connects


		std::cout << "Client connected! FD: " << client_fd << std::endl;

		char buffer[1024] = {0}; // Creat memory to store the incoming data

		read(client_fd, buffer, 1024); //Reads data from client socket and stores it in buffer we just created

		std::cout << "Request received:\n" << buffer << std::endl; // prints the HTTP request

		const char* response = 
			"HTTP/1.1 200 OK\r|n"
			"Content-Type; test/plain\r\n"
			"Content-Length: 12\r\n"
			"\r\n"
			"Hello World\n";

		write(client_fd, response, strlen(response));
		close(client_fd); //closes connection with client , frees file decriptor , prevent resource leak

	}

	

		
	return 0;
}
