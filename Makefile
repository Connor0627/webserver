CC = gcc
CFLAGS = -Wall
SRC_DIR = src
BUILD_DIR = build
BIN_DIR = bin

# 列出所有源文件
SRCS = $(wildcard $(SRC_DIR)/*.c)

# 生成所有的目标可执行文件，将 .c 替换为 .out
TARGETS = $(patsubst $(SRC_DIR)/%.c, $(BIN_DIR)/%.out, $(SRCS))

all: $(TARGETS)

# 通配符规则：编译所有源文件并生成对应的可执行文件
$(BIN_DIR)/%.out: $(SRC_DIR)/%.c
	$(CC) $< -o $@ $(CFLAGS)

clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR)
