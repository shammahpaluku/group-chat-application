/*
 * SCS3304 Assignment 1 - Standalone Group Chat Application
 * Authentication Module Header
 * 
 * Handles user registration, login, logout, and user lookup functions.
 */

#ifndef AUTH_H
#define AUTH_H

#include "config.h"

// ============================================================================
// FUNCTION PROTOTYPES - AUTHENTICATION LAYER
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
unsigned int hash_password(const char *password);

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
int register_user(User *users, int *count, int *current_user_id, const char *username, const char *display_name, const char *password);

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
int login_user(User *users, int user_count, int *current_user_id, const char *username, const char *password);

/* FUNCTION: logout_user
 * PURPOSE : Log out current user and clear session
 * INPUT   : current_user_id - pointer to current session user ID
 * OUTPUT  : SUCCESS
 * STEPS   : 1. Set current_user_id to -1 (no user logged in)
 *           2. Return SUCCESS
 */
int logout_user(int *current_user_id);

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
int find_user_by_name(User *users, int count, const char *username);

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
int find_user_by_id(User *users, int count, int user_id);

#endif // AUTH_H
