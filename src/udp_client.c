#include "../inc/wrap.h"

#define SERV_PORT 8888

void sys_err(const char *str)
{
    perror(str);
    exit(1);
}

int main()
{
    int sockfd;
    char buf[1024];
    char str[1024];
    struct sockaddr_in srv_addr;
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(SERV_PORT);
    inet_pton(AF_INET, "127.0.0.1", &srv_addr.sin_addr.s_addr);
    sockfd = Socket(AF_INET, SOCK_DGRAM, 0);
    if (connect(sockfd, (struct sockaddr *)&srv_addr, sizeof(srv_addr)) == -1)
    {
        sys_err("Failed to connect()!");
    }
    while (1)
    {
        socklen_t srv_len = sizeof(srv_addr);
        scanf("%s", str);
        sendto(sockfd, str, strlen(str), 0, (struct sockaddr *)&srv_addr, srv_len);
        int ret = recvfrom(sockfd, buf, sizeof(buf), 0, NULL, 0);
        write(STDOUT_FILENO, buf, ret);
        printf("\n");
    }
    close(sockfd);
    return 0;
}
