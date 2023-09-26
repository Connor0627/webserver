#include "../inc/wrap.h"
#include <sys/un.h>
#include <stddef.h>

#define SRV_SOCK "srv.sock"

void sys_err(const char *str)
{
    perror(str);
    exit(1);
}

int main()
{
    int listenfd;
    struct sockaddr_un serv_addr, clien_addr;
    char buf[1024];
    char str[INET_ADDRSTRLEN];
    int len;
    serv_addr.sun_family = AF_UNIX;
    strcpy(serv_addr.sun_path, SRV_SOCK);
    len = offsetof(struct sockaddr_un, sun_path) + strlen(SRV_SOCK);

    listenfd = Socket(AF_UNIX, SOCK_STREAM, 0);

    if (Bind(listenfd, (struct sockaddr *)&serv_addr, len) == -1)
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
        confd = Accept(listenfd, (struct sockaddr *)&clien_addr, &clien_len);
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
    close(listenfd);
    return 0;
}
