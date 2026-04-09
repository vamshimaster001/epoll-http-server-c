Epoll-Based HTTP Server in C

Overview

This project is a high-performance, non-blocking HTTP server implemented in C using the Linux epoll API.
It follows a single-threaded, event-driven architecture to efficiently handle multiple concurrent client connections.

The server demonstrates core systems programming concepts such as non-blocking I/O, event loops, and per-client state management.

Features
Non-blocking sockets using O_NONBLOCK
epoll-based event loop
Handles multiple concurrent clients
Partial recv() and send() handling
Basic HTTP/1.1 request parsing (method, path, headers)
Supports HTTP GET requests
Keep-alive connection support
Static file serving (HTML, CSS, JS, text files)
Chunked file transfer using fixed-size buffers
Per-client state management using a state machine
Proper HTTP responses:
200 OK
400 Bad Request
405 Method Not Allowed
500 Internal Server Error
Architecture

The server uses an event-driven model:

Create a non-blocking listening socket
Register the socket with epoll
Wait for events using epoll_wait()
Handle events:
EPOLLIN: receive and parse request
EPOLLOUT: send response (headers and file data)
Maintain state for each connected client
Repeat

This design allows a single thread to handle many connections efficiently.

Design Highlights
Event-driven architecture using epoll
Per-client state machine (receive → process → send)
Handles partial sends using offset tracking
Efficient file streaming without loading entire file into memory
Supports persistent connections using Connection: keep-alive
Project Structure

server.c - main server implementation
Makefile - build configuration
README.md - project documentation

Build Instructions

Using Makefile:

make

Or compile manually:

gcc -o server server.c

Run

Start the server:

./server

The server listens on:

http://localhost:9999

Testing

Basic request:

curl http://127.0.0.1:9999/

Test different endpoints:

curl http://127.0.0.1:9999/about

curl http://127.0.0.1:9999/style.css

Invalid method:

curl -X POST http://127.0.0.1:9999/

Load testing:

ab -n 1000 -c 100 http://127.0.0.1:9999/

Example Response
<html> <head> <link rel="stylesheet" href="/style.css"> <script src="/app.js"></script> </head> <body> <h1>Hello from HTTP server</h1> <button onclick="showMessage()">Click Me</button> </body> </html>
Limitations
Supports only basic HTTP (GET requests)
No HTTPS/TLS support
No request body parsing (POST not supported)
No advanced routing framework
No HTTP pipelining optimization
Future Improvements
Add multi-threading (thread pool)
Support POST and PUT methods
Improve HTTP parsing robustness
Add structured logging and metrics
Implement zero-copy file transfer using sendfile
Add HTTPS support
What I Learned
Working with epoll for scalable I/O
Handling non-blocking sockets correctly
Designing event-driven systems
Managing partial reads and writes
Building a real HTTP server from scratch
Author

Vamshi Bijula

Summary

This project demonstrates how to build a high-performance, event-driven HTTP server from scratch using C and epoll, applying core systems programming and networking concepts used in real-world servers.
