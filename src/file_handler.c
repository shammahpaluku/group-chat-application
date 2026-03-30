/*
 * SCS3304 Assignment 1 - Standalone Group Chat Application
 * File Handler Module Implementation
 * 
 * Handles loading and saving of all data entities using pipe-delimited text files.
 * Uses fgets() for reading and strtok() for parsing as required.
 * Only module allowed to call fopen(), fclose(), fgets(), fprintf() directly.
 */

#include "file_handler.h"

// ============================================================================
// FILE STORAGE LAYER IMPLEMENTATION
// ============================================================================

/* FUNCTION: load_users
 * PURPOSE : Load all users from users.txt into memory
 * INPUT   : users - array to store User structs
 *           count - pointer to store number of users loaded
 * OUTPUT  : None (populates users array and updates count)
 * STEPS   : 1. Open users.txt for reading
 *           2. Initialize count to 0
 *           3. Read each line with fgets()
 *           4. Parse line with strtok() using "|" delimiter
 *           5. Extract fields: user_id, username, password, display_name, is_active, created_at
 *           6. Populate User struct in array
 *           7. Increment count for each valid user
 *           8. Close file
 */
void load_users(User *users, int *count) {
    /* --- ACCEPT REQUEST --- */
    FILE *fp = fopen(USERS_FILE, "r");
    if (fp == NULL) {
        *count = 0;
        return; // File doesn't exist yet, that's OK
    }
    
    /* --- VALIDATE REQUEST --- */
    char line[256];
    *count = 0;
    
    /* --- PROCESS REQUEST --- */
    while (fgets(line, sizeof(line), fp) != NULL && *count < MAX_USERS) {
        // Remove whitespace and skip empty lines
        int i = strlen(line) - 1;
        while (i >= 0 && (line[i] == ' ' || line[i] == '\t' || line[i] == '\n' || line[i] == '\r')) {
            line[i--] = '\0';
        }
        if (strlen(line) == 0) {
            continue; // Skip empty lines
        }
        
        User *u = &users[*count];
        
        // Parse fields with strtok
        char *token = strtok(line, "|");
        if (token == NULL) continue;
        
        // Field 1: user_id
        u->user_id = atoi(token);
        
        // Field 2: username
        token = strtok(NULL, "|");
        if (token != NULL) {
            strncpy(u->username, token, MAX_NAME_LEN);
            u->username[MAX_NAME_LEN] = '\0';
        }
        
        // Field 3: password (hash)
        token = strtok(NULL, "|");
        if (token != NULL) {
            strncpy(u->password, token, MAX_PASS_LEN);
            u->password[MAX_PASS_LEN] = '\0';
        }
        
        // Field 4: display_name
        token = strtok(NULL, "|");
        if (token != NULL) {
            strncpy(u->display_name, token, MAX_NAME_LEN);
            u->display_name[MAX_NAME_LEN] = '\0';
        }
        
        // Field 5: is_active
        token = strtok(NULL, "|");
        if (token != NULL) {
            u->is_active = atoi(token);
        }
        
        // Field 6: created_at
        token = strtok(NULL, "|");
        if (token != NULL) {
            u->created_at = atol(token);
        }
        
        (*count)++;
    }
    
    /* --- SEND REPLY --- */
    fclose(fp);
}

/* FUNCTION: save_users
 * PURPOSE : Save all users from memory to users.txt
 * INPUT   : users - array of User structs
 *           count - number of users in array
 * OUTPUT  : None (writes to users.txt file)
 * STEPS   : 1. Open users.txt for writing
 *           2. Iterate through users array
 *           3. Write each user as pipe-delimited line
 *           4. Format: user_id|username|password|display_name|is_active|created_at
 *           5. Close file
 */
void save_users(User *users, int count) {
    /* --- ACCEPT REQUEST --- */
    FILE *fp = fopen(USERS_FILE, "w");
    if (fp == NULL) {
        return; // Could not open file for writing
    }
    
    /* --- PROCESS REQUEST --- */
    for (int i = 0; i < count; i++) {
        User *u = &users[i];
        fprintf(fp, "%d|%s|%s|%s|%d|%ld\n",
                u->user_id,
                u->username,
                u->password,
                u->display_name,
                u->is_active,
                u->created_at);
    }
    
    /* --- SEND REPLY --- */
    fclose(fp);
}

/* FUNCTION: load_groups
 * PURPOSE : Load all groups from groups.txt into memory
 * INPUT   : groups - array to store Group structs
 *           count - pointer to store number of groups loaded
 * OUTPUT  : None (populates groups array and updates count)
 * STEPS   : 1. Open groups.txt for reading
 *           2. Initialize count to 0
 *           3. Read each line with fgets()
 *           4. Parse line with strtok() using "|" delimiter
 *           5. Extract fields: group_id, group_name, description, creator_id
 *           6. Parse member IDs (colon-separated)
 *           7. Extract: member_count, is_active, created_at
 *           8. Populate Group struct in array
 *           9. Increment count for each valid group
 *           10. Close file
 */
void load_groups(Group *groups, int *count) {
    /* --- ACCEPT REQUEST --- */
    FILE *fp = fopen(GROUPS_FILE, "r");
    if (fp == NULL) {
        *count = 0;
        return; // File doesn't exist yet, that's OK
    }
    
    /* --- VALIDATE REQUEST --- */
    char line[512];
    *count = 0;
    
    /* --- PROCESS REQUEST --- */
    while (fgets(line, sizeof(line), fp) != NULL && *count < MAX_GROUPS) {
        // Remove whitespace and skip empty lines
        int i = strlen(line) - 1;
        while (i >= 0 && (line[i] == ' ' || line[i] == '\t' || line[i] == '\n' || line[i] == '\r')) {
            line[i--] = '\0';
        }
        if (strlen(line) == 0) {
            continue; // Skip empty lines
        }
        
        Group *g = &groups[*count];
        
        // Parse fields with strtok
        char *token = strtok(line, "|");
        if (token == NULL) continue;
        
        // Field 1: group_id
        g->group_id = atoi(token);
        
        // Field 2: group_name
        token = strtok(NULL, "|");
        if (token != NULL) {
            strncpy(g->group_name, token, MAX_NAME_LEN);
            g->group_name[MAX_NAME_LEN] = '\0';
        }
        
        // Field 3: description
        token = strtok(NULL, "|");
        if (token != NULL) {
            strncpy(g->description, token, MAX_DESC_LEN);
            g->description[MAX_DESC_LEN] = '\0';
        }
        
        // Field 4: creator_id
        token = strtok(NULL, "|");
        if (token != NULL) {
            g->creator_id = atoi(token);
        }
        
        // Field 5: member_ids (colon-separated)
        token = strtok(NULL, "|");
        if (token != NULL) {
            char *members_str = token;
            char *member_token = strtok(members_str, ":");
            g->member_count = 0;
            
            while (member_token != NULL && g->member_count < MAX_MEMBERS) {
                g->member_ids[g->member_count] = atoi(member_token);
                g->member_count++;
                member_token = strtok(NULL, ":");
            }
        }
        
        // Field 6: member_count (redundant but stored)
        token = strtok(NULL, "|");
        if (token != NULL) {
            // This field is redundant since we parse member_ids above
            // but we read it to maintain file format compatibility
            (void)token; // Suppress unused variable warning
        }
        
        // Field 7: is_active
        token = strtok(NULL, "|");
        if (token != NULL) {
            g->is_active = atoi(token);
        }
        
        // Ensure all groups are active
        g->is_active = 1;
        
        // Field 8: created_at
        token = strtok(NULL, "|");
        if (token != NULL) {
            g->created_at = atol(token);
        }
        
        (*count)++;
    }
    
    /* --- SEND REPLY --- */
    fclose(fp);
}

/* FUNCTION: save_groups
 * PURPOSE : Save all groups from memory to groups.txt
 * INPUT   : groups - array of Group structs
 *           count - number of groups in array
 * OUTPUT  : None (writes to groups.txt file)
 * STEPS   : 1. Open groups.txt for writing
 *           2. Iterate through groups array
 *           3. Write each group as pipe-delimited line
 *           4. Convert member array to colon-separated string
 *           5. Format: group_id|group_name|description|creator_id|member_ids|member_count|is_active|created_at
 *           6. Close file
 */
void save_groups(Group *groups, int count) {
    /* --- ACCEPT REQUEST --- */
    FILE *fp = fopen(GROUPS_FILE, "w");
    if (fp == NULL) {
        return; // Could not open file for writing
    }
    
    /* --- PROCESS REQUEST --- */
    for (int i = 0; i < count; i++) {
        Group *g = &groups[i];
        
        // Build member_ids string (colon-separated)
        char members_str[256] = "";
        for (int j = 0; j < g->member_count; j++) {
            char temp[16];
            sprintf(temp, "%s%d", (j > 0) ? ":" : "", g->member_ids[j]);
            strcat(members_str, temp);
        }
        
        fprintf(fp, "%d|%s|%s|%d|%s|%d|%d|%ld\n",
                g->group_id,
                g->group_name,
                g->description,
                g->creator_id,
                members_str,
                g->member_count,
                1,  // Always save as active
                g->created_at);
    }
    
    /* --- SEND REPLY --- */
    fclose(fp);
}

/* FUNCTION: load_messages
 * PURPOSE : Load all messages from messages.txt into memory
 * INPUT   : messages - array to store Message structs
 *           count - pointer to store number of messages loaded
 * OUTPUT  : None (populates messages array and updates count)
 * STEPS   : 1. Open messages.txt for reading
 *           2. Initialize count to 0
 *           3. Read each line with fgets()
 *           4. Parse line with strtok() using "|" delimiter
 *           5. Extract fields: msg_id, group_id, sender_id, reply_to, sent_at, content
 *           6. Populate Message struct in array
 *           7. Set is_deleted to 0 (active)
 *           8. Increment count for each valid message
 *           9. Close file
 */
void load_messages(Message *messages, int *count) {
    /* --- ACCEPT REQUEST --- */
    FILE *fp = fopen(MESSAGES_FILE, "r");
    if (fp == NULL) {
        *count = 0;
        return; // File doesn't exist yet, that's OK
    }
    
    /* --- VALIDATE REQUEST --- */
    char line[1024];
    *count = 0;
    
    /* --- PROCESS REQUEST --- */
    while (fgets(line, sizeof(line), fp) != NULL && *count < MAX_MESSAGES) {
        // Remove whitespace and skip empty lines
        int i = strlen(line) - 1;
        while (i >= 0 && (line[i] == ' ' || line[i] == '\t' || line[i] == '\n' || line[i] == '\r')) {
            line[i--] = '\0';
        }
        if (strlen(line) == 0) {
            continue; // Skip empty lines
        }
        
        Message *m = &messages[*count];
        
        // Parse fields with strtok
        char *token = strtok(line, "|");
        if (token == NULL) continue;
        
        // Field 1: msg_id
        m->msg_id = atoi(token);
        
        // Field 2: group_id
        token = strtok(NULL, "|");
        if (token != NULL) {
            m->group_id = atoi(token);
        }
        
        // Field 3: sender_id
        token = strtok(NULL, "|");
        if (token != NULL) {
            m->sender_id = atoi(token);
        }
        
        // Field 4: reply_to
        token = strtok(NULL, "|");
        if (token != NULL) {
            m->reply_to = atoi(token);
        }
        
        // Field 5: sent_at
        token = strtok(NULL, "|");
        if (token != NULL) {
            m->sent_at = atol(token);
        }
        
        // Field 6: content (may contain pipes, so it's last field)
        token = strtok(NULL, "");
        if (token != NULL) {
            // Remove leading | if present
            if (token[0] == '|') {
                token++;
            }
            strncpy(m->content, token, MAX_MSG_LEN);
            m->content[MAX_MSG_LEN] = '\0';
        }
        
        // Set is_deleted to 0 (active message)
        m->is_deleted = 0;
        
        (*count)++;
    }
    
    /* --- SEND REPLY --- */
    fclose(fp);
}

/* FUNCTION: save_messages
 * PURPOSE : Save all messages from memory to messages.txt
 * INPUT   : messages - array of Message structs
 *           count - number of messages in array
 * OUTPUT  : None (writes to messages.txt file)
 * STEPS   : 1. Open messages.txt for writing
 *           2. Iterate through messages array
 *           3. Write each non-deleted message as pipe-delimited line
 *           4. Format: msg_id|group_id|sender_id|reply_to|sent_at|content
 *           5. Close file
 */
void save_messages(Message *messages, int count) {
    /* --- ACCEPT REQUEST --- */
    FILE *fp = fopen(MESSAGES_FILE, "w");
    if (fp == NULL) {
        return; // Could not open file for writing
    }
    
    /* --- PROCESS REQUEST --- */
    for (int i = 0; i < count; i++) {
        Message *m = &messages[i];
        
        // Only save non-deleted messages
        if (!m->is_deleted) {
            fprintf(fp, "%d|%d|%d|%d|%ld|%s\n",
                    m->msg_id,
                    m->group_id,
                    m->sender_id,
                    m->reply_to,
                    m->sent_at,
                    m->content);
        }
    }
    
    /* --- SEND REPLY --- */
    fclose(fp);
}
