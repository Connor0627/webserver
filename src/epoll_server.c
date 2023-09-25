#include <sys/epoll.h>
#include "../inc/wrap.h"

#define MAXLINE 80
#define SERV_PORT 8888
#define FILE_MAX 1024

int main(int argc, char *argv[])
{
	int i, listenfd, connfd;
	int nready, epfd; 
	ssize_t n;
	char buf[MAXLINE];
	char str[INET_ADDRSTRLEN]; 	
	socklen_t cliaddr_len;
	struct sockaddr_in cliaddr, servaddr;
    struct epoll_event tep, ep[FILE_MAX];

	listenfd = Socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(SERV_PORT);

    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, (void *)&opt, sizeof (opt));

	Bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
	Listen(listenfd, 20); 

    epfd = epoll_create(FILE_MAX);
    tep.events = EPOLLIN;
    tep.data.fd = listenfd;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd, &tep);
    while (1) {
        nready = epoll_wait(epfd, ep, EPOLLIN, -1);
        for (i = 0; i < nready; ++i)
        {
            int fd = ep[i].data.fd;
            if (fd == listenfd) {
                cliaddr_len = sizeof(cliaddr);
                connfd = Accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddr_len);
                tep.events = EPOLLIN;
                tep.data.fd = connfd;
                epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &tep);
            } else {
                n = read(fd, buf, sizeof(buf));
                if (n == 0) {
                    close(fd);
                    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
                } else if (n > 0) {
                    for (i = 0; i < n; i++)
                    {
                        buf[i] = toupper(buf[i]);
                    }
                    Write(fd, buf, n);
                    Write(STDOUT_FILENO, buf, n);
                }
            }
        }
        
    }
    

	return 0;
}
