#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <mqueue.h>
#include "config.h"
#include "file_io.h"
#include "utils.h"
#include "auth.h"
#include "groups.h"
#include "messaging.h"
#include "net_handler.h"

static int s_quit = 0;

static const char *err_string(int code) {
    switch (code) {
        case ERR_NOT_FOUND:  return "ERR_NOT_FOUND";
        case ERR_DUPLICATE:  return "ERR_DUPLICATE";
        case ERR_AUTH:       return "ERR_AUTH";
        case ERR_FULL:       return "ERR_FULL";
        case ERR_PERMISSION: return "ERR_PERMISSION";
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
        nh_send_to(sock, err_string(result), client_addr);
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

static void cmd_logout(int sock, struct sockaddr_in *client_addr) {
    auth_logout();
    nh_send_to(sock, "OK", client_addr);
}

static void cmd_create_group(int sock, struct sockaddr_in *client_addr, char *args) {
    char *group_name = strtok(args, " ");
    char *description = args + strlen(group_name) + 1;
    
    if (!group_name) {
        nh_send_to(sock, "ERR_AUTH", client_addr);
        return;
    }
    
    if (!description || utils_is_empty(description)) {
        description = "No description";
    }
    
    wire_to_str(group_name);
    
    int result = grp_create(group_name, description);
    if (result > 0) {
        char response[64];
        snprintf(response, sizeof(response), "OK %d", result);
        nh_send_to(sock, response, client_addr);
    } else {
        nh_send_to(sock, err_string(result), client_addr);
    }
}

static void cmd_join_group(int sock, struct sockaddr_in *client_addr, char *args) {
    int group_id = atoi(args);
    int result = grp_join(group_id);
    
    if (result == SUCCESS) {
        nh_send_to(sock, "OK", client_addr);
    } else {
        nh_send_to(sock, err_string(result), client_addr);
    }
}

static void cmd_leave_group(int sock, struct sockaddr_in *client_addr, char *args) {
    int group_id = atoi(args);
    int result = grp_leave(group_id);
    
    if (result == SUCCESS) {
        nh_send_to(sock, "OK", client_addr);
    } else {
        nh_send_to(sock, err_string(result), client_addr);
    }
}

static void cmd_search_groups(int sock, struct sockaddr_in *client_addr, char *args) {
    char *keyword = args;
    utils_trim(keyword);
    
    int result_ids[MAX_GROUPS];
    int found = grp_search(keyword, result_ids, MAX_GROUPS);
    
    if (found == 0) {
        nh_send_to(sock, "ERR_NOT_FOUND", client_addr);
        return;
    }
    
    for (int i = 0; i < found; i++) {
        int idx = grp_find_by_id(result_ids[i]);
        Group *g = &g_groups[idx];
        
        char wire_name[MAX_NAME_LEN];
        char wire_desc[MAX_DESC_LEN];
        
        strncpy(wire_name, g->group_name, sizeof(wire_name) - 1);
        wire_name[sizeof(wire_name) - 1] = '\0';
        str_to_wire(wire_name);
        
        strncpy(wire_desc, g->description, sizeof(wire_desc) - 1);
        wire_desc[sizeof(wire_desc) - 1] = '\0';
        str_to_wire(wire_desc);
        
        char line[256];
        snprintf(line, sizeof(line), "%d %s %d %s", 
                g->group_id, wire_name, g->member_count, wire_desc);
        nh_send_to(sock, line, client_addr);
    }
    
    nh_send_to(sock, "END", client_addr);
}

static void cmd_list_my_groups(int sock, struct sockaddr_in *client_addr) {
    if (!auth_is_logged_in()) {
        nh_send_to(sock, "ERR_AUTH", client_addr);
        return;
    }
    
    int user_id = auth_get_user_id();
    int group_ids[MAX_GROUPS];
    int found = grp_get_member_groups(user_id, group_ids, MAX_GROUPS);
    
    if (found == 0) {
        nh_send_to(sock, "ERR_EMPTY", client_addr);
        return;
    }
    
    for (int i = 0; i < found; i++) {
        int idx = grp_find_by_id(group_ids[i]);
        Group *g = &g_groups[idx];
        
        char wire_name[MAX_NAME_LEN];
        char wire_desc[MAX_DESC_LEN];
        
        strncpy(wire_name, g->group_name, sizeof(wire_name) - 1);
        wire_name[sizeof(wire_name) - 1] = '\0';
        str_to_wire(wire_name);
        
        strncpy(wire_desc, g->description, sizeof(wire_desc) - 1);
        wire_desc[sizeof(wire_desc) - 1] = '\0';
        str_to_wire(wire_desc);
        
        char line[256];
        snprintf(line, sizeof(line), "%d %s %d %s", 
                g->group_id, wire_name, g->member_count, wire_desc);
        nh_send_to(sock, line, client_addr);
    }
    
    nh_send_to(sock, "END", client_addr);
}

static void cmd_list_all_groups(int sock, struct sockaddr_in *client_addr) {
    int found = 0;
    
    for (int i = 0; i < g_group_count; i++) {
        if (!g_groups[i].is_active) continue;
        
        Group *g = &g_groups[i];
        char wire_name[MAX_NAME_LEN];
        char wire_desc[MAX_DESC_LEN];
        
        strncpy(wire_name, g->group_name, sizeof(wire_name) - 1);
        wire_name[sizeof(wire_name) - 1] = '\0';
        str_to_wire(wire_name);
        
        strncpy(wire_desc, g->description, sizeof(wire_desc) - 1);
        wire_desc[sizeof(wire_desc) - 1] = '\0';
        str_to_wire(wire_desc);
        
        char line[256];
        snprintf(line, sizeof(line), "%d %s %d %s", 
                g->group_id, wire_name, g->member_count, wire_desc);
        nh_send_to(sock, line, client_addr);
        found++;
    }
    
    if (found == 0) {
        nh_send_to(sock, "ERR_EMPTY", client_addr);
        return;
    }
    
    nh_send_to(sock, "END", client_addr);
}

static void cmd_list_members(int sock, struct sockaddr_in *client_addr, char *args) {
    int group_id = atoi(args);
    int idx = grp_find_by_id(group_id);
    
    if (idx == ERR_NOT_FOUND) {
        nh_send_to(sock, "ERR_NOT_FOUND", client_addr);
        return;
    }
    
    Group *g = &g_groups[idx];
    
    for (int i = 0; i < g->member_count; i++) {
        int uidx = auth_find_user_by_id(g->member_ids[i]);
        char line[128];
        
        if (uidx >= 0) {
            char wire_name[MAX_NAME_LEN];
            strncpy(wire_name, g_users[uidx].display_name, sizeof(wire_name) - 1);
            wire_name[sizeof(wire_name) - 1] = '\0';
            str_to_wire(wire_name);
            
            snprintf(line, sizeof(line), "%d %s %s", 
                    g_users[uidx].user_id, wire_name, g_users[uidx].username);
        } else {
            snprintf(line, sizeof(line), "%d Unknown unknown", g->member_ids[i]);
        }
        
        nh_send_to(sock, line, client_addr);
    }
    
    nh_send_to(sock, "END", client_addr);
}

static void cmd_send_msg(int sock, struct sockaddr_in *client_addr, char *args) {
    char *group_id_str = strtok(args, " ");
    char *reply_to_str = strtok(NULL, " ");
    char *content = strtok(NULL, "");
    
    if (!group_id_str || !reply_to_str || !content) {
        nh_send_to(sock, "ERR_UNKNOWN", client_addr);
        return;
    }
    
    int group_id = atoi(group_id_str);
    int reply_to = atoi(reply_to_str);
    
    int result = msg_send(group_id, content, reply_to);
    if (result > 0) {
        char response[64];
        snprintf(response, sizeof(response), "OK %d", result);
        nh_send_to(sock, response, client_addr);
    } else {
        nh_send_to(sock, err_string(result), client_addr);
    }
}

static void cmd_view_msgs(int sock, struct sockaddr_in *client_addr, char *args) {
    int group_id = atoi(args);
    
    int idx = grp_find_by_id(group_id);
    if (idx == ERR_NOT_FOUND) {
        nh_send_to(sock, "ERR_NOT_FOUND", client_addr);
        return;
    }
    
    if (!auth_is_logged_in() || !grp_is_member(idx, auth_get_user_id())) {
        nh_send_to(sock, "ERR_PERMISSION", client_addr);
        return;
    }
    
    int top_indices[MAX_MESSAGES];
    int top_count = msg_get_for_group(group_id, top_indices, MAX_MESSAGES);
    
    if (top_count == 0) {
        nh_send_to(sock, "ERR_EMPTY", client_addr);
        return;
    }
    
    for (int i = 0; i < top_count; i++) {
        Message *msg = &g_messages[top_indices[i]];
        
        const char *sender_name = "Unknown";
        int uidx = auth_find_user_by_id(msg->sender_id);
        if (uidx >= 0) {
            sender_name = g_users[uidx].display_name;
        }
        
        char wire_name[MAX_NAME_LEN];
        strncpy(wire_name, sender_name, sizeof(wire_name) - 1);
        wire_name[sizeof(wire_name) - 1] = '\0';
        str_to_wire(wire_name);
        
        char time_buf[64];
        utils_format_time(msg->sent_at, time_buf, sizeof(time_buf));
        str_to_wire(time_buf);
        
        char line[CMD_BUF_LEN];
        snprintf(line, sizeof(line), "MSG %d %s %s %s", 
                msg->msg_id, wire_name, time_buf, msg->content);
        nh_send_to(sock, line, client_addr);
        
        int reply_indices[MAX_MESSAGES];
        int reply_count = msg_get_replies(msg->msg_id, reply_indices, MAX_MESSAGES);
        
        for (int j = 0; j < reply_count; j++) {
            Message *reply = &g_messages[reply_indices[j]];
            
            const char *reply_name = "Unknown";
            int reply_uidx = auth_find_user_by_id(reply->sender_id);
            if (reply_uidx >= 0) {
                reply_name = g_users[reply_uidx].display_name;
            }
            
            char reply_wire_name[MAX_NAME_LEN];
            strncpy(reply_wire_name, reply_name, sizeof(reply_wire_name) - 1);
            reply_wire_name[sizeof(reply_wire_name) - 1] = '\0';
            str_to_wire(reply_wire_name);
            
            char reply_time_buf[64];
            utils_format_time(reply->sent_at, reply_time_buf, sizeof(reply_time_buf));
            str_to_wire(reply_time_buf);
            
            char reply_line[CMD_BUF_LEN];
            snprintf(reply_line, sizeof(reply_line), "REPLY %d %s %s %s", 
                    reply->msg_id, reply_wire_name, reply_time_buf, reply->content);
            nh_send_to(sock, reply_line, client_addr);
        }
    }
    
    nh_send_to(sock, "END", client_addr);
}

static void cmd_delete_msg(int sock, struct sockaddr_in *client_addr, char *args) {
    int msg_id = atoi(args);
    int result = msg_delete(msg_id);
    
    if (result == SUCCESS) {
        nh_send_to(sock, "OK", client_addr);
    } else {
        nh_send_to(sock, err_string(result), client_addr);
    }
}

static void cmd_list_users(int sock, struct sockaddr_in *client_addr) {
    if (!auth_is_logged_in()) {
        nh_send_to(sock, "ERR_AUTH", client_addr);
        return;
    }
    
    int found = 0;
    for (int i = 0; i < g_user_count; i++) {
        if (!g_users[i].is_active) continue;
        
        char wire_name[MAX_NAME_LEN];
        strncpy(wire_name, g_users[i].display_name, sizeof(wire_name) - 1);
        wire_name[sizeof(wire_name) - 1] = '\0';
        str_to_wire(wire_name);
        
        char line[128];
        snprintf(line, sizeof(line), "%d %s %s", 
                g_users[i].user_id, wire_name, g_users[i].username);
        nh_send_to(sock, line, client_addr);
        found++;
    }
    
    if (found == 0) {
        nh_send_to(sock, "ERR_EMPTY", client_addr);
        return;
    }
    
    nh_send_to(sock, "END", client_addr);
}

static void cmd_quit(int sock, struct sockaddr_in *client_addr) {
    nh_send_to(sock, "OK", client_addr);
    s_quit = 1;
}

static void dispatch_command(int sock, struct sockaddr_in *client_addr, char *cmd_buf) {
    char cmd_copy[CMD_BUF_LEN];
    strncpy(cmd_copy, cmd_buf, sizeof(cmd_copy) - 1);
    cmd_copy[sizeof(cmd_copy) - 1] = '\0';
    
    char *verb = strtok(cmd_copy, " ");
    if (!verb) {
        nh_send_to(sock, "ERR_UNKNOWN", client_addr);
        return;
    }
    
    char *args = cmd_buf + strlen(verb) + 1;
    if (strlen(verb) + 1 >= strlen(cmd_buf)) {
        args = "";
    }
    
    if (strcmp(verb, "REGISTER") == 0) {
        cmd_register(sock, client_addr, args);
    } else if (strcmp(verb, "LOGIN") == 0) {
        cmd_login(sock, client_addr, args);
    } else if (strcmp(verb, "LOGOUT") == 0) {
        cmd_logout(sock, client_addr);
    } else if (strcmp(verb, "CREATE_GROUP") == 0) {
        cmd_create_group(sock, client_addr, args);
    } else if (strcmp(verb, "JOIN_GROUP") == 0) {
        cmd_join_group(sock, client_addr, args);
    } else if (strcmp(verb, "LEAVE_GROUP") == 0) {
        cmd_leave_group(sock, client_addr, args);
    } else if (strcmp(verb, "SEARCH_GROUPS") == 0) {
        cmd_search_groups(sock, client_addr, args);
    } else if (strcmp(verb, "LIST_MY_GROUPS") == 0) {
        cmd_list_my_groups(sock, client_addr);
    } else if (strcmp(verb, "LIST_ALL_GROUPS") == 0) {
        cmd_list_all_groups(sock, client_addr);
    } else if (strcmp(verb, "LIST_MEMBERS") == 0) {
        cmd_list_members(sock, client_addr, args);
    } else if (strcmp(verb, "SEND_MSG") == 0) {
        cmd_send_msg(sock, client_addr, args);
    } else if (strcmp(verb, "VIEW_MSGS") == 0) {
        cmd_view_msgs(sock, client_addr, args);
    } else if (strcmp(verb, "DELETE_MSG") == 0) {
        cmd_delete_msg(sock, client_addr, args);
    } else if (strcmp(verb, "LIST_USERS") == 0) {
        cmd_list_users(sock, client_addr);
    } else if (strcmp(verb, "QUIT") == 0) {
        cmd_quit(sock, client_addr);
    } else {
        nh_send_to(sock, "ERR_UNKNOWN", client_addr);
    }
    
    printf("CMD [%s] from %s:%d, user_id=%d\n", verb,
           inet_ntoa(client_addr->sin_addr), ntohs(client_addr->sin_port), auth_get_user_id());
}

// Message struct for master-slave communication via POSIX message queue
typedef struct {
    char cmd_buf[CMD_BUF_LEN];
    struct sockaddr_in client_addr;
} DgramMsg;

// SIGCHLD handler to prevent zombie slave processes
static void sigchld_handler(int sig) {
    (void)sig;
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

int main() {
    if (fio_init_files() != SUCCESS) {
        printf("Failed to initialize data files.\n");
        return 1;
    }
    
    if (fio_load_all() != SUCCESS) {
        printf("Failed to load data.\n");
        return 1;
    }
    
    int server_fd = nh_server_init(SERVER_PORT);
    if (server_fd == ERR_CONN) {
        printf("Failed to start UDP server.\n");
        return 1;
    }
    
    printf("Group Chat UDP Server ready. Waiting for datagrams...\n");

    // Install SIGCHLD handler to prevent zombie slave processes
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDWAIT;
    sigaction(SIGCHLD, &sa, NULL);

    // Open POSIX message queue for master-slave communication
    struct mq_attr attr;
    attr.mq_flags = 0;
    attr.mq_maxmsg = MQ_MAX_MSG;
    attr.mq_msgsize = sizeof(DgramMsg);
    attr.mq_curmsgs = 0;
    mqd_t mq = mq_open(MQ_NAME, O_CREAT | O_RDWR, 0666, &attr);
    if (mq == (mqd_t)-1) {
        perror("mq_open");
        return 1;
    }

    // Main loop - master process receives datagrams and forks slaves
    while (1) {
        DgramMsg msg;

        // MASTER receives the datagram from any client
        int r = nh_recv_from(server_fd, msg.cmd_buf, CMD_BUF_LEN, &msg.client_addr);
        if (r == ERR_CONN) continue;

        // MASTER pushes the full datagram (content + client address) into the queue
        if (mq_send(mq, (char *)&msg, sizeof(DgramMsg), 0) == -1) {
            perror("mq_send");
            continue;
        }

        // MASTER forks a slave to handle this one datagram
        pid_t pid = fork();

        if (pid < 0) {
            perror("fork");
            continue;
        }

        if (pid == 0) {
            // SLAVE PROCESS
            // Slave reads the datagram that the master placed in the queue
            DgramMsg slave_msg;
            if (mq_receive(mq, (char *)&slave_msg, sizeof(DgramMsg), NULL) == -1) {
                perror("mq_receive");
                exit(1);
            }

            // Slave resets auth state — each datagram is stateless
            auth_logout();

            // Slave processes the command using the datagram content and client address from the queue
            dispatch_command(server_fd, &slave_msg.client_addr, slave_msg.cmd_buf);

            // Slave saves data to file after processing
            fio_save_all();

            // Slave exits — it handled exactly one datagram
            mq_close(mq);
            exit(0);
        }

        // MASTER loops back immediately to receive the next datagram
        // It does not wait for the slave to finish
    }

    // Cleanup (unreachable in infinite loop but good practice)
    mq_close(mq);
    mq_unlink(MQ_NAME);

    nh_close(server_fd);
    return 0;
}
