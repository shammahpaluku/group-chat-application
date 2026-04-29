#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include "shared_memory.h"
#include "file_io.h"
#include "utils.h"

// Global shared memory instances
SharedUsers *g_shared_users = NULL;
SharedGroups *g_shared_groups = NULL;
SharedMessages *g_shared_messages = NULL;

int shared_memory_init(void) {
    // Allocate shared memory
    g_shared_users = malloc(sizeof(SharedUsers));
    g_shared_groups = malloc(sizeof(SharedGroups));
    g_shared_messages = malloc(sizeof(SharedMessages));
    
    if (!g_shared_users || !g_shared_groups || !g_shared_messages) {
        return ERR_FILE;
    }
    
    // Initialize mutexes
    pthread_mutex_init(&g_shared_users->users_mutex, NULL);
    pthread_mutex_init(&g_shared_groups->groups_mutex, NULL);
    pthread_mutex_init(&g_shared_messages->messages_mutex, NULL);
    
    // Initialize counts
    g_shared_users->user_count = 0;
    g_shared_groups->group_count = 0;
    g_shared_messages->message_count = 0;
    
    return shared_memory_load_from_files();
}

void shared_memory_destroy(void) {
    if (g_shared_users) {
        pthread_mutex_destroy(&g_shared_users->users_mutex);
        free(g_shared_users);
    }
    if (g_shared_groups) {
        pthread_mutex_destroy(&g_shared_groups->groups_mutex);
        free(g_shared_groups);
    }
    if (g_shared_messages) {
        pthread_mutex_destroy(&g_shared_messages->messages_mutex);
        free(g_shared_messages);
    }
}

int shared_memory_load_from_files(void) {
    // Load users
    pthread_mutex_lock(&g_shared_users->users_mutex);
    g_shared_users->user_count = fio_load_users(g_shared_users->users, MAX_USERS);
    pthread_mutex_unlock(&g_shared_users->users_mutex);
    
    // Load groups
    pthread_mutex_lock(&g_shared_groups->groups_mutex);
    g_shared_groups->group_count = fio_load_groups(g_shared_groups->groups, MAX_GROUPS);
    pthread_mutex_unlock(&g_shared_groups->groups_mutex);
    
    // Load messages
    pthread_mutex_lock(&g_shared_messages->messages_mutex);
    g_shared_messages->message_count = fio_load_messages(g_shared_messages->messages, MAX_MESSAGES);
    pthread_mutex_unlock(&g_shared_messages->messages_mutex);
    
    return SUCCESS;
}

int shared_memory_save_to_files(void) {
    int result = SUCCESS;
    
    // Save users
    pthread_mutex_lock(&g_shared_users->users_mutex);
    if (fio_save_users(g_shared_users->users, g_shared_users->user_count) != SUCCESS) {
        result = ERR_FILE;
    }
    pthread_mutex_unlock(&g_shared_users->users_mutex);
    
    // Save groups
    pthread_mutex_lock(&g_shared_groups->groups_mutex);
    if (fio_save_groups(g_shared_groups->groups, g_shared_groups->group_count) != SUCCESS) {
        result = ERR_FILE;
    }
    pthread_mutex_unlock(&g_shared_groups->groups_mutex);
    
    // Save messages
    pthread_mutex_lock(&g_shared_messages->messages_mutex);
    if (fio_save_messages(g_shared_messages->messages, g_shared_messages->message_count) != SUCCESS) {
        result = ERR_FILE;
    }
    pthread_mutex_unlock(&g_shared_messages->messages_mutex);
    
    return result;
}

int shared_add_user(const User *user) {
    pthread_mutex_lock(&g_shared_users->users_mutex);
    
    if (g_shared_users->user_count >= MAX_USERS) {
        pthread_mutex_unlock(&g_shared_users->users_mutex);
        return ERR_FULL;
    }
    
    // Check for duplicate username
    for (int i = 0; i < g_shared_users->user_count; i++) {
        if (strcmp(g_shared_users->users[i].username, user->username) == 0) {
            pthread_mutex_unlock(&g_shared_users->users_mutex);
            return ERR_DUPLICATE;
        }
    }
    
    // Add user
    g_shared_users->users[g_shared_users->user_count] = *user;
    g_shared_users->user_count++;
    
    pthread_mutex_unlock(&g_shared_users->users_mutex);
    return SUCCESS;
}

int shared_find_user(const char *username) {
    pthread_mutex_lock(&g_shared_users->users_mutex);
    
    for (int i = 0; i < g_shared_users->user_count; i++) {
        if (strcmp(g_shared_users->users[i].username, username) == 0) {
            pthread_mutex_unlock(&g_shared_users->users_mutex);
            return i;
        }
    }
    
    pthread_mutex_unlock(&g_shared_users->users_mutex);
    return ERR_NOT_FOUND;
}

int shared_add_group(const Group *group) {
    pthread_mutex_lock(&g_shared_groups->groups_mutex);
    
    if (g_shared_groups->group_count >= MAX_GROUPS) {
        pthread_mutex_unlock(&g_shared_groups->groups_mutex);
        return ERR_FULL;
    }
    
    // Check for duplicate group name
    for (int i = 0; i < g_shared_groups->group_count; i++) {
        if (strcmp(g_shared_groups->groups[i].group_name, group->group_name) == 0) {
            pthread_mutex_unlock(&g_shared_groups->groups_mutex);
            return ERR_DUPLICATE;
        }
    }
    
    // Add group
    g_shared_groups->groups[g_shared_groups->group_count] = *group;
    g_shared_groups->group_count++;
    
    pthread_mutex_unlock(&g_shared_groups->groups_mutex);
    return SUCCESS;
}

int shared_find_group(const char *group_name) {
    pthread_mutex_lock(&g_shared_groups->groups_mutex);
    
    for (int i = 0; i < g_shared_groups->group_count; i++) {
        if (strcmp(g_shared_groups->groups[i].group_name, group_name) == 0) {
            pthread_mutex_unlock(&g_shared_groups->groups_mutex);
            return i;
        }
    }
    
    pthread_mutex_unlock(&g_shared_groups->groups_mutex);
    return ERR_NOT_FOUND;
}

int shared_add_message(const Message *message) {
    pthread_mutex_lock(&g_shared_messages->messages_mutex);
    
    if (g_shared_messages->message_count >= MAX_MESSAGES) {
        pthread_mutex_unlock(&g_shared_messages->messages_mutex);
        return ERR_FULL;
    }
    
    // Add message
    g_shared_messages->messages[g_shared_messages->message_count] = *message;
    g_shared_messages->message_count++;
    
    pthread_mutex_unlock(&g_shared_messages->messages_mutex);
    return SUCCESS;
}

void shared_get_groups_by_user(int user_id, Group *result, int *count) {
    *count = 0;
    pthread_mutex_lock(&g_shared_groups->groups_mutex);
    
    for (int i = 0; i < g_shared_groups->group_count; i++) {
        // Check if user is a member of this group
        for (int j = 0; j < g_shared_groups->groups[i].member_count; j++) {
            if (g_shared_groups->groups[i].member_ids[j] == user_id) {
                result[*count] = g_shared_groups->groups[i];
                (*count)++;
                break;
            }
        }
    }
    
    pthread_mutex_unlock(&g_shared_groups->groups_mutex);
}

void shared_get_messages_by_group(int group_id, Message *result, int *count) {
    *count = 0;
    pthread_mutex_lock(&g_shared_messages->messages_mutex);
    
    for (int i = 0; i < g_shared_messages->message_count; i++) {
        if (g_shared_messages->messages[i].group_id == group_id) {
            result[*count] = g_shared_messages->messages[i];
            (*count)++;
        }
    }
    
    pthread_mutex_unlock(&g_shared_messages->messages_mutex);
}
