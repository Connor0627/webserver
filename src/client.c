#include "../inc/wrap.h"

#define SERV_PORT 8888

void sys_err(const char *str)
{
    perror(str);
    exit(1);
}

int main()
{
    int sockfd, n;
    char buf[1024];
    char str[1024];
    struct sockaddr_in srv_addr;
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(SERV_PORT);
    inet_pton(AF_INET, "10.211.55.3", &srv_addr.sin_addr.s_addr);
    sockfd = Socket(AF_INET, SOCK_STREAM, 0);
    if (connect(sockfd, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) == -1)
    {
        sys_err("Failed to connect()!");
    }
    n = 10;
    while (n--)
    {
        scanf("%s", str);
        write(sockfd, str, strlen(str));
        int ret = read(sockfd, buf, sizeof(buf));
        write(STDOUT_FILENO, buf, ret);
        printf("\n");
        sleep(1);
    }
    close(sockfd);
    return 0;
}
