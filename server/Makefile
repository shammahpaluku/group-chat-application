CC     = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -Iinclude

SRCS = src/file_io.c \
       src/utils.c \
       src/auth.c \
       src/groups.c \
       src/messaging.c \
       src/net_handler.c \
       src/server_main.c

SHARED_SRCS = src/file_io.c \
       src/utils.c \
       src/auth.c \
       src/groups.c \
       src/messaging.c \
       src/net_handler.c \
       src/shared_memory.c \
       src/server_main_threaded.c

TARGET = bin/chat_server
THREADED_TARGET = bin/chat_server_threaded

all: $(TARGET) $(THREADED_TARGET)

$(TARGET): $(SRCS)
	@mkdir -p bin
	@mkdir -p data
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

$(THREADED_TARGET): $(SHARED_SRCS)
	@mkdir -p bin
	@mkdir -p data
	$(CC) $(CFLAGS) -pthread $(SHARED_SRCS) -o $(THREADED_TARGET)

run: all
	./$(TARGET)

run-threaded: $(THREADED_TARGET)
	./$(THREADED_TARGET)

clean:
	rm -rf bin/
