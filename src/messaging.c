/*
 * SCS3304 Assignment 1 - Standalone Group Chat Application
 * Messaging Module Implementation
 * 
 * Handles message sending, replying, viewing, and message lookup functions.
 * Manages message persistence and formatting.
 */

#include "msgs.h"
#include "file_handler.h"
#include "auth.h"
#include "groups.h"

// ============================================================================
// MESSAGING LAYER IMPLEMENTATION
// ============================================================================

/* FUNCTION: send_message
 * PURPOSE : Send a message to a group (new message or reply)
 * INPUT   : messages - array of Message structs
 *           msg_count - pointer to message count (updated on success)
 *           groups - array of Group structs
 *           group_count - number of groups
 *           current_user_id - ID of user sending the message
 *           group_id - target group ID
 *           content - message text
 *           reply_to - parent message ID or NO_REPLY for new message
 * OUTPUT  : Message ID on success, error codes on failure
 * STEPS   : 1. Validate user is logged in and group exists
 *           2. Check user is group member
 *           3. Validate reply_to if specified
 *           4. Create message record
 *           5. Save to file and return message ID
 */
int send_message(Message *messages, int *msg_count, Group *groups, int group_count, int current_user_id, int group_id, const char *content, int reply_to) {
    /* --- ACCEPT REQUEST --- */
    
    /* --- VALIDATE REQUEST --- */
    if (current_user_id == -1) {
        return ERR_PERMISSION;
    }
    
    if (strlen(content) == 0) {
        return ERR_INVALID_INPUT;
    }
    
    int gidx = find_group_by_id(groups, group_count, group_id);
    if (gidx == ERR_NOT_FOUND) {
        return ERR_NOT_FOUND;
    }
    
    if (!is_member(&groups[gidx], current_user_id)) {
        return ERR_PERMISSION;
    }
    
    if (reply_to != NO_REPLY) {
        int midx = find_message_by_id(messages, *msg_count, reply_to);
        if (midx == ERR_NOT_FOUND || messages[midx].group_id != group_id) {
            return ERR_NOT_FOUND; // parent message not in this group
        }
    }
    
    if (*msg_count >= MAX_MESSAGES) {
        return ERR_FULL;
    }
    
    /* --- PROCESS REQUEST --- */
    Message *new_msg = &messages[*msg_count];
    
    // Generate unique message ID
    int max_id = 0;
    for (int i = 0; i < *msg_count; i++) {
        if (messages[i].msg_id > max_id) {
            max_id = messages[i].msg_id;
        }
    }
    new_msg->msg_id = max_id + 1;
    
    new_msg->group_id = group_id;
    new_msg->sender_id = current_user_id;
    new_msg->reply_to = reply_to;
    strncpy(new_msg->content, content, MAX_MSG_LEN);
    new_msg->content[MAX_MSG_LEN] = '\0';
    new_msg->is_deleted = 0;
    new_msg->sent_at = time(NULL);
    
    (*msg_count)++;
    
    /* --- FORMULATE REPLY --- */
    save_messages(messages, *msg_count);
    
    /* --- SEND REPLY --- */
    return new_msg->msg_id;
}

/* FUNCTION: view_messages
 * PURPOSE : Display all messages in a group with threading
 * INPUT   : messages - array of Message structs
 *           msg_count - number of messages
 *           groups - array of Group structs
 *           group_count - number of groups
 *           users - array of User structs
 *           user_count - number of users
 *           current_user_id - ID of current user
 *           group_id - group ID to view
 * OUTPUT  : SUCCESS on success, error codes on failure
 * STEPS   : 1. Validate user is logged in and group exists
 *           2. Check user is group member
 *           3. Display top-level messages
 *           4. Display replies indented under parents
 *           5. Format with timestamps and sender names
 */
int view_messages(Message *messages, int msg_count, Group *groups, int group_count, User *users, int user_count, int current_user_id, int group_id) {
    /* --- ACCEPT REQUEST --- */
    
    /* --- VALIDATE REQUEST --- */
    if (current_user_id == -1) {
        printf("Please log in first to view messages.\n");
        return ERR_PERMISSION;
    }
    
    int gidx = find_group_by_id(groups, group_count, group_id);
    if (gidx == ERR_NOT_FOUND) {
        printf("Error: Group not found.\n");
        return ERR_NOT_FOUND;
    }
    
    if (!is_member(&groups[gidx], current_user_id)) {
        printf("Error: You are not a member of this group.\n");
        return ERR_PERMISSION;
    }
    
    /* --- PROCESS REQUEST --- */
    printf("\n=== MESSAGES IN GROUP: %s ===\n", groups[gidx].group_name);
    
    // Count messages in this group
    int group_msg_count = 0;
    for (int i = 0; i < msg_count; i++) {
        if (messages[i].group_id == group_id && !messages[i].is_deleted) {
            group_msg_count++;
        }
    }
    
    if (group_msg_count == 0) {
        printf("No messages in this group yet. Be the first to say something!\n\n");
        return SUCCESS;
    }
    
    printf("Total messages: %d\n\n", group_msg_count);
    
    // First pass: display top-level messages
    int display_count = 0;
    for (int i = 0; i < msg_count; i++) {
        Message *m = &messages[i];
        
        if (m->group_id != group_id || m->is_deleted || m->reply_to != NO_REPLY) {
            continue;
        }
        
        display_count++;
        
        // Find sender username
        int sender_idx = find_user_by_id(users, user_count, m->sender_id);
        char *sender_name = "Unknown";
        if (sender_idx != ERR_NOT_FOUND) {
            sender_name = users[sender_idx].display_name;
        }
        
        // Format timestamp
        char time_buf[32];
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", localtime(&m->sent_at));
        
        /* --- FORMULATE REPLY --- */
        printf("┌─ Message #%d ───────────────────────\n", m->msg_id);
        printf("│ 👤 %s • %s\n", sender_name, time_buf);
        printf("│\n");
        printf("│ %s\n", m->content);
        printf("└─────────────────────────────────────\n\n");
        
        // Second pass: display replies for this message
        for (int j = 0; j < msg_count; j++) {
            Message *r = &messages[j];
            
            if (r->reply_to == m->msg_id && !r->is_deleted) {
                int reply_sender_idx = find_user_by_id(users, user_count, r->sender_id);
                char *reply_sender_name = "Unknown";
                if (reply_sender_idx != ERR_NOT_FOUND) {
                    reply_sender_name = users[reply_sender_idx].display_name;
                }
                
                char reply_time_buf[32];
                strftime(reply_time_buf, sizeof(reply_time_buf), "%Y-%m-%d %H:%M:%S", localtime(&r->sent_at));
                
                printf("  ├─ Reply #%d ────────────────────\n", r->msg_id);
                printf("  │ 👤 %s • %s\n", reply_sender_name, reply_time_buf);
                printf("  │\n");
                printf("  │ %s\n", r->content);
                printf("  └─────────────────────────────────\n\n");
            }
        }
    }
    
    /* --- SEND REPLY --- */
    return SUCCESS;
}

/* FUNCTION: find_message_by_id
 * PURPOSE : Find message index by message ID
 * INPUT   : messages - array of Message structs
 *           msg_count - number of messages in array
 *           msg_id - message ID to search for
 * OUTPUT  : Array index if found, ERR_NOT_FOUND if not found
 * STEPS   : 1. Iterate through messages array
 *           2. For each non-deleted message, compare message IDs
 *           3. Return index if match found
 *           4. Return ERR_NOT_FOUND if no match
 */
int find_message_by_id(Message *messages, int msg_count, int msg_id) {
    /* --- ACCEPT REQUEST --- */
    
    /* --- PROCESS REQUEST --- */
    for (int i = 0; i < msg_count; i++) {
        if (!messages[i].is_deleted && messages[i].msg_id == msg_id) {
            /* --- SEND REPLY --- */
            return i;
        }
    }
    
    /* --- SEND REPLY --- */
    return ERR_NOT_FOUND;
}

/* FUNCTION: search_messages
 * PURPOSE : Search for messages in a group by keyword or sender
 * INPUT   : messages - array of Message structs
 *           msg_count - number of messages
 *           groups - array of Group structs
 *           group_count - number of groups
 *           users - array of User structs
 *           user_count - number of users
 *           current_user_id - ID of current user
 *           group_id - group ID to search in
 *           keyword - search term (empty = all messages)
 * OUTPUT  : SUCCESS on success, error codes on failure
 * STEPS   : 1. Validate user is logged in and group exists
 *           2. Check user is group member
 *           3. Search messages by keyword (case-insensitive)
 *           4. Display matching messages with context
 */
int search_messages(Message *messages, int msg_count, Group *groups, int group_count, User *users, int user_count, int current_user_id, int group_id, const char *keyword) {
    /* --- ACCEPT REQUEST --- */
    
    /* --- VALIDATE REQUEST --- */
    if (current_user_id == -1) {
        printf("Please log in first to search messages.\n");
        return ERR_PERMISSION;
    }
    
    int gidx = find_group_by_id(groups, group_count, group_id);
    if (gidx == ERR_NOT_FOUND) {
        printf("Error: Group not found.\n");
        return ERR_NOT_FOUND;
    }
    
    if (!is_member(&groups[gidx], current_user_id)) {
        printf("Error: You are not a member of this group.\n");
        return ERR_PERMISSION;
    }
    
    /* --- PROCESS REQUEST --- */
    printf("\n=== SEARCH RESULTS IN GROUP: %s ===\n", groups[gidx].group_name);
    
    char keyword_lower[MAX_MSG_LEN + 1] = "";
    if (keyword != NULL && strlen(keyword) > 0) {
        for (int i = 0; keyword[i] && i < MAX_MSG_LEN; i++) {
            keyword_lower[i] = tolower(keyword[i]);
        }
        keyword_lower[strlen(keyword)] = '\0';
        printf("Searching for: '%s'\n\n", keyword);
    } else {
        printf("Showing all messages\n\n");
    }
    
    int found = 0;
    for (int i = 0; i < msg_count; i++) {
        Message *m = &messages[i];
        
        if (m->group_id != group_id || m->is_deleted) {
            continue;
        }
        
        // Check if message matches keyword
        if (strlen(keyword_lower) > 0) {
            char content_lower[MAX_MSG_LEN + 1];
            for (int j = 0; m->content[j] && j < MAX_MSG_LEN; j++) {
                content_lower[j] = tolower(m->content[j]);
            }
            content_lower[strlen(m->content)] = '\0';
            
            if (strstr(content_lower, keyword_lower) == NULL) {
                continue; // Skip if keyword not found
            }
        }
        
        // Find sender username
        int sender_idx = find_user_by_id(users, user_count, m->sender_id);
        char *sender_name = "Unknown";
        if (sender_idx != ERR_NOT_FOUND) {
            sender_name = users[sender_idx].display_name;
        }
        
        // Format timestamp
        char time_buf[32];
        strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", localtime(&m->sent_at));
        
        /* --- FORMULATE REPLY --- */
        printf("┌─ Message #%d ───────────────────────\n", m->msg_id);
        printf("│ 👤 %s • %s\n", sender_name, time_buf);
        printf("│\n");
        printf("│ %s\n", m->content);
        printf("└─────────────────────────────────────\n\n");
        
        found++;
    }
    
    if (found == 0) {
        printf("No messages found");
        if (strlen(keyword_lower) > 0) {
            printf(" matching '%s'", keyword);
        }
        printf(".\n\n");
    } else {
        printf("Found %d message%s\n\n", found, found == 1 ? "" : "s");
    }
    
    /* --- SEND REPLY --- */
    return SUCCESS;
}

/* FUNCTION: edit_message
 * PURPOSE : Edit an existing message (only by original sender)
 * INPUT   : messages - array of Message structs
 *           msg_count - number of messages
 *           current_user_id - ID of user editing the message
 *           msg_id - ID of message to edit
 *           new_content - new message content
 * OUTPUT  : SUCCESS on success, error codes on failure
 * STEPS   : 1. Validate user is logged in
 *           2. Find message and verify ownership
 *           3. Update message content and save
 */
int edit_message(Message *messages, int msg_count, int current_user_id, int msg_id, const char *new_content) {
    /* --- ACCEPT REQUEST --- */
    
    /* --- VALIDATE REQUEST --- */
    if (current_user_id == -1) {
        printf("Please log in first to edit messages.\n");
        return ERR_PERMISSION;
    }
    
    if (strlen(new_content) == 0) {
        printf("Error: Message content cannot be empty.\n");
        return ERR_INVALID_INPUT;
    }
    
    int midx = find_message_by_id(messages, msg_count, msg_id);
    if (midx == ERR_NOT_FOUND) {
        printf("Error: Message not found.\n");
        return ERR_NOT_FOUND;
    }
    
    if (messages[midx].sender_id != current_user_id) {
        printf("Error: You can only edit your own messages.\n");
        return ERR_PERMISSION;
    }
    
    if (messages[midx].is_deleted) {
        printf("Error: Cannot edit a deleted message.\n");
        return ERR_NOT_FOUND;
    }
    
    /* --- PROCESS REQUEST --- */
    strncpy(messages[midx].content, new_content, MAX_MSG_LEN);
    messages[midx].content[MAX_MSG_LEN] = '\0';
    
    /* --- FORMULATE REPLY --- */
    save_messages(messages, msg_count);
    
    printf("[+] Message #%d updated successfully.\n", msg_id);
    
    /* --- SEND REPLY --- */
    return SUCCESS;
}

/* FUNCTION: delete_message
 * PURPOSE : Delete an existing message (only by original sender)
 * INPUT   : messages - array of Message structs
 *           msg_count - number of messages
 *           current_user_id - ID of user deleting the message
 *           msg_id - ID of message to delete
 * OUTPUT  : SUCCESS on success, error codes on failure
 * STEPS   : 1. Validate user is logged in
 *           2. Find message and verify ownership
 *           3. Mark message as deleted and save
 */
int delete_message(Message *messages, int msg_count, int current_user_id, int msg_id) {
    /* --- ACCEPT REQUEST --- */
    
    /* --- VALIDATE REQUEST --- */
    if (current_user_id == -1) {
        printf("Please log in first to delete messages.\n");
        return ERR_PERMISSION;
    }
    
    int midx = find_message_by_id(messages, msg_count, msg_id);
    if (midx == ERR_NOT_FOUND) {
        printf("Error: Message not found.\n");
        return ERR_NOT_FOUND;
    }
    
    if (messages[midx].sender_id != current_user_id) {
        printf("Error: You can only delete your own messages.\n");
        return ERR_PERMISSION;
    }
    
    if (messages[midx].is_deleted) {
        printf("Error: Message is already deleted.\n");
        return ERR_NOT_FOUND;
    }
    
    /* --- PROCESS REQUEST --- */
    messages[midx].is_deleted = 1;
    
    /* --- FORMULATE REPLY --- */
    save_messages(messages, msg_count);
    
    printf("[+] Message #%d deleted successfully.\n", msg_id);
    
    /* --- SEND REPLY --- */
    return SUCCESS;
}
