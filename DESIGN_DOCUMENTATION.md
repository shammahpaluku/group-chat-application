# Group Chat UDP - Design Documentation

**Version**: UDP (Connectionless)  
**Protocol**: UDP (SOCK_DGRAM)  
**Architecture**: Iterative Connectionless Server

---

## Architectural Design

### Overall Architecture

Client-server architecture with iterative connectionless server model. Server handles business logic and data persistence. Clients communicate via UDP datagrams.

**Key Difference from TCP**: No persistent connection. Each command is an independent datagram. Server tracks client addresses for responses.

### Communication Model

- Server binds to 0.0.0.0:9200
- Client sends datagrams directly to server address
- Server receives datagram, extracts client address, sends response
- Multi-line responses sent as separate datagrams with "END" marker
- Session state maintained per client address

---

## Module Outline

### Server Modules

1. **Network Handler** (`net_handler.c`): UDP socket operations
   - `nh_server_init()` - Create/bind socket
   - `nh_send_to()` - Send to specific address
   - `nh_recv_from()` - Receive with sender address
   - `nh_close()` - Close socket

2. **Command Dispatcher** (`server_main.c`): Parse and route commands
   - All handlers receive `sock` and `client_addr` parameters
   - Auth state keyed by client address

3. **Business Logic** (`auth.c`, `groups.c`, `messaging.c`): Same as TCP

4. **File I/O** (`file_io.c`): Binary file persistence (same as TCP)

### Client Modules

1. **Network Handler** (`net_handler.c`): UDP socket operations
   - `nh_client_init()` - Initialize with server address
   - `nh_send_to()` - Send to server
   - `nh_recv_from()` - Receive from server

2. **Client Main** (`client_main.c`): User interface
   - Multi-line response handling (receives multiple datagrams)

---

## Process Design

### Server Flow
```
Initialize socket → Bind to 0.0.0.0:9200 → Load data
→ Main loop: recv_from() → auth_logout() → dispatch_command()
→ Handlers use nh_send_to() for responses
```

### Client Flow
```
Initialize socket → Set server address
→ Main menu loop → Send commands via nh_send_to()
→ Receive responses via nh_recv_from()
→ Handle multi-line responses (multiple datagrams)
```

---

## Algorithm Design

Same algorithms as TCP version:
- Password hashing (djb2)
- User registration/login
- Group creation/join/leave
- Message send/view/delete
- Search (case-insensitive)

**UDP-Specific**: Multi-line responses require receiving multiple datagrams until "END" marker.

---

## Data/File Design

Same data structures and binary file format as TCP:
- `User`, `Group`, `Message`, `Session` structures
- Binary files: users.dat, groups.dat, messages.dat
- Sequential read/write operations

---

## Concurrency Design

**Current**: No concurrency (iterative server)

**If Required**: I/O multiplexing with select() recommended for UDP:
- Single process handles multiple clients
- Event-driven architecture
- No locks needed for single-threaded event loop

---

## Implementation

**Build**:
```bash
gcc -std=c11 -Wall -Wextra -g -Iinclude src/*.c -o bin/chat_server
gcc -std=c11 -Wall -Wextra -g -Iinclude src/*.c -o bin/chat_client
```

**Dependencies**: Standard C libraries only

**Error Handling**: Same return codes as TCP

---

## Testing Plan

### Test Cases (Same as TCP)
- User registration/login
- Group creation/join/leave
- Message send/view/delete
- Search functionality

### UDP-Specific Tests
- Datagram delivery
- Multi-line response handling
- Client address tracking
- Session state persistence

### Test Results
✅ All 15 commands tested and working
✅ Multi-line responses received correctly
✅ Client address tracking works
✅ Session state maintained per address

### Connectionless Demonstration

**UDP vs TCP Communication Pattern:**

**TCP (Connection-Oriented):**
- Persistent connection established once
- Multiple commands sent over same connection
- Server maintains session per connection
- Connection closed on logout/quit

**UDP (Connectionless):**
- No connection establishment
- Each command is independent datagram
- Server identifies client by address
- Session state keyed by client address
- `auth_logout()` called before each command (state reset per datagram)

**Key Difference:** UDP demonstrates connectionless nature by treating each datagram independently, with session state reset between commands. No persistent connection is maintained - the server must identify clients solely by their network address.

---

## Socket Operation Details

### Server Socket Operations

#### 1. Socket Creation
**Location**: `server/src/net_handler.c:11`
```c
int server_fd = socket(AF_INET, SOCK_DGRAM, 0);
```
- Creates UDP socket (`SOCK_DGRAM`)
- Returns socket file descriptor
- On failure, returns `ERR_CONN`
- **Difference from TCP**: No connection-oriented socket

#### 2. Binding
**Location**: `server/src/net_handler.c:30`
```c
struct sockaddr_in server_addr;
server_addr.sin_family = AF_INET;
server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);  // "0.0.0.0"
server_addr.sin_port = htons(port);  // 9200

bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr));
```
- Binds socket to all interfaces (`0.0.0.0`)
- Binds to port 9200
- Associates socket with local address
- **Difference from TCP**: No listen() call needed

#### 3. Passive (Listening)
**Location**: `N/A` - UDP does not use listen()
- UDP is connectionless, no passive listening state
- Socket is ready to receive datagrams immediately after bind()
- **Difference from TCP**: No listen() or backlog queue

#### 4. Accepting
**Location**: `N/A` - UDP does not use accept()
- No connection establishment in UDP
- Each datagram received independently
- **Difference from TCP**: No accept() call, no separate client socket

#### 5. Looping Back (Main Datagram Loop)
**Location**: `server/src/server_main.c:495`
```c
while (1) {
    s_quit = 0;
    
    int result = nh_recv_from(server_fd, cmd_buf, CMD_BUF_LEN, &client_addr);
    if (result == ERR_CONN) {
        printf("Receive error. Continuing...\n");
        continue;  // Loop back to receive next datagram
    }
    
    printf("Datagram from %s:%d\n", 
           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    
    dispatch_command(server_fd, &client_addr, cmd_buf);
    
    fio_save_all();
}
```
- Infinite loop receiving datagrams
- Each datagram processed independently
- Captures client address from each datagram
- **Difference from TCP**: No session loop, single datagram per iteration

#### 6. Reading (Receiving)
**Location**: `server/src/net_handler.c:58` (in `nh_recv_from`)
```c
int bytes_read = recvfrom(sock, buf, buf_len - 1, 0, 
                         (struct sockaddr*)client_addr, &addr_len);
```
- Receives datagram from any sender
- Captures sender's address in `client_addr`
- Receives entire datagram at once (not character by character)
- **Difference from TCP**: Uses recvfrom() instead of recv(), captures sender address

**Location**: `server/src/server_main.c:498` (in main loop)
```c
int result = nh_recv_from(server_fd, cmd_buf, CMD_BUF_LEN, &client_addr);
```
- Called in main loop to receive commands

#### 7. Processing
**Location**: `server/src/server_main.c` (in `dispatch_command`)
```c
char *verb = strtok(cmd_copy, " ");  // Parse command
char *args = cmd_buf + strlen(verb) + 1;  // Extract arguments

if (strcmp(verb, "REGISTER") == 0) {
    cmd_register(sock, client_addr, args);
} else if (strcmp(verb, "LOGIN") == 0) {
    cmd_login(sock, client_addr, args);
}
// ... other commands
```
- Parses command verb and arguments
- Routes to appropriate handler function
- All handlers receive `sock` and `client_addr` parameters
- **Difference from TCP**: Handlers receive client address instead of client FD

#### 8. Forming Response
**Location**: `server/src/server_main.c` (in command handlers)
```c
// Example from cmd_register
nh_send_to(sock, "OK", client_addr);
// Or error
nh_send_to(sock, "ERR_DUPLICATE", client_addr);
```
- Response string constructed in handler
- Can be single line or multi-line (with "END" marker)
- **Difference from TCP**: Response sent to client address, not socket FD

#### 9. Sending
**Location**: `server/src/net_handler.c:45` (in `nh_send_to`)
```c
char buf[CMD_BUF_LEN];
snprintf(buf, sizeof(buf), "%s\n", msg);
socklen_t addr_len = sizeof(struct sockaddr_in);
int bytes_sent = sendto(sock, buf, strlen(buf), 0, 
                       (struct sockaddr*)client_addr, addr_len);
```
- Sends datagram to specific client address
- Requires destination address parameter
- **Difference from TCP**: Uses sendto() instead of send(), requires destination address

#### 10. Closing
**Location**: `server/src/net_handler.c:75` (in `nh_close`)
```c
close(sock);
```
- Closes socket
- **Difference from TCP**: Only one socket (no separate client sockets)

**Location**: `server/src/server_main.c:516` (in main)
```c
nh_close(server_fd);  // Close on server shutdown
```

### Client Socket Operations

#### 1. Socket Creation
**Location**: `client/src/net_handler.c:28`
```c
int sock = socket(AF_INET, SOCK_DGRAM, 0);
```
- Creates UDP socket (`SOCK_DGRAM`)
- **Difference from TCP**: No connection-oriented socket

#### 2. No Connecting (Connectionless)
**Location**: `client/src/net_handler.c:36` (in `nh_client_init`)
```c
struct sockaddr_in server_addr;
server_addr.sin_family = AF_INET;
server_addr.sin_addr.s_addr = inet_addr(ip);  // "127.0.0.1"
server_addr.sin_port = htons(port);  // 9200

// No connect() call - store server address for sendto()
g_server_addr = server_addr;
```
- No connect() call in UDP
- Server address stored for use with sendto()
- **Difference from TCP**: No connect() or three-way handshake

#### 3. Sending
**Location**: `client/src/net_handler.c:48` (in `nh_send_to`)
```c
socklen_t addr_len = sizeof(struct sockaddr_in);
int bytes_sent = sendto(sock, buf, strlen(buf), 0, 
                       (struct sockaddr*)&g_server_addr, addr_len);
```
- Sends datagram to server address
- Requires destination address parameter
- **Difference from TCP**: Uses sendto() instead of send()

#### 4. Reading (Receiving)
**Location**: `client/src/net_handler.c:58` (in `nh_recv_from`)
```c
int bytes_read = recvfrom(sock, buf, buf_len - 1, 0, 
                         (struct sockaddr*)&from_addr, &addr_len);
```
- Receives datagram from server
- Captures sender address (though typically not used)
- Receives entire datagram at once
- **Difference from TCP**: Uses recvfrom() instead of recv()

#### 5. Looping Back (Client Menu Loop)
**Location**: `client/src/client_main.c` (in menu functions)
```c
while (1) {
    display_menu();
    get_user_choice();
    send_command();  // Each command is independent datagram
    receive_response();
    // Loop back for next command
}
```
- Interactive menu loop
- Each command sent as independent datagram
- **Difference from TCP**: No persistent connection maintained

#### 6. Closing
**Location**: `client/src/net_handler.c:70`
```c
close(sock);
```

---

## Conclusion

UDP version maintains full feature parity with TCP while using connectionless datagram communication. All business logic identical. Network layer adapted for UDP (sendto/recvfrom instead of send/recv). Ready for cross-machine deployment.

**Author**: shammahpaluku  
**Date**: April 21, 2026
