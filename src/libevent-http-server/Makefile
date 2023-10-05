myArgs = -Wall -g -levent
target = libevent_server
CC = gcc
src = main.c libevent_http.c
obj = $(src:.c=.o)

$(target): $(obj)
	$(CC) $^ -o $@ $(myArgs)

%.o: %.c
	$(CC) -c $< -o $@ $(myArgs)

clean:
	-rm -f $(obj) $(target)

.PHONY: clean
