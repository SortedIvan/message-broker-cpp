# message-broker-cpp
Implementation of a message broker server with support for multiple clients. 
It includes creating topics and having clients be either publisher or subscribers (or both). Publishers can send messages to other subscribers.
Built with C++ and SFML's (simple fast multi-media library) networking module. The repository consists of two executables, one for client and one for server. The client is initialized with a pre-defined port, together with localhost IP.

A message broker is a server client application that serves as an implementatation to the <b>pub-sub paradigm</b>. Pub-sub stands for publisher-subscriber and it describes a type of application behaviour where some application instances can be <b>publishers</b> (submit information) and others can be <b>subscribers</b> (receive information). Information is published and received under the form of messages, which are just encapsulated data, representing different actions. Messages are stored in <b>topics</b>, which are simply queues that follow FIFO (First in first out). 


<h4> Client architecture </h4>

![client drawio](https://github.com/user-attachments/assets/c0fc3216-3657-42e7-8f4b-879b89e1b391)

<h4> Server architecture </h4>

![server drawio](https://github.com/user-attachments/assets/c9984c22-54b1-4290-b8ae-6ac89f885868)


To install and run in debug:
1) Clone the repository
2) Download latest SFML version from [here](https://www.sfml-dev.org/download.php)
3) Extract SFML & navigate to bin folder
4) Copy all dll's that have -d at the end (and openal32.dll) 
5) Navigate to client -> x64 -> debug and paste all files here
6) Navigate to server -> x64 -> debug and paste all files here
7) Run the server and run n instances of the client
