#include "../inc/wrap.h"

#define SERV_PORT 8888

void sys_err(const char *str)
{
    perror(str);
    exit(1);
}

int main()
{
    int listenfd = Socket(AF_INET, SOCK_DGRAM, 0);
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
    while (1)
    {
        socklen_t clien_len = sizeof(clien_addr);
        while (1)
        {
            int n = recvfrom(listenfd, buf, sizeof(buf), 0, (struct sockaddr *)&clien_addr, &clien_len);
            write(STDOUT_FILENO, buf, n);
            printf("Received from IP %s at PORT %d\n",
                   inet_ntop(AF_INET, &clien_addr.sin_addr.s_addr, str, sizeof(str)),
                   ntohs(clien_addr.sin_port));
            for (size_t i = 0; i < n; i++)
            {
                buf[i] = toupper(buf[i]);
            }
            sendto(listenfd, buf, n, 0, (struct sockaddr *)&clien_addr, sizeof(clien_addr));
        }
    }
    close(listenfd);
    return 0;
}
