#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "file_io.h"
#include "utils.h"
#include "auth.h"

// Static session state
static Session current_session = {0, -1, ""};

// User lookup helpers
// NOTE: These functions return ARRAY INDICES, not user_ids
int auth_find_user_by_name(const char *username) {
    if (!username) return ERR_NOT_FOUND;
    
    char search_lower[MAX_UNAME_LEN];
    strncpy(search_lower, username, sizeof(search_lower) - 1);
    search_lower[sizeof(search_lower) - 1] = '\0';
    utils_to_lowercase(search_lower);
    
    for (int i = 0; i < g_user_count; i++) {
        if (!g_users[i].is_active) continue;
        
        char user_lower[MAX_UNAME_LEN];
        strncpy(user_lower, g_users[i].username, sizeof(user_lower) - 1);
        user_lower[sizeof(user_lower) - 1] = '\0';
        utils_to_lowercase(user_lower);
        
        if (strcmp(user_lower, search_lower) == 0) {
            return i; // Return array index
        }
    }
    
    return ERR_NOT_FOUND;
}

// NOTE: This function returns ARRAY INDEX, not user_id
int auth_find_user_by_id(int user_id) {
    for (int i = 0; i < g_user_count; i++) {
        if (!g_users[i].is_active) continue;
        if (g_users[i].user_id == user_id) {
            return i; // Return array index
        }
    }
    
    return ERR_NOT_FOUND;
}

int auth_user_exists(const char *username) {
    return auth_find_user_by_name(username) >= 0;
}

// Registration and login
int auth_register(const char *username, const char *display_name, const char *plain_password) {
    // Validate inputs
    if (utils_is_empty(username) || utils_is_empty(display_name) || utils_is_empty(plain_password)) {
        return ERR_AUTH;
    }
    
    if (strlen(plain_password) < 4) {
        return ERR_AUTH;
    }
    
    // Check for duplicate username
    if (auth_user_exists(username)) {
        return ERR_DUPLICATE;
    }
    
    // Check capacity
    if (g_user_count >= MAX_USERS) {
        return ERR_FULL;
    }
    
    // Hash password
    char hashed[MAX_NAME_LEN];
    utils_hash_password(plain_password, hashed, sizeof(hashed));
    
    // Populate new User record
    User *new_user = &g_users[g_user_count];
    new_user->user_id = utils_next_user_id();
    
    // Store username in lowercase
    strncpy(new_user->username, username, sizeof(new_user->username) - 1);
    new_user->username[sizeof(new_user->username) - 1] = '\0';
    utils_to_lowercase(new_user->username);
    
    strncpy(new_user->password, hashed, sizeof(new_user->password) - 1);
    new_user->password[sizeof(new_user->password) - 1] = '\0';
    
    strncpy(new_user->display_name, display_name, sizeof(new_user->display_name) - 1);
    new_user->display_name[sizeof(new_user->display_name) - 1] = '\0';
    
    new_user->is_active = 1;
    new_user->created_at = utils_now();
    
    // Increment count
    g_user_count++;
    
    // Save to file
    if (fio_save_users(g_users, g_user_count) != SUCCESS) {
        g_user_count--; // Rollback on save failure
        return ERR_FILE;
    }
    
    return new_user->user_id; // Return new user_id on success
}

int auth_login(const char *username, const char *plain_password) {
    int idx = auth_find_user_by_name(username);
    if (idx == ERR_NOT_FOUND) {
        return ERR_NOT_FOUND;
    }
    
    // Hash the provided password
    char hashed[MAX_NAME_LEN];
    utils_hash_password(plain_password, hashed, sizeof(hashed));
    
    // Compare with stored hash
    if (strcmp(hashed, g_users[idx].password) != 0) {
        return ERR_AUTH;
    }
    
    // Populate current_session
    current_session.logged_in = 1;
    current_session.user_id = g_users[idx].user_id;
    strncpy(current_session.display_name, g_users[idx].display_name, sizeof(current_session.display_name) - 1);
    current_session.display_name[sizeof(current_session.display_name) - 1] = '\0';
    
    return g_users[idx].user_id; // Return logged-in user's id
}

void auth_logout(void) {
    memset(&current_session, 0, sizeof(Session));
    current_session.user_id = -1;
}

// Session queries
int auth_is_logged_in(void) {
    return current_session.logged_in;
}

int auth_get_user_id(void) {
    return current_session.user_id;
}

const char *auth_get_display_name(void) {
    return current_session.display_name;
}
