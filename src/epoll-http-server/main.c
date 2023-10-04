#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "epoll_server.h"

int main(int argc, const char* argv[])
{
    if(argc < 3)
    {
        printf("eg: ./a.out port path\n");
        exit(1);
    }

    int port = atoi(argv[1]);

    int ret = chdir(argv[2]);
    if(ret == -1)
    {
        perror("chdir error");
        exit(1);
    }
    
    epoll_run(port);

    return 0;
}
