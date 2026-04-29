#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "config.h"
#include "net_handler.h"

int nh_client_init(const char *server_ip, int port, struct sockaddr_in *server_addr) {
    int client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_fd < 0) {
        perror("socket() failed");
        return ERR_CONN;
    }
    
    // Store server address for sendto
    server_addr->sin_family = AF_INET;
    server_addr->sin_addr.s_addr = inet_addr(server_ip);
    server_addr->sin_port = htons(port);
    
    printf("UDP Client initialized, server: %s:%d\n", server_ip, port);
    return client_fd;
}

int nh_send_to(int sock, const char *msg, struct sockaddr_in *server_addr) {
    char buf[CMD_BUF_LEN];
    snprintf(buf, sizeof(buf), "%s\n", msg);
    
    socklen_t addr_len = sizeof(struct sockaddr_in);
    int bytes_sent = sendto(sock, buf, strlen(buf), 0, 
                             (struct sockaddr*)server_addr, addr_len);
    if (bytes_sent <= 0) {
        return ERR_CONN;
    }
    
    return SUCCESS;
}

int nh_recv_from(int sock, char *buf, int buf_len) {
    struct sockaddr_in from_addr;
    socklen_t addr_len = sizeof(from_addr);
    
    // Receive entire datagram at once
    int bytes_read = recvfrom(sock, buf, buf_len - 1, 0, 
                             (struct sockaddr*)&from_addr, &addr_len);
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
