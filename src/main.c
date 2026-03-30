/*
 * SCS3304 Assignment 1 - Standalone Group Chat Application
 * Main Entry Point and Presentation Layer
 * 
 * Handles user interface, menus, input reading, and routing only.
 * No direct fopen() calls. No validation logic.
 * Only calls functions from Layer 2 (Application Logic).
 */

#include "config.h"
#include "auth.h"
#include "groups.h"
#include "msgs.h"
#include "utils.h"
#include "file_handler.h"

// ============================================================================
// GLOBAL DATA STORAGE
// ============================================================================

User g_users[MAX_USERS];
int g_user_count = 0;

Group g_groups[MAX_GROUPS];
int g_group_count = 0;

Message g_messages[MAX_MESSAGES];
int g_message_count = 0;

int g_current_user_id = -1;

// ============================================================================
// PRESENTATION LAYER IMPLEMENTATION
// ============================================================================

/* FUNCTION: show_main_menu
 * PURPOSE : Display main menu options for logged-out users
 * INPUT   : None
 * OUTPUT  : None (prints menu to stdout)
 * STEPS   : 1. Print menu header
 *           2. Print numbered options: Register, Login, Search Groups, Exit
 *           3. Print prompt for user input
 */
void show_main_menu(void) {
    /* --- PROCESS REQUEST --- */
    printf("\n=== MAIN MENU ===\n");
    printf("1. Register\n");
    printf("2. Login\n");
    printf("3. Search Groups\n");
    printf("0. Exit\n");
    printf("> ");
}

/* FUNCTION: show_user_menu
 * PURPOSE : Display user menu options for logged-in users
 * INPUT   : None
 * OUTPUT  : None (prints menu to stdout)
 * STEPS   : 1. Print menu header with current user's display name
 *           2. Print numbered options for group and message operations
 *           3. Print logout and exit options
 *           4. Print prompt for user input
 */
void show_user_menu(void) {
    /* --- PROCESS REQUEST --- */
    int user_idx = find_user_by_id(g_users, g_user_count, g_current_user_id);
    char *display_name = "User";
    if (user_idx != ERR_NOT_FOUND) {
        display_name = g_users[user_idx].display_name;
    }
    
    printf("\n=== USER MENU (Logged in as: %s) ===\n", display_name);
    printf("1. Create Group\n");
    printf("2. Join Group\n");
    printf("3. Leave Group\n");
    printf("4. Search Groups\n");
    printf("5. My Groups\n");
    printf("6. View Messages\n");
    printf("7. Send Message\n");
    printf("8. Reply to Message\n");
    printf("9. Search Messages\n");
    printf("10. Edit Message\n");
    printf("11. Delete Message\n");
    printf("12. Logout\n");
    printf("0. Exit\n");
    printf("> ");
}

/* FUNCTION: handle_main_menu
 * PURPOSE : Process main menu choices and execute corresponding actions
 * INPUT   : choice - menu option selected by user
 * OUTPUT  : None (performs action and prints results)
 * STEPS   : 1. Use switch statement to handle menu choice
 *           2. Case 1 (Register): read user data, call register_user, show result
 *           3. Case 2 (Login): read credentials, call login_user, show result
 *           4. Case 3 (Search): read keyword, call search_groups
 *           5. Case 0 (Exit): return to allow program termination
 *           6. Default: show invalid choice message
 */
void handle_main_menu(int choice) {
    /* --- ACCEPT REQUEST --- */
    char username[MAX_NAME_LEN + 1];
    char display_name[MAX_NAME_LEN + 1];
    char password[MAX_PASS_LEN + 1];
    char keyword[MAX_DESC_LEN + 1];
    int result;
    
    /* --- PROCESS REQUEST --- */
    switch (choice) {
        case 1: // Register
            printf("\n=== USER REGISTRATION ===\n");
            read_line("Username: ", username, sizeof(username));
            read_line("Display Name: ", display_name, sizeof(display_name));
            read_line("Password (min 4 chars): ", password, sizeof(password));
            
            result = register_user(g_users, &g_user_count, &g_current_user_id, username, display_name, password);
            if (result == SUCCESS) {
                printf("[+] User '%s' registered successfully!\n", username);
            } else if (result == ERR_DUPLICATE) {
                printf("Error: Username already exists.\n");
            } else if (result == ERR_INVALID_INPUT) {
                printf("Error: Invalid input (username cannot be empty, password must be at least 4 characters).\n");
            } else if (result == ERR_FULL) {
                printf("Error: Maximum user capacity reached.\n");
            }
            break;
            
        case 2: // Login
            printf("\n=== USER LOGIN ===\n");
            read_line("Username: ", username, sizeof(username));
            read_line("Password: ", password, sizeof(password));
            
            result = login_user(g_users, g_user_count, &g_current_user_id, username, password);
            if (result == SUCCESS) {
                int user_idx = find_user_by_name(g_users, g_user_count, username);
                if (user_idx != ERR_NOT_FOUND) {
                    printf("[+] Welcome back, %s!\n", g_users[user_idx].display_name);
                }
            } else if (result == ERR_NOT_FOUND) {
                printf("Error: User not found.\n");
            } else if (result == ERR_AUTH) {
                printf("Error: Incorrect password.\n");
            }
            break;
            
        case 3: // Search Groups
            printf("\n=== SEARCH GROUPS ===\n");
            read_line("Enter keyword: ", keyword, sizeof(keyword));
            search_groups(g_groups, g_group_count, g_current_user_id, keyword);
            break;
            
        case 0: // Exit
            // Handled by main loop
            break;
            
        default:
            printf("Invalid choice. Please try again.\n");
    }
    
    /* --- SEND REPLY --- */
}

/* FUNCTION: handle_user_menu
 * PURPOSE : Process user menu choices and execute corresponding actions
 * INPUT   : choice - menu option selected by user
 * OUTPUT  : 0 to continue, -1 for logout, -99 for exit
 * STEPS   : 1. Use switch statement to handle menu choice
 *           2. Call appropriate business logic functions
 *           3. Handle logout and exit signals
 *           4. Return appropriate control code
 */
int handle_user_menu(int choice) {
    /* --- ACCEPT REQUEST --- */
    char group_name[MAX_NAME_LEN + 1];
    char description[MAX_DESC_LEN + 1];
    char content[MAX_MSG_LEN + 1];
    char keyword[MAX_DESC_LEN + 1];
    int group_id, reply_to, msg_id, result;
    
    /* --- PROCESS REQUEST --- */
    switch (choice) {
        case 1: // Create Group
            printf("\n=== CREATE GROUP ===\n");
            read_line("Group Name: ", group_name, sizeof(group_name));
            read_line("Description: ", description, sizeof(description));
            
            result = create_group(g_groups, &g_group_count, g_current_user_id, group_name, description);
            if (result > 0) {
                printf("[+] Group '%s' created successfully!\n", group_name);
            } else if (result == ERR_DUPLICATE) {
                printf("Error: Group name already exists.\n");
            } else if (result == ERR_INVALID_INPUT) {
                printf("Error: Group name cannot be empty.\n");
            } else if (result == ERR_FULL) {
                printf("Error: Maximum group capacity reached.\n");
            }
            break;
            
        case 2: // Join Group
            printf("\n=== JOIN GROUP ===\n");
            printf("Available groups:\n");
            search_groups(g_groups, g_group_count, g_current_user_id, ""); // Show all groups
            
            group_id = get_int_input("Enter Group ID to join: ", 1, 999999);
            if (group_id >= 0) {
                result = join_group(g_groups, g_group_count, g_current_user_id, group_id);
                if (result == SUCCESS) {
                    printf("[+] You joined group %d.\n", group_id);
                } else if (result == ERR_NOT_FOUND) {
                    printf("Error: Group not found.\n");
                } else if (result == ERR_DUPLICATE) {
                    printf("Error: You are already a member of this group.\n");
                } else if (result == ERR_FULL) {
                    printf("Error: Group is at maximum capacity.\n");
                }
            }
            break;
            
        case 3: // Leave Group
            printf("\n=== LEAVE GROUP ===\n");
            list_my_groups(g_groups, g_group_count, g_current_user_id);
            
            group_id = get_int_input("Enter Group ID to leave: ", 1, 999999);
            if (group_id >= 0) {
                result = leave_group(g_groups, g_group_count, g_current_user_id, group_id);
                if (result == SUCCESS) {
                    printf("[+] You left group %d.\n", group_id);
                } else if (result == ERR_NOT_FOUND) {
                    printf("Error: Group not found.\n");
                } else if (result == ERR_PERMISSION) {
                    printf("Error: You are not a member of this group.\n");
                }
            }
            break;
            
        case 4: // Search Groups
            printf("\n=== SEARCH GROUPS ===\n");
            read_line("Enter keyword: ", keyword, sizeof(keyword));
            search_groups(g_groups, g_group_count, g_current_user_id, keyword);
            break;
            
        case 5: // My Groups
            list_my_groups(g_groups, g_group_count, g_current_user_id);
            break;
            
        case 6: // View Messages
            printf("\n=== VIEW MESSAGES ===\n");
            list_my_groups(g_groups, g_group_count, g_current_user_id);
            
            group_id = get_int_input("Enter Group ID to view messages: ", 1, 999999);
            if (group_id >= 0) {
                result = view_messages(g_messages, g_message_count, g_groups, g_group_count, g_users, g_user_count, g_current_user_id, group_id);
                if (result != SUCCESS) {
                    printf("Error viewing messages.\n");
                }
            }
            break;
            
        case 7: // Send Message
            printf("\n=== SEND MESSAGE ===\n");
            list_my_groups(g_groups, g_group_count, g_current_user_id);
            
            group_id = get_int_input("Enter Group ID: ", 1, 999999);
            if (group_id >= 0) {
                read_line("Message: ", content, sizeof(content));
                
                result = send_message(g_messages, &g_message_count, g_groups, g_group_count, g_current_user_id, group_id, content, NO_REPLY);
                if (result > 0) {
                    printf("[+] Message sent (ID: %d).\n", result);
                } else if (result == ERR_NOT_FOUND) {
                    printf("Error: Group not found.\n");
                } else if (result == ERR_PERMISSION) {
                    printf("Error: You are not a member of this group.\n");
                } else if (result == ERR_INVALID_INPUT) {
                    printf("Error: Message cannot be empty.\n");
                } else if (result == ERR_FULL) {
                    printf("Error: Maximum message capacity reached.\n");
                }
            }
            break;
            
        case 8: // Reply to Message
            printf("\n=== REPLY TO MESSAGE ===\n");
            list_my_groups(g_groups, g_group_count, g_current_user_id);
            
            group_id = get_int_input("Enter Group ID: ", 1, 999999);
            if (group_id >= 0) {
                // Show messages to choose reply target
                view_messages(g_messages, g_message_count, g_groups, g_group_count, g_users, g_user_count, g_current_user_id, group_id);
                
                reply_to = get_int_input("Enter Message ID to reply to: ", 1, 999999);
                if (reply_to >= 0) {
                    read_line("Reply: ", content, sizeof(content));
                    
                    result = send_message(g_messages, &g_message_count, g_groups, g_group_count, g_current_user_id, group_id, content, reply_to);
                    if (result > 0) {
                        printf("[+] Reply sent (ID: %d).\n", result);
                    } else if (result == ERR_NOT_FOUND) {
                        printf("Error: Group or message not found, or message not in this group.\n");
                    } else if (result == ERR_PERMISSION) {
                        printf("Error: You are not a member of this group.\n");
                    } else if (result == ERR_INVALID_INPUT) {
                        printf("Error: Reply cannot be empty.\n");
                    } else if (result == ERR_FULL) {
                        printf("Error: Maximum message capacity reached.\n");
                    }
                }
            }
            break;
            
        case 9: // Search Messages
            printf("\n=== SEARCH MESSAGES ===\n");
            list_my_groups(g_groups, g_group_count, g_current_user_id);
            
            group_id = get_int_input("Enter Group ID to search: ", 1, 999999);
            if (group_id >= 0) {
                read_line("Enter keyword (empty for all): ", keyword, sizeof(keyword));
                result = search_messages(g_messages, g_message_count, g_groups, g_group_count, g_users, g_user_count, g_current_user_id, group_id, keyword);
                if (result != SUCCESS) {
                    printf("Error searching messages.\n");
                }
            }
            break;
            
        case 10: // Edit Message
            printf("\n=== EDIT MESSAGE ===\n");
            list_my_groups(g_groups, g_group_count, g_current_user_id);
            
            group_id = get_int_input("Enter Group ID: ", 1, 999999);
            if (group_id >= 0) {
                view_messages(g_messages, g_message_count, g_groups, g_group_count, g_users, g_user_count, g_current_user_id, group_id);
                
                msg_id = get_int_input("Enter Message ID to edit: ", 1, 999999);
                if (msg_id >= 0) {
                    read_line("New message content: ", content, sizeof(content));
                    result = edit_message(g_messages, g_message_count, g_current_user_id, msg_id, content);
                    if (result != SUCCESS) {
                        printf("Error editing message.\n");
                    }
                }
            }
            break;
            
        case 11: // Delete Message
            printf("\n=== DELETE MESSAGE ===\n");
            list_my_groups(g_groups, g_group_count, g_current_user_id);
            
            group_id = get_int_input("Enter Group ID: ", 1, 999999);
            if (group_id >= 0) {
                view_messages(g_messages, g_message_count, g_groups, g_group_count, g_users, g_user_count, g_current_user_id, group_id);
                
                msg_id = get_int_input("Enter Message ID to delete: ", 1, 999999);
                if (msg_id >= 0) {
                    result = delete_message(g_messages, g_message_count, g_current_user_id, msg_id);
                    if (result != SUCCESS) {
                        printf("Error deleting message.\n");
                    }
                }
            }
            break;
            
        case 12: // Logout
            {
                int user_idx = find_user_by_id(g_users, g_user_count, g_current_user_id);
                char *name = "User";
                if (user_idx != ERR_NOT_FOUND) {
                    name = g_users[user_idx].display_name;
                }
                printf("[+] Goodbye, %s!\n", name);
                
                result = logout_user(&g_current_user_id);
                return -1; // Signal logout
            }
            
        case 0: // Exit
            return -99; // Signal exit
            
        default:
            printf("Invalid choice. Please try again.\n");
    }
    
    /* --- SEND REPLY --- */
    return 0; // Continue normal operation
}

/* FUNCTION: main
 * PURPOSE : Main entry point and application control loop
 * INPUT   : None (standard C main signature)
 * OUTPUT  : 0 on successful program termination
 * STEPS   : 1. Load all data from files
 *           2. Print application banner with current record counts
 *           3. Enter main while(1) loop
 *           4. If no user logged in: show main menu, read choice, call handle_main_menu
 *           5. If user logged in: show user menu, read choice, call handle_user_menu
 *           6. Handle logout signal (-1) and exit signal (-99)
 *           7. On exit: save all data to files
 *           8. Print goodbye message and return 0
 */
int main(void) {
    /* --- ACCEPT REQUEST --- */
    
    /* --- PROCESS REQUEST --- */
    // Load all data from files
    load_users(g_users, &g_user_count);
    load_groups(g_groups, &g_group_count);
    load_messages(g_messages, &g_message_count);
    
    // Print application banner
    print_banner(g_user_count, g_group_count, g_message_count);
    
    // Main application loop
    while (1) {
        int choice;
        int menu_result;
        
        if (g_current_user_id == -1) {
            // Logged out state
            show_main_menu();
            scanf("%d", &choice);
            while (getchar() != '\n'); // Clear input buffer
            
            if (choice == 0) {
                break; // Exit application
            }
            
            handle_main_menu(choice);
        } else {
            // Logged in state
            show_user_menu();
            scanf("%d", &choice);
            while (getchar() != '\n'); // Clear input buffer
            
            menu_result = handle_user_menu(choice);
            
            if (menu_result == -99) {
                break; // Exit application
            } else if (menu_result == -1) {
                continue; // Logout, continue with main menu
            }
            // menu_result == 0 means continue normal operation
        }
        
        printf("\nPress Enter to continue...");
        getchar(); // Pause for readability
    }
    
    // Save all data before exit
    save_users(g_users, g_user_count);
    save_groups(g_groups, g_group_count);
    save_messages(g_messages, g_message_count);
    
    printf("\n[+] Data saved. Goodbye!\n");
    
    /* --- SEND REPLY --- */
    return 0;
}
