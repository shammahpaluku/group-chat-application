CC     = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -Iinclude
LDFLAGS = -lrt

SRCS = src/file_io.c \
       src/utils.c \
       src/auth.c \
       src/groups.c \
       src/messaging.c \
       src/net_handler.c \
       src/server_main.c

TARGET = bin/chat_server

all: $(TARGET)

$(TARGET): $(SRCS)
	@mkdir -p bin
	@mkdir -p data
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET) $(LDFLAGS)

run: all
	./$(TARGET)

clean:
	rm -rf bin/
