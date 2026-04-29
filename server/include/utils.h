#ifndef UTILS_H
#define UTILS_H

// Hashing
void utils_hash_password(const char *plain, char *out, int out_len);

// String helpers
void utils_trim(char *str);
void utils_to_lowercase(char *str);
int  utils_is_empty(const char *str);
void utils_replace_char(char *str, char from, char to);
int  utils_str_contains(const char *haystack, const char *needle);
void utils_spaces_to_wire(char *str);
void utils_wire_to_spaces(char *str);

// ID generation
int  utils_next_user_id(void);
int  utils_next_group_id(void);
int  utils_next_msg_id(void);

// Timestamp
long utils_now(void);
void utils_format_time(long timestamp, char *out, int out_len);

// Display helpers
void utils_print_separator(void);
void utils_print_header(const char *title);
void utils_clear_screen(void);
void utils_pause(void);

// Input helpers
void utils_get_string(const char *prompt, char *buf, int buf_len);
int  utils_get_int(const char *prompt, int min, int max);

#endif // UTILS_H
