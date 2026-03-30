# SCS3304 Group Chat Application

A comprehensive group chat application available in both standalone and client-server architectures.

## 🚀 Available Versions

### 📱 Standalone Version (v1.0)
Single executable with all functionality in one process. Features professional messaging interface with search, editing, and threading capabilities.

### 🌐 Client-Server Version (v2.0)
Distributed architecture with TCP server and separate client processes.

---

## � Standalone Version Features

### Core Functionality
- **User Management**: Registration, login, authentication
- **Group Management**: Create, join, leave, search groups
- **Messaging**: Send messages, threaded replies, real-time updates
- **Search**: Find groups and messages by keyword
- **Message Actions**: Edit and delete own messages
- **Data Persistence**: Pipe-delimited text file storage

### Advanced Features
- **Beautiful UI**: Boxed message formatting with visual separators
- **Message Threading**: Hierarchical reply structure
- **Search Functionality**: Case-insensitive keyword search
- **User-Friendly Interface**: Intuitive menu navigation
- **Cross-Platform**: ANSI C99 standard compliance

## � Client-Server Version Features

### Architecture
- **TCP Server**: Handles all business logic and data storage
- **TCP Client**: Terminal UI only, communicates with server
- **Network Protocol**: Custom TCP protocol for client-server communication
- **Data Storage**: Binary files stored on server side only

### Key Differences from Standalone
- **Distributed**: Server and client run as separate processes
- **Network Access**: Clients can connect from different machines
- **Centralized Data**: All data managed by server
- **Scalability**: Multiple clients can connect (iterative server)

---

## �🏗️ Architecture Comparison

### Standalone (Three-Layer Modular Design)
```
┌─────────────────────────────────────┐
│        PRESENTATION LAYER           │
│         (main.c)                   │
│  • Menu handling & user input      │
│  • Routing & display logic         │
└─────────────────────────────────────┘
            ↓
┌─────────────────────────────────────┐
│      APPLICATION LOGIC LAYER        │
│   (auth.c, groups.c, messaging.c)  │
│  • Business logic & algorithms     │
│  • Data validation & processing    │
└─────────────────────────────────────┘
            ↓
┌─────────────────────────────────────┐
│        FILE STORAGE LAYER           │
│      (file_handler.c)              │
│  • File I/O operations only        │
│  • Data persistence & loading       │
└─────────────────────────────────────┘
```

### Client-Server (Network Architecture)
```
Client Process                    Server Process
┌─────────────────┐         ┌─────────────────────────┐
│  Terminal UI    │         │   Business Logic       │
│  Network Client │◄─TCP────►│   File Storage         │
│  (client/)      │         │   (server/)            │
└─────────────────┘         └─────────────────────────┘
```

---

## 📁 Project Structure

### Standalone Version (v1.0)
```
group_chat_application/
├── include/           # Header files
│   ├── config.h      # Constants & data structures
│   ├── file_handler.h # File I/O prototypes
│   ├── auth.h        # Authentication prototypes
│   ├── groups.h      # Group management prototypes
│   ├── msgs.h        # Messaging prototypes
│   └── utils.h       # Utility prototypes
├── src/              # Source files
│   ├── main.c        # Presentation layer
│   ├── auth.c        # Authentication logic
│   ├── groups.c      # Group management logic
│   ├── messaging.c   # Messaging logic
│   ├── utils.c       # Utility functions
│   └── file_handler.c # File storage layer
├── build/            # Compiled binary
├── docs/             # Documentation
└── *.txt            # Runtime data files
```

### Client-Server Version (v2.0)
```
group-chat/
├── server/           # TCP server
│   ├── include/     # Server headers
│   ├── src/         # Server source
│   ├── data/        # Binary data files
│   └── Makefile     # Server build
├── client/           # TCP client
│   ├── include/     # Client headers
│   ├── src/         # Client source
│   └── Makefile     # Client build
└── README.md        # This file
```

---

## 🛠️ Installation & Setup

### Standalone Version (v1.0)
```bash
cd group_chat_application
gcc -Wall -Wextra -std=c99 -I./include -o build/group_chat \
    src/main.c src/auth.c src/groups.c src/messaging.c src/utils.c src/file_handler.c
./build/group_chat
```

### Client-Server Version (v2.0)
```bash
# Step 1: Build the server
cd server && make

# Step 2: Build the client
cd client && make

# Step 3: Run the server first (Terminal 1)
cd server && make run

# Step 4: Run the client (Terminal 2)
cd client && make run
```

---

## 📖 Usage Guide

### Getting Started
1. **Register a new account** or **Login** with existing credentials
2. **Create a group** or **Join existing groups**
3. **Send messages** and **participate in conversations**

### Menu Navigation

#### Main Menu (Logged Out)
```
1. Register    - Create new user account
2. Login       - Authenticate existing user
3. Search Groups - Discover public groups
0. Exit        - Quit application
```

#### User Menu (Logged In)
```
1. Create Group     - Start a new conversation
2. Join Group       - Join existing group
3. Leave Group      - Leave a group
4. Search Groups    - Find groups by keyword
5. My Groups        - List your groups
6. View Messages    - Read group conversations
7. Send Message     - Post new message
8. Reply to Message - Respond to messages
9. Search Messages  - Find messages by keyword
10. Edit Message    - Modify your messages
11. Delete Message  - Remove your messages
12. Logout          - Sign out
0. Exit            - Quit application
```

### Message Features

#### Beautiful Formatting
```
┌─ Message #1 ───────────────────────
│ 👤 shammah • 2026-03-12 21:37:10
│
│ Hello everyone! Welcome to our group.
└─────────────────────────────────────

  ├─ Reply #2 ────────────────────
  │ 👤 alice • 2026-03-12 21:38:15
  │
  │ Thanks for setting this up!
  └─────────────────────────────────
```

#### Search Capabilities
- **Group Search**: Find groups by name/description
- **Message Search**: Search messages by keyword (case-insensitive)
- **Empty Search**: Show all items when keyword is empty

#### Message Management
- **Edit Messages**: Modify your own messages
- **Delete Messages**: Remove your own messages
- **Threaded Replies**: Hierarchical conversation structure

---

## 💾 Data Storage

### Standalone Version (Pipe-Delimited Text)
- **users.txt**: `user_id|username|display_name|password_hash|created_at`
- **groups.txt**: `group_id|name|description|creator_id|members|member_count|is_active|created_at`
- **messages.txt**: `msg_id|group_id|sender_id|content|reply_to|is_deleted|sent_at`

### Client-Server Version (Binary Files)
- **server/data/users.dat**: Binary user data
- **server/data/groups.dat**: Binary group data  
- **server/data/messages.dat**: Binary message data

### Security Features
- **Password Hashing**: djb2 hash algorithm
- **Data Validation**: Input sanitization and length checks
- **Access Control**: Users can only edit/delete own messages

---

## 🔧 Technical Details

### Key Algorithms
- **Hash Function**: djb2 for password hashing
- **Search**: Case-insensitive substring matching
- **ID Generation**: Max ID + 1 for unique identifiers
- **File I/O**: Sequential read/write with token parsing

### Memory Management
- **Static Arrays**: Fixed-size arrays for users, groups, messages
- **Constants**: Configurable limits in `config.h`
- **Buffer Safety**: String length checks and null termination

### Error Handling
- **Return Codes**: Consistent error reporting system
- **User Feedback**: Clear error messages and prompts
- **Graceful Degradation**: Safe handling of invalid inputs

---

## 🎯 Design Principles

### Software Engineering
- **Modularity**: Clear separation of concerns
- **Maintainability**: Well-documented, structured code
- **Extensibility**: Easy to add new features
- **Testability**: Modular functions for unit testing

### User Experience
- **Intuitive Navigation**: Logical menu flow
- **Visual Clarity**: Formatted output with separators
- **Error Prevention**: Input validation and helpful messages
- **Consistency**: Uniform interface patterns

---

## 🚀 Future Enhancements

### Planned Features
- [ ] Real-time messaging with sockets (✅ Client-server version)
- [ ] Private messaging between users
- [ ] File attachment support
- [ ] Message reactions/likes
- [ ] User profiles and avatars
- [ ] Group administration features
- [ ] Message encryption
- [ ] Web interface

### Technical Improvements
- [ ] Dynamic memory allocation
- [ ] Database integration (SQLite)
- [ ] Configuration file support
- [ ] Logging system
- [ ] Unit testing framework
- [ ] Internationalization support

---

## 📊 Project Statistics

- **Languages**: C (ANSI C99)
- **Files**: 15+ source files per version
- **Lines of Code**: 2,632+ (standalone), 3,000+ (client-server)
- **Architecture**: 3-layer modular design + Network architecture
- **Standards**: ANSI C99 compliance

---

## 🤝 Contributing

### Development Setup
1. Fork the repository
2. Create feature branch: `git checkout -b feature-name`
3. Make changes and test thoroughly
4. Commit changes: `git commit -m "Add feature description"`
5. Push to branch: `git push origin feature-name`
6. Submit pull request

### Coding Standards
- Follow ANSI C99 standards
- Use consistent naming conventions
- Add comprehensive comments
- Maintain modular architecture
- Test all functionality

---

## 📝 License

This project is developed for educational purposes as part of SCS3304 coursework.

---

## 🙏 Acknowledgments

- **SCS3304 Course** - Systems Programming concepts
- **C Standard Library** - String handling, time functions
- **ANSI/ISO Standards** - C99 compliance guidelines

---

**Author**: shammahpaluku  
**Email**: skyssando@gmail.com  
**Course**: SCS3304 - Systems Programming  
**Versions**: 
- v1.0 - Standalone Complete Messaging Interface
- v2.0 - Client-Server Network Implementation

---

> 🚀 **Built with passion for systems programming and clean software architecture!**
