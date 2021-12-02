#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>

char buffer[4096 * 100];
char str[4096 * 100 + 500];

int sockfd, max_fd;
unsigned long long int g_id;

typedef struct s_client {
	int fd;
	unsigned long long int id;
	struct s_client *next;
}		t_client;

void ft_print(int fd, char *str) {
	write(fd, str, strlen(str));
}

void clear_clients(t_client *list) {
	t_client *tmp;

	while (list) {
		tmp = list;
		list = (*list).next;
		close((*tmp).fd);
		free(tmp);
	}
}

int add_client(t_client **list, int fd) {
	t_client *new;
	t_client *tmp;

	if (list == NULL || !(new = malloc(sizeof(t_client)))) {
		ft_print(2, "Fatal error\n");
		if (list)
			clear_clients(*list);
		close(sockfd);
		exit(1);
	}
	(*new).fd = fd;
	(*new).id = g_id++;
	(*new).next = NULL;
	if (*list == NULL)
		*list = new;
	else {
		tmp = *list;
		while (tmp && (*tmp).next != NULL)
			tmp = (*tmp).next;
		(*tmp).next = new;
	}
	return ((*new).id);
}

int remove_client(t_client **list, int fd) {
	t_client *tmp;
	t_client *prev;
	int id = -1;

	if (list == NULL) {
		ft_print(2, "Fatal error\n");
		close(sockfd);
		exit(1);
	}
	tmp = *list;
	if (tmp && (*tmp).fd == fd) {
		*list = (*tmp).next;
		close((*tmp).fd);
		id = (*tmp).id;
		free(tmp);
	} else if (tmp) {
		while (tmp && (*tmp).fd != fd) {
			prev = tmp;
			tmp = (*tmp).next;
		}
		(*prev).next = (*tmp).next;
		id = (*tmp).id;
		close((*tmp).fd);
		free(tmp);
	}
	return (id);
}

void init_fd(fd_set *read, t_client *list) {
	FD_ZERO(read);

	max_fd = sockfd;
	while (list) {
		FD_SET((*list).fd, read);
		if ((*list).fd > max_fd)
			max_fd = (*list).fd;
		list = (*list).next;
	}
	FD_SET(sockfd, read);
}

void sendall(int fd, t_client *list) {
	while (list) {
		if ((*list).fd != fd)
			send((*list).fd, &str, strlen(str), 0);
		list = (*list).next;
	}
	for (size_t i = 0; i < strlen(str); i++)
		str[i] = 0;
}

void extract_msg(int fd, int id, t_client *clients, ssize_t size) {
	char msg [4096 * 100];	

	for (int i = 0, j = 0; i < size; i++, j++) {
		msg[j] = buffer[i];
		if (msg[j] == '\n') {
			msg[j] = '\0';
			sprintf(str, "client %d : %s\n", id, msg);
			sendall(fd, clients);
			j = -1;
		}
	} 

}

void clear_buffer() {
	for (size_t i = 0; i < strlen(buffer); i++)
		buffer[i] = 0;
}

int main(int ac, char **av) {
	int id, port, ret, connfd;
	socklen_t len;
	fd_set set_read;
	ssize_t size;
	struct sockaddr_in servaddr, cli; 

	t_client *clients;
	t_client *tmp;

	if (ac != 2) {
		ft_print(2, "Wrong number of arguments\n");
		exit(1);
	}
	port = atoi(av[1]);
	if (port <= 0) {
		ft_print(2, "Fatal error\n");
		exit(1);
	}

	// socket create and verification 
	sockfd = socket(AF_INET, SOCK_STREAM, 0); 
	if (sockfd == -1) { 
		ft_print(2, "Fatal error\n"); 
		exit(1); 
	} 
	bzero(&servaddr, sizeof(servaddr)); 

	// assign IP, PORT 
	servaddr.sin_family = AF_INET; 
	servaddr.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	servaddr.sin_port = htons(port); 
  
	// Binding newly created socket to given IP and verification 
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))) != 0) { 
		ft_print(2, "Fatal error\n");
		close(sockfd); 
		exit(1); 
	} 
	if (listen(sockfd, 10) != 0) {
		ft_print(2, "Fatal error\n");
		close(sockfd); 
		exit(1); 
	}
	len = sizeof(cli);
	g_id = 0;
	clients = NULL;
	tmp = NULL;
	
	while (1) {
		init_fd(&set_read, clients);
		ret = select(max_fd + 1, &set_read, NULL, NULL, NULL);
		if (ret > 0) {
			if (FD_ISSET(sockfd, &set_read)) {
				connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
				if (connfd > 0) {
					id = g_id;
					if (add_client(&clients, connfd) != -1) {
						sprintf(str, "server: client %d just arrived\n", id);
						sendall(connfd, clients);
						if (connfd > max_fd)
							max_fd = connfd;
					}
				}
			} else {
				tmp = clients;
				while (tmp) {
					connfd = (*tmp).fd;
					id = (*tmp).id;
					tmp = (*tmp).next;
					clear_buffer();
					if (FD_ISSET(connfd, &set_read)) {
						size = recv(connfd, &buffer, 4096 * 100, 0);
						if (size <= 0) {
							if (remove_client(&clients, connfd) != -1) {
								sprintf(str, "server: client %d just left\n", id);
								sendall(connfd, clients);
							}
						} else if (size > 0) {
							extract_msg(connfd, id, clients, size);
						}
					}
				}
			}
		}
	}
	if (clients)
		clear_clients(clients);
	if (sockfd)
		close(sockfd);
	return (0);
}

