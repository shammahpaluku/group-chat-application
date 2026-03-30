/*
 * SCS3304 Assignment 1 - Standalone Group Chat Application
 * Utilities Module Header
 * 
 * Contains utility functions for input handling, time formatting, and display functions.
 */

#ifndef UTILS_H
#define UTILS_H

#include "config.h"

// ============================================================================
// FUNCTION PROTOTYPES - UTILITIES LAYER
// ============================================================================

/* FUNCTION: read_line
 * PURPOSE : Read a line from stdin safely
 * INPUT   : prompt - prompt to display
 *           buffer - input buffer
 *           buffer_size - size of input buffer
 * OUTPUT  : None (input stored in buffer)
 * STEPS   : 1. Display prompt
 *           2. Read line with fgets
 *           3. Remove newline character
 *           4. Store in buffer
 */
void read_line(const char *prompt, char *buffer, size_t buffer_size);

/* FUNCTION: format_time
 * PURPOSE : Format a Unix timestamp as a readable date-time string
 * INPUT   : timestamp - Unix epoch time value
 *           buffer - character buffer to store formatted result
 *           buffer_size - size of the buffer
 * OUTPUT  : None (formatted string stored in buffer)
 * STEPS   : 1. Convert timestamp to local time structure
 *           2. Use strftime to format as "YYYY-MM-DD HH:MM:SS"
 *           3. Store result in buffer
 */
void format_time(time_t timestamp, char *buffer, size_t buffer_size);

/* FUNCTION: print_banner
 * PURPOSE : Print application banner with current statistics
 * INPUT   : user_count - number of users in system
 *           group_count - number of groups in system
 *           message_count - number of messages in system
 * OUTPUT  : None (prints formatted banner)
 * STEPS   : 1. Print separator line
 *           2. Print application title
 *           3. Print current record counts
 *           4. Print separator line
 */
void print_banner(int user_count, int group_count, int message_count);

/* FUNCTION: get_int_input
 * PURPOSE : Read integer input from stdin with validation
 * INPUT   : prompt - prompt message to display
 *           min_val - minimum valid value (optional)
 *           max_val - maximum valid value (optional)
 * OUTPUT  : Integer value entered by user
 * STEPS   : 1. Read input line using read_line
 *           2. Convert to integer using atoi
 *           3. Validate against min/max bounds if provided
 *           4. Return validated value or -1 on error
 */
int get_int_input(const char *prompt, int min_val, int max_val);

#endif // UTILS_H
