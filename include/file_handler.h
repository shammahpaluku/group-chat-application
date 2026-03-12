/*
 * SCS3304 Assignment 1 - Standalone Group Chat Application
 * File Handler Module Header
 * 
 * Handles all file I/O operations. Only module allowed to call fopen(),
 * fclose(), fgets(), fprintf() directly.
 */

#ifndef FILE_HANDLER_H
#define FILE_HANDLER_H

#include "config.h"

// ============================================================================
// FUNCTION PROTOTYPES - FILE STORAGE LAYER
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
void load_users(User *users, int *count);

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
void save_users(User *users, int count);

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
void load_groups(Group *groups, int *count);

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
void save_groups(Group *groups, int count);

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
void load_messages(Message *messages, int *count);

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
void save_messages(Message *messages, int count);

#endif // FILE_HANDLER_H
