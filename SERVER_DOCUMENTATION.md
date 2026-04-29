# Group Chat Concurrent UDP Server - Technical Documentation

## Table of Contents
1. [Overview](#overview)
2. [Architecture](#architecture)
3. [Main Algorithm](#main-algorithm)
4. [Function Documentation](#function-documentation)
5. [Concurrency Model](#concurrency-model)
6. [Data Flow](#data-flow)
7. [UDP vs TCP Comparison](#udp-vs-tcp-comparison)

---

## Overview

The Group Chat Concurrent UDP Server is a multi-user chat application that allows users to create groups, join groups, send messages, and reply to messages. It uses a fork-based concurrent architecture with POSIX message queues for master-slave coordination, operating over UDP (connectionless protocol).

**Key Features:**
- Concurrent datagram handling using fork-based master-slave model
- POSIX message queue for passing datagrams from master to slaves
- Stateless processing (each datagram is independent)
- User authentication (registration and login)
- Group creation and management
- Message sending with reply threading
- Group search and discovery
- File-based data persistence

---

## Architecture

### Server Components

```
┌─────────────────────────────────────────────────────────────┐
│                     Master Process                          │
│  - Listens on UDP port 9200                                 │
│  - Receives datagrams from any client                       │
│  - Pushes datagrams to POSIX message queue                  │
│  - Forks slave processes for each datagram                  │
│  - Reaps zombie processes via SIGCHLD handler              │
└─────────────────────────────────────────────────────────────┘
                            │
                            │ fork()
                            ▼
┌─────────────────────────────────────────────────────────────┐
│                     Slave Process                            │
│  - Reads one datagram from message queue                    │
│  - Resets auth state (stateless UDP)                        │
│  - Processes command via dispatch_command()                 │
│  - Saves all data to files                                  │
│  - Exits immediately after handling one datagram            │
└─────────────────────────────────────────────────────────────┘
```

### POSIX Message Queue

The message queue (`/groupchat_mq`) serves as the communication channel between master and slave processes:

```
┌─────────────┐     mq_send()     ┌──────────────┐     mq_receive()     ┌─────────────┐
│   Master    │ ──────────────────>│ Message Queue│ ──────────────────>│   Slave     │
│  Process    │   DgramMsg struct  │  /groupchat_mq│   DgramMsg struct  │  Process    │
└─────────────┘                    └──────────────┘                    └─────────────┘
```

**DgramMsg Structure:**
```c
typedef struct {
    char cmd_buf[CMD_BUF_LEN];          // Command text
    struct sockaddr_in client_addr;      // Client's IP and port
} DgramMsg;
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

---

## Main Algorithm

### `main()` Function

**Purpose:** Initialize the server and enter the master receive loop.

**Algorithm:**
```
1. Initialize data files (fio_init_files)
2. Load all data from files (fio_load_all)
3. Create UDP listening socket (nh_server_init)
4. Install SIGCHLD signal handler
   - Prevents zombie slave processes
   - Uses SA_RESTART to restart interrupted system calls
   - Uses SA_NOCLDWAIT to automatically reap child processes
5. Create POSIX message queue
   - Set queue attributes (max messages, message size)
   - Open with O_CREAT | O_RDWR
   - Permissions: 0666
6. Enter infinite receive loop:
   a. Receive UDP datagram (nh_recv_from)
      - Stores command text in msg.cmd_buf
      - Stores client address in msg.client_addr
   b. Push datagram to message queue (mq_send)
   c. Fork a child process
   d. If fork fails:
      - Log error
      - Continue to next iteration
   e. If child process (pid == 0):
      - Read datagram from message queue (mq_receive)
      - Reset auth state (auth_logout) - stateless UDP
      - Process command (dispatch_command)
      - Save all data to files (fio_save_all)
      - Close message queue
      - Exit (handles exactly one datagram)
   f. If parent process (pid > 0):
      - Loop back immediately to receive next datagram
7. Cleanup (unreachable):
   - Close message queue
   - Unlink message queue
   - Close socket
```

**Key Design Decisions:**
- **Message queue:** Enables master to pass datagram context (command + address) to slave
- **Stateless processing:** Each slave resets auth state since UDP is connectionless
- **One datagram per slave:** Slave exits after processing one command
- **Save all data:** Unlike TCP version, saves all data after each command (simpler but less efficient)
- **Non-blocking master:** Master immediately loops back after forking

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

#### `str_to_wire(char *str)`
**Purpose:** Replace spaces with underscores (display format to wire format).

**Algorithm:**
```
Call utils_replace_char(str, ' ', '_')
```

---

### Authentication Commands

**Note:** All command handlers in the UDP version take a socket descriptor and a `struct sockaddr_in*` for the client address, unlike the TCP version which only takes a file descriptor.

#### `cmd_register(int sock, struct sockaddr_in *client_addr, char *args)`
**Purpose:** Register a new user account.

**Algorithm:**
```
1. Parse args: "username display_name password"
2. Validate all three tokens are present
3. Convert wire format to display format (underscores to spaces)
4. Call auth_register(username, display_name, password)
5. If successful:
   - Send "OK <user_id>" to client via nh_send_to()
6. If failed:
   - Send error string via nh_send_to()
```

**UDP-Specific:** Uses `nh_send_to()` which requires the client address to send the response.

**Note:** Unlike TCP version, does not save users immediately. Data is saved by `fio_save_all()` after command processing.

#### `cmd_login(int sock, struct sockaddr_in *client_addr, char *args)`
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

**Stateless Note:** In UDP, login is per-datagram. The next datagram from the same client will require re-authentication unless credentials are included.

#### `cmd_logout(int sock, struct sockaddr_in *client_addr)`
**Purpose:** Clear the current authentication session.

**Algorithm:**
```
1. Call auth_logout() to clear session state
2. Send "OK" to client
```

**Stateless Note:** Since UDP is connectionless, logout is per-datagram. The next datagram will require re-authentication.

---

### Group Management Commands

#### `cmd_create_group(int sock, struct sockaddr_in *client_addr, char *args)`
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
7. If failed:
   - Send error string
```

**Note:** Data is saved by `fio_save_all()` after command processing.

#### `cmd_join_group(int sock, struct sockaddr_in *client_addr, char *args)`
**Purpose:** Join an existing group.

**Algorithm:**
```
1. Parse args: "group_id"
2. Call grp_join(group_id)
3. If successful:
   - Send "OK" to client
4. If failed:
   - Send error string
```

#### `cmd_leave_group(int sock, struct sockaddr_in *client_addr, char *args)`
**Purpose:** Leave a group.

**Algorithm:**
```
1. Parse args: "group_id"
2. Call grp_leave(group_id)
3. If successful:
   - Send "OK" to client
4. If failed:
   - Send error string
```

#### `cmd_search_groups(int sock, struct sockaddr_in *client_addr, char *args)`
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

#### `cmd_list_my_groups(int sock, struct sockaddr_in *client_addr)`
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

#### `cmd_list_all_groups(int sock, struct sockaddr_in *client_addr)`
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

#### `cmd_list_members(int sock, struct sockaddr_in *client_addr, char *args)`
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

#### `cmd_send_msg(int sock, struct sockaddr_in *client_addr, char *args)`
**Purpose:** Send a message to a group.

**Algorithm:**
```
1. Parse args: "group_id reply_to_id content"
2. Validate all tokens are present
3. Convert group_id and reply_to to integers
4. Call msg_send(group_id, content, reply_to)
5. If successful:
   - Send "OK <msg_id>" to client
6. If failed:
   - Send error string
```

**Note:** Data is saved by `fio_save_all()` after command processing.

#### `cmd_view_msgs(int sock, struct sockaddr_in *client_addr, char *args)`
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

#### `cmd_delete_msg(int sock, struct sockaddr_in *client_addr, char *args)`
**Purpose:** Delete a message.

**Algorithm:**
```
1. Parse args: "msg_id"
2. Call msg_delete(msg_id)
3. If successful:
   - Send "OK" to client
4. If failed:
   - Send error string
```

**Note:** Data is saved by `fio_save_all()` after command processing.

---

### User Commands

#### `cmd_list_users(int sock, struct sockaddr_in *client_addr)`
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

#### `cmd_quit(int sock, struct sockaddr_in *client_addr)`
**Purpose:** Client requests to disconnect (no-op in UDP).

**Algorithm:**
```
1. Send "OK" to client
2. Set s_quit = 1 (signals command processed)
```

**UDP Note:** Since UDP is connectionless, QUIT doesn't actually disconnect anything. It's a protocol convention.

#### `dispatch_command(int sock, struct sockaddr_in *client_addr, char *cmd_buf)`
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
6. Log command with client address and user ID
```

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

### Master-Slave Architecture with Message Queue

**Master Process:**
- Single process that runs for the lifetime of the server
- Receives UDP datagrams from any client
- Pushes datagram (command + client address) to message queue
- Forks a slave for each datagram
- Does not process any commands
- Immediately loops back to receive next datagram

**Slave Process:**
- One slave per datagram (not per client)
- Reads one datagram from message queue
- Resets authentication state (stateless)
- Processes exactly one command
- Sends response directly to client using stored address
- Saves all data to files
- Exits immediately after processing

### Message Queue Communication

**Why Message Queue?**
- UDP is connectionless, so the master cannot pass the socket to the slave
- The slave needs both the command text AND the client address to respond
- Message queue provides a clean IPC mechanism for passing this data

**Queue Attributes:**
- Name: `/groupchat_mq`
- Max messages: 10
- Message size: sizeof(DgramMsg) = 1024 + 16 = 1040 bytes
- Permissions: 0666 (read/write for all)

### Stateless Processing

**Key Difference from TCP:**
- TCP: One slave per client session, maintains state across multiple commands
- UDP: One slave per datagram, no state between datagrams

**Implications:**
- Each slave calls `auth_logout()` before processing
- Clients must re-authenticate for each command (or include credentials)
- No session management
- Simpler but more chatty protocol

### Data Saving Strategy

**TCP Version:**
- Saves individual data files after specific operations
- Example: `fio_save_users()` after registration
- More efficient but requires careful coordination

**UDP Version:**
- Saves all data after every command: `fio_save_all()`
- Simpler implementation
- Less efficient (rewrites all files even for small changes)
- Acceptable for low-to-medium load

### Process Isolation

Each slave process has:
- **Independent memory space:** No shared variables
- **Independent file descriptors:** Each has its own socket copy
- **Independent authentication state:** Each datagram is independent
- **Copy of global data:** Loaded from files at process start

### Zombie Process Prevention

Same as TCP version:
```
sigchld_handler() {
    while (waitpid(-1, NULL, WNOHANG) > 0);
}
```

**Flags:**
- `SA_RESTART`: Automatically restart interrupted system calls
- `SA_NOCLDWAIT`: Kernel automatically reaps children

---

## Data Flow

### Typical Datagram Exchange

```
Client                    Master Process           Message Queue           Slave Process
  |                           |                          |                     |
  |-- UDP Datagram --------->|                          |                     |
  |   (REGISTER user pass)    |                          |                     |
  |                           |-- mq_send() ----------->|                     |
  |                           |                          |                     |
  |                           |-- fork() -------------->|                     |
  |                           |                          |                     |
  |                           |                          |-- mq_receive() ---->|
  |                           |                          |                     |
  |                           |                          |                     |-- auth_logout()
  |                           |                          |                     |-- auth_register()
  |                           |                          |                     |-- fio_save_all()
  |                           |                          |                     |
  |<-- UDP Response ---------|--------------------------|<-- nh_send_to() ---|
  |   (OK 1)                  |                          |                     |
  |                           |                          |                     |-- exit()
  |                           |                          |                     |
```

### Multiple Concurrent Datagrams

```
Time →
      Datagram 1     Datagram 2     Datagram 3
Client1 ──────┐
              │
Master ───────┼─── MQ ──── Slave1 ──┐
              │                            │
Client2 ──────┼─── MQ ──── Slave2 ──┤     │
              │                            │     │
Client3 ──────┼─── MQ ──── Slave3 ──┼─────┼─────┘
              │                            │
Master ───────┴────────────────────────────┴─────> Ready for next
```

### File Access Pattern

**Read Operations:**
- Load entire file into memory array at process start
- Search/filter in memory
- No locking needed for reads (concurrent reads safe)

**Write Operations:**
- Rewrite entire file with updated data
- **No file locking in UDP version** (simpler but less safe)
- Relies on OS file system for atomicity
- Potential for data corruption under high contention

**Note:** Unlike TCP version, UDP version does not implement file locking. This is a trade-off for simplicity and may need to be addressed for production use.

---

## UDP vs TCP Comparison

### Architecture Differences

| Aspect | TCP Server | UDP Server |
|--------|-----------|------------|
| Connection | Connection-oriented (TCP) | Connectionless (UDP) |
| Slave Lifetime | Per client session | Per datagram |
| State Management | Maintains session state | Stateless (reset per datagram) |
| IPC Mechanism | Socket inheritance | POSIX message queue |
| Client Tracking | Socket descriptor | Client address (IP:port) |
| Multi-command Support | Yes (session loop) | No (one command per datagram) |
| Data Saving | Selective (per operation) | All data after each command |
| File Locking | Yes (flock) | No (simpler but less safe) |

### Code Differences

**TCP Command Handler:**
```c
static void cmd_register(int client_fd, char *args) {
    // Process registration
    nh_send_line(client_fd, "OK");
    fio_save_users(fio_get_users(), *fio_get_user_count());
}
```

**UDP Command Handler:**
```c
static void cmd_register(int sock, struct sockaddr_in *client_addr, char *args) {
    // Process registration
    nh_send_to(sock, "OK", client_addr);
    // No individual save - saved by fio_save_all() after dispatch
}
```

**TCP Session Loop:**
```c
static void handle_session(int client_fd) {
    while (!s_quit) {
        nh_recv_line(client_fd, cmd_buf, CMD_BUF_LEN);
        dispatch_command(client_fd, cmd_buf);
    }
}
```

**UDP Main Loop:**
```c
while (1) {
    nh_recv_from(server_fd, msg.cmd_buf, CMD_BUF_LEN, &msg.client_addr);
    mq_send(mq, (char *)&msg, sizeof(DgramMsg), 0);
    fork();
    if (pid == 0) {
        mq_receive(mq, (char *)&slave_msg, sizeof(DgramMsg), NULL);
        auth_logout(); // Stateless reset
        dispatch_command(server_fd, &slave_msg.client_addr, slave_msg.cmd_buf);
        fio_save_all(); // Save all data
        exit(0); // One datagram only
    }
}
```

### Performance Characteristics

| Metric | TCP | UDP |
|--------|-----|-----|
| Connection Overhead | High (3-way handshake) | None |
| Per-Command Overhead | Low (session established) | High (fork per command) |
| Latency | Higher (connection setup) | Lower (no connection) |
| Throughput | Higher (session reuse) | Lower (fork overhead) |
| Data Saving | Selective (efficient) | All data (inefficient) |
| File Locking | Yes (safe) | No (potential corruption) |
| Scalability | Limited by connections | Limited by fork rate |
| Reliability | Built-in (TCP) | Application-level |

### Use Cases

**Choose TCP when:**
- Multiple commands per session
- Session state is important
- Reliable delivery is critical
- Higher throughput needed
- File locking for concurrent writes is required

**Choose UDP when:**
- Single command per request
- Stateless operation is acceptable
- Low latency is critical
- Simple request-response pattern
- File locking is not a concern

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

All errors are sent as plain text datagrams:
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
- Passwords are hashed using SHA-256
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

### UDP-Specific Concerns
- No built-in authentication (IP spoofing possible)
- No built-in encryption (plaintext on wire)
- No built-in rate limiting (DoS vulnerability)
- Message queue permissions (0666 allows any local process to read)
- No file locking (potential data corruption under high load)

**Recommendations for Production:**
- Implement application-level authentication tokens
- Add rate limiting per IP address
- Use message queue with stricter permissions
- Consider DTLS for encrypted UDP
- Implement file locking for concurrent writes
- Consider selective data saving instead of `fio_save_all()`

---

## Performance Characteristics

### Scalability
- **Datagram handling:** Limited by fork rate (~100-1000/sec)
- **Memory usage:** ~2MB per slave process (short-lived)
- **Message queue:** 10 messages max (configurable)
- **File I/O:** Each slave reads/writes independently
- **Data saving:** All files rewritten after each command (inefficient)

### Bottlenecks
- **File I/O:** All files rewritten after each command
- **Process creation:** fork() overhead per datagram
- **Message queue:** Limited to 10 pending messages
- **File loading:** Each slave loads entire files
- **No file locking:** Potential corruption under high contention

### Optimization Opportunities
- Use thread pool instead of fork (reduce overhead)
- Use shared memory for read-only data
- Cache frequently accessed data
- Increase message queue size
- Implement selective data saving (like TCP version)
- Add file locking for concurrent writes
- Implement connection pooling (if switching to TCP)

---

## Testing Recommendations

### Unit Tests
- Test each command handler independently
- Test error conditions (invalid input, missing auth)
- Test file I/O operations
- Test message queue operations

### Integration Tests
- Test complete datagram exchanges
- Test concurrent access (multiple clients)
- Test group workflow (create, join, message)
- Test message threading (replies)

### Load Tests
- Test with high datagram rate
- Test message queue under contention
- Monitor fork rate and process creation
- Monitor memory usage
- Test file I/O under high load

### UDP-Specific Tests
- Test lost datagrams (simulate packet loss)
- Test out-of-order delivery
- Test duplicate datagrams
- Test message queue overflow
- Test data corruption under concurrent writes

---

## Conclusion

The Group Chat Concurrent UDP Server implements a stateless, message-queue-based concurrent architecture that provides:
- **Simplicity:** No session management to maintain
- **Low Latency:** No connection overhead
- **Isolation:** Each datagram in its own process
- **Flexibility:** Stateless design allows easy scaling

The master-slave model with POSIX message queues ensures the server can handle multiple concurrent datagrams without blocking, while the stateless design simplifies the code at the cost of requiring re-authentication for each command and less efficient data saving.

**Trade-offs:**
- **Pros:** Simple, low latency, no connection overhead
- **Cons:** High per-command overhead, stateless, inefficient data saving, no file locking

**Best suited for:** Simple request-response protocols where each interaction is independent and low latency is more important than throughput or data efficiency.

**Production Considerations:** Requires additional security measures (authentication tokens, rate limiting), file locking for concurrent writes, and selective data saving for better performance.
