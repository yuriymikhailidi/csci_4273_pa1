# Programming Assigment 1

### Yuriy Mikhailidi

# Introduction

The directrory contains the neccessary files to run the applications localy.
The applications are UDP client and server. Running the "make all" command will create two executable files "clientapp" and "serverapp". Using the commands the user can send and recieve the files to and from server.
Client can print the directory and delete files as desired. Running the command exit will exit the client application.

# Usage

Client application takes user command and filename sends the information to server. Following commands are:
- get [filename]
  - get command receives file from the server to client.
    The server finds file in the directory, copies it to buffer, then sends the buffer to the client.
    Server and client will acknowledge the transmission and print the information for success or fail.
- put [filename]
  - put command sends file to the server.
    The server copies it from buffer sent by client, then saves the buffer to file.
    Server and client will acknowledge the transmission and print the information for success or fail.
- delete [filename]
    - delete command finds file in the directory, then deletes the file.
      Server and client will acknowledge the transmission and print the information for success or fail.
- ls  
  - ls command prints the server directory.
    Server and client will acknowledge the transmission and print the information for success or fail.
- exit
  - exit command will be acknowledged by the server.
    The client exits while the server continuous to run in the background.

#### Note: If user does not provide the filename, the applications will throw and error.

# Resources used

- Course Material: Open source book “Computer Networks: A Systems Approach”
- https://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program/17683417
- https://www.tutorialkart.com/c-programming/c-delete-file/
- https://stackoverflow.com/questions/238603/how-can-i-get-a-files-size-in-c
- https://www.tutorialkart.com/c-programming/c-delete-file/
- https://stackoverflow.com/questions/238603/how-can-i-get-a-files-size-in-c
- https://www.geeksforgeeks.org/udp-server-client-implementation-c/
- https://stackoverflow.com/questions/57794550/sending-large-files-over-udp

