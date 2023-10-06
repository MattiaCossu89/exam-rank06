#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


void	func(int sockfd)
{
	char buff[1024 * 128];

	for (int i= 0; i < 100000; ++i)
		buff[i] = (i % 10000 == 0 ? '\n' : (i%5 == 0 ? ' ' : ('a' + (i % 26))));
	buff[1022] = '\n';
	buff[100000 - 1] = 0;
	write(sockfd, buff, strlen(buff));
	printf("%s\n", buff);

}
int main(int ac, char **av) {
	int sockfd, connfd, len;
	struct sockaddr_in servaddr, cli; 

	if (ac < 2) {
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}
	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		printf("socket creation failed...\n"); 
		exit(0); 
	} 
	else
		printf("Socket successfully created..\n"); 
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = inet_addr("127.0.0.1"); //127.0.0.1
	servaddr.sin_port = htons(atoi(av[1])); 
  
	// Binding newly created socket to given IP and verification 
	if ((connect(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) { 
		printf("socket bind failed...\n"); 
		exit(0); 
	} 
	else
		printf("Socket successfully binded..\n");
	func(sockfd);
	close(sockfd);
	return (0);
}
