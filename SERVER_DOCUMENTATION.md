# Group Chat Concurrent TCP Server - Technical Documentation

## Table of Contents
1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Main Algorithm](#main-algorithm)
4. [Function Documentation](#function-documentation)
5. [Concurrency Model](#concurrency-model)
6. [Data Flow](#data-flow)
7. [File Locking Strategy](#file-locking-strategy)

---

## Overview

The Group Chat Concurrent TCP Server is a multi-user chat application that allows users to create groups, join groups, send messages, and reply to messages. It uses a fork-based concurrent architecture to handle multiple clients simultaneously.

**Key Features:**
- Concurrent client handling using fork-based master-slave model
- User authentication (registration and login)
- Group creation and management
- Message sending with reply threading
- Group search and discovery
- File-based data persistence with concurrent access support
- File locking for safe concurrent writes

---

## Architecture

### Server Components

```
┌─────────────────────────────────────────────────────────────┐
│                     Master Process                          │
│  - Listens on TCP port 9200                                 │
│  - Accepts incoming TCP connections                         │
│  - Forks slave processes for each client                    │
│  - Reaps zombie processes via SIGCHLD handler              │
└─────────────────────────────────────────────────────────────┘
                            │
                            │ fork()
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                     Slave Process                            │
│  - Handles one client session                               │
│  - Manages authentication state                            │
│  - Processes commands via dispatch_command()               │
│  - Saves data to files with locking                         │
│  - Exits when client disconnects or sends QUIT              │
└─────────────────────────────────────────────────────────────┘
```

### Data Storage

All data is persisted in text files in the `server/data/` directory:
- `users.txt` - Registered users
- `groups.txt` - Chat groups
- `messages.txt` - Chat messages

### Global Data Structures

The server uses global arrays loaded from files:
- `g_users[]` - Array of User structures
- `g_groups[]` - Array of Group structures
- `g_messages[]` - Array of Message structures

These are accessed via accessor functions to ensure thread safety:
- `fio_get_users()`, `fio_get_user_count()`
- `fio_get_groups()`, `fio_get_group_count()`
- `fio_get_messages()`, `fio_get_msg_count()`

---

## Main Algorithm

### `main()` Function

**Purpose:** Initialize the server and enter the master accept loop.

**Algorithm:**
```
1. Initialize data files (fio_init_files)
2. Load all data from files (fio_load_all)
3. Create TCP listening socket (nh_server_init)
4. Install SIGCHLD signal handler
   - Prevents zombie slave processes
   - Uses SA_RESTART to restart interrupted system calls
   - Uses SA_NOCLDWAIT to automatically reap child processes
5. Enter infinite accept loop:
   a. Accept incoming client connection
   b. Fork a child process
   c. If fork fails:
      - Log error
      - Close client socket
      - Continue to next iteration
   d. If child process (pid == 0):
      - Close listening socket (not needed in slave)
      - Call handle_session(client_fd)
      - Exit when session ends
   e. If parent process (pid > 0):
      - Close client socket (slave owns it)
      - Loop back to accept next connection
```

**Key Design Decisions:**
- **Fork-based concurrency:** Each client gets its own process, providing isolation
- **SIGCHLD handling:** Prevents zombie processes from accumulating
- **Non-blocking master:** Master immediately loops back after forking
- **Global data with accessor functions:** Enables safe concurrent access

---

## Function Documentation

### Helper Functions

#### `err_string(int code)`
**Purpose:** Map error codes to human-readable strings.

**Algorithm:**
```
Switch on error code:
  ERR_NOT_FOUND → "ERR_NOT_FOUND"
  ERR_DUPLICATE → "ERR_DUPLICATE"
  ERR_AUTH → "ERR_AUTH"
  ERR_FULL → "ERR_FULL"
  ERR_PERMISSION → "ERR_PERMISSION"
  ERR_FILE → "ERR_FILE"
  Default → "ERR_UNKNOWN"
```

#### `wire_to_str(char *str)`
**Purpose:** Replace underscores with spaces (wire format to display format).

**Algorithm:**
```
Call utils_replace_char(str, '_', ' ')
```

**Use Case:** Converts wire format (e.g., "John_Doe") to display format (e.g., "John Doe").

#### `str_to_wire(char *str)`
**Purpose:** Replace spaces with underscores (display format to wire format).

**Algorithm:**
```
Call utils_replace_char(str, ' ', '_')
```

**Use Case:** Converts display format to wire format for network transmission.

---

### Authentication Commands

#### `cmd_register(int client_fd, char *args)`
**Purpose:** Register a new user account.

**Algorithm:**
```
1. Parse args: "username display_name password"
2. Validate all three tokens are present
3. Convert wire format to display format (underscores to spaces)
4. Call auth_register(username, display_name, password)
5. If successful:
   - Send "OK <user_id>" to client
   - Save users to file (fio_save_users)
6. If failed:
   - Send error string (err_string)
```

**Security Note:** Password is hashed before storage.

#### `cmd_login(int client_fd, char *args)`
**Purpose:** Authenticate a user.

**Algorithm:**
```
1. Parse args: "username password"
2. Validate both tokens are present
3. Convert username from wire to display format
4. Call auth_login(username, password)
5. If successful:
   - Get display name from session
   - Convert to wire format
   - Send "OK <user_id> <display_name>" to client
6. If failed:
   - Send error string
```

**Session Management:** Sets session state on successful login.

#### `cmd_logout(int client_fd)`
**Purpose:** Clear the current authentication session.

**Algorithm:**
```
1. Call auth_logout() to clear session state
2. Send "OK" to client
```

---

### Group Management Commands

#### `cmd_create_group(int client_fd, char *args)`
**Purpose:** Create a new chat group.

**Algorithm:**
```
1. Parse args: "group_name [description]"
2. Validate group_name is present
3. If description is empty, use "No description"
4. Convert group_name from wire to display format
5. Call grp_create(group_name, description)
6. If successful:
   - Send "OK <group_id>" to client
   - Save groups to file (fio_save_groups)
7. If failed:
   - Send error string
```

**Validation:** Group name must be unique.

#### `cmd_join_group(int client_fd, char *args)`
**Purpose:** Join an existing group.

**Algorithm:**
```
1. Parse args: "group_id"
2. Call grp_join(group_id)
3. If successful:
   - Send "OK" to client
   - Save groups to file (fio_save_groups)
4. If failed:
   - Send error string
```

**Validation:** User must be logged in, group must exist.

#### `cmd_leave_group(int client_fd, char *args)`
**Purpose:** Leave a group.

**Algorithm:**
```
1. Parse args: "group_id"
2. Call grp_leave(group_id)
3. If successful:
   - Send "OK" to client
   - Save groups to file (fio_save_groups)
4. If failed:
   - Send error string
```

#### `cmd_search_groups(int client_fd, char *args)`
**Purpose:** Search for groups by keyword.

**Algorithm:**
```
1. Get keyword from args
2. Trim whitespace
3. Call grp_search(keyword, result_ids, MAX_GROUPS)
4. If no results:
   - Send "ERR_NOT_FOUND"
   - Return
5. For each matching group:
   - Get group by ID
   - Convert name and description to wire format
   - Send "id name member_count description"
6. Send "END" marker
```

**Search Logic:** Searches group names for substring match.

#### `cmd_list_my_groups(int client_fd)`
**Purpose:** List groups the current user is a member of.

**Algorithm:**
```
1. Check if user is logged in
2. Get user ID from session
3. Call grp_get_member_groups(user_id, group_ids, MAX_GROUPS)
4. If no groups:
   - Send "ERR_EMPTY"
   - Return
5. For each group:
   - Get group by ID
   - Convert name and description to wire format
   - Send "id name member_count description"
6. Send "END" marker
```

#### `cmd_list_all_groups(int client_fd)`
**Purpose:** List all active groups.

**Algorithm:**
```
1. Iterate through all groups
2. Skip inactive groups
3. For each active group:
   - Convert name and description to wire format
   - Send "id name member_count description"
4. If no groups:
   - Send "ERR_EMPTY"
   - Return
5. Send "END" marker
```

#### `cmd_list_members(int client_fd, char *args)`
**Purpose:** List all members of a group.

**Algorithm:**
```
1. Parse args: "group_id"
2. Find group by ID
3. If not found:
   - Send "ERR_NOT_FOUND"
   - Return
4. For each member in group:
   - Get user by ID
   - Convert display name to wire format
   - Send "user_id display_name username"
5. Send "END" marker
```

---

### Messaging Commands

#### `cmd_send_msg(int client_fd, char *args)`
**Purpose:** Send a message to a group.

**Algorithm:**
```
1. Parse args: "group_id reply_to_id content"
2. Validate all tokens are present
3. Convert group_id and reply_to to integers
4. Call msg_send(group_id, content, reply_to)
5. If successful:
   - Send "OK <msg_id>" to client
   - Save messages to file (fio_save_messages)
6. If failed:
   - Send error string
```

**Reply Logic:** If reply_to is 0, it's a top-level message. Otherwise, it's a reply.

#### `cmd_view_msgs(int client_fd, char *args)`
**Purpose:** View messages in a group with replies.

**Algorithm:**
```
1. Parse args: "group_id"
2. Find group by ID
3. If not found:
   - Send "ERR_NOT_FOUND"
   - Return
4. Check if user is logged in and is a member
5. If not:
   - Send "ERR_PERMISSION"
   - Return
6. Get messages for group (msg_get_for_group)
7. If no messages:
   - Send "ERR_EMPTY"
   - Return
8. For each top-level message:
   - Get sender display name
   - Format timestamp
   - Convert to wire format
   - Send "MSG msg_id sender timestamp content"
   - Get replies for this message
   - For each reply:
     - Get sender display name
     - Format timestamp
     - Convert to wire format
     - Send "REPLY msg_id sender timestamp content"
9. Send "END" marker
```

**Message Ordering:** Messages are ordered by timestamp (newest first).

#### `cmd_delete_msg(int client_fd, char *args)`
**Purpose:** Delete a message.

**Algorithm:**
```
1. Parse args: "msg_id"
2. Call msg_delete(msg_id)
3. If successful:
   - Send "OK" to client
   - Save messages to file (fio_save_messages)
4. If failed:
   - Send error string
```

**Validation:** Only message sender or admin can delete (implemented in msg_delete).

---

### User Commands

#### `cmd_list_users(int client_fd)`
**Purpose:** List all active users.

**Algorithm:**
```
1. Check if user is logged in
2. Iterate through all users
3. Skip inactive users
4. For each active user:
   - Convert display name to wire format
   - Send "user_id display_name username"
5. If no users:
   - Send "ERR_EMPTY"
   - Return
6. Send "END" marker
```

---

### Session Management

#### `cmd_quit(int client_fd)`
**Purpose:** Client requests to disconnect.

**Algorithm:**
```
1. Send "OK" to client
2. Set s_quit = 1 (signals handle_session to break)
```

#### `dispatch_command(int client_fd, char *cmd_buf)`
**Purpose:** Parse command and route to appropriate handler.

**Algorithm:**
```
1. Copy command buffer (to avoid modifying original)
2. Extract verb (first word before space)
3. Extract args (remainder after verb)
4. Compare verb against known commands:
   - REGISTER → cmd_register
   - LOGIN → cmd_login
   - LOGOUT → cmd_logout
   - CREATE_GROUP → cmd_create_group
   - JOIN_GROUP → cmd_join_group
   - LEAVE_GROUP → cmd_leave_group
   - SEARCH_GROUPS → cmd_search_groups
   - LIST_MY_GROUPS → cmd_list_my_groups
   - LIST_ALL_GROUPS → cmd_list_all_groups
   - LIST_MEMBERS → cmd_list_members
   - SEND_MSG → cmd_send_msg
   - VIEW_MSGS → cmd_view_msgs
   - DELETE_MSG → cmd_delete_msg
   - LIST_USERS → cmd_list_users
   - QUIT → cmd_quit
5. If unknown verb:
   - Send "ERR_UNKNOWN"
6. Log command with user ID
```

#### `handle_session(int client_fd)`
**Purpose:** Manage a complete client session from connection to disconnection.

**Algorithm:**
```
1. Reset authentication state (auth_logout)
2. Reset quit flag (s_quit = 0)
3. Enter command loop:
   a. Receive command from client (nh_recv_line)
   b. If receive fails (disconnect):
      - Print "Client disconnected"
      - Break loop
   c. Dispatch command (dispatch_command)
   d. If s_quit is set:
      - Break loop
4. Close client socket
5. Print "Session ended"
```

**State Management:** Each session has independent auth state.

#### `sigchld_handler(int sig)`
**Purpose:** Signal handler to reap zombie child processes.

**Algorithm:**
```
1. Call waitpid(-1, NULL, WNOHANG) in a loop
2. Continue until no more child processes to reap
3. Return
```

---

## Concurrency Model

### Master-Slave Architecture

**Master Process:**
- Single process that runs for the lifetime of the server
- Only responsibility: Accept connections and fork slaves
- Does not process any client commands
- Closes client socket immediately after forking

**Slave Process:**
- One slave per client connection
- Handles entire client session independently
- Has its own copy of global data (loaded from files)
- Writes to files with locking (flock)
- Exits when client disconnects

### Process Isolation

Each slave process has:
- **Independent memory space:** No shared variables between clients
- **Independent file descriptors:** Each has its own socket
- **Independent authentication state:** Sessions don't interfere
- **Copy of global data:** Loaded from files at session start

### Zombie Process Prevention

**Problem:** When a child exits, it becomes a zombie until parent calls wait().

**Solution:** SIGCHLD handler with WNOHANG
```
sigchld_handler() {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}
```

**Flags:**
- `SA_RESTART`: Automatically restart interrupted system calls (like accept())
- `SA_NOCLDWAIT`: Kernel automatically reaps children (Linux-specific)

---

## Data Flow

### Typical Client Session

```
Client                    Server Master              Server Slave
  |                           |                          |
  |-- CONNECT -------------->|                          |
  |                           |-- fork() -------------->|
  |                           |                          |
  |-- REGISTER -------------->|                          |
  |                           |                          |-- auth_register()
  |                           |                          |-- write users.txt (locked)
  |<-- OK 1 -----------------|                          |
  |                           |                          |
  |-- LOGIN ----------------->|                          |
  |                           |                          |-- auth_login()
  |                           |                          |-- read users.txt
  |<-- OK 1 John_Doe --------|                          |
  |                           |                          |
  |-- CREATE_GROUP ---------->|                          |
  |                           |                          |-- grp_create()
  |                           |                          |-- write groups.txt (locked)
  |<-- OK 1 -----------------|                          |
  |                           |                          |
  |-- SEND_MSG 1 0 Hello ---->|                          |
  |                           |                          |-- msg_send()
  |                           |                          |-- write messages.txt (locked)
  |<-- OK 1 -----------------|                          |
  |                           |                          |
  |-- QUIT ------------------>|                          |
  |                           |                          |-- exit()
  |                           |                          |
  |-- DISCONNECT ------------|                          |
```

### File Access Pattern

**Read Operations:**
- Load entire file into memory array at session start
- Search/filter in memory
- No locking needed for reads (concurrent reads safe)

**Write Operations:**
- Rewrite entire file with updated data
- **Requires file locking (flock)** to prevent corruption
- Implemented in file_io.c with LOCK_EX

### Global Data Access

**Problem:** Global arrays are shared across all processes after fork().

**Solution:** Accessor functions that return pointers to global data:
```c
User *fio_get_users(void);
int *fio_get_user_count(void);
```

**Usage:**
```c
User *users = fio_get_users();
int count = *fio_get_user_count();
```

**Note:** Each process has its own copy after fork(), so this is safe for reads. Writes require file locking.

---

## File Locking Strategy

### Why File Locking?

**Problem:** Multiple slave processes may write to the same file simultaneously, causing data corruption.

**Example Race Condition:**
```
Process A reads users.txt (10 users)
Process B reads users.txt (10 users)
Process A adds user, writes 11 users
Process B adds user, writes 11 users
Result: User added by Process A is lost
```

**Solution:** Use `flock()` to ensure exclusive access during writes.

### Lock Implementation

**In file_io.c:**
```c
int fio_save_users(User *users, int count) {
    FILE *fp = fopen(USERS_FILE, "w");
    if (!fp) return ERR_FILE;
    
    // Acquire exclusive lock
    if (flock(fileno(fp), LOCK_EX) != 0) {
        fclose(fp);
        return ERR_FILE;
    }
    
    // Write data
    for (int i = 0; i < count; i++) {
        fprintf(fp, "%d|%s|%s|%s\n", ...);
    }
    
    fflush(fp);
    
    // Release lock
    flock(fileno(fp), LOCK_UN);
    fclose(fp);
    
    return SUCCESS;
}
```

### Lock Types

- **LOCK_EX (Exclusive):** Only one process can write
- **LOCK_SH (Shared):** Multiple processes can read (not used here)
- **LOCK_UN (Unlock):** Release the lock

### Lock Behavior

- **Blocking:** If lock is held, `flock()` blocks until released
- **Automatic release:** Lock is released when file is closed
- **Process-scoped:** Locks are per-process, not per-thread

### Locked Operations

All write operations are locked:
- `fio_save_users()` - Save user data
- `fio_save_groups()` - Save group data
- `fio_save_messages()` - Save message data

Read operations are not locked (concurrent reads are safe):
- `fio_load_users()` - Load user data
- `fio_load_groups()` - Load group data
- `fio_load_messages()` - Load message data

### Performance Impact

- **Write contention:** Processes wait for lock during writes
- **Read performance:** No impact (no locking on reads)
- **Lock duration:** Short (only during file write operation)

---

## Error Handling

### Error Codes

| Code | Value | Meaning |
|------|-------|---------|
| SUCCESS | 0 | Operation succeeded |
| ERR_FILE | -1 | File I/O error |
| ERR_NOT_FOUND | -2 | Record not found |
| ERR_DUPLICATE | -3 | Duplicate record |
| ERR_AUTH | -4 | Authentication failed |
| ERR_FULL | -5 | Capacity limit reached |
| ERR_PERMISSION | -6 | Permission denied |
| ERR_UNKNOWN | -7 | Unknown error |

### Error Response Format

All errors are sent as plain text lines:
```
ERR_AUTH
ERR_NOT_FOUND
ERR_DUPLICATE
ERR_FULL
ERR_PERMISSION
ERR_FILE
ERR_UNKNOWN
```

---

## Security Considerations

### Password Storage
- Passwords are hashed using SHA-256 (via utils_hash_password)
- Plain text passwords are never stored
- Hash comparison for authentication

### Access Control
- Most commands require `auth_is_logged_in()` check
- Group-specific commands require membership check
- Message deletion requires sender or admin check

### Input Validation
- Buffer sizes enforced (strncpy with length limits)
- Token validation (check for NULL after strtok)
- Type validation (atoi for integers)

### File Permissions
- Data files in `server/data/` directory
- Server runs with user permissions
- No special file permissions required

---

## Performance Characteristics

### Scalability
- **Connection handling:** Unlimited (process-based)
- **Memory usage:** ~2MB per slave process
- **File I/O:** Each slave reads/writes independently
- **Lock contention:** Limited to write operations

### Bottlenecks
- **File locking:** Contention on writes (mitigated by flock)
- **Process creation:** fork() overhead per connection
- **File loading:** Each slave loads entire files at session start

### Optimization Opportunities
- Use shared memory for read-only data
- Implement connection pooling
- Cache frequently accessed data
- Use threads instead of processes (lower overhead)

---

## Testing Recommendations

### Unit Tests
- Test each command handler independently
- Test error conditions (invalid input, missing auth)
- Test file I/O operations
- Test file locking under contention

### Integration Tests
- Test complete client sessions
- Test concurrent access (multiple clients)
- Test group workflow (create, join, message)
- Test message threading (replies)

### Load Tests
- Test with many concurrent clients
- Test file locking under contention
- Monitor memory usage
- Monitor fork rate

### Concurrency Tests
- Test simultaneous writes to same file
- Test simultaneous group creation
- Test simultaneous message sending
- Verify no data corruption

---

## Conclusion

The Group Chat Concurrent TCP Server implements a robust, fork-based concurrent architecture that provides:
- **Isolation:** Each client in its own process
- **Simplicity:** Straightforward code structure
- **Reliability:** Proper signal handling and file locking
- **Functionality:** Complete group chat system with messaging

The master-slave model ensures the server can handle multiple clients simultaneously without blocking, while file locking ensures data integrity in concurrent write scenarios. The accessor function pattern provides safe access to global data across processes.
