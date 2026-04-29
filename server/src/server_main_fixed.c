#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include "config.h"
#include "net_handler.h"
#include "file_io.h"
#include "utils.h"
#include "auth.h"
#include "groups.h"
#include "messaging.h"

static int s_quit = 0;

static const char *err_string(int code) {
    switch (code) {
        case ERR_NOT_FOUND:  return "ERR_NOT_FOUND";
        case ERR_DUPLICATE:  return "ERR_DUPLICATE";
        case ERR_AUTH:       return "ERR_AUTH";
        case ERR_FULL:       return "ERR_FULL";
        case ERR_FILE:       return "ERR_FILE";
        default:             return "ERR_UNKNOWN";
    }
}

static void wire_to_str(char *str) { utils_replace_char(str, '_', ' '); }
static void str_to_wire(char *str) { utils_replace_char(str, ' ', '_'); }

// Command handlers
static void cmd_register(int sock, struct sockaddr_in *client_addr, char *args) {
    char *username = strtok(args, " ");
    char *display_name = strtok(NULL, " ");
    char *password = strtok(NULL, " ");
    
    if (!username || !display_name || !password) {
        nh_send_to(sock, "ERR_AUTH", client_addr);
        return;
    }
    
    wire_to_str(username);
    wire_to_str(display_name);
    
    int result = auth_register(username, display_name, password);
    if (result > 0) {
        char response[64];
        snprintf(response, sizeof(response), "OK %d", result);
        nh_send_to(sock, response, client_addr);
    } else {
        nh_send_to(sock, result == ERR_DUPLICATE ? "ERR_DUPLICATE" : "ERR_AUTH", client_addr);
    }
}

static void cmd_login(int sock, struct sockaddr_in *client_addr, char *args) {
    char *username = strtok(args, " ");
    char *password = strtok(NULL, " ");
    
    if (!username || !password) {
        nh_send_to(sock, "ERR_AUTH", client_addr);
        return;
    }
    
    wire_to_str(username);
    
    int result = auth_login(username, password);
    if (result > 0) {
        const char *display_name = auth_get_display_name();
        char wire_name[MAX_NAME_LEN];
        strncpy(wire_name, display_name, sizeof(wire_name) - 1);
        wire_name[sizeof(wire_name) - 1] = '\0';
        str_to_wire(wire_name);
        
        char response[128];
        snprintf(response, sizeof(response), "OK %d %s", result, wire_name);
        nh_send_to(sock, response, client_addr);
    } else {
        nh_send_to(sock, err_string(result), client_addr);
    }
}

static void cmd_logout(int sock, struct sockaddr_in *client_addr, char *args) {
    (void)args;
    auth_logout();
    nh_send_to(sock, "OK", client_addr);
}

static void dispatch_command(int sock, struct sockaddr_in *client_addr, char *cmd_buf) {
    char cmd_copy[CMD_BUF_LEN];
    strncpy(cmd_copy, cmd_buf, CMD_BUF_LEN - 1);
    cmd_copy[CMD_BUF_LEN - 1] = '\0';
    
    char *verb = strtok(cmd_copy, " ");
    if (!verb) {
        nh_send_to(sock, "ERR_UNKNOWN", client_addr);
        return;
    }
    
    char *args = NULL;
    if (strlen(verb) < strlen(cmd_buf)) {
        args = cmd_buf + strlen(verb) + 1;
    }
    
    if (strcmp(verb, "REGISTER") == 0) {
        cmd_register(sock, client_addr, args);
    } else if (strcmp(verb, "LOGIN") == 0) {
        cmd_login(sock, client_addr, args);
    } else if (strcmp(verb, "LOGOUT") == 0) {
        cmd_logout(sock, client_addr, args);
    } else {
        nh_send_to(sock, "ERR_UNKNOWN", client_addr);
    }
}

int main(void) {
    // Load data from files
    int result = fio_load_all();
    if (result != SUCCESS) {
        printf("Failed to load data files. Exiting.\n");
        return 1;
    }
    
    int server_fd = nh_server_init(SERVER_PORT);
    if (server_fd == ERR_CONN) {
        printf("Failed to start UDP server.\n");
        return 1;
    }
    
    printf("Group Chat UDP Server ready. Waiting for datagrams...\n");
    
    // Main UDP server loop
    while (!s_quit) {
        char cmd_buf[CMD_BUF_LEN];
        struct sockaddr_in client_addr;
        
        int r = nh_recv_from(server_fd, cmd_buf, CMD_BUF_LEN, &client_addr);
        if (r == ERR_CONN) {
            printf("Receive failed. Waiting...\n");
            continue;
        }
        
        printf("CMD [%s] from %s:%d\n", cmd_buf, 
               inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        
        dispatch_command(server_fd, &client_addr, cmd_buf);
    }
    
    nh_close(server_fd);
    return 0;
}
