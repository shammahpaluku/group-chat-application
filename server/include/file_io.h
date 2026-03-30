#ifndef FILE_IO_H
#define FILE_IO_H

#include "config.h"

// Global in-memory arrays (extern declarations)
extern User     g_users[MAX_USERS];
extern Group    g_groups[MAX_GROUPS];
extern Message  g_messages[MAX_MESSAGES];
extern int      g_user_count;
extern int      g_group_count;
extern int      g_msg_count;

// File initialisation
int fio_init_files(void);

// Bulk operations
int fio_load_all(void);
int fio_save_all(void);

// User file operations
int fio_load_users(User *users, int max);
int fio_save_users(const User *users, int count);

// Group file operations
int fio_load_groups(Group *groups, int max);
int fio_save_groups(const Group *groups, int count);

// Message file operations
int fio_load_messages(Message *messages, int max);
int fio_save_messages(const Message *messages, int count);

#endif // FILE_IO_H
