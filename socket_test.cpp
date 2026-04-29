#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

int main() {
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8080);
	addr.sin_addr.s_addr = INADDR_ANY;
	
	bind(sock_fd, (struct sockaddr*)&addr, sizeof(addr));
	listen(sock_fd, 5);

	std::cout << "Waiting for client..." << std::endl;

	int client_fd = accept(sock_fd, NULL, NULL); // waits until client connects


	std::cout << "Client connected! FD: " << client_fd << std::endl;

	char buffer[1024] = {0}; // Creat memory to store the incoming data

	read(client_fd, buffer, 1024); //Reads data from client socket and stores it in buffer we just created

	const char* response = 
		"HTTP/1.1 200 OK\r|n"
		"Content-Type; test/plain\r\n"
		"Content-Length: 12\r\n"
		"\r\n"
		"Hello World\n";

	write(client_fd, response, strlen(response));


	std::cout << "Request received:\n" << buffer << std::endl; // prints the HTTP request

	sleep(60);	
	return 0;
}
