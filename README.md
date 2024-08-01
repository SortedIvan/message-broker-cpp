# message-queue-cpp
Implementation of a message queue broker server with support for multiple clients. 
It includes creating topics and having clients be either publisher or subscribers (or both). Publishers can send messages to other subscribers.

Built with C++ and SFML's (simple fast multi-media library) networking module. The repository consists of two executables, one for client and one for server. The client is initialized with a pre-defined port, together with localhost IP.
