/*
 * SCS3304 Assignment 1 - Standalone Group Chat Application
 * Messaging Module Header
 * 
 * Handles message sending, replying, viewing, and message lookup functions.
 */

#ifndef MSGS_H
#define MSGS_H

#include "config.h"

// ============================================================================
// FUNCTION PROTOTYPES - MESSAGING LAYER
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
int send_message(Message *messages, int *msg_count, Group *groups, int group_count, int current_user_id, int group_id, const char *content, int reply_to);

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
int view_messages(Message *messages, int msg_count, Group *groups, int group_count, User *users, int user_count, int current_user_id, int group_id);

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
int find_message_by_id(Message *messages, int msg_count, int msg_id);

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
int search_messages(Message *messages, int msg_count, Group *groups, int group_count, User *users, int user_count, int current_user_id, int group_id, const char *keyword);

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
int edit_message(Message *messages, int msg_count, int current_user_id, int msg_id, const char *new_content);

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
int delete_message(Message *messages, int msg_count, int current_user_id, int msg_id);

#endif // MSGS_H
