myArgs = -Wall -g
target = epoll_server
CC = gcc
src = main.c epoll_server.c
obj = $(src:.c=.o)

$(target): $(obj)
	$(CC) $^ -o $@ $(myArgs)

%.o: %.c
	$(CC) -c $< -o $@ $(myArgs)

clean:
	-rm -f $(obj) $(target)

.PHONY: clean
