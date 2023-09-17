#include <signal.h>
#include <strings.h>
#include <sys/wait.h>
#include <pthread.h>
#include "../inc/wrap.h"

#define SERV_PORT 8888

struct s_info {
    struct sockaddr_in cliaddr;
    int cfd;
};

void * do_work(void * arg) {
    int n, i;
    struct s_info ts = *(struct s_info*)arg;
    char buf[1024];
    char str[INET_ADDRSTRLEN];
    while (1)
    {
        n = Read(ts.cfd, buf, sizeof(buf));
        if (n == 0)
        {
            break;
        }
        printf("Received from Client %s at Port %d: ",
                    inet_ntop(AF_INET, &ts.cliaddr.sin_addr.s_addr, str, sizeof(str)),
                    ntohs(ts.cliaddr.sin_port));
        fflush(stdout);
        Write(STDOUT_FILENO, buf, n);
        putchar('\n');
        for (i = 0; i < n; i++)
        {
            buf[i] = toupper(buf[i]);
        }
        Write(ts.cfd, buf, n);
    }
    Close(ts.cfd);
}

int main()
{
    int lfd, cfd;
    struct sockaddr_in saddr, caddr;
    socklen_t len;

    pthread_t tid;
    struct s_info ts[256];
    int i = 0;

    lfd = Socket(AF_INET, SOCK_STREAM, 0);

    bzero(&saddr, sizeof(saddr));
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(SERV_PORT);
    saddr.sin_addr.s_addr = htonl(INADDR_ANY);

    Bind(lfd, (struct sockaddr *)&saddr, sizeof(saddr));
    Listen(lfd, 128);

    printf("Server starts......");

    while (1)
    {
        len = sizeof(caddr);
        cfd = Accept(lfd, (struct sockaddr *)&caddr, &len);
        ts[i].cliaddr = caddr;
        ts[i].cfd = cfd;
        pthread_create(&tid, NULL, do_work, (void *)&ts[i]);
        ++i;
    }

    return 0;
    
}