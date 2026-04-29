#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "session_manager.h"

static ClientSession sessions[MAX_CLIENTS];
static int session_count = 0;

// Compare two sockaddr_in structures
static int addr_equal(const struct sockaddr_in *a, const struct sockaddr_in *b) {
    return a->sin_family == b->sin_family &&
           a->sin_port == b->sin_port &&
           a->sin_addr.s_addr == b->sin_addr.s_addr;
}

void session_init(void) {
    memset(sessions, 0, sizeof(sessions));
    session_count = 0;
}

int session_find_index(const struct sockaddr_in *client_addr) {
    for (int i = 0; i < session_count; i++) {
        if (sessions[i].active && addr_equal(&sessions[i].client_addr, client_addr)) {
            return i;
        }
    }
    return -1;
}

int session_find_or_create(const struct sockaddr_in *client_addr) {
    int index = session_find_index(client_addr);
    
    if (index == -1) {
        // Create new session
        if (session_count >= MAX_CLIENTS) {
            return -1;
        }
        
        index = session_count;
        sessions[index].client_addr = *client_addr;
        sessions[index].user_id = -1;
        sessions[index].display_name[0] = '\0';
        sessions[index].username[0] = '\0';
        sessions[index].active = 1;
        sessions[index].last_activity = time(NULL);
        session_count++;
    }
    
    sessions[index].last_activity = time(NULL);
    return index;
}

int session_login(const struct sockaddr_in client_addr, int user_id, const char *display_name, const char *username) {
    int index = session_find_or_create(client_addr);
    if (index == -1) return -1;
    
    sessions[index].user_id = user_id;
    strncpy(sessions[index].display_name, display_name, MAX_NAME_LEN - 1);
    sessions[index].display_name[MAX_NAME_LEN - 1] = '\0';
    strncpy(sessions[index].username, username, MAX_UNAME_LEN - 1);
    sessions[index].username[MAX_UNAME_LEN - 1] = '\0';
    sessions[index].last_activity = time(NULL);
    
    return 0;
}

void session_logout(const struct sockaddr_in *client_addr) {
    int index = session_find_index(client_addr);
    if (index != -1) {
        sessions[index].active = 0;
        memset(&sessions[index], 0, sizeof(ClientSession));
    }
}

int session_get_user_id(const struct sockaddr_in *client_addr) {
    int index = session_find_index(client_addr);
    return (index != -1 && sessions[index].active) ? sessions[index].user_id : -1;
}

const char *session_get_display_name(const struct sockaddr_in *client_addr) {
    int index = session_find_index(client_addr);
    if (index != -1 && sessions[index].active) {
        return sessions[index].display_name;
    }
    return "";
}

int session_is_logged_in(const struct sockaddr_in *client_addr) {
    int index = session_find_index(client_addr);
    return (index != -1 && sessions[index].active && sessions[index].user_id != -1);
}

void session_cleanup_old(void) {
    time_t now = time(NULL);
    
    for (int i = 0; i < session_count; i++) {
        if (sessions[i].active && (now - sessions[i].last_activity > 300)) { // 5 minutes
            printf("[+] Session timeout for %s:%d\n", 
                   inet_ntoa(sessions[i].client_addr.sin_addr),
                   ntohs(sessions[i].client_addr.sin_port));
            sessions[i].active = 0;
        }
    }
}
