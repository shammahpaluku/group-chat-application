/*
 * SCS3304 Assignment 1 - Standalone Group Chat Application
 * Authentication Module Implementation
 * 
 * Handles user registration, login, logout, and user lookup functions.
 * Uses djb2 hash algorithm for password security.
 */

#include "auth.h"
#include "file_handler.h"

// ============================================================================
// AUTHENTICATION LAYER IMPLEMENTATION
// ============================================================================

/* FUNCTION: hash_password
 * PURPOSE : Hash a password using the djb2 algorithm
 * INPUT   : password - plain text password to hash
 * OUTPUT  : Unsigned integer hash value
 * STEPS   : 1. Initialize hash to 5381
 *           2. For each character in password:
 *              hash = ((hash << 5) + hash) + character
 *              (equivalent to hash * 33 + character)
 *           3. Return final hash value
 */
unsigned int hash_password(const char *password) {
    /* --- PROCESS REQUEST --- */
    unsigned int hash = 5381;
    int c;
    
    while ((c = *password++)) {
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    }
    
    /* --- SEND REPLY --- */
    return hash;
}

/* FUNCTION: register_user
 * PURPOSE : Register a new user with username, display name, and password
 * INPUT   : users - array of User structs
 *           count - pointer to user count (updated on success)
 *           current_user_id - pointer to current session user ID
 *           username - unique login handle
 *           display_name - friendly name shown in messages
 *           password - plain text password (will be hashed)
 * OUTPUT  : SUCCESS on success, ERR_DUPLICATE if username exists, 
 *           ERR_FULL if user limit reached, ERR_INVALID_INPUT if invalid
 * STEPS   : 1. Validate input (username not empty, password >= 4 chars)
 *           2. Check if username already exists
 *           3. Check user capacity limit
 *           4. Create new user with hashed password
 *           5. Save to file and return SUCCESS
 */
int register_user(User *users, int *count, int *current_user_id, const char *username, const char *display_name, const char *password) {
    /* --- ACCEPT REQUEST --- */
    
    /* --- VALIDATE REQUEST --- */
    if (strlen(username) == 0 || strlen(password) < 4) {
        return ERR_INVALID_INPUT;
    }
    
    if (find_user_by_name(users, *count, username) != ERR_NOT_FOUND) {
        return ERR_DUPLICATE; // username already taken
    }
    
    if (*count >= MAX_USERS) {
        return ERR_FULL; // storage limit reached
    }
    
    /* --- PROCESS REQUEST --- */
    User *new_user = &users[*count];
    new_user->user_id = 1; // Will be updated by caller with proper ID generation
    strncpy(new_user->username, username, MAX_NAME_LEN);
    new_user->username[MAX_NAME_LEN] = '\0';
    strncpy(new_user->display_name, display_name, MAX_NAME_LEN);
    new_user->display_name[MAX_NAME_LEN] = '\0';
    
    // Hash password
    unsigned int hash = hash_password(password);
    snprintf(new_user->password, MAX_PASS_LEN + 1, "%u", hash);
    
    new_user->is_active = 1;
    new_user->created_at = time(NULL);
    
    (*count)++;
    
    /* --- FORMULATE REPLY --- */
    save_users(users, *count);
    
    /* --- SEND REPLY --- */
    return SUCCESS;
}

/* FUNCTION: login_user
 * PURPOSE : Authenticate user with username and password
 * INPUT   : users - array of User structs
 *           user_count - number of users in array
 *           current_user_id - pointer to current session user ID
 *           username - login handle
 *           password - plain text password
 * OUTPUT  : SUCCESS on success, ERR_NOT_FOUND if user not found,
 *           ERR_AUTH if password incorrect
 * STEPS   : 1. Find user by username
 *           2. Hash provided password
 *           3. Compare with stored hash
 *           4. Set current user ID on success
 */
int login_user(User *users, int user_count, int *current_user_id, const char *username, const char *password) {
    /* --- ACCEPT REQUEST --- */
    
    /* --- VALIDATE REQUEST --- */
    int idx = find_user_by_name(users, user_count, username);
    if (idx == ERR_NOT_FOUND) {
        return ERR_NOT_FOUND;
    }
    
    /* --- PROCESS REQUEST --- */
    unsigned int computed_hash = hash_password(password);
    char stored_hash[MAX_PASS_LEN + 1];
    snprintf(stored_hash, sizeof(stored_hash), "%u", computed_hash);
    
    if (strcmp(users[idx].password, stored_hash) != 0) {
        return ERR_AUTH; // wrong password
    }
    
    /* --- FORMULATE REPLY --- */
    *current_user_id = users[idx].user_id;
    
    /* --- SEND REPLY --- */
    return SUCCESS;
}

/* FUNCTION: logout_user
 * PURPOSE : Log out current user and clear session
 * INPUT   : current_user_id - pointer to current session user ID
 * OUTPUT  : SUCCESS
 * STEPS   : 1. Set current_user_id to -1 (no user logged in)
 *           2. Return SUCCESS
 */
int logout_user(int *current_user_id) {
    /* --- ACCEPT REQUEST --- */
    
    /* --- PROCESS REQUEST --- */
    *current_user_id = -1;
    
    /* --- SEND REPLY --- */
    return SUCCESS;
}

/* FUNCTION: find_user_by_name
 * PURPOSE : Find user index by username (case-insensitive)
 * INPUT   : users - array of User structs
 *           count - number of users in array
 *           username - username to search for
 * OUTPUT  : Array index if found, ERR_NOT_FOUND if not found
 * STEPS   : 1. Iterate through users array
 *           2. For each active user, compare usernames case-insensitively
 *           3. Return index if match found
 *           4. Return ERR_NOT_FOUND if no match
 */
int find_user_by_name(User *users, int count, const char *username) {
    /* --- ACCEPT REQUEST --- */
    
    /* --- PROCESS REQUEST --- */
    for (int i = 0; i < count; i++) {
        if (users[i].is_active && strcasecmp(users[i].username, username) == 0) {
            /* --- SEND REPLY --- */
            return i;
        }
    }
    
    /* --- SEND REPLY --- */
    return ERR_NOT_FOUND;
}

/* FUNCTION: find_user_by_id
 * PURPOSE : Find user index by user ID
 * INPUT   : users - array of User structs
 *           count - number of users in array
 *           user_id - user ID to search for
 * OUTPUT  : Array index if found, ERR_NOT_FOUND if not found
 * STEPS   : 1. Iterate through users array
 *           2. For each active user, compare user IDs
 *           3. Return index if match found
 *           4. Return ERR_NOT_FOUND if no match
 */
int find_user_by_id(User *users, int count, int user_id) {
    /* --- ACCEPT REQUEST --- */
    
    /* --- PROCESS REQUEST --- */
    for (int i = 0; i < count; i++) {
        if (users[i].is_active && users[i].user_id == user_id) {
            /* --- SEND REPLY --- */
            return i;
        }
    }
    
    /* --- SEND REPLY --- */
    return ERR_NOT_FOUND;
}
