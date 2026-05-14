## High-Performance Event-Driven HTTP Server (C++ | Linux)

### Project Overview

Designed and developed a scalable HTTP server from scratch in C++ using low-level Linux networking APIs and event-driven architecture principles.

### Key Features

* Implemented TCP socket communication using Linux socket APIs
* Built non-blocking server architecture using `fcntl()` and `O_NONBLOCK`
* Integrated `epoll` for efficient I/O multiplexing and concurrent client handling
* Used Edge-Triggered (`EPOLLET`) event handling for high-performance networking
* Developed asynchronous reactor-style event loop
* Implemented HTTP request parsing and dynamic response generation
* Added support for multiple routes/endpoints
* Implemented persistent per-client buffering for partial TCP packet handling
* Added Keep-Alive connection support and connection lifecycle management
* Handled partial reads, fragmented packets, and disconnect scenarios robustly
* Designed scalable architecture similar to modern high-performance servers

### Technical Concepts Used

* Linux System Programming
* TCP/IP Networking
* Socket Programming
* Event-driven Architecture
* Non-blocking I/O
* epoll API
* HTTP Protocol
* Reactor Pattern
* Concurrent Client Handling
* Buffer Management

### Technologies Used

* C++
* Linux
* POSIX Socket APIs
* epoll
* TCP/IP
* HTTP/1.1

### Skills Demonstrated

* Low-level systems programming
* Backend server architecture
* Linux networking internals
* Asynchronous programming
* Performance-oriented design
* Debugging and connection management
* Scalable server development
