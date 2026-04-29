#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "net_handler.h"

// Forward declarations
void print_error(const char *response);
void display_group_list_line(const char *line);
void display_message_line(const char *line);
void handle_view_messages(int group_id);
void handle_send_message(int group_id);
void handle_delete_message();
void group_menu(int group_id, const char *group_name);
void handle_my_groups();
void handle_browse_groups();
void handle_create_group();
void handle_leave_group();
void handle_list_users();
void main_menu_authenticated();
void handle_register();
void handle_login();

// Global state
static int  g_sock        = -1;
static struct sockaddr_in g_server_addr;
static int  g_user_id     = -1;
static char g_display_name[64];
static char g_username[64];

// Local cache for current group context
static int  g_current_group_id   = -1;
static char g_current_group_name[64];

// Core helper functions
void client_send_recv(const char *cmd, char *response, int resp_len) {
    if (nh_send_to(g_sock, cmd, &g_server_addr) != SUCCESS) {
        printf("\n[!] Failed to send command. Exiting.\n");
        nh_close(g_sock);
        exit(1);
    }
    
    if (nh_recv_from(g_sock, response, resp_len) != SUCCESS) {
        printf("\n[!] Failed to receive response. Exiting.\n");
        nh_close(g_sock);
        exit(1);
    }
}

void client_recv_multiline() {
    char line[CMD_BUF_LEN];
    while (1) {
        if (nh_recv_from(g_sock, line, CMD_BUF_LEN) != SUCCESS) {
            printf("\n[!] Failed to receive response. Exiting.\n");
            nh_close(g_sock);
            exit(1);
        }
        
        if (strcmp(line, "END") == 0) break;
        if (strncmp(line, "ERR_", 4) == 0) {
            print_error(line);
            return;
        }
        printf("%s\n", line);
    }
}

void client_recv_into(char lines[][CMD_BUF_LEN], int max_lines, int *count) {
    *count = 0;
    while (*count < max_lines) {
        if (nh_recv_from(g_sock, lines[*count], CMD_BUF_LEN) != SUCCESS) {
            printf("\n[!] Failed to receive response. Exiting.\n");
            nh_close(g_sock);
            exit(1);
        }
        
        if (strcmp(lines[*count], "END") == 0) break;
        if (strncmp(lines[*count], "ERR_", 4) == 0) {
            strcpy(lines[0], lines[*count]);
            *count = -1;
            return;
        }
        (*count)++;
    }
}

void print_header(const char *title) {
    printf("\033[2J\033[H");
    printf("============================================================\n");
    printf("  GROUP CHAT APPLICATION (UDP)\n");
    printf("  %s\n", title);
    printf("============================================================\n");
    printf("\n");
}

void print_separator() {
    printf("------------------------------------------------------------\n");
}

void print_error(const char *response) {
    const char *msg;
    if (strcmp(response, "ERR_NOT_FOUND") == 0) {
        msg = "Not found.";
    } else if (strcmp(response, "ERR_AUTH") == 0) {
        msg = "Authentication failed or not logged in.";
    } else if (strcmp(response, "ERR_DUPLICATE") == 0) {
        msg = "Already exists.";
    } else if (strcmp(response, "ERR_FULL") == 0) {
        msg = "Maximum limit reached.";
    } else if (strcmp(response, "ERR_PERMISSION") == 0) {
        msg = "You do not have permission to do that.";
    } else if (strcmp(response, "ERR_EMPTY") == 0) {
        msg = "No records found.";
    } else if (strcmp(response, "ERR_UNKNOWN") == 0) {
        msg = "Unknown command.";
    } else {
        msg = response;
    }
    printf("[!] %s\n", msg);
}

int is_error(const char *response) {
    return strncmp(response, "ERR_", 4) == 0;
}

void pause_for_input() {
    printf("\nPress Enter to continue...");
    fflush(stdout);
    getchar();
}

int read_int(const char *prompt, int min, int max) {
    char buffer[64];
    int value;
    
    while (1) {
        printf("%s", prompt);
        fflush(stdout);
        if (!fgets(buffer, sizeof(buffer), stdin)) continue;
        
        if (sscanf(buffer, "%d", &value) == 1) {
            if (value >= min && value <= max) {
                return value;
            }
        }
        printf("Invalid input. Enter a number between %d and %d: ", min, max);
    }
}

void read_string(const char *prompt, char *buf, int buf_len) {
    printf("%s", prompt);
    fflush(stdout);
    if (!fgets(buf, buf_len, stdin)) {
        buf[0] = '\0';
        return;
    }
    
    // Strip trailing newline
    size_t len = strlen(buf);
    if (len > 0 && buf[len-1] == '\n') {
        buf[len-1] = '\0';
        len--;
    }
    
    // Trim leading whitespace
    int start = 0;
    while (start < (int)len && (buf[start] == ' ' || buf[start] == '\t')) {
        start++;
    }
    
    if (start > 0) {
        memmove(buf, buf + start, len - start + 1);
        len -= start;
    }
    
    // Trim trailing whitespace
    while (len > 0 && (buf[len-1] == ' ' || buf[len-1] == '\t')) {
        len--;
    }
    buf[len] = '\0';
}

void spaces_to_wire(char *buf) {
    for (int i = 0; buf[i]; i++) {
        if (buf[i] == ' ') buf[i] = '_';
    }
}

void wire_to_spaces(char *buf) {
    for (int i = 0; buf[i]; i++) {
        if (buf[i] == '_') buf[i] = ' ';
    }
}

// Group browsing and messaging functions
void display_group_list_line(const char *line) {
    int group_id, member_count;
    char group_name[MAX_NAME_LEN], description[256];
    
    if (sscanf(line, "%d %s %d", &group_id, group_name, &member_count) == 3) {
        // Find description after the three fields
        const char *desc_start = line;
        for (int i = 0; i < 3; i++) {
            while (*desc_start && *desc_start != ' ') desc_start++;
            while (*desc_start && *desc_start == ' ') desc_start++;
        }
        
        strncpy(description, desc_start, sizeof(description) - 1);
        description[sizeof(description) - 1] = '\0';
        
        wire_to_spaces(group_name);
        wire_to_spaces(description);
        
        printf("  [%d] %-20s  %d members\n", group_id, group_name, member_count);
        printf("      %s\n", description);
    }
}

void display_message_line(const char *line) {
    char type[16], sender_name[MAX_NAME_LEN], timestamp[32], content[MAX_MSG_LEN];
    int msg_id;
    
    if (sscanf(line, "%s %d %s %s", type, &msg_id, sender_name, timestamp) >= 4) {
        // Find content after the four fields
        const char *content_start = line;
        for (int i = 0; i < 4; i++) {
            while (*content_start && *content_start != ' ') content_start++;
            while (*content_start && *content_start == ' ') content_start++;
        }
        
        strncpy(content, content_start, sizeof(content) - 1);
        content[sizeof(content) - 1] = '\0';
        
        wire_to_spaces(sender_name);
        wire_to_spaces(timestamp);
        
        if (strcmp(type, "MSG") == 0) {
            print_separator();
            printf("[MSG #%d] %s  at %s\n", msg_id, sender_name, timestamp);
            printf("%s\n", content);
        } else if (strcmp(type, "REPLY") == 0) {
            printf("    \\__ [#%d] %s  at %s\n", msg_id, sender_name, timestamp);
            printf("        %s\n", content);
        }
    }
}

void handle_view_messages(int group_id) {
    print_header("Messages");
    printf("Group: %s\n\n", g_current_group_name);
    
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "VIEW_MSGS %d", group_id);
    
    char response[CMD_BUF_LEN];
    client_send_recv(cmd, response, sizeof(response));
    
    if (is_error(response)) {
        print_error(response);
        pause_for_input();
        return;
    }
    
    if (strcmp(response, "ERR_EMPTY") == 0) {
        printf("No messages yet. Be the first to send one!\n");
        pause_for_input();
        return;
    }
    
    char lines[MAX_MESSAGES][CMD_BUF_LEN];
    int count = 0;
    
    while (count < MAX_MESSAGES) {
        if (nh_recv_from(g_sock, lines[count], CMD_BUF_LEN) != SUCCESS) {
            printf("\n[!] Failed to receive response. Exiting.\n");
            nh_close(g_sock);
            exit(1);
        }
        
        if (strcmp(lines[count], "END") == 0) break;
        if (is_error(lines[count])) {
            print_error(lines[count]);
            break;
        }
        display_message_line(lines[count]);
        count++;
    }
    
    printf("\nTotal messages shown: %d\n", count);
    pause_for_input();
}

void handle_send_message(int group_id) {
    print_header("Send Message");
    printf("Group: %s\n\n", g_current_group_name);
    
    printf("Enter your message (or 0 to cancel):\n> ");
    char content[512];
    fgets(content, sizeof(content), stdin);
    
    // Strip newline
    size_t len = strlen(content);
    if (len > 0 && content[len-1] == '\n') {
        content[len-1] = '\0';
    }
    
    if (strcmp(content, "0") == 0) return;
    
    int reply_to = read_int("Reply to a message? Enter message ID (0 for none): ", 0, 99999);
    if (reply_to == 0) reply_to = NO_REPLY;
    
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "SEND_MSG %d %d %s", group_id, reply_to, content);
    
    char response[CMD_BUF_LEN];
    client_send_recv(cmd, response, sizeof(response));
    
    if (strncmp(response, "OK", 2) == 0) {
        int msg_id;
        if (sscanf(response, "OK %d", &msg_id) == 1) {
            printf("[+] Message sent. ID: #%d\n", msg_id);
        }
    } else {
        print_error(response);
    }
    pause_for_input();
}

void handle_delete_message() {
    print_header("Delete Message");
    int msg_id = read_int("Enter message ID to delete: ", 1, 99999);
    
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "DELETE_MSG %d", msg_id);
    
    char response[CMD_BUF_LEN];
    client_send_recv(cmd, response, sizeof(response));
    
    if (strcmp(response, "OK") == 0) {
        printf("[+] Message deleted.\n");
    } else {
        print_error(response);
    }
    pause_for_input();
}

// Group menu
void group_menu(int group_id, const char *group_name) {
    g_current_group_id = group_id;
    strncpy(g_current_group_name, group_name, sizeof(g_current_group_name) - 1);
    g_current_group_name[sizeof(g_current_group_name) - 1] = '\0';
    
    while (1) {
        print_header("Group Menu");
        printf("Group: %s\n\n", group_name);
        printf("1. View Messages\n");
        printf("2. Send Message\n");
        printf("3. Delete My Message\n");
        printf("4. View Members\n");
        printf("0. Leave this menu (back to My Groups)\n");
        printf("Choice: ");
        
        int choice = read_int("", 0, 4);
        
        switch (choice) {
            case 1:
                handle_view_messages(group_id);
                break;
            case 2:
                handle_send_message(group_id);
                break;
            case 3:
                handle_delete_message();
                break;
            case 4: {
                print_header("Members");
                char cmd[64];
                snprintf(cmd, sizeof(cmd), "LIST_MEMBERS %d", group_id);
                
                char line[CMD_BUF_LEN];
                client_send_recv(cmd, line, sizeof(line));
                
                if (is_error(line)) {
                    print_error(line);
                } else {
                    // Listen for multiline response
                    while (1) {
                        if (nh_recv_from(g_sock, line, CMD_BUF_LEN) != SUCCESS) {
                            printf("\n[!] Failed to receive response. Exiting.\n");
                            nh_close(g_sock);
                            exit(1);
                        }
                        
                        if (strcmp(line, "END") == 0) break;
                        if (is_error(line)) {
                            print_error(line);
                            break;
                        }
                        
                        int user_id;
                        char display_name[MAX_NAME_LEN], username[MAX_NAME_LEN];
                        if (sscanf(line, "%d %s %s", &user_id, display_name, username) == 3) {
                            wire_to_spaces(display_name);
                            printf("  %s (@%s)\n", display_name, username);
                        }
                    }
                }
                pause_for_input();
                break;
            }
            case 0:
                return;
            default:
                printf("Invalid choice.\n");
        }
    }
}

// My Groups menu
void handle_my_groups() {
    while (1) {
        print_header("My Groups");
        
        nh_send_to(g_sock, "LIST_MY_GROUPS", &g_server_addr);
        
        char lines[MAX_GROUPS][CMD_BUF_LEN];
        int count = 0;
        client_recv_into(lines, MAX_GROUPS, &count);
        
        if (count == -1) {
            print_error(lines[0]);
            printf("\nYou are not a member of any groups yet.\n");
            pause_for_input();
            return;
        }
        
        if (count == 0) {
            printf("You have not joined any groups yet.\n");
            pause_for_input();
            return;
        }
        
        printf("Your groups:\n\n");
        for (int i = 0; i < count; i++) {
            display_group_list_line(lines[i]);
            printf("\n");
        }
        
        printf("\nEnter group ID to open (0 to go back): ");
        int choice = read_int("", 0, 99999);
        if (choice == 0) return;
        
        // Find matching group
        int found = 0;
        for (int i = 0; i < count; i++) {
            int group_id;
            if (sscanf(lines[i], "%d", &group_id) == 1) {
                if (group_id == choice) {
                    char group_name[MAX_NAME_LEN];
                    int dummy;
                    sscanf(lines[i], "%d %s %d", &group_id, group_name, &dummy);
                    wire_to_spaces(group_name);
                    group_menu(choice, group_name);
                    found = 1;
                    break;
                }
            }
        }
        
        if (!found) {
            printf("[!] Group ID not in your list.\n");
        }
    }
}

// Browse & Join Groups menu
void handle_browse_groups() {
    while (1) {
        print_header("Browse Groups");
        printf("1. Search groups by keyword\n");
        printf("2. List all groups\n");
        printf("0. Back\n");
        int choice = read_int("Choice: ", 0, 2);
        
        switch (choice) {
            case 1: {
                char keyword[128];
                read_string("Search keyword: ", keyword, sizeof(keyword));
                if (strlen(keyword) == 0) {
                    printf("[!] Enter a keyword.\n");
                    continue;
                }
                
                char cmd[256];
                snprintf(cmd, sizeof(cmd), "SEARCH_GROUPS %s", keyword);
                nh_send_to(g_sock, cmd, &g_server_addr);
                
                print_header("Search Results");
                printf("Results for: \"%s\"\n\n", keyword);
                
                char lines[MAX_GROUPS][CMD_BUF_LEN];
                int count = 0;
                client_recv_into(lines, MAX_GROUPS, &count);
                
                if (count == -1) {
                    print_error(lines[0]);
                } else if (count == 0) {
                    printf("No groups matched.\n");
                    pause_for_input();
                } else {
                    for (int i = 0; i < count; i++) {
                        display_group_list_line(lines[i]);
                        printf("\n");
                    }
                    
                    printf("\nEnter group ID to join (0 to skip): ");
                    int join_id = read_int("", 0, 99999);
                    if (join_id > 0) {
                        snprintf(cmd, sizeof(cmd), "JOIN_GROUP %d", join_id);
                        char resp[CMD_BUF_LEN];
                        client_send_recv(cmd, resp, sizeof(resp));
                        if (strcmp(resp, "OK") == 0) {
                            printf("[+] Joined group successfully!\n");
                        } else {
                            print_error(resp);
                        }
                    }
                }
                pause_for_input();
                break;
            }
            case 2: {
                nh_send_to(g_sock, "LIST_ALL_GROUPS", &g_server_addr);
                print_header("All Groups");
                
                char line[CMD_BUF_LEN];
                int count = 0;
                
                while (1) {
                    if (nh_recv_from(g_sock, line, CMD_BUF_LEN) != SUCCESS) {
                        printf("\n[!] Failed to receive response. Exiting.\n");
                        nh_close(g_sock);
                        exit(1);
                    }
                    
                    if (strcmp(line, "END") == 0) break;
                    if (is_error(line)) {
                        print_error(line);
                        break;
                    }
                    display_group_list_line(line);
                    printf("\n");
                    count++;
                }
                
                if (count == 0) {
                    printf("No groups exist yet.\n");
                    pause_for_input();
                } else {
                    printf("\nEnter group ID to join (0 to skip): ");
                    int join_id = read_int("", 0, 99999);
                    if (join_id > 0) {
                        char cmd[64];
                        snprintf(cmd, sizeof(cmd), "JOIN_GROUP %d", join_id);
                        char resp[CMD_BUF_LEN];
                        client_send_recv(cmd, resp, sizeof(resp));
                        if (strcmp(resp, "OK") == 0) {
                            printf("[+] Joined group successfully!\n");
                        } else {
                            print_error(resp);
                        }
                    }
                }
                pause_for_input();
                break;
            }
            case 0:
                return;
        }
    }
}

// Create Group
void handle_create_group() {
    print_header("Create Group");
    
    char name[64], desc[256];
    read_string("Group name: ", name, sizeof(name));
    if (strlen(name) == 0) {
        printf("[!] Name cannot be empty.\n");
        return;
    }
    
    read_string("Description (optional): ", desc, sizeof(desc));
    if (strlen(desc) == 0) {
        strncpy(desc, "No description", sizeof(desc));
    }
    
    char wire_name[64], wire_desc[256];
    strncpy(wire_name, name, sizeof(wire_name) - 1);
    wire_name[sizeof(wire_name) - 1] = '\0';
    spaces_to_wire(wire_name);
    
    strncpy(wire_desc, desc, sizeof(wire_desc) - 1);
    wire_desc[sizeof(wire_desc) - 1] = '\0';
    spaces_to_wire(wire_desc);
    
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "CREATE_GROUP %s %s", wire_name, wire_desc);
    
    char response[CMD_BUF_LEN];
    client_send_recv(cmd, response, sizeof(response));
    
    if (strncmp(response, "OK", 2) == 0) {
        int group_id;
        if (sscanf(response, "OK %d", &group_id) == 1) {
            printf("[+] Group created! ID: %d\n", group_id);
            printf("    You have been added as the first member.\n");
        }
    } else {
        print_error(response);
    }
    pause_for_input();
}

// Leave Group
void handle_leave_group() {
    print_header("Leave a Group");
    
    // Show user their groups
    nh_send_to(g_sock, "LIST_MY_GROUPS", &g_server_addr);
    char line[CMD_BUF_LEN];
    int found = 0;
    
    while (1) {
        if (nh_recv_from(g_sock, line, CMD_BUF_LEN) != SUCCESS) {
            printf("\n[!] Failed to receive response. Exiting.\n");
            nh_close(g_sock);
            exit(1);
        }
        
        if (strcmp(line, "END") == 0) break;
        if (is_error(line)) {
            print_error(line);
            break;
        }
        display_group_list_line(line);
        printf("\n");
        found++;
    }
    
    if (found == 0) {
        printf("You are not in any groups.\n");
        pause_for_input();
        return;
    }
    
    int group_id = read_int("Enter group ID to leave (0 to cancel): ", 0, 99999);
    if (group_id == 0) return;
    
    char cmd[64];
    snprintf(cmd, sizeof(cmd), "LEAVE_GROUP %d", group_id);
    
    char response[CMD_BUF_LEN];
    client_send_recv(cmd, response, sizeof(response));
    
    if (strcmp(response, "OK") == 0) {
        printf("[+] You have left the group.\n");
    } else {
        print_error(response);
    }
    pause_for_input();
}

// List Users
void handle_list_users() {
    print_header("All Users");
    nh_send_to(g_sock, "LIST_USERS", &g_server_addr);
    
    char line[CMD_BUF_LEN];
    int count = 0;
    
    while (1) {
        if (nh_recv_from(g_sock, line, CMD_BUF_LEN) != SUCCESS) {
            printf("\n[!] Failed to receive response. Exiting.\n");
            nh_close(g_sock);
            exit(1);
        }
        
        if (strcmp(line, "END") == 0) break;
        if (is_error(line)) {
            print_error(line);
            break;
        }
        
        int user_id;
        char display_name[MAX_NAME_LEN], username[MAX_NAME_LEN];
        if (sscanf(line, "%d %s %s", &user_id, display_name, username) == 3) {
            wire_to_spaces(display_name);
            printf("  [%d] %-20s @%s\n", user_id, display_name, username);
            count++;
        }
    }
    
    printf("\nTotal users: %d\n", count);
    pause_for_input();
}

// Main authenticated menu
void main_menu_authenticated() {
    while (1) {
        print_header("Home");
        printf("Welcome, %s (@%s)\n\n", g_display_name, g_username);
        printf("1. My Groups\n");
        printf("2. Browse & Join Groups\n");
        printf("3. Create New Group\n");
        printf("4. Leave a Group\n");
        printf("5. View All Users\n");
        printf("0. Logout\n\n");
        
        int choice = read_int("Choice: ", 0, 5);
        
        switch (choice) {
            case 1:
                handle_my_groups();
                break;
            case 2:
                handle_browse_groups();
                break;
            case 3:
                handle_create_group();
                break;
            case 4:
                handle_leave_group();
                break;
            case 5:
                handle_list_users();
                break;
            case 0: {
                char response[CMD_BUF_LEN];
                client_send_recv("LOGOUT", response, sizeof(response));
                g_user_id = -1;
                memset(g_display_name, 0, sizeof(g_display_name));
                memset(g_username, 0, sizeof(g_username));
                printf("[+] Logged out.\n");
                return;
            }
            default:
                printf("Invalid choice.\n");
        }
    }
}

// Registration and Login
void handle_register() {
    print_header("Register");
    
    char username[64], display_name[64], password[64];
    read_string("Username (no spaces): ", username, sizeof(username));
    if (strlen(username) == 0) {
        printf("[!] Username cannot be empty.\n");
        return;
    }
    
    read_string("Display name: ", display_name, sizeof(display_name));
    if (strlen(display_name) == 0) {
        strncpy(display_name, username, sizeof(display_name));
    }
    
    read_string("Password (min 4 chars): ", password, sizeof(password));
    if (strlen(password) == 0) {
        printf("[!] Password cannot be empty.\n");
        return;
    }
    
    char wire_user[64], wire_disp[64];
    strncpy(wire_user, username, sizeof(wire_user) - 1);
    wire_user[sizeof(wire_user) - 1] = '\0';
    spaces_to_wire(wire_user);
    
    strncpy(wire_disp, display_name, sizeof(wire_disp) - 1);
    wire_disp[sizeof(wire_disp) - 1] = '\0';
    spaces_to_wire(wire_disp);
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "REGISTER %s %s %s", wire_user, wire_disp, password);
    
    char response[CMD_BUF_LEN];
    client_send_recv(cmd, response, sizeof(response));
    
    if (strncmp(response, "OK", 2) == 0) {
        int user_id;
        if (sscanf(response, "OK %d", &user_id) == 1) {
            printf("[+] Account created! Your user ID: %d\n", user_id);
            printf("    You can now log in with your username.\n");
        }
    } else {
        print_error(response);
    }
    pause_for_input();
}

void handle_login() {
    print_header("Login");
    
    char username[64], password[64];
    read_string("Username: ", username, sizeof(username));
    read_string("Password: ", password, sizeof(password));
    
    if (strlen(username) == 0 || strlen(password) == 0) {
        printf("[!] Fields cannot be empty.\n");
        return;
    }
    
    char wire_user[64];
    strncpy(wire_user, username, sizeof(wire_user) - 1);
    wire_user[sizeof(wire_user) - 1] = '\0';
    spaces_to_wire(wire_user);
    
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "LOGIN %s %s", wire_user, password);
    
    char response[CMD_BUF_LEN];
    client_send_recv(cmd, response, sizeof(response));
    
    if (strncmp(response, "OK", 2) == 0) {
        char wire_disp[64];
        if (sscanf(response, "OK %d %s", &g_user_id, wire_disp) == 2) {
            strncpy(g_display_name, wire_disp, sizeof(g_display_name) - 1);
            g_display_name[sizeof(g_display_name) - 1] = '\0';
            wire_to_spaces(g_display_name);
            
            strncpy(g_username, username, sizeof(g_username) - 1);
            g_username[sizeof(g_username) - 1] = '\0';
            
            printf("[+] Welcome back, %s!\n", g_display_name);
            pause_for_input();
            main_menu_authenticated();
        }
    } else {
        print_error(response);
        pause_for_input();
    }
}

// Main function
int main(int argc, char *argv[]) {
    const char *server_ip = SERVER_IP;
    
    // Allow server IP as command line argument
    if (argc > 1) {
        server_ip = argv[1];
        printf("[+] Using server IP: %s\n", server_ip);
    } else {
        printf("[+] Using default server IP: %s\n", server_ip);
    }
    
    // Initialize UDP client
    g_sock = nh_client_init(server_ip, SERVER_PORT, &g_server_addr);
    if (g_sock == ERR_CONN) {
        printf("============================================\n");
        printf("  Cannot connect to Group Chat UDP server.\n");
        printf("  Expected: %s:%d\n", server_ip, SERVER_PORT);
        printf("  Please start server first.\n");
        printf("============================================\n");
        return 1;
    }
    printf("[+] Connected to Group Chat UDP server at %s:%d.\n", server_ip, SERVER_PORT);
    
    // Main unauthenticated menu loop
    while (1) {
        print_header("Welcome");
        printf("1. Login\n");
        printf("2. Register\n");
        printf("0. Exit\n\n");
        
        int choice = read_int("Choice: ", 0, 2);
        
        switch (choice) {
            case 1:
                handle_login();
                break;
            case 2:
                handle_register();
                break;
            case 0: {
                char response[CMD_BUF_LEN];
                client_send_recv("QUIT", response, sizeof(response));
                nh_close(g_sock);
                printf("Goodbye.\n");
                return 0;
            }
            default:
                printf("Invalid choice.\n");
        }
    }
    
    return 0;
}
