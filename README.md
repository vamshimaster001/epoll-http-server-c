Epoll-Based HTTP Server in C

Overview
--------
This project is a high-performance, non-blocking HTTP server implemented in C using the Linux epoll API.  
It follows a single-threaded, event-driven architecture to efficiently handle multiple concurrent client connections.

The server demonstrates core systems programming concepts such as non-blocking I/O, event loops, and per-client state management.

Features
--------
- Non-blocking sockets using O_NONBLOCK
- epoll-based event loop
- Handles multiple concurrent clients
- Partial recv() and send() handling
- Basic HTTP request parsing (GET)
- Keep-alive connection support
- Per-client state management

Architecture
------------
The server uses an event-driven model:

1. Create a non-blocking listening socket
2. Register the socket with epoll
3. Wait for events using epoll_wait()
4. Handle events:
   - EPOLLIN: receive data from client
   - EPOLLOUT: send response to client
5. Maintain state for each connected client
6. Repeat

This design allows a single thread to handle many connections efficiently.

Project Structure
-----------------
server.c    - main server implementation
Makefile    - build configuration
README.md   - project documentation

Build Instructions
------------------
Run:
    make

Or compile manually:
    gcc -o server server.c

Run
---
Start the server:
    ./server

The server listens on:
    http://localhost:9999

Testing
-------
Using curl:
    curl http://localhost:9999

Basic load testing:
    ab -n 1000 -c 100 http://127.0.0.1:9999/

Limitations
-----------
- Supports only basic HTTP (GET requests)
- No HTTPS/TLS support
- Minimal HTTP header parsing
- No routing or static file serving

Future Improvements
-------------------
- Add multi-threading (thread pool)
- Support POST/PUT methods
- Serve static files
- Improve HTTP parsing
- Add logging and metrics
- Add HTTPS support

What I Learned
--------------
- Working with epoll for scalable I/O
- Handling non-blocking sockets correctly
- Designing event-driven systems
- Managing partial reads and writes
- Building a simple HTTP server from scratch

Topics
------
c
linux
epoll
http-server
socket-programming
network-programming
systems-programming
non-blocking-io
event-driven
io-multiplexing
event-loop
tcp-ip
Author
------
Vamshi Bijula
