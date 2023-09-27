#include "../inc/wrap.h"
#include <fcntl.h> 
#include <sys/stat.h>
#include <event.h>  
#include <event2/util.h>  


void writecb(evutil_socket_t fd, short what, void* v) {
    char buf[1024] = "hello world!\n";
    write(fd, buf, strlen(buf));
    sleep(1);
}

int main() 
{
    int fd;
    fd = open("event.fifo", O_WRONLY | O_NONBLOCK);

    struct event_base *base;
    base = event_base_new();

    struct event *ev;
    ev = event_new(base, (evutil_socket_t)fd, EV_WRITE | EV_PERSIST, writecb, NULL);
    event_add(ev, NULL);

    event_base_dispatch(base);

    event_free(ev);
    event_base_free(base);
    close(fd);
}