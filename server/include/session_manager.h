#ifndef SESSION_MANAGER_H
#define SESSION_MANAGER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "config.h"

// Client session tracking for UDP
typedef struct {
    struct sockaddr_in client_addr;
    int user_id;
    char display_name[MAX_NAME_LEN];
    char username[MAX_UNAME_LEN];
    time_t last_activity;
    int active;
} ClientSession;

#define MAX_CLIENTS 100

// Session management functions
void session_init(void);
int session_find_or_create(const struct sockaddr_in *client_addr);
int session_login(const struct sockaddr_in client_addr, int user_id, const char *display_name, const char *username);
void session_logout(const struct sockaddr_in *client_addr);
int session_get_user_id(const struct sockaddr_in *client_addr);
const char *session_get_display_name(const struct sockaddr_in *client_addr);
int session_is_logged_in(const struct sockaddr_in *client_addr);
void session_cleanup_old(void);

#endif // SESSION_MANAGER_H
