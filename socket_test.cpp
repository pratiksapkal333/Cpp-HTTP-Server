#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int main() {
	int sock_fd = socket(AF_INET, SOCK_STREAM, 0);

	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(8080);
	addr.sin_addr.s_addr = INADDR_ANY;
	
	bind(sock_fd, (struct sockaddr*)&addr, sizeof(addr));
	
	listen(sock_fd, 5);

	std::cout << "Bound to port 8080" << std::endl;

	sleep(100);	
	return 0;
}
