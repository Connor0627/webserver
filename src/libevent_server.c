#include "../inc/wrap.h"
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>

#define SERV_PORT 8888

void read_cb(struct bufferevent *bev, void *cbarg)
{
    char buf[1024];
    char str[10];
    int n = bufferevent_read(bev, buf, sizeof(buf));
    write(STDOUT_FILENO, buf, n);
    putchar('\n');
    strcpy(buf, "I have gotten the data, size is ");
    sprintf(str, "%d", n);
    strcat(buf, str);
    bufferevent_write(bev, buf, strlen(buf));
}

void write_cb(struct bufferevent *bev, void *cbarg)
{
    write(STDOUT_FILENO, "Write Success!", 14);
}

void event_cb(struct bufferevent *bev, short events, void* ctx)
{
    if (events & BEV_EVENT_EOF) {
        printf("EOF error!\n");
    } else if(events & BEV_EVENT_ERROR) {
        printf("ERROR!\n");
    }
    fflush(stdout);
    bufferevent_free(bev);
}

void listen_cb(struct evconnlistener *listen, 
                evutil_socket_t fd, 
                struct sockaddr *srv_addr, 
                int socklen, 
                void * arg)
{
    struct event_base *base = (struct event_base *)arg;
    struct bufferevent *bev = bufferevent_socket_new(base, fd, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_setcb(bev, read_cb, write_cb, event_cb, NULL);
    bufferevent_enable(bev, EV_READ);
}

int main() 
{
    struct event_base *base = event_base_new();

    struct sockaddr_in srv_addr;
    memset(&srv_addr, 0, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(SERV_PORT);

    struct evconnlistener *listen = evconnlistener_new_bind(base, listen_cb, base, 
        LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE, 32, (struct sockaddr*)&srv_addr, sizeof(srv_addr));
    event_base_dispatch(base);

    event_base_free(base);
    evconnlistener_free(listen);
}