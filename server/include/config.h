#ifndef CONFIG_H
#define CONFIG_H

// File paths
#define USERS_FILE    "data/users.dat"
#define GROUPS_FILE   "data/groups.dat"
#define MESSAGES_FILE "data/messages.dat"

// Size limits
#define MAX_USERS       100
#define MAX_GROUPS      50
#define MAX_MESSAGES    1000
#define MAX_MEMBERS     50
#define MAX_MSG_LEN     512
#define MAX_NAME_LEN    32
#define MAX_UNAME_LEN   32
#define MAX_DESC_LEN    128
#define MAX_LINE_LEN    1024

// Server config
#define SERVER_IP       "127.0.0.1"
#define SERVER_PORT     9200
#define CMD_BUF_LEN     1024
#define BACKLOG         5

// Return codes
#define SUCCESS         0
#define ERR_FILE       -1
#define ERR_NOT_FOUND  -2
#define ERR_DUPLICATE  -3
#define ERR_AUTH       -4
#define ERR_FULL       -5
#define ERR_PERMISSION -6
#define ERR_CONN       -7
#define ERR_UNKNOWN    -8

// Special value
#define NO_REPLY       -1   // used in Message.reply_to

// Data structures

typedef struct {
    int    user_id;
    char   username[MAX_UNAME_LEN];
    char   password[MAX_NAME_LEN];   // stores djb2 hash as string
    char   display_name[MAX_NAME_LEN];
    int    is_active;                // 1=active, 0=soft-deleted
    long   created_at;               // unix timestamp
} User;

typedef struct {
    int    group_id;
    char   group_name[MAX_NAME_LEN];
    char   description[MAX_DESC_LEN];
    int    creator_id;
    int    member_ids[MAX_MEMBERS];
    int    member_count;
    int    is_active;
    long   created_at;
} Group;

typedef struct {
    int    msg_id;
    int    group_id;
    int    sender_id;
    int    reply_to;                 // NO_REPLY or parent msg_id
    char   content[MAX_MSG_LEN];
    int    is_deleted;
    long   sent_at;
} Message;

typedef struct {
    int    logged_in;
    int    user_id;
    char   display_name[MAX_NAME_LEN];
} Session;

#endif // CONFIG_H
