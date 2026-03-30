/*
 * SCS3304 Assignment 1 - Standalone Group Chat Application
 * Groups Module Header
 * 
 * Handles group creation, joining, leaving, searching, and group management functions.
 */

#ifndef GROUPS_H
#define GROUPS_H

#include "config.h"

// ============================================================================
// FUNCTION PROTOTYPES - GROUP MANAGEMENT LAYER
// ============================================================================

/* FUNCTION: create_group
 * PURPOSE : Create a new group with current user as creator
 * INPUT   : groups - array of Group structs
 *           count - pointer to group count (updated on success)
 *           current_user_id - ID of user creating the group
 *           group_name - unique group name
 *           description - group description
 * OUTPUT  : Group ID on success, ERR_DUPLICATE if name exists,
 *           ERR_FULL if group limit reached, ERR_PERMISSION if not logged in
 * STEPS   : 1. Validate user is logged in
 *           2. Check group name uniqueness
 *           3. Check capacity limit
 *           4. Create group with creator as first member
 *           5. Save to file and return group ID
 */
int create_group(Group *groups, int *count, int current_user_id, const char *group_name, const char *description);

/* FUNCTION: join_group
 * PURPOSE : Add current user to an existing group
 * INPUT   : groups - array of Group structs
 *           group_count - number of groups in array
 *           current_user_id - ID of user joining the group
 *           group_id - ID of group to join
 * OUTPUT  : SUCCESS on success, ERR_NOT_FOUND if group not found,
 *           ERR_DUPLICATE if already member, ERR_FULL if group full
 * STEPS   : 1. Validate user is logged in
 *           2. Find group by ID
 *           3. Check if already member
 *           4. Check member capacity
 *           5. Add user to member list and save
 */
int join_group(Group *groups, int group_count, int current_user_id, int group_id);

/* FUNCTION: leave_group
 * PURPOSE : Remove current user from a group
 * INPUT   : groups - array of Group structs
 *           group_count - number of groups in array
 *           current_user_id - ID of user leaving the group
 *           group_id - ID of group to leave
 * OUTPUT  : SUCCESS on success, ERR_NOT_FOUND if group not found,
 *           ERR_PERMISSION if not member
 * STEPS   : 1. Validate user is logged in
 *           2. Find group by ID
 *           3. Check if user is member
 *           4. Remove user from member list
 *           5. Deactivate group if no members left
 *           6. Save changes
 */
int leave_group(Group *groups, int group_count, int current_user_id, int group_id);

/* FUNCTION: search_groups
 * PURPOSE : Search for groups by keyword in name or description
 * INPUT   : groups - array of Group structs
 *           group_count - number of groups in array
 *           current_user_id - ID of current user (for membership status)
 *           keyword - search term
 * OUTPUT  : None (prints results to stdout)
 * STEPS   : 1. Convert keyword to lowercase
 *           2. Iterate through active groups
 *           3. Check if keyword matches name or description
 *           4. Print matching groups with details
 */
void search_groups(Group *groups, int group_count, int current_user_id, const char *keyword);

/* FUNCTION: list_my_groups
 * PURPOSE : Display all groups current user has joined
 * INPUT   : groups - array of Group structs
 *           group_count - number of groups in array
 *           current_user_id - ID of current user
 * OUTPUT  : None (prints results to stdout)
 * STEPS   : 1. Validate user is logged in
 *           2. Iterate through groups
 *           3. Check if current user is member
 *           4. Print group details
 */
void list_my_groups(Group *groups, int group_count, int current_user_id);

/* FUNCTION: find_group_by_name
 * PURPOSE : Find group index by group name (case-sensitive)
 * INPUT   : groups - array of Group structs
 *           group_count - number of groups in array
 *           group_name - group name to search for
 * OUTPUT  : Array index if found, ERR_NOT_FOUND if not found
 * STEPS   : 1. Iterate through groups array
 *           2. For each active group, compare group names
 *           3. Return index if match found
 *           4. Return ERR_NOT_FOUND if no match
 */
int find_group_by_name(Group *groups, int group_count, const char *group_name);

/* FUNCTION: find_group_by_id
 * PURPOSE : Find group index by group ID
 * INPUT   : groups - array of Group structs
 *           group_count - number of groups in array
 *           group_id - group ID to search for
 * OUTPUT  : Array index if found, ERR_NOT_FOUND if not found
 * STEPS   : 1. Iterate through groups array
 *           2. For each active group, compare group IDs
 *           3. Return index if match found
 *           4. Return ERR_NOT_FOUND if no match
 */
int find_group_by_id(Group *groups, int group_count, int group_id);

/* FUNCTION: is_member
 * PURPOSE : Check if user is member of a group
 * INPUT   : grp - pointer to Group struct
 *           user_id - user ID to check
 * OUTPUT  : 1 if member, 0 if not
 * STEPS   : 1. Get group from index
 *           2. Iterate through member IDs
 *           3. Check for matching user ID
 *           4. Return result
 */
int is_member(Group *grp, int user_id);

#endif // GROUPS_H
