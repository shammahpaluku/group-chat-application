#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "config.h"
#include "file_io.h"
#include "utils.h"

// Hashing
void utils_hash_password(const char *plain, char *out, int out_len) {
    unsigned long hash = 5381;
    int c;
    
    while ((c = *plain++)) {
        hash = (hash * 33) + c;
    }
    
    snprintf(out, out_len, "%lu", hash);
}

// String helpers
void utils_trim(char *str) {
    if (!str) return;
    
    // Remove leading whitespace
    int i = 0;
    while (str[i] == ' ' || str[i] == '\t' || str[i] == '\n' || str[i] == '\r') {
        i++;
    }
    
    if (i > 0) {
        memmove(str, str + i, strlen(str + i) + 1);
    }
    
    // Remove trailing whitespace
    int len = strlen(str);
    while (len > 0 && (str[len-1] == ' ' || str[len-1] == '\t' || 
                       str[len-1] == '\n' || str[len-1] == '\r')) {
        len--;
    }
    str[len] = '\0';
}

void utils_to_lowercase(char *str) {
    if (!str) return;
    
    for (int i = 0; str[i]; i++) {
        str[i] = tolower(str[i]);
    }
}

int utils_is_empty(const char *str) {
    if (!str) return 1;
    
    char temp[MAX_LINE_LEN];
    strncpy(temp, str, sizeof(temp) - 1);
    temp[sizeof(temp) - 1] = '\0';
    
    utils_trim(temp);
    return strlen(temp) == 0;
}

void utils_replace_char(char *str, char from, char to) {
    if (!str) return;
    
    for (int i = 0; str[i]; i++) {
        if (str[i] == from) {
            str[i] = to;
        }
    }
}

int utils_str_contains(const char *haystack, const char *needle) {
    if (!haystack || !needle) return 0;
    
    char haystack_copy[MAX_LINE_LEN];
    char needle_copy[MAX_LINE_LEN];
    
    strncpy(haystack_copy, haystack, sizeof(haystack_copy) - 1);
    haystack_copy[sizeof(haystack_copy) - 1] = '\0';
    
    strncpy(needle_copy, needle, sizeof(needle_copy) - 1);
    needle_copy[sizeof(needle_copy) - 1] = '\0';
    
    utils_to_lowercase(haystack_copy);
    utils_to_lowercase(needle_copy);
    
    return strstr(haystack_copy, needle_copy) != NULL;
}

// ID generation
int utils_next_user_id(void) {
    int max_id = 0;
    
    for (int i = 0; i < g_user_count; i++) {
        if (g_users[i].user_id > max_id) {
            max_id = g_users[i].user_id;
        }
    }
    
    return max_id + 1;
}

int utils_next_group_id(void) {
    int max_id = 0;
    
    for (int i = 0; i < g_group_count; i++) {
        if (g_groups[i].group_id > max_id) {
            max_id = g_groups[i].group_id;
        }
    }
    
    return max_id + 1;
}

int utils_next_msg_id(void) {
    int max_id = 0;
    
    for (int i = 0; i < g_msg_count; i++) {
        if (g_messages[i].msg_id > max_id) {
            max_id = g_messages[i].msg_id;
        }
    }
    
    return max_id + 1;
}

// Timestamp
long utils_now(void) {
    return (long)time(NULL);
}

void utils_format_time(long timestamp, char *out, int out_len) {
    time_t rawtime = timestamp;
    struct tm *timeinfo = localtime(&rawtime);
    
    if (timeinfo) {
        strftime(out, out_len, "%Y-%m-%d %H:%M", timeinfo);
    } else {
        strncpy(out, "Unknown time", out_len - 1);
        out[out_len - 1] = '\0';
    }
}

// Display helpers
void utils_print_separator(void) {
    printf("------------------------------------------------------------\n");
}

void utils_print_header(const char *title) {
    printf("\033[2J\033[H");
    utils_print_separator();
    printf("GROUP CHAT APPLICATION\n");
    printf("%s\n", title);
    utils_print_separator();
}

void utils_clear_screen(void) {
    printf("\033[2J\033[H");
}

void utils_pause(void) {
    printf("\nPress Enter to continue...");
    getchar();
}

// Input helpers
void utils_get_string(const char *prompt, char *buf, int buf_len) {
    printf("%s", prompt);
    
    if (fgets(buf, buf_len, stdin)) {
        // Remove trailing newline
        size_t len = strlen(buf);
        if (len > 0 && buf[len-1] == '\n') {
            buf[len-1] = '\0';
        }
        utils_trim(buf);
    }
}

int utils_get_int(const char *prompt, int min, int max) {
    char buffer[64];
    int value;
    
    while (1) {
        printf("%s", prompt);
        
        if (!fgets(buffer, sizeof(buffer), stdin)) {
            continue;
        }
        
        if (sscanf(buffer, "%d", &value) == 1) {
            if (value >= min && value <= max) {
                return value;
            }
        }
        
        printf("Invalid input. Enter a number between %d and %d: ", min, max);
    }
}

// Wire format helpers
void utils_spaces_to_wire(char *str) {
    for (int i = 0; str[i]; i++) {
        if (str[i] == ' ') str[i] = '_';
    }
}

void utils_wire_to_spaces(char *str) {
    for (int i = 0; str[i]; i++) {
        if (str[i] == '_') str[i] = ' ';
    }
}
