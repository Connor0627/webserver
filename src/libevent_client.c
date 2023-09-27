#include "../inc/wrap.h"
#include <event2/event.h>
#include <event2/listener.h>
#include <event2/bufferevent.h>

#define SERV_PORT 8888

void read_cb(struct bufferevent *bev, void *cbarg)
{
    char buf[1024];
    int n = bufferevent_read(bev, buf, sizeof(buf));
    write(STDOUT_FILENO, buf, n);
}

void write_cb(struct bufferevent *bev, void *cbarg)
{
    write(STDOUT_FILENO, "Write Success!???", 17);
}

void event_cb(struct bufferevent *bev, short events, void* ctx)
{
    if (events & BEV_EVENT_EOF) {
        printf("EOF error!\n");
    } else if(events & BEV_EVENT_ERROR) {
        printf("ERROR!\n");
    } else if(events & BEV_EVENT_CONNECTED) {
        printf("已经连接服务器...\n");
        return;
    }
    fflush(stdout);
    bufferevent_free(bev);
}

void read_terminal(evutil_socket_t fd, short what, void* v) {
    char buf[1024];
    int n = read(fd, buf, sizeof(buf));
    bufferevent_write((struct bufferevent*)v, buf, n + 1);
}

int main() 
{
    struct event_base *base = event_base_new();
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in srv_addr;
    memset(&srv_addr, 0, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(SERV_PORT);
    inet_pton(AF_INET, "127.0.0.1", &srv_addr.sin_addr.s_addr);

    struct bufferevent *bev = bufferevent_socket_new(base, sockfd, BEV_OPT_CLOSE_ON_FREE);
    bufferevent_socket_connect(bev, (struct sockaddr*)&srv_addr, sizeof(srv_addr));
    bufferevent_setcb(bev, read_cb, write_cb, event_cb, NULL);
    bufferevent_enable(bev, EV_READ);

    struct event *ev;
    ev = event_new(base, STDIN_FILENO, EV_READ | EV_PERSIST, read_terminal, bev);
    event_add(ev, NULL);
    
    event_base_dispatch(base);

    event_base_free(base);
    bufferevent_free(bev);
    event_free(ev);
}