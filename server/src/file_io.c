#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include "config.h"
#include "file_io.h"

// Global in-memory arrays
User     g_users[MAX_USERS];
Group    g_groups[MAX_GROUPS];
Message  g_messages[MAX_MESSAGES];
int      g_user_count    = 0;
int      g_group_count   = 0;
int      g_msg_count     = 0;

int fio_init_files(void) {
    // Create data directory
    if (mkdir("data", 0755) == -1 && errno != EEXIST) {
        return ERR_FILE;
    }

    // Check and initialise each data file
    const char *files[] = {USERS_FILE, GROUPS_FILE, MESSAGES_FILE};
    struct stat st;
    
    for (int i = 0; i < 3; i++) {
        if (stat(files[i], &st) == -1) {
            // File doesn't exist, create it
            FILE *fp = fopen(files[i], "wb");
            if (!fp) return ERR_FILE;
            
            int zero = 0;
            if (fwrite(&zero, sizeof(int), 1, fp) != 1) {
                fclose(fp);
                return ERR_FILE;
            }
            fclose(fp);
        }
    }
    
    printf("Data files initialised.\n");
    return SUCCESS;
}

int fio_load_users(User *users, int max) {
    FILE *fp = fopen(USERS_FILE, "rb");
    if (!fp) return ERR_FILE;
    
    int count;
    if (fread(&count, sizeof(int), 1, fp) != 1) {
        fclose(fp);
        return ERR_FILE;
    }
    
    if (count > max) count = max;  // safety cap
    
    if (count > 0) {
        if (fread(users, sizeof(User), count, fp) != (size_t)count) {
            fclose(fp);
            return ERR_FILE;
        }
    }
    
    fclose(fp);
    return count;
}

int fio_save_users(const User *users, int count) {
    FILE *fp = fopen(USERS_FILE, "wb");
    if (!fp) return ERR_FILE;
    
    if (fwrite(&count, sizeof(int), 1, fp) != 1) {
        fclose(fp);
        return ERR_FILE;
    }
    
    if (count > 0) {
        if (fwrite(users, sizeof(User), count, fp) != (size_t)count) {
            fclose(fp);
            return ERR_FILE;
        }
    }
    
    fflush(fp);
    fclose(fp);
    return SUCCESS;
}

int fio_load_groups(Group *groups, int max) {
    FILE *fp = fopen(GROUPS_FILE, "rb");
    if (!fp) return ERR_FILE;
    
    int count;
    if (fread(&count, sizeof(int), 1, fp) != 1) {
        fclose(fp);
        return ERR_FILE;
    }
    
    if (count > max) count = max;  // safety cap
    
    if (count > 0) {
        if (fread(groups, sizeof(Group), count, fp) != (size_t)count) {
            fclose(fp);
            return ERR_FILE;
        }
    }
    
    fclose(fp);
    return count;
}

int fio_save_groups(const Group *groups, int count) {
    FILE *fp = fopen(GROUPS_FILE, "wb");
    if (!fp) return ERR_FILE;
    
    if (fwrite(&count, sizeof(int), 1, fp) != 1) {
        fclose(fp);
        return ERR_FILE;
    }
    
    if (count > 0) {
        if (fwrite(groups, sizeof(Group), count, fp) != (size_t)count) {
            fclose(fp);
            return ERR_FILE;
        }
    }
    
    fflush(fp);
    fclose(fp);
    return SUCCESS;
}

int fio_load_messages(Message *messages, int max) {
    FILE *fp = fopen(MESSAGES_FILE, "rb");
    if (!fp) return ERR_FILE;
    
    int count;
    if (fread(&count, sizeof(int), 1, fp) != 1) {
        fclose(fp);
        return ERR_FILE;
    }
    
    if (count > max) count = max;  // safety cap
    
    if (count > 0) {
        if (fread(messages, sizeof(Message), count, fp) != (size_t)count) {
            fclose(fp);
            return ERR_FILE;
        }
    }
    
    fclose(fp);
    return count;
}

int fio_save_messages(const Message *messages, int count) {
    FILE *fp = fopen(MESSAGES_FILE, "wb");
    if (!fp) return ERR_FILE;
    
    if (fwrite(&count, sizeof(int), 1, fp) != 1) {
        fclose(fp);
        return ERR_FILE;
    }
    
    if (count > 0) {
        if (fwrite(messages, sizeof(Message), count, fp) != (size_t)count) {
            fclose(fp);
            return ERR_FILE;
        }
    }
    
    fflush(fp);
    fclose(fp);
    return SUCCESS;
}

int fio_load_all(void) {
    int result = SUCCESS;
    
    int user_result = fio_load_users(g_users, MAX_USERS);
    if (user_result >= 0) {
        g_user_count = user_result;
    } else {
        result = ERR_FILE;
    }
    
    int group_result = fio_load_groups(g_groups, MAX_GROUPS);
    if (group_result >= 0) {
        g_group_count = group_result;
    } else {
        result = ERR_FILE;
    }
    
    int msg_result = fio_load_messages(g_messages, MAX_MESSAGES);
    if (msg_result >= 0) {
        g_msg_count = msg_result;
    } else {
        result = ERR_FILE;
    }
    
    if (result == SUCCESS) {
        printf("Loaded: %d users, %d groups, %d messages\n",
               g_user_count, g_group_count, g_msg_count);
    }
    
    return result;
}

int fio_save_all(void) {
    int result = SUCCESS;
    
    if (fio_save_users(g_users, g_user_count) != SUCCESS) {
        result = ERR_FILE;
    }
    if (fio_save_groups(g_groups, g_group_count) != SUCCESS) {
        result = ERR_FILE;
    }
    if (fio_save_messages(g_messages, g_msg_count) != SUCCESS) {
        result = ERR_FILE;
    }
    
    return result;
}
