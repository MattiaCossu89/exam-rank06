#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>

typedef struct	s_client {
	int				id;
	int				fd;
	struct s_client	*next;
}				t_client;

int					max_fd;
int					sockfd;
int					last_id;
char				*msg;
char				*sent;
char				buff[1025];
fd_set				set;
fd_set				reads;
t_client			*clist;
struct sockaddr_in	servaddr;

void	fail()
{
	t_client	*tmp;

	tmp = clist;
	FD_ZERO(&set);
	FD_ZERO(&reads);
	close(sockfd);
	while (tmp)
	{
		close(tmp->fd);
		tmp = tmp->next;
		free(clist);
		clist = tmp;
	}
	free(msg);
	free(sent);
	write(2, "Fatal error\n", 12);
	exit(1);
}

void	init(int port)
{
	last_id = 0;
	msg = 0;
	sent = 0;
	bzero(buff, 1025);
	FD_ZERO(&set);
	clist = 0;
	bzero(&servaddr, sizeof(servaddr));
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0)
		fail();
	fcntl(sockfd, F_SETFL, O_NONBLOCK);
	max_fd = sockfd;
	FD_SET(sockfd, &set);
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(2130706433);
	servaddr.sin_port = htons(port);
	if ((bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr))))
		fail();
	if (listen(sockfd, 10))
		fail();
}

void	send_all(char *msg, t_client *sender)
{
	t_client	*tmp;

	tmp = clist;
	while (tmp)
	{
		if (tmp != sender && msg &&
			send(tmp->fd, msg, strlen(msg), 0) < 0)
			fail();
		tmp = tmp->next;
	}
}

t_client *last_client()
{
	t_client	*tmp;

	tmp = clist;
	if (!clist)
		return (0);
	while (tmp->next)
		tmp = tmp->next;
	return (tmp);
}

int extract_message(char **buf, char **msg)
{
	char	*newbuf;
	int	i;

	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}

char *str_join(char *buf, char *add)
{
	char	*newbuf;
	int		len;

	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}

void	add_client()
{
	t_client	*new_cli;

	new_cli = calloc(1, sizeof(t_client));
	if (!new_cli)
		fail();
	new_cli->id = last_id++;
	new_cli->fd = accept(sockfd, 0, 0);
	if (new_cli->fd < 0)
	{
		free(new_cli);
		fail();
	}
	FD_SET(new_cli->fd, &set);
	if (new_cli->fd > max_fd)
		max_fd = new_cli->fd;
	if (clist)
	{	
		sprintf(buff, "server: client %d just arrived\n", new_cli->id);
		send_all(buff, new_cli);
		last_client()->next = new_cli;
		bzero(buff, strlen(buff));
	}
	else
		clist = new_cli;
}

void	remove_client(int fd)
{
	t_client	*tmp;
	t_client	*prev;

	tmp = clist;
	prev = clist;
	while (tmp)
	{
		if (tmp->fd == fd)
		{
			if (tmp == clist)
				clist = clist->next;
			else
				prev->next = tmp->next;
			break ;
		}
		prev = tmp;
		tmp = tmp->next;
	}
	if (!tmp)
		return ;
	sprintf(buff, "server: client %d just left\n", tmp->id);
	send_all(buff, tmp);
	bzero(buff, strlen(buff));
	FD_CLR(tmp->fd, &set);
	free(tmp);
}

void	manage_message(t_client *sender)
{
	int		res;

	res = extract_message(&sent, &msg);
	sprintf(buff, "client %d: ", sender->id);
	while (res > 0)
	{
		send_all(buff, sender);
		send_all(msg, sender);
		free(msg);
		res = extract_message(&sent, &msg);
	}
	if (res < 0)
		fail();
	free(msg);
	free(sent);
	msg = 0;
	sent = 0;
	bzero(buff, strlen(buff));
}

void	receive_message(t_client *sender)
{
	int		rec;
	int		i;
	char	t;

	rec = recv(sender->fd, &t, 1, 0);
	if (rec == 0)
	{
		remove_client(sender->fd);
		return ;
	}
	i = 0;
	bzero(buff, 1025);
	while (rec > 0)
	{
		buff[i++] = t;
		if (i == 1024)
		{
			sent = str_join(sent, buff);
			bzero(buff, rec);
			i = 0;
		}
		rec = recv(sender->fd, &t, 1, 0);
	}
	if (rec < 0)
		fail();
	sent = str_join(sent, buff);
	bzero(buff, strlen(buff));
	manage_message(sender);
}

int main(int ac, char **av)
{
	int			sel;
	t_client	*tmp;

	if (ac != 2)
	{
		write(2, "Wrong number of arguments\n", 26);
		exit(1);
	}
	init(atoi(av[1]));
	while ( 42 )
	{
		reads = set;
		sel = select(max_fd + 1, &reads, 0, 0, 0);
		if (sel < 0)
			continue ;
		if (FD_ISSET(sockfd, &reads))
			add_client();
		tmp = clist;
		while (tmp)
		{
			if (FD_ISSET(tmp->fd, &reads))
				receive_message(tmp);
			tmp = tmp->next;
		}
	}
}

