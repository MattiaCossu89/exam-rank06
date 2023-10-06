#include<stdlib.h>
#include<stdio.h>
#include<unistd.h>
#include<sys/socket.h>
#include<string.h>
#include<arpa/inet.h>
#include<fcntl.h>
//
typedef struct	s_clist {
	struct s_clist *next;
	int fd;
	int id;
}				t_clist;

int max_fd;
int last_id;
int sockfd;
char buf[1024];
char *msg;
t_clist *clist;
fd_set set;
struct sockaddr_in sockaddr;

void fail() {
	t_clist  *it;

	it = clist;
	while (it) {
		close(it->fd);
		clist = it->next;
		free(it);
		it = clist;
	}
	free(msg);
	close(sockfd);
	write(2, "Fatal error\n", 12);
	exit(1);
}

void init(int port) {
	last_id = 0;
	msg = 0;
	bzero(buf, 1024);
	FD_ZERO(&set);
	clist = 0;
	bzero(&sockaddr, sizeof(sockaddr));
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		fail();
	fcntl(sockfd, F_SETFL, O_NONBLOCK);
	max_fd = sockfd;
	FD_SET(sockfd, &set);
	sockaddr.sin_family = AF_INET;
	sockaddr.sin_addr.s_addr = htonl(2130706433);
	sockaddr.sin_port = htons(port);
	if ((bind(sockfd, (const struct sockaddr *)&sockaddr, sizeof(sockaddr))))
		fail();
	if (listen(sockfd, 10))
		fail();
}
/*
char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc((len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}
*/
char *str_join(char *bufx, char *add) {
	int		len;
	int		i;

	if (!bufx)
		len = 0;
	else
		len = strlen(bufx);
	if (!add)
		return bufx;
	bufx = realloc(bufx, len + strlen(add) + 1);
	if (!bufx)
		fail();
	i = -1;
	while (add[++i])
		bufx[len + i] = add[i];
	bufx[len + i] = 0;
	return bufx;	
}
/* char *str_join(char *msg, char *buf) {
	int buf_len;
	int msg_len;

	if (!buf)
		return msg;
	buf_len = strlen(buf);
	if (msg)
		msg_len = strlen(msg);
	else
		msg_len = 0;
	msg = realloc(msg, msg_len + buf_len + 1);
	if (!msg)
		fail();
	for (int i = 0; i < buf_len; ++i)
		msg[msg_len + i] = buf[i];
	return msg;
} */

/* int extract_message(char **msg, char **buff) {
	int i;

	i = 0;
	*msg = 0;
	if (!*buff)
		return 0;
	while ((*buff)[i]) {
		if ((*buff)[i] == '\n') {
			*msg = *buff;
			*buff = calloc(strlen(*buff + i + 1), 1);
			if (!*buff)
				fail();
			strcpy(*buff, *msg + i + 1);
			(*msg)[i + 1] = 0;
			return 1;
		}
		++i;
	}
	*msg = *buff;
	return 0;
} */
void send_all(char *msgx, t_clist *sender) {
	if (!msgx) {
		printf("No GOOD\n");
		return ;
	}
	for (t_clist *it = clist; it; it = it->next) {
		if  (it != sender)
		{
			send(it->fd, msgx, strlen(msgx), 0);
		}
	}
}

int extract_message(char **bufx, char **msg)
{
	char	*newbufx;
	int		i;

	*msg = 0;
	if (*bufx == 0)
		return (0);
	i = 0;
	while ((*bufx)[i])
	{
		if ((*bufx)[i] == '\n')
		{
			newbufx = calloc(1, (strlen(*bufx + i + 1) + 1));
			if (newbufx == 0)
				return (-1);

			strcpy(newbufx, *bufx + i + 1);
			*msg = *bufx;
			(*msg)[i + 1] = 0;
			*bufx = newbufx;
			return (1);
		}
		i++;
	}
	return (0);
}
/**/
void elaborate_message(t_clist *sender) {
	char	*line;
	int		ret;

	if (!msg)
		return ;
	sprintf(buf, "client %d: ", sender->id);
	ret = extract_message(&msg, &line);
	while (ret > 0) {
		send_all(buf, sender);
		send_all(line, sender);
		free(line);
		ret = extract_message(&msg, &line);
	}
	//free(msg);
	msg = 0;
	bzero(buf, strlen(buf));
}

void remove_client(t_clist *client) {
	t_clist *it, *prev = 0;

	it = clist;
	if (clist == client)
		clist = clist->next;
	else
		while (it && it != client) {
			prev = it;
			it = it->next;
		}
	if (!prev || !it)
		return ;
	prev->next = it->next;
	close(client->id);
	FD_CLR(client->fd, &set);
	sprintf(buf, "server: client %d just left\n", client->id);
	send_all(buf, client);
	bzero(buf, strlen(buf));
	free(client);
}

void receive_message(t_clist *sender) {
	int received;

	if (!sender)
		return ;
	received = recv(sender->fd, buf, 1023, 0);
	if (!received) {
		remove_client(sender);
		return ;
	}
	while (received > 0) {
		buf[received] = 0;
		msg = str_join(msg, buf);
		received = recv(sender->fd, buf, 1023, 0);
	}
	printf("client %d says:\n%s\n", sender->id, msg);
	elaborate_message(sender);
}

t_clist *last_client() {
	t_clist *it;

	it = clist;
	if (!it)
		return (0);
	while (it->next)
		it = it->next;
	return (it);
}

void add_client() {
	t_clist *last;
	t_clist *mew;

	last = last_client();
	mew = calloc(1, sizeof(t_clist));
	if (!mew)
		fail();
	mew->fd = accept(sockfd, 0, 0);
	if (mew->fd < 0) {
		free(mew);
		fail();
	}
	FD_SET(mew->fd, &set);
	if (mew->fd > max_fd)
		max_fd = mew->fd;
	mew->id = ++last_id;
	sprintf(buf, "server: client %d just arrived\n", mew->id);
	send_all(buf, mew);
	bzero(buf, strlen(buf));
	if (!last)
		clist = mew;
	else
		last->next = mew;
}

int main(int ac, char **av) {
	fd_set	to_read;
	int		sel;

	if (ac != 2) {
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}
	init(atoi(av[1]));
	while ( 42 ) {
		FD_ZERO(&to_read);
		to_read = set;
		sel = select(max_fd + 1, &to_read, 0, 0, 0);
		if (sel <= 0)
			continue ;
		if (FD_ISSET(sockfd, &to_read)) {
			add_client();
			printf("client %d just arrived\n", last_client()->id);
		}
		for (t_clist *it = clist; it; it = it->next) {
			if (FD_ISSET(it->fd, &to_read)) {
				printf("message sent by: client %d\n", it->id);
				receive_message(it);
			}
		}
	}
}
