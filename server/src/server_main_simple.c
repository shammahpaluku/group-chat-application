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
#include "session_manager.h"
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
static void cmd_register(int sock, struct sockaddr_in *client_addr, char *args);
static void cmd_login(int sock, struct sockaddr_in *client_addr, char *args);
static void cmd_logout(int sock, struct sockaddr_in *client_addr, char *args);
static void cmd_create_group(int sock, struct sockaddr_in *client_addr, char *args);
static void cmd_list_my_groups(int sock, struct sockaddr_in *client_addr, char *args);
static void cmd_browse_groups(int sock, struct sockaddr_in *client_addr, char *args);
static void cmd_join_group(int sock, struct sockaddr_in *client_addr, char *args);
static void cmd_leave_group(int sock, struct sockaddr_in *client_addr, char *args);
static void cmd_list_users(int sock, struct sockaddr_in *client_addr, char *args);
static void cmd_send_message(int sock, struct sockaddr_in *client_addr, char *args);
static void cmd_delete_message(int sock, struct sockaddr_in *client_addr, char *args);

void dispatch_command(int sock, struct sockaddr_in *client_addr, char *cmd_buf);

int main(void) {
    // Initialize session management
    session_init();
    
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
        
        printf("CMD [%s] from %s:%d, user_id=%d\n", 
               cmd_buf, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), 
               session_get_user_id(&client_addr));
        
        dispatch_command(sock, &client_addr, cmd_buf);
    }
    
    nh_close(server_fd);
    return 0;
}
