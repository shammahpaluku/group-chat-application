#ifndef AUTH_H
#define AUTH_H

// Registration and login
int  auth_register(const char *username, const char *display_name, const char *plain_password);
int  auth_login(const char *username, const char *plain_password);
void auth_logout(void);

// Session queries
int         auth_is_logged_in(void);
int         auth_get_user_id(void);
const char *auth_get_display_name(void);

// User lookup helpers (used by other modules)
int auth_find_user_by_name(const char *username);
int auth_find_user_by_id(int user_id);
int auth_user_exists(const char *username);

#endif // AUTH_H
