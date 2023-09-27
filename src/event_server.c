#include "../inc/wrap.h"
#include <fcntl.h> 
#include <sys/stat.h>
#include <event.h>  
#include <event2/util.h>  


void readcb(evutil_socket_t fd, short what, void* v) {
    char buf[1024];
    int n = read(fd, buf, sizeof(buf));
    write(STDOUT_FILENO, buf, n);
    sleep(1);
}

int main() 
{
    int fd;
    unlink("event.fifo");
    mkfifo("event.fifo", 0664);
    fd = open("event.fifo", O_RDONLY | O_NONBLOCK);

    struct event_base *base;
    base = event_base_new();

    struct event *ev;
    ev = event_new(base, (evutil_socket_t)fd, EV_READ | EV_PERSIST, readcb, NULL);
    event_add(ev, NULL);

    event_base_dispatch(base);

    event_free(ev);
    event_base_free(base);
    close(fd);
}