#include "../inc/wrap.h"

#define SERV_PORT 8888

void sys_err(const char *str)
{
    perror(str);
    exit(1);
}

int main()
{
    int listenfd = Socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in serv_addr, clien_addr;
    char buf[1024];
    char str[INET_ADDRSTRLEN];
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERV_PORT);
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
    {
        sys_err("Failed to bind()!");
    }
    if (listen(listenfd, 128) == -1)
    {
        sys_err("Failed to listen()!");
    }
    int confd;
    while (1)
    {
        socklen_t clien_len = sizeof(clien_addr);
        confd = accept(listenfd, (struct sockaddr *)&clien_addr, &clien_len);
        if (confd == -1)
        {
            sys_err("Failed to connect!");
        }
        while (1)
        {
            int n = read(confd, buf, 1024);
            write(STDOUT_FILENO, buf, n);

            if (n == -1)
            {
                close(confd);
                sys_err("Failed to read()!");
            }
            printf("Received from IP %s at PORT %d\n",
                   inet_ntop(AF_INET, &clien_addr.sin_addr.s_addr, str, sizeof(str)),
                   ntohs(clien_addr.sin_port));
            for (size_t i = 0; i < n; i++)
            {
                buf[i] = toupper(buf[i]);
            }
            if (write(confd, buf, n) == -1)
            {
                sys_err("Failed to write()");
            }
        }
        close(confd);
    }
    return 0;
}
