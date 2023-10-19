CC = gcc
CFLAGS = -Wall

# 列出所有源文件
SRCS = $(wildcard ./*.c)

# 生成所有的目标可执行文件，将 .c 替换为 .out
TARGETS = $(patsubst ./%.c, ./%.out, $(SRCS))

all: $(TARGETS)

# 通配符规则：编译所有源文件并生成对应的可执行文件
$(TARGETS): ./%.out : ./%.c
	$(CC) $< -o $@ $(CFLAGS) 

clean:
	rm -f $(TARGETS)