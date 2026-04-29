#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "file_io.h"
#include "utils.h"
#include "auth.h"
#include "groups.h"
#include "messaging.h"

// NOTE: This function returns ARRAY INDEX, not msg_id
int msg_find_by_id(int msg_id) {
    for (int i = 0; i < g_msg_count; i++) {
        if (!g_messages[i].is_deleted && g_messages[i].msg_id == msg_id) {
            return i; // Return array index
        }
    }
    return ERR_NOT_FOUND;
}

int msg_get_for_group(int group_id, int *msg_indices, int max) {
    int found = 0;
    
    for (int i = 0; i < g_msg_count && found < max; i++) {
        if (!g_messages[i].is_deleted &&
            g_messages[i].group_id == group_id &&
            g_messages[i].reply_to == NO_REPLY) {
            msg_indices[found] = i;
            found++;
        }
    }
    
    return found;
}

int msg_get_replies(int parent_msg_id, int *reply_indices, int max) {
    int found = 0;
    
    for (int i = 0; i < g_msg_count && found < max; i++) {
        if (!g_messages[i].is_deleted &&
            g_messages[i].reply_to == parent_msg_id) {
            reply_indices[found] = i;
            found++;
        }
    }
    
    return found;
}

int msg_count_for_group(int group_id) {
    int count = 0;
    
    for (int i = 0; i < g_msg_count; i++) {
        if (!g_messages[i].is_deleted && g_messages[i].group_id == group_id) {
            count++;
        }
    }
    
    return count;
}

int msg_send(int group_id, const char *content, int reply_to) {
    // Check authentication
    if (!auth_is_logged_in()) return ERR_AUTH;
    
    // Validate content
    if (utils_is_empty(content)) return ERR_AUTH;
    if (strlen(content) >= MAX_MSG_LEN) return ERR_AUTH;
    
    // Validate group membership
    int gidx = grp_find_by_id(group_id);
    if (gidx == ERR_NOT_FOUND) return ERR_NOT_FOUND;
    
    int user_id = auth_get_user_id();
    if (!grp_is_member(gidx, user_id)) return ERR_PERMISSION;
    
    // Validate reply_to if not NO_REPLY
    if (reply_to != NO_REPLY) {
        int midx = msg_find_by_id(reply_to);
        if (midx == ERR_NOT_FOUND) return ERR_NOT_FOUND;
        if (g_messages[midx].group_id != group_id) {
            return ERR_NOT_FOUND; // Cannot reply to message from different group
        }
    }
    
    // Check message capacity
    if (g_msg_count >= MAX_MESSAGES) return ERR_FULL;
    
    // Populate new Message
    Message *new_msg = &g_messages[g_msg_count];
    new_msg->msg_id = utils_next_msg_id();
    new_msg->group_id = group_id;
    new_msg->sender_id = auth_get_user_id();
    new_msg->reply_to = reply_to;
    
    strncpy(new_msg->content, content, sizeof(new_msg->content) - 1);
    new_msg->content[sizeof(new_msg->content) - 1] = '\0';
    
    new_msg->is_deleted = 0;
    new_msg->sent_at = utils_now();
    
    // Increment count
    g_msg_count++;
    
    // Save to file
    if (fio_save_messages(g_messages, g_msg_count) != SUCCESS) {
        g_msg_count--; // Rollback on save failure
        return ERR_FILE;
    }
    
    return new_msg->msg_id; // Return new msg_id on success
}

int msg_delete(int msg_id) {
    // Check authentication
    if (!auth_is_logged_in()) return ERR_AUTH;
    
    // Find message
    int idx = msg_find_by_id(msg_id);
    if (idx == ERR_NOT_FOUND) return ERR_NOT_FOUND;
    
    // Check ownership
    int user_id = auth_get_user_id();
    if (g_messages[idx].sender_id != user_id) {
        return ERR_PERMISSION; // Users can only delete their own messages
    }
    
    // Soft delete
    g_messages[idx].is_deleted = 1;
    strncpy(g_messages[idx].content, "[message deleted]", MAX_MSG_LEN - 1);
    g_messages[idx].content[MAX_MSG_LEN - 1] = '\0';
    
    // Save to file
    return fio_save_messages(g_messages, g_msg_count);
}

void msg_display_thread(int group_id) {
    // Check authentication
    if (!auth_is_logged_in()) {
        printf("Not logged in.\n");
        return;
    }
    
    // Check group exists
    int gidx = grp_find_by_id(group_id);
    if (gidx == ERR_NOT_FOUND) {
        printf("Group not found.\n");
        return;
    }
    
    // Load top-level message indices
    int top_indices[MAX_MESSAGES];
    int top_count = msg_get_for_group(group_id, top_indices, MAX_MESSAGES);
    
    if (top_count == 0) {
        printf("No messages in this group yet.\n");
        return;
    }
    
    // Display each top-level message and its replies
    for (int i = 0; i < top_count; i++) {
        Message *msg = &g_messages[top_indices[i]];
        
        // Find sender display name
        const char *display_name = "Unknown";
        int user_idx = auth_find_user_by_id(msg->sender_id);
        if (user_idx >= 0) {
            display_name = g_users[user_idx].display_name;
        }
        
        // Format timestamp
        char time_buf[64];
        utils_format_time(msg->sent_at, time_buf, sizeof(time_buf));
        
        // Print top-level message
        printf("[%d] %s (%s)\n", msg->msg_id, display_name, time_buf);
        printf("    %s\n", msg->content);
        
        // Load and display replies
        int reply_indices[MAX_MESSAGES];
        int reply_count = msg_get_replies(msg->msg_id, reply_indices, MAX_MESSAGES);
        
        for (int j = 0; j < reply_count; j++) {
            Message *reply = &g_messages[reply_indices[j]];
            
            // Find reply sender display name
            const char *reply_name = "Unknown";
            int reply_user_idx = auth_find_user_by_id(reply->sender_id);
            if (reply_user_idx >= 0) {
                reply_name = g_users[reply_user_idx].display_name;
            }
            
            // Format reply timestamp
            char reply_time_buf[64];
            utils_format_time(reply->sent_at, reply_time_buf, sizeof(reply_time_buf));
            
            // Print indented reply
            printf("    \\__ [%d] %s (%s)\n", reply->msg_id, reply_name, reply_time_buf);
            printf("        %s\n", reply->content);
        }
        
        // Separator between top-level messages
        if (i < top_count - 1) {
            utils_print_separator();
        }
    }
}
