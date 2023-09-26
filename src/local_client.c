#include "../inc/wrap.h"
#include <sys/un.h>
#include <stddef.h>

#define CLT_SOCK "clt.sock"
#define SRV_SOCK "srv.sock"

void sys_err(const char *str)
{
    perror(str);
    exit(1);
}

int main()
{
    int sockfd, n, len;
    char buf[1024];
    char str[1024];
    char name[1024];
    struct sockaddr_un srv_addr, clt_addr;

    clt_addr.sun_family = AF_UNIX;
    strcpy(clt_addr.sun_path, CLT_SOCK);
    sockfd = Socket(AF_UNIX, SOCK_STREAM, 0);
    len = offsetof(struct sockaddr_un, sun_path) + strlen(CLT_SOCK);
    unlink(CLT_SOCK);
    Bind(sockfd, (struct sockaddr*)&clt_addr, len);

    srv_addr.sun_family = AF_UNIX;
    strcpy(srv_addr.sun_path, SRV_SOCK);
    len = offsetof(struct sockaddr_un, sun_path) + strlen(SRV_SOCK);
    if (connect(sockfd, (struct sockaddr *)&srv_addr, len) == -1)
    {
        sys_err("Failed to connect()!");
    }

    while (1)
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
