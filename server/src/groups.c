#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include "file_io.h"
#include "utils.h"
#include "auth.h"
#include "groups.h"

// NOTE: This function returns ARRAY INDEX, not group_id
int grp_find_by_id(int group_id) {
    for (int i = 0; i < g_group_count; i++) {
        if (g_groups[i].is_active && g_groups[i].group_id == group_id) {
            return i; // Return array index
        }
    }
    return ERR_NOT_FOUND;
}

// NOTE: This function returns ARRAY INDEX, not group_id
int grp_find_by_name(const char *group_name) {
    if (!group_name) return ERR_NOT_FOUND;
    
    char search_lower[MAX_NAME_LEN];
    strncpy(search_lower, group_name, sizeof(search_lower) - 1);
    search_lower[sizeof(search_lower) - 1] = '\0';
    utils_to_lowercase(search_lower);
    
    for (int i = 0; i < g_group_count; i++) {
        if (!g_groups[i].is_active) continue;
        
        char group_lower[MAX_NAME_LEN];
        strncpy(group_lower, g_groups[i].group_name, sizeof(group_lower) - 1);
        group_lower[sizeof(group_lower) - 1] = '\0';
        utils_to_lowercase(group_lower);
        
        if (strcmp(group_lower, search_lower) == 0) {
            return i; // Return array index
        }
    }
    return ERR_NOT_FOUND;
}

int grp_is_member(int group_idx, int user_id) {
    if (group_idx < 0 || group_idx >= g_group_count) return 0;
    
    Group *group = &g_groups[group_idx];
    for (int i = 0; i < group->member_count; i++) {
        if (group->member_ids[i] == user_id) {
            return 1;
        }
    }
    return 0;
}

int grp_get_member_groups(int user_id, int *group_ids, int max) {
    int found = 0;
    
    for (int i = 0; i < g_group_count && found < max; i++) {
        if (!g_groups[i].is_active) continue;
        
        if (grp_is_member(i, user_id)) {
            group_ids[found] = g_groups[i].group_id;
            found++;
        }
    }
    
    return found;
}

int grp_create(const char *group_name, const char *description) {
    // Check authentication
    if (!auth_is_logged_in()) return ERR_AUTH;
    
    // Validate inputs
    if (utils_is_empty(group_name)) return ERR_AUTH;
    if (strlen(group_name) >= MAX_NAME_LEN) return ERR_AUTH;
    
    // Check for duplicate group name
    if (grp_find_by_name(group_name) >= 0) return ERR_DUPLICATE;
    
    // Check capacity
    if (g_group_count >= MAX_GROUPS) return ERR_FULL;
    
    // Get current user
    int creator_id = auth_get_user_id();
    
    // Populate new Group
    Group *new_group = &g_groups[g_group_count];
    new_group->group_id = utils_next_group_id();
    
    strncpy(new_group->group_name, group_name, sizeof(new_group->group_name) - 1);
    new_group->group_name[sizeof(new_group->group_name) - 1] = '\0';
    
    if (utils_is_empty(description)) {
        strncpy(new_group->description, "No description", sizeof(new_group->description) - 1);
    } else {
        strncpy(new_group->description, description, sizeof(new_group->description) - 1);
    }
    new_group->description[sizeof(new_group->description) - 1] = '\0';
    
    new_group->creator_id = creator_id;
    new_group->member_ids[0] = creator_id;
    new_group->member_count = 1;
    new_group->is_active = 1;
    new_group->created_at = utils_now();
    
    // Zero out remaining member_ids slots
    memset(&new_group->member_ids[1], 0, sizeof(new_group->member_ids) - sizeof(int));
    
    // Increment count
    g_group_count++;
    
    // Save to file
    if (fio_save_groups(g_groups, g_group_count) != SUCCESS) {
        g_group_count--; // Rollback on save failure
        return ERR_FILE;
    }
    
    return new_group->group_id; // Return new group_id on success
}

int grp_join(int group_id) {
    // Check authentication
    if (!auth_is_logged_in()) return ERR_AUTH;
    
    // Find group
    int idx = grp_find_by_id(group_id);
    if (idx == ERR_NOT_FOUND) return ERR_NOT_FOUND;
    
    int user_id = auth_get_user_id();
    
    // Check if already member
    if (grp_is_member(idx, user_id)) return ERR_DUPLICATE;
    
    // Check member capacity
    if (g_groups[idx].member_count >= MAX_MEMBERS) return ERR_FULL;
    
    // Add user to group
    g_groups[idx].member_ids[g_groups[idx].member_count] = user_id;
    g_groups[idx].member_count++;
    
    // Save to file
    return fio_save_groups(g_groups, g_group_count);
}

int grp_leave(int group_id) {
    // Check authentication
    if (!auth_is_logged_in()) return ERR_AUTH;
    
    // Find group
    int idx = grp_find_by_id(group_id);
    if (idx == ERR_NOT_FOUND) return ERR_NOT_FOUND;
    
    int user_id = auth_get_user_id();
    
    // Check if member
    if (!grp_is_member(idx, user_id)) return ERR_PERMISSION;
    
    // Find user position in member_ids
    int pos = -1;
    for (int i = 0; i < g_groups[idx].member_count; i++) {
        if (g_groups[idx].member_ids[i] == user_id) {
            pos = i;
            break;
        }
    }
    
    if (pos == -1) return ERR_PERMISSION; // Should not happen
    
    // Remove user from member_ids array
    memmove(&g_groups[idx].member_ids[pos], &g_groups[idx].member_ids[pos + 1],
            (g_groups[idx].member_count - pos - 1) * sizeof(int));
    g_groups[idx].member_ids[g_groups[idx].member_count - 1] = 0;
    g_groups[idx].member_count--;
    
    // If no members left, dissolve group
    if (g_groups[idx].member_count == 0) {
        g_groups[idx].is_active = 0;
        printf("Group '%s' dissolved — no members remaining.\n", g_groups[idx].group_name);
    }
    
    // Save to file
    return fio_save_groups(g_groups, g_group_count);
}

int grp_search(const char *keyword, int *result_ids, int max_results) {
    if (utils_is_empty(keyword)) return 0;
    
    int found = 0;
    
    for (int i = 0; i < g_group_count && found < max_results; i++) {
        if (!g_groups[i].is_active) continue;
        
        if (utils_str_contains(g_groups[i].group_name, keyword) ||
            utils_str_contains(g_groups[i].description, keyword)) {
            result_ids[found] = g_groups[i].group_id;
            found++;
        }
    }
    
    return found;
}

void grp_list_all(void) {
    printf("ID  | Name                | Members | Description\n");
    printf("----|---------------------|---------|------------------\n");
    
    int found = 0;
    for (int i = 0; i < g_group_count; i++) {
        if (!g_groups[i].is_active) continue;
        
        printf("%-4d| %-20s| %-8d| %s\n",
               g_groups[i].group_id,
               g_groups[i].group_name,
               g_groups[i].member_count,
               g_groups[i].description);
        found++;
    }
    
    if (found == 0) {
        printf("No groups found.\n");
    }
}

void grp_list_members(int group_id) {
    int idx = grp_find_by_id(group_id);
    if (idx == ERR_NOT_FOUND) {
        printf("Group not found.\n");
        return;
    }
    
    printf("Members of '%s':\n", g_groups[idx].group_name);
    
    for (int i = 0; i < g_groups[idx].member_count; i++) {
        int user_idx = auth_find_user_by_id(g_groups[idx].member_ids[i]);
        if (user_idx >= 0) {
            printf("  - %s (@%s)\n", g_users[user_idx].display_name, g_users[user_idx].username);
        } else {
            printf("  - [unknown user id %d]\n", g_groups[idx].member_ids[i]);
        }
    }
    
    printf("Total members: %d\n", g_groups[idx].member_count);
}
