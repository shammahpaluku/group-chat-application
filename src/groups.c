/*
 * SCS3304 Assignment 1 - Standalone Group Chat Application
 * Groups Module Implementation
 * 
 * Handles group creation, joining, leaving, searching, and group management functions.
 * Manages group membership and permissions.
 */

#include "groups.h"
#include "file_handler.h"

// ============================================================================
// GROUP MANAGEMENT LAYER IMPLEMENTATION
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
int create_group(Group *groups, int *count, int current_user_id, const char *group_name, const char *description) {
    /* --- ACCEPT REQUEST --- */
    
    /* --- VALIDATE REQUEST --- */
    if (current_user_id == -1) {
        return ERR_PERMISSION;
    }
    
    if (strlen(group_name) == 0) {
        return ERR_INVALID_INPUT;
    }
    
    if (find_group_by_name(groups, *count, group_name) != ERR_NOT_FOUND) {
        return ERR_DUPLICATE; // group name already exists
    }
    
    if (*count >= MAX_GROUPS) {
        return ERR_FULL;
    }
    
    /* --- PROCESS REQUEST --- */
    Group *new_group = &groups[*count];
    
    // Generate unique group ID
    int max_id = 0;
    for (int i = 0; i < *count; i++) {
        if (groups[i].group_id > max_id) {
            max_id = groups[i].group_id;
        }
    }
    new_group->group_id = max_id + 1;
    strncpy(new_group->group_name, group_name, MAX_NAME_LEN);
    new_group->group_name[MAX_NAME_LEN] = '\0';
    strncpy(new_group->description, description, MAX_DESC_LEN);
    new_group->description[MAX_DESC_LEN] = '\0';
    new_group->creator_id = current_user_id;
    new_group->member_ids[0] = current_user_id; // creator auto-joins
    new_group->member_count = 1;
    new_group->is_active = 1;
    new_group->created_at = time(NULL);
    
    (*count)++;
    
    /* --- FORMULATE REPLY --- */
    save_groups(groups, *count);
    
    /* --- SEND REPLY --- */
    return new_group->group_id;
}

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
int join_group(Group *groups, int group_count, int current_user_id, int group_id) {
    /* --- ACCEPT REQUEST --- */
    
    /* --- VALIDATE REQUEST --- */
    if (current_user_id == -1) {
        return ERR_PERMISSION;
    }
    
    int group_idx = find_group_by_id(groups, group_count, group_id);
    if (group_idx == ERR_NOT_FOUND) {
        return ERR_NOT_FOUND;
    }
    
    if (is_member(&groups[group_idx], current_user_id)) {
        return ERR_DUPLICATE; // already a member
    }
    
    if (groups[group_idx].member_count >= MAX_MEMBERS) {
        return ERR_FULL;
    }
    
    /* --- PROCESS REQUEST --- */
    groups[group_idx].member_ids[groups[group_idx].member_count] = current_user_id;
    groups[group_idx].member_count++;
    
    /* --- FORMULATE REPLY --- */
    save_groups(groups, group_count);
    
    /* --- SEND REPLY --- */
    return SUCCESS;
}

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
int leave_group(Group *groups, int group_count, int current_user_id, int group_id) {
    /* --- ACCEPT REQUEST --- */
    
    /* --- VALIDATE REQUEST --- */
    if (current_user_id == -1) {
        return ERR_PERMISSION;
    }
    
    int group_idx = find_group_by_id(groups, group_count, group_id);
    if (group_idx == ERR_NOT_FOUND) {
        return ERR_NOT_FOUND;
    }
    
    if (!is_member(&groups[group_idx], current_user_id)) {
        return ERR_PERMISSION;
    }
    
    /* --- PROCESS REQUEST --- */
    // Remove user by shifting the members array left
    for (int i = 0; i < groups[group_idx].member_count; i++) {
        if (groups[group_idx].member_ids[i] == current_user_id) {
            // Shift remaining members left by one
            for (int j = i; j < groups[group_idx].member_count - 1; j++) {
                groups[group_idx].member_ids[j] = groups[group_idx].member_ids[j + 1];
            }
            groups[group_idx].member_count--;
            break;
        }
    }
    
    if (groups[group_idx].member_count == 0) {
        groups[group_idx].is_active = 0; // dissolve empty group
    }
    
    /* --- FORMULATE REPLY --- */
    save_groups(groups, group_count);
    
    /* --- SEND REPLY --- */
    return SUCCESS;
}

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
void search_groups(Group *groups, int group_count, int current_user_id, const char *keyword) {
    /* --- ACCEPT REQUEST --- */
    
    /* --- VALIDATE REQUEST --- */
    char keyword_lower[MAX_DESC_LEN + 1];
    for (int i = 0; keyword[i] && i < MAX_DESC_LEN; i++) {
        keyword_lower[i] = tolower(keyword[i]);
    }
    keyword_lower[strlen(keyword)] = '\0';
    
    /* --- PROCESS REQUEST --- */
    int found = 0;
    printf("\n=== SEARCH RESULTS ===\n");
    
    for (int i = 0; i < group_count; i++) {
        if (!groups[i].is_active) continue;
        
        char name_lower[MAX_NAME_LEN + 1];
        char desc_lower[MAX_DESC_LEN + 1];
        
        for (int j = 0; groups[i].group_name[j] && j < MAX_NAME_LEN; j++) {
            name_lower[j] = tolower(groups[i].group_name[j]);
        }
        name_lower[strlen(groups[i].group_name)] = '\0';
        
        for (int j = 0; groups[i].description[j] && j < MAX_DESC_LEN; j++) {
            desc_lower[j] = tolower(groups[i].description[j]);
        }
        desc_lower[strlen(groups[i].description)] = '\0';
        
        // Show all groups if keyword is empty, or if keyword matches name/description
        if (strlen(keyword_lower) == 0 || 
            strstr(name_lower, keyword_lower) != NULL || 
            strstr(desc_lower, keyword_lower) != NULL) {
            
            /* --- FORMULATE REPLY --- */
            printf("[%d] %s (%d members)", groups[i].group_id,
                   groups[i].group_name, groups[i].member_count);
            
            if (current_user_id != -1 && is_member(&groups[i], current_user_id)) {
                printf(" - JOINED");
            }
            printf("\n");
            printf("    %s\n\n", groups[i].description);
            
            found++;
        }
    }
    
    if (found == 0) {
        printf("No groups found matching '%s'\n\n", keyword);
    }
    
    /* --- SEND REPLY --- */
}

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
void list_my_groups(Group *groups, int group_count, int current_user_id) {
    /* --- ACCEPT REQUEST --- */
    
    /* --- VALIDATE REQUEST --- */
    if (current_user_id == -1) {
        printf("Please log in first to view your groups.\n");
        return;
    }
    
    /* --- PROCESS REQUEST --- */
    printf("\n=== MY GROUPS ===\n");
    int found = 0;
    
    for (int i = 0; i < group_count; i++) {
        if (!groups[i].is_active) continue;
        
        if (is_member(&groups[i], current_user_id)) {
            /* --- FORMULATE REPLY --- */
            printf("[%d] %s (%d members)\n", groups[i].group_id,
                   groups[i].group_name, groups[i].member_count);
            printf("    %s\n\n", groups[i].description);
            found++;
        }
    }
    
    if (found == 0) {
        printf("You haven't joined any groups yet.\n\n");
    }
    
    /* --- SEND REPLY --- */
}

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
int find_group_by_name(Group *groups, int group_count, const char *group_name) {
    /* --- ACCEPT REQUEST --- */
    
    /* --- PROCESS REQUEST --- */
    for (int i = 0; i < group_count; i++) {
        if (groups[i].is_active && strcmp(groups[i].group_name, group_name) == 0) {
            /* --- SEND REPLY --- */
            return i;
        }
    }
    
    /* --- SEND REPLY --- */
    return ERR_NOT_FOUND;
}

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
int find_group_by_id(Group *groups, int group_count, int group_id) {
    /* --- ACCEPT REQUEST --- */
    
    /* --- PROCESS REQUEST --- */
    for (int i = 0; i < group_count; i++) {
        if (groups[i].is_active && groups[i].group_id == group_id) {
            /* --- SEND REPLY --- */
            return i;
        }
    }
    
    /* --- SEND REPLY --- */
    return ERR_NOT_FOUND;
}

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
int is_member(Group *grp, int user_id) {
    /* --- ACCEPT REQUEST --- */
    
    /* --- PROCESS REQUEST --- */
    for (int i = 0; i < grp->member_count; i++) {
        if (grp->member_ids[i] == user_id) {
            /* --- SEND REPLY --- */
            return 1;
        }
    }
    
    /* --- SEND REPLY --- */
    return 0;
}
