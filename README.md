🚀 Epoll-Based HTTP Server in C
📌 Overview

This project is a high-performance, non-blocking HTTP server built in C using Linux epoll.
It uses a single-threaded, event-driven architecture to efficiently handle multiple concurrent client connections.

The server demonstrates core systems programming concepts such as non-blocking I/O, event loops, and stateful connection handling — similar to how real-world high-performance servers operate.

🧠 Key Concepts
Non-blocking sockets (O_NONBLOCK)
epoll-based event-driven I/O
Edge-triggered / level-triggered event handling
Partial recv() / send() handling
HTTP request parsing
Keep-alive connections
Per-client state management
Efficient connection lifecycle handling
🏗️ Architecture

The server follows an event loop model:

Create a non-blocking listening socket
Register it with epoll
Wait for events using epoll_wait()
Handle events:
EPOLLIN → read incoming data
EPOLLOUT → send response
Maintain per-client state (buffers, progress)
Repeat

This allows a single thread to handle thousands of connections efficiently.

🔄 Event Flow

Client Request → EPOLLIN → recv() → Parse HTTP → Prepare Response → EPOLLOUT → send() → Done / Keep-alive

📁 Project Structure
.
├── server.c        # Main server implementation
├── Makefile        # Build configuration
├── README.md       # Project documentation
⚙️ Build Instructions
make

Or manually:

gcc -o server server.c
▶️ Run
./server

Server runs on:

http://localhost:9999
🧪 Test the Server
Using curl
curl http://localhost:9999
Benchmark (optional)
ab -n 1000 -c 100 http://127.0.0.1:9999/
🧩 Features
Handles multiple clients concurrently
Supports HTTP/1.1 basic requests
Keep-alive connection support
Handles partial reads/writes (non-blocking safe)
Efficient memory usage with per-client buffers
⚠️ Limitations
Supports basic HTTP (GET only)
No TLS/HTTPS
No advanced routing
Minimal header parsing
🔮 Future Improvements
Multi-threaded version (thread pool)
Support for POST/PUT requests
Static file serving
HTTP pipelining support
Logging & metrics
TLS (HTTPS) support
📚 What I Learned
Deep understanding of epoll and event-driven systems
Handling non-blocking I/O correctly
Designing per-connection state machines
Managing partial reads/writes safely
Building scalable network servers in C
