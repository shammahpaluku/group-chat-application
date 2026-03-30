#ifndef GROUPS_H
#define GROUPS_H

// Core group operations
int  grp_create(const char *group_name, const char *description);
int  grp_join(int group_id);
int  grp_leave(int group_id);
int  grp_search(const char *keyword, int *result_ids, int max_results);

// Member validation
int  grp_is_member(int group_idx, int user_id);
int  grp_find_by_id(int group_id);
int  grp_find_by_name(const char *group_name);
int  grp_get_member_groups(int user_id, int *group_ids, int max);

// Display helpers (server-side terminal only)
void grp_list_all(void);
void grp_list_members(int group_id);

#endif // GROUPS_H
