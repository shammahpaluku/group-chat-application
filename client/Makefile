CC     = gcc
CFLAGS = -std=c11 -Wall -Wextra -g -Iinclude

SRCS = src/net_handler.c \
       src/client_main.c

TARGET = bin/chat_client

all: $(TARGET)

$(TARGET): $(SRCS)
	@mkdir -p bin
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

run: all
	./$(TARGET)

clean:
	rm -rf bin/
