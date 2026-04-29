#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include "config.h"
#include "shared_memory.h"
#include "auth.h"
#include "groups.h"
#include "messaging.h"
#include "net_handler.h"

// Thread-safe session structure
typedef struct {
    int client_fd;
    int user_id;
    char username[MAX_UNAME_LEN];
    char display_name[MAX_NAME_LEN];
    int logged_in;
} ClientSession;

// Global quit flag
static volatile int s_quit = 0;

// Signal handler for graceful shutdown
void sigint_handler(int sig) {
    (void)sig; // Suppress unused parameter warning
    s_quit = 1;
    printf("\n[+] Server shutting down gracefully...\n");
}

// Helper functions
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

static void wire_to_str(char *str) {
    for (int i = 0; str[i]; i++) {
        if (str[i] == '_') str[i] = ' ';
    }
}

static void str_to_wire(char *str) {
    for (int i = 0; str[i]; i++) {
        if (str[i] == ' ') str[i] = '_';
    }
}

// Command handlers
static void cmd_register(ClientSession *session, char *args) {
    char *username = strtok(args, " ");
    char *display_name = strtok(NULL, " ");
    char *password = strtok(NULL, " ");
    
    if (!username || !display_name || !password) {
        nh_send_line(session->client_fd, "ERR_AUTH");
        return;
    }
    
    wire_to_str(username);
    wire_to_str(display_name);
    
    User user;
    user.user_id = g_shared_users->user_count + 1;
    strncpy(user.username, username, MAX_UNAME_LEN - 1);
    strncpy(user.display_name, display_name, MAX_NAME_LEN - 1);
    strncpy(user.password, password, MAX_NAME_LEN - 1);
    user.is_active = 1;
    user.created_at = time(NULL);
    
    int result = shared_add_user(&user);
    if (result == SUCCESS) {
        nh_send_line(session->client_fd, "OK");
        printf("[+] User registered: %s\n", username);
    } else {
        nh_send_line(session->client_fd, err_string(result));
    }
}

static void cmd_login(ClientSession *session, char *args) {
    char *username = strtok(args, " ");
    char *password = strtok(NULL, " ");
    
    if (!username || !password) {
        nh_send_line(session->client_fd, "ERR_AUTH");
        return;
    }
    
    wire_to_str(username);
    
    int user_index = shared_find_user(username);
    if (user_index == ERR_NOT_FOUND) {
        nh_send_line(session->client_fd, "ERR_AUTH");
        return;
    }
    
    User *user = &g_shared_users->users[user_index];
    if (strcmp(user->password, password) != 0) {
        nh_send_line(session->client_fd, "ERR_AUTH");
        return;
    }
    
    session->user_id = user->user_id;
    strncpy(session->username, user->username, MAX_UNAME_LEN - 1);
    strncpy(session->display_name, user->display_name, MAX_NAME_LEN - 1);
    session->logged_in = 1;
    
    char response[256];
    snprintf(response, sizeof(response), "OK %d %s", user->user_id, user->display_name);
    str_to_wire(response);
    nh_send_line(session->client_fd, response);
    
    printf("[+] User logged in: %s\n", username);
}

static void cmd_create_group(ClientSession *session, char *args) {
    if (!session->logged_in) {
        nh_send_line(session->client_fd, "ERR_AUTH");
        return;
    }
    
    char *name = strtok(args, " ");
    char *desc = strtok(NULL, "");
    
    if (!name) {
        nh_send_line(session->client_fd, "ERR_AUTH");
        return;
    }
    
    wire_to_str(name);
    if (desc) wire_to_str(desc);
    else desc = "No description";
    
    Group group;
    group.group_id = g_shared_groups->group_count + 1;
    strncpy(group.group_name, name, MAX_NAME_LEN - 1);
    strncpy(group.description, desc, MAX_DESC_LEN - 1);
    group.creator_id = session->user_id;
    group.member_ids[0] = session->user_id;
    group.member_count = 1;
    group.is_active = 1;
    group.created_at = time(NULL);
    
    int result = shared_add_group(&group);
    if (result == SUCCESS) {
        char response[256];
        snprintf(response, sizeof(response), "OK %d", group.group_id);
        nh_send_line(session->client_fd, response);
        printf("[+] Group created: %s by %s\n", name, session->username);
    } else {
        nh_send_line(session->client_fd, err_string(result));
    }
}

static void cmd_list_groups(ClientSession *session) {
    if (!session->logged_in) {
        nh_send_line(session->client_fd, "ERR_AUTH");
        return;
    }
    
    pthread_mutex_lock(&g_shared_groups->groups_mutex);
    
    for (int i = 0; i < g_shared_groups->group_count; i++) {
        Group *g = &g_shared_groups->groups[i];
        if (g->is_active) {
            char line[512];
            snprintf(line, sizeof(line), "%d %s %d %s", 
                     g->group_id, g->group_name, g->member_count, g->description);
            str_to_wire(line);
            nh_send_line(session->client_fd, line);
        }
    }
    
    pthread_mutex_unlock(&g_shared_groups->groups_mutex);
    nh_send_line(session->client_fd, "END");
}

static void cmd_join_group(ClientSession *session, char *args) {
    if (!session->logged_in) {
        nh_send_line(session->client_fd, "ERR_AUTH");
        return;
    }
    
    char *group_id_str = strtok(args, " ");
    if (!group_id_str) {
        nh_send_line(session->client_fd, "ERR_AUTH");
        return;
    }
    
    int group_id = atoi(group_id_str);
    
    pthread_mutex_lock(&g_shared_groups->groups_mutex);
    
    // Find group
    int found = 0;
    for (int i = 0; i < g_shared_groups->group_count; i++) {
        if (g_shared_groups->groups[i].group_id == group_id && 
            g_shared_groups->groups[i].is_active) {
            
            Group *g = &g_shared_groups->groups[i];
            
            // Check if already a member
            for (int j = 0; j < g->member_count; j++) {
                if (g->member_ids[j] == session->user_id) {
                    pthread_mutex_unlock(&g_shared_groups->groups_mutex);
                    nh_send_line(session->client_fd, "ERR_DUPLICATE");
                    return;
                }
            }
            
            // Add to group
            if (g->member_count < MAX_MEMBERS) {
                g->member_ids[g->member_count] = session->user_id;
                g->member_count++;
                found = 1;
                break;
            } else {
                pthread_mutex_unlock(&g_shared_groups->groups_mutex);
                nh_send_line(session->client_fd, "ERR_FULL");
                return;
            }
        }
    }
    
    pthread_mutex_unlock(&g_shared_groups->groups_mutex);
    
    if (found) {
        nh_send_line(session->client_fd, "OK");
        printf("[+] %s joined group %d\n", session->username, group_id);
    } else {
        nh_send_line(session->client_fd, "ERR_NOT_FOUND");
    }
}

static void cmd_send_message(ClientSession *session, char *args) {
    if (!session->logged_in) {
        nh_send_line(session->client_fd, "ERR_AUTH");
        return;
    }
    
    char *group_id_str = strtok(args, " ");
    char *reply_id_str = strtok(NULL, " ");
    char *content = strtok(NULL, "");
    
    if (!group_id_str || !content) {
        nh_send_line(session->client_fd, "ERR_AUTH");
        return;
    }
    
    int group_id = atoi(group_id_str);
    int reply_to = reply_id_str ? atoi(reply_id_str) : -1;
    
    wire_to_str(content);
    
    Message msg;
    msg.msg_id = g_shared_messages->message_count + 1;
    msg.group_id = group_id;
    msg.sender_id = session->user_id;
    msg.reply_to = reply_to;
    strncpy(msg.content, content, MAX_MSG_LEN - 1);
    msg.is_deleted = 0;
    msg.sent_at = time(NULL);
    
    int result = shared_add_message(&msg);
    if (result == SUCCESS) {
        char response[256];
        snprintf(response, sizeof(response), "OK %d", msg.msg_id);
        nh_send_line(session->client_fd, response);
        printf("[+] Message sent in group %d by %s\n", group_id, session->username);
    } else {
        nh_send_line(session->client_fd, err_string(result));
    }
}

static void cmd_view_messages(ClientSession *session, char *args) {
    if (!session->logged_in) {
        nh_send_line(session->client_fd, "ERR_AUTH");
        return;
    }
    
    char *group_id_str = strtok(args, " ");
    if (!group_id_str) {
        nh_send_line(session->client_fd, "ERR_AUTH");
        return;
    }
    
    int group_id = atoi(group_id_str);
    
    Message messages[MAX_MESSAGES];
    int count;
    shared_get_messages_by_group(group_id, messages, &count);
    
    if (count == 0) {
        nh_send_line(session->client_fd, "ERR_EMPTY");
        return;
    }
    
    // Send messages
    for (int i = 0; i < count; i++) {
        if (messages[i].is_deleted) continue;
        
        // Find sender name
        char sender_name[MAX_NAME_LEN] = "Unknown";
        pthread_mutex_lock(&g_shared_users->users_mutex);
        for (int j = 0; j < g_shared_users->user_count; j++) {
            if (g_shared_users->users[j].user_id == messages[i].sender_id) {
                strncpy(sender_name, g_shared_users->users[j].display_name, MAX_NAME_LEN - 1);
                break;
            }
        }
        pthread_mutex_unlock(&g_shared_users->users_mutex);
        
        char line[1024];
        if (messages[i].reply_to == -1) {
            snprintf(line, sizeof(line), "MSG %d %s %ld %s", 
                     messages[i].msg_id, sender_name, messages[i].sent_at, messages[i].content);
        } else {
            snprintf(line, sizeof(line), "REPLY %d %s %ld %s", 
                     messages[i].msg_id, sender_name, messages[i].sent_at, messages[i].content);
        }
        
        str_to_wire(line);
        nh_send_line(session->client_fd, line);
    }
    
    nh_send_line(session->client_fd, "END");
}

static void dispatch_command(ClientSession *session, char *cmd_buf) {
    char *cmd = strtok(cmd_buf, " ");
    
    if (!cmd) return;
    
    if (strcmp(cmd, "REGISTER") == 0) {
        cmd_register(session, cmd_buf + strlen(cmd) + 1);
    } else if (strcmp(cmd, "LOGIN") == 0) {
        cmd_login(session, cmd_buf + strlen(cmd) + 1);
    } else if (strcmp(cmd, "CREATE_GROUP") == 0) {
        cmd_create_group(session, cmd_buf + strlen(cmd) + 1);
    } else if (strcmp(cmd, "LIST_ALL_GROUPS") == 0) {
        cmd_list_groups(session);
    } else if (strcmp(cmd, "JOIN_GROUP") == 0) {
        cmd_join_group(session, cmd_buf + strlen(cmd) + 1);
    } else if (strcmp(cmd, "SEND_MSG") == 0) {
        cmd_send_message(session, cmd_buf + strlen(cmd) + 1);
    } else if (strcmp(cmd, "VIEW_MSGS") == 0) {
        cmd_view_messages(session, cmd_buf + strlen(cmd) + 1);
    } else if (strcmp(cmd, "QUIT") == 0) {
        nh_send_line(session->client_fd, "OK");
        session->logged_in = 0;
    } else {
        nh_send_line(session->client_fd, "ERR_UNKNOWN");
    }
}

// Client handler thread
void *handle_client_thread(void *arg) {
    ClientSession *session = (ClientSession *)arg;
    char cmd_buf[CMD_BUF_LEN];
    
    printf("[+] New client connected: fd=%d\n", session->client_fd);
    
    while (!s_quit && session->logged_in >= 0) {
        int result = nh_recv_line(session->client_fd, cmd_buf, CMD_BUF_LEN);
        if (result != SUCCESS) {
            break;
        }
        
        dispatch_command(session, cmd_buf);
        
        if (!session->logged_in && session->user_id == -1) {
            break; // Client quit
        }
    }
    
    printf("[+] Client disconnected: fd=%d, user=%s\n", 
           session->client_fd, session->username);
    
    nh_close(session->client_fd);
    free(session);
    return NULL;
}

int main(void) {
    // Setup signal handler
    signal(SIGINT, sigint_handler);
    signal(SIGTERM, sigint_handler);
    
    // Initialize shared memory
    int result = shared_memory_init();
    if (result != SUCCESS) {
        printf("Failed to initialize shared memory\n");
        return 1;
    }
    
    // Start server
    int server_fd = nh_server_init(SERVER_PORT);
    if (server_fd == ERR_CONN) {
        printf("Failed to start server\n");
        shared_memory_destroy();
        return 1;
    }
    
    printf("Group Chat Server (Threaded) ready. Waiting for connections...\n");
    printf("Port: %d\n", SERVER_PORT);
    
    // Main accept loop
    while (!s_quit) {
        int client_fd = nh_server_accept(server_fd);
        if (client_fd == ERR_CONN) {
            if (s_quit) break;
            printf("Accept failed. Waiting...\n");
            continue;
        }
        
        // Create session structure
        ClientSession *session = malloc(sizeof(ClientSession));
        session->client_fd = client_fd;
        session->user_id = -1;
        session->logged_in = 0;
        memset(session->username, 0, sizeof(session->username));
        memset(session->display_name, 0, sizeof(session->display_name));
        
        // Create thread for this client
        pthread_t thread;
        if (pthread_create(&thread, NULL, handle_client_thread, session) != 0) {
            perror("pthread_create");
            nh_close(client_fd);
            free(session);
            continue;
        }
        
        // Detach thread - it will clean up itself
        pthread_detach(thread);
    }
    
    // Cleanup
    printf("[+] Saving data and shutting down...\n");
    shared_memory_save_to_files();
    shared_memory_destroy();
    nh_close(server_fd);
    
    return 0;
}
