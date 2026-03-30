# Group Chat Application
SCS3304 Assignment 2 — Client-Server Version

## Project Structure
  server/   — TCP server, all business logic and binary data files
  client/   — TCP client, terminal UI only

## How to Build and Run

  Step 1: Build the server
    cd server && make

  Step 2: Build the client
    cd client && make

  Step 3: Run the server first (Terminal 1)
    cd server && make run

  Step 4: Run the client (Terminal 2)
    cd client && make run

## Notes
  - Server must be running before the client is started
  - Server binds to 127.0.0.1:9200
  - All data files are stored in server/data/ as binary files
  - Client has no access to data files directly
  - One client session is handled at a time (iterative server)
