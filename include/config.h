/*
 * SCS3304 Assignment 1 - Standalone Group Chat Application
 * Configuration Header
 * 
 * Contains all constants, struct definitions, and shared includes.
 * Included by every other module.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <ctype.h>
#include <time.h>
#include <stdbool.h>

// ============================================================================
// CONSTANTS & MACROS
// ============================================================================

// Scalability limits
#define MAX_USERS      100
#define MAX_GROUPS     50
#define MAX_MESSAGES   1000
#define MAX_MEMBERS    50
#define MAX_NAME_LEN   32
#define MAX_PASS_LEN   32
#define MAX_DESC_LEN   128
#define MAX_MSG_LEN    512

// File names
#define USERS_FILE     "users.txt"
#define GROUPS_FILE    "groups.txt"
#define MESSAGES_FILE  "messages.txt"

// Return codes
#define SUCCESS         0
#define ERR_NOT_FOUND  -1
#define ERR_DUPLICATE  -2
#define ERR_FULL       -3
#define ERR_AUTH       -4
#define ERR_PERMISSION -5
#define ERR_INVALID_INPUT -6

// Special values
#define NO_REPLY       -1

// Field delimiters
#define FIELD_DELIMITER  '|'
#define MEMBER_DELIMITER ':'

// ============================================================================
// DATA STRUCTURES
// ============================================================================

typedef struct {
    int user_id;
    char username[MAX_NAME_LEN + 1];
    char password[MAX_PASS_LEN + 1];  // djb2 hash as string
    char display_name[MAX_NAME_LEN + 1];
    int is_active;
    time_t created_at;
} User;

typedef struct {
    int group_id;
    char group_name[MAX_NAME_LEN + 1];
    char description[MAX_DESC_LEN + 1];
    int creator_id;
    int member_ids[MAX_MEMBERS];
    int member_count;
    int is_active;
    time_t created_at;
} Group;

typedef struct {
    int msg_id;
    int group_id;
    int sender_id;
    int reply_to;
    char content[MAX_MSG_LEN + 1];
    int is_deleted;
    time_t sent_at;
} Message;

#endif // CONFIG_H
