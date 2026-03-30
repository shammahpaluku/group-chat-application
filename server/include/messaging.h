#ifndef MESSAGING_H
#define MESSAGING_H

// Core message operations
int  msg_send(int group_id, const char *content, int reply_to);
int  msg_delete(int msg_id);
int  msg_find_by_id(int msg_id);

// Message retrieval
int  msg_get_for_group(int group_id, int *msg_indices, int max);
int  msg_get_replies(int parent_msg_id, int *reply_indices, int max);
int  msg_count_for_group(int group_id);

// Display helpers (server-side terminal only)
void msg_display_thread(int group_id);

#endif // MESSAGING_H
