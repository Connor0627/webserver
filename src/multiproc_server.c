#include <signal.h>
#include <sys/wait.h>
#include "../inc/wrap.h"

#define SERV_PORT 8888

void catch_child(int signum)
{
    while (waitpid(0, NULL, WNOHANG) > 0) ;
}

int main()
{
    int lfd, cfd;
    struct sockaddr_in saddr, caddr;
    socklen_t len;
    int i, n;
    char buf[1024];
    char str[INET_ADDRSTRLEN];
    pid_t pid;
    struct sigaction newact;

    newact.sa_handler = catch_child;
    sigemptyset(&newact.sa_mask);
    newact.sa_flags = 0;
    sigaction(SIGCHLD, &newact, NULL);

    lfd = Socket(AF_INET, SOCK_STREAM, 0);
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
        pid = fork();
        if (pid == 0) {
            Close(lfd);
            while (1)
            {
                n = Read(cfd, buf, sizeof(buf));
                if (n == 0)
                {
                    printf("Client %s at Port %d stopped the connection.",
                        inet_ntop(AF_INET, &caddr.sin_addr.s_addr, str, sizeof(str)),
                        ntohs(caddr.sin_port));
                    fflush(stdout);
                    break;
                }
                printf("Received from Client %s at Port %d: ",
                    inet_ntop(AF_INET, &caddr.sin_addr.s_addr, str, sizeof(str)),
                    ntohs(caddr.sin_port));
                fflush(stdout);
                for (i = 0; i < n; i++)
                {
                    buf[i] = toupper(buf[i]);
                }
                Write(cfd, buf, n);
                Write(STDOUT_FILENO, buf, n);
            }
            Close(cfd);
            return 0;
        } else if (pid > 0) {
            Close(cfd);
        } else {
            perr_exit("fork error");
        }
        
    }
    
}