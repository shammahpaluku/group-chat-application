#ifndef SHARED_MEMORY_H
#define SHARED_MEMORY_H

#include <pthread.h>
#include "config.h"

// Shared data structures
typedef struct {
    User users[MAX_USERS];
    int user_count;
    pthread_mutex_t users_mutex;
} SharedUsers;

typedef struct {
    Group groups[MAX_GROUPS];
    int group_count;
    pthread_mutex_t groups_mutex;
} SharedGroups;

typedef struct {
    Message messages[MAX_MESSAGES];
    int message_count;
    pthread_mutex_t messages_mutex;
} SharedMessages;

// Global shared memory
extern SharedUsers *g_shared_users;
extern SharedGroups *g_shared_groups;
extern SharedMessages *g_shared_messages;

// Shared memory functions
int shared_memory_init(void);
void shared_memory_destroy(void);
int shared_memory_load_from_files(void);
int shared_memory_save_to_files(void);

// Thread-safe access functions
int shared_add_user(const User *user);
int shared_find_user(const char *username);
int shared_add_group(const Group *group);
int shared_find_group(const char *group_name);
int shared_add_message(const Message *message);
void shared_get_groups_by_user(int user_id, Group *result, int *count);
void shared_get_messages_by_group(int group_id, Message *result, int *count);

#endif // SHARED_MEMORY_H
