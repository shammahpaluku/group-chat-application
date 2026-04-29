#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "config.h"
#include "net_handler.h"

int nh_server_init(int port) {
    int server_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_fd < 0) {
        perror("socket() failed");
        return ERR_CONN;
    }
    
    // Set SO_REUSEADDR to allow immediate port reuse
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt() failed");
        close(server_fd);
        return ERR_CONN;
    }
    
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
    server_addr.sin_port = htons(port);
    
    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind() failed");
        close(server_fd);
        return ERR_CONN;
    }
    
    printf("Group Chat UDP Server listening on %s:%d\n", SERVER_IP, port);
    return server_fd;
}

int nh_send_to(int sock, const char *msg, struct sockaddr_in *client_addr) {
    char buf[CMD_BUF_LEN];
    snprintf(buf, sizeof(buf), "%s\n", msg);
    
    socklen_t addr_len = sizeof(struct sockaddr_in);
    int bytes_sent = sendto(sock, buf, strlen(buf), 0, 
                             (struct sockaddr*)client_addr, addr_len);
    if (bytes_sent <= 0) {
        return ERR_CONN;
    }
    
    return SUCCESS;
}

int nh_recv_from(int sock, char *buf, int buf_len, struct sockaddr_in *client_addr) {
    socklen_t addr_len = sizeof(struct sockaddr_in);
    
    // Receive entire datagram at once
    int bytes_read = recvfrom(sock, buf, buf_len - 1, 0, 
                             (struct sockaddr*)client_addr, &addr_len);
    if (bytes_read <= 0) {
        return ERR_CONN;
    }
    
    // Remove trailing newline if present
    if (bytes_read > 0 && buf[bytes_read - 1] == '\n') {
        buf[bytes_read - 1] = '\0';
    } else {
        buf[bytes_read] = '\0';
    }
    
    return SUCCESS;
}

void nh_close(int sock) {
    close(sock);
}
