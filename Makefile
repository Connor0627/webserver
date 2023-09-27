CC = gcc
CFLAGS = -Wall
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin
INCLUDES = inc
LDFLAGS = -levent  # 添加链接 libevent 库的标志

# 列出所有源文件
SRCS = $(wildcard $(SRC_DIR)/*.c)
SHARED_SRCS = $(wildcard $(SRC_DIR)/shared/*.c)

# 生成所有的目标可执行文件，将 .c 替换为 .out
TARGETS = $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%.out, $(SRCS))

all: $(TARGETS)

# 通配符规则：编译所有源文件并生成对应的可执行文件
$(BIN_DIR)/%.out: $(SRC_DIR)/%.c $(SHARED_SRCS)
	$(CC) $< $(SHARED_SRCS) -o $@ -I$(INCLUDE_DIR) $(CFLAGS) $(LDFLAGS)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
