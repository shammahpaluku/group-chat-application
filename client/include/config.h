#ifndef CONFIG_H
#define CONFIG_H

// Server connection
#define SERVER_IP      "127.0.0.1"
#define SERVER_PORT    9200
#define CMD_BUF_LEN    1024
#define BACKLOG        5

// Size limits
#define MAX_NAME_LEN   32
#define MAX_MSG_LEN    512
#define MAX_GROUPS     50
#define MAX_MEMBERS    50
#define MAX_MESSAGES   1000

// Special values and error codes
#define NO_REPLY       -1
#define SUCCESS         0
#define ERR_CONN       -7
#define ERR_UNKNOWN    -8

#endif // CONFIG_H
