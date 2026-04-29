#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include "config.h"
#include "file_io.h"
#include "utils.h"
#include "auth.h"
#include "groups.h"
#include "messaging.h"
#include "net_handler.h"

// Static quit flag
static int s_quit = 0;

// Helper - map return code to error string
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

// Helper - replace characters in a local buffer
static void wire_to_str(char *str) {
    utils_replace_char(str, '_', ' ');
}

static void str_to_wire(char *str) {
    utils_replace_char(str, ' ', '_');
}

// Command handlers
static void cmd_register(int client_fd, char *args) {
    char *username = strtok(args, " ");
    char *display_name = strtok(NULL, " ");
    char *password = strtok(NULL, " ");
    
    if (!username || !display_name || !password) {
        nh_send_line(client_fd, "ERR_AUTH");
        return;
    }
    
    wire_to_str(username);
    wire_to_str(display_name);
    
    int result = auth_register(username, display_name, password);
    if (result > 0) {
        char response[64];
        snprintf(response, sizeof(response), "OK %d", result);
        nh_send_line(client_fd, response);
        fio_save_users(fio_get_users(), *fio_get_user_count());  // Save after registration
    } else {
        nh_send_line(client_fd, err_string(result));
    }
}

static void cmd_login(int client_fd, char *args) {
    char *username = strtok(args, " ");
    char *password = strtok(NULL, " ");
    
    if (!username || !password) {
        nh_send_line(client_fd, "ERR_AUTH");
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
        nh_send_line(client_fd, response);
    } else {
        nh_send_line(client_fd, err_string(result));
    }
}

static void cmd_logout(int client_fd) {
    auth_logout();
    nh_send_line(client_fd, "OK");
}

static void cmd_create_group(int client_fd, char *args) {
    char *group_name = strtok(args, " ");
    char *description = args + strlen(group_name) + 1;
    
    if (!group_name) {
        nh_send_line(client_fd, "ERR_AUTH");
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
        nh_send_line(client_fd, response);
        fio_save_groups(fio_get_groups(), *fio_get_group_count());  // Save after group creation
    } else {
        nh_send_line(client_fd, err_string(result));
    }
}

static void cmd_join_group(int client_fd, char *args) {
    int group_id = atoi(args);
    int result = grp_join(group_id);
    
    if (result == SUCCESS) {
        nh_send_line(client_fd, "OK");
        fio_save_groups(fio_get_groups(), *fio_get_group_count());  // Save after join
    } else {
        nh_send_line(client_fd, err_string(result));
    }
}

static void cmd_leave_group(int client_fd, char *args) {
    int group_id = atoi(args);
    int result = grp_leave(group_id);
    
    if (result == SUCCESS) {
        nh_send_line(client_fd, "OK");
        fio_save_groups(fio_get_groups(), *fio_get_group_count());  // Save after leave
    } else {
        nh_send_line(client_fd, err_string(result));
    }
}

static void cmd_search_groups(int client_fd, char *args) {
    char *keyword = args;
    utils_trim(keyword);
    
    int result_ids[MAX_GROUPS];
    int found = grp_search(keyword, result_ids, MAX_GROUPS);
    
    if (found == 0) {
        nh_send_line(client_fd, "ERR_NOT_FOUND");
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
        nh_send_line(client_fd, line);
    }
    
    nh_send_line(client_fd, "END");
}

static void cmd_list_my_groups(int client_fd) {
    if (!auth_is_logged_in()) {
        nh_send_line(client_fd, "ERR_AUTH");
        return;
    }
    
    int user_id = auth_get_user_id();
    int group_ids[MAX_GROUPS];
    int found = grp_get_member_groups(user_id, group_ids, MAX_GROUPS);
    
    if (found == 0) {
        nh_send_line(client_fd, "ERR_EMPTY");
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
        nh_send_line(client_fd, line);
    }
    
    nh_send_line(client_fd, "END");
}

static void cmd_list_all_groups(int client_fd) {
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
        nh_send_line(client_fd, line);
        found++;
    }
    
    if (found == 0) {
        nh_send_line(client_fd, "ERR_EMPTY");
        return;
    }
    
    nh_send_line(client_fd, "END");
}

static void cmd_list_members(int client_fd, char *args) {
    int group_id = atoi(args);
    int idx = grp_find_by_id(group_id);
    
    if (idx == ERR_NOT_FOUND) {
        nh_send_line(client_fd, "ERR_NOT_FOUND");
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
        
        nh_send_line(client_fd, line);
    }
    
    nh_send_line(client_fd, "END");
}

static void cmd_send_msg(int client_fd, char *args) {
    char *group_id_str = strtok(args, " ");
    char *reply_to_str = strtok(NULL, " ");
    char *content = strtok(NULL, "");
    
    if (!group_id_str || !reply_to_str || !content) {
        nh_send_line(client_fd, "ERR_UNKNOWN");
        return;
    }
    
    int group_id = atoi(group_id_str);
    int reply_to = atoi(reply_to_str);
    
    int result = msg_send(group_id, content, reply_to);
    if (result > 0) {
        char response[64];
        snprintf(response, sizeof(response), "OK %d", result);
        nh_send_line(client_fd, response);
        fio_save_messages(fio_get_messages(), *fio_get_msg_count());  // Save after message send
    } else {
        nh_send_line(client_fd, err_string(result));
    }
}

static void cmd_view_msgs(int client_fd, char *args) {
    int group_id = atoi(args);
    
    int idx = grp_find_by_id(group_id);
    if (idx == ERR_NOT_FOUND) {
        nh_send_line(client_fd, "ERR_NOT_FOUND");
        return;
    }
    
    if (!auth_is_logged_in() || !grp_is_member(idx, auth_get_user_id())) {
        nh_send_line(client_fd, "ERR_PERMISSION");
        return;
    }
    
    int top_indices[MAX_MESSAGES];
    int top_count = msg_get_for_group(group_id, top_indices, MAX_MESSAGES);
    
    if (top_count == 0) {
        nh_send_line(client_fd, "ERR_EMPTY");
        return;
    }
    
    for (int i = 0; i < top_count; i++) {
        Message *msg = &g_messages[top_indices[i]];
        
        // Get sender display name
        const char *sender_name = "Unknown";
        int uidx = auth_find_user_by_id(msg->sender_id);
        if (uidx >= 0) {
            sender_name = g_users[uidx].display_name;
        }
        
        char wire_name[MAX_NAME_LEN];
        strncpy(wire_name, sender_name, sizeof(wire_name) - 1);
        wire_name[sizeof(wire_name) - 1] = '\0';
        str_to_wire(wire_name);
        
        // Format timestamp
        char time_buf[64];
        utils_format_time(msg->sent_at, time_buf, sizeof(time_buf));
        str_to_wire(time_buf);
        
        char line[CMD_BUF_LEN];
        snprintf(line, sizeof(line), "MSG %d %s %s %s", 
                msg->msg_id, wire_name, time_buf, msg->content);
        nh_send_line(client_fd, line);
        
        // Load replies
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
            nh_send_line(client_fd, reply_line);
        }
    }
    
    nh_send_line(client_fd, "END");
}

static void cmd_delete_msg(int client_fd, char *args) {
    int msg_id = atoi(args);
    int result = msg_delete(msg_id);
    
    if (result == SUCCESS) {
        nh_send_line(client_fd, "OK");
        fio_save_messages(fio_get_messages(), *fio_get_msg_count());  // Save after message deletion
    } else {
        nh_send_line(client_fd, err_string(result));
    }
}

static void cmd_list_users(int client_fd) {
    if (!auth_is_logged_in()) {
        nh_send_line(client_fd, "ERR_AUTH");
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
        nh_send_line(client_fd, line);
        found++;
    }
    
    if (found == 0) {
        nh_send_line(client_fd, "ERR_EMPTY");
        return;
    }
    
    nh_send_line(client_fd, "END");
}

static void cmd_quit(int client_fd) {
    nh_send_line(client_fd, "OK");
    s_quit = 1;
}

// Command dispatcher
static void dispatch_command(int client_fd, char *cmd_buf) {
    char cmd_copy[CMD_BUF_LEN];
    strncpy(cmd_copy, cmd_buf, sizeof(cmd_copy) - 1);
    cmd_copy[sizeof(cmd_copy) - 1] = '\0';
    
    char *verb = strtok(cmd_copy, " ");
    if (!verb) {
        nh_send_line(client_fd, "ERR_UNKNOWN");
        return;
    }
    
    char *args = cmd_buf + strlen(verb) + 1;
    if (strlen(verb) + 1 >= strlen(cmd_buf)) {
        args = "";
    }
    
    if (strcmp(verb, "REGISTER") == 0) {
        cmd_register(client_fd, args);
    } else if (strcmp(verb, "LOGIN") == 0) {
        cmd_login(client_fd, args);
    } else if (strcmp(verb, "LOGOUT") == 0) {
        cmd_logout(client_fd);
    } else if (strcmp(verb, "CREATE_GROUP") == 0) {
        cmd_create_group(client_fd, args);
    } else if (strcmp(verb, "JOIN_GROUP") == 0) {
        cmd_join_group(client_fd, args);
    } else if (strcmp(verb, "LEAVE_GROUP") == 0) {
        cmd_leave_group(client_fd, args);
    } else if (strcmp(verb, "SEARCH_GROUPS") == 0) {
        cmd_search_groups(client_fd, args);
    } else if (strcmp(verb, "LIST_MY_GROUPS") == 0) {
        cmd_list_my_groups(client_fd);
    } else if (strcmp(verb, "LIST_ALL_GROUPS") == 0) {
        cmd_list_all_groups(client_fd);
    } else if (strcmp(verb, "LIST_MEMBERS") == 0) {
        cmd_list_members(client_fd, args);
    } else if (strcmp(verb, "SEND_MSG") == 0) {
        cmd_send_msg(client_fd, args);
    } else if (strcmp(verb, "VIEW_MSGS") == 0) {
        cmd_view_msgs(client_fd, args);
    } else if (strcmp(verb, "DELETE_MSG") == 0) {
        cmd_delete_msg(client_fd, args);
    } else if (strcmp(verb, "LIST_USERS") == 0) {
        cmd_list_users(client_fd);
    } else if (strcmp(verb, "QUIT") == 0) {
        cmd_quit(client_fd);
    } else {
        nh_send_line(client_fd, "ERR_UNKNOWN");
    }
    
    printf("CMD [%s] from user_id=%d\n", verb, auth_get_user_id());
}

// Session handler
static void handle_session(int client_fd) {
    auth_logout(); // Reset session at start
    s_quit = 0;
    
    printf("New session started.\n");
    
    char cmd_buf[CMD_BUF_LEN];
    while (!s_quit) {
        int result = nh_recv_line(client_fd, cmd_buf, CMD_BUF_LEN);
        if (result == ERR_CONN) {
            printf("Client disconnected unexpectedly.\n");
            break;
        }
        
        dispatch_command(client_fd, cmd_buf);
    }
    
    nh_close(client_fd);
    printf("Session ended.\n");
}

// SIGCHLD handler to prevent zombie slave processes
static void sigchld_handler(int sig) {
    (void)sig;
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

// Main function
int main() {
    // Initialize data files
    if (fio_init_files() != SUCCESS) {
        printf("Failed to initialise data files.\n");
        return 1;
    }
    
    // Load data
    if (fio_load_all() != SUCCESS) {
        printf("Failed to load data.\n");
        return 1;
    }
    
    // Start server
    int server_fd = nh_server_init(SERVER_PORT);
    if (server_fd == ERR_CONN) {
        printf("Failed to start server.\n");
        return 1;
    }
    
    printf("Group Chat Server ready. Waiting for connections...\n");
    
    // Install SIGCHLD handler to prevent zombie slave processes
    struct sigaction sa;
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDWAIT;
    sigaction(SIGCHLD, &sa, NULL);
    
    // Main accept loop - fork-based concurrent server
    while (1) {
        int client_fd = nh_server_accept(server_fd);
        if (client_fd == ERR_CONN) {
            printf("Accept failed. Waiting...\n");
            continue;
        }

        pid_t pid = fork();

        if (pid < 0) {
            // fork failed — close this client and try again
            perror("fork");
            nh_close(client_fd);
            continue;
        }

        if (pid == 0) {
            // SLAVE PROCESS
            // The slave does not need the listening socket — close it
            close(server_fd);
            // Handle the client session fully — auth, commands, data save
            handle_session(client_fd);
            // Slave exits when the session ends
            exit(0);
        }

        // MASTER PROCESS
        // Master does not own this client connection — slave does. Close master's copy.
        nh_close(client_fd);
        // Master loops back immediately to accept the next client
    }
    
    return 0;
}
