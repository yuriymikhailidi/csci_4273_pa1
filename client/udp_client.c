/*
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

#define BUFSIZE 1024

/*
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

int main(int argc, char **argv) {
    int sockfd, portno, sendToBytes, bytesSent, bytesRead,
            j, s, bytesReceived, fileBufferBytes, fileSizeReceiver, lsDirReceiver, deleteIndicator;
    int serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char command[BUFSIZE], fileName[BUFSIZE];
    char buf[BUFSIZE],
            fileStorage[BUFSIZE], lsString[BUFSIZE], deleteMessage[BUFSIZE], exitMessage[BUFSIZE];
    size_t  fileSize;


    /* check command line arguments */
    if (argc != 3) {
        fprintf(stderr, "usage: %s <hostname> <port>\n", argv[0]);
        exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr, "ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *) server->h_addr,
          (char *) &serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

    /* get a message from the user */
    while (1) {
        /* get a message from the user */
        bzero(buf, BUFSIZE);

        printf("Enter get <filename>, put <filename>, delete <filename>, ls or exit:");
        fgets(buf, BUFSIZE, stdin);
        sscanf(buf, "%s %s", &command, &fileName);
        /* send the message to the server */
        serverlen = sizeof(serveraddr);
        sendToBytes = sendto(sockfd, buf, strlen(buf), 0,
                             &serveraddr, serverlen);

        if (sendToBytes < 0)
            error("ERROR in sendto client side");

        if (strcmp(command, "get") == 0) {
            char path[] = "./client/";
            j = recvfrom(sockfd, command, BUFSIZE, 0, (struct sockaddr *) &serveraddr,& serverlen);
            if(j < 0)
                error("ERROR in ack command");

            s = recvfrom(sockfd, fileName, BUFSIZE, 0, (struct sockaddr *) &serveraddr, &serverlen);
            if(s < 0)
                error("ERROR in ack file name");

            printf("server received command: %s\n", command);
            printf("server received file name: %s\n", fileName);

            /*get file size from server */
            fileSizeReceiver = recvfrom(sockfd, &fileSize, sizeof(fileSize), 0, (struct sockaddr *) &serveraddr, &serverlen);
            if (fileSizeReceiver < 0)
                error("ERROR in recvfrom on server side for file size");
            strcat(path, fileName);
            FILE*  filePointer = fopen(path, "wb");
            if (!filePointer)
                error("ERROR in fopen on client side");

            //file opened, so receive the file
            bytesReceived = 0;
            while (bytesReceived < fileSize) {
                bzero(fileStorage, BUFSIZE);
                /* receive the buffer */
                fileBufferBytes = recvfrom(sockfd, fileStorage, BUFSIZE, 0, (struct sockaddr *) &serveraddr, &serverlen);
                if (fileBufferBytes < 0)
                    error("ERROR in recvfrom get on client side");

                /*write as many as received till fileSize*/
                bytesReceived += fwrite(fileStorage, 1, fileBufferBytes, filePointer);
            }
            printf("Client Received %d bytes \n", bytesReceived);

            /*ack the file name success */
            fclose(filePointer);
            s = recvfrom(sockfd, fileName, BUFSIZE, 0, (struct sockaddr *) &serveraddr, &serverlen);
            if(s < 0)
                error("ERROR in ack file name");
            printf("File received %s successfully\n", fileName);
            bzero(command, BUFSIZE);
            bzero(fileName, BUFSIZE);
        }
        else if (strcmp(command, "put") == 0) {
            /* ack the command and file name */
            char path[] = "./client/";
            j = recvfrom(sockfd, command, BUFSIZE, 0, (struct sockaddr *) &serveraddr,& serverlen);
            if(j < 0)
                error("ERROR in ack command");
            s = recvfrom(sockfd, fileName, BUFSIZE, 0, (struct sockaddr *) &serveraddr, &serverlen);

            if(s < 0)
                error("ERROR in ack file name");

            printf("server received command: %s\n", command);
            printf("server received file name: %s\n", fileName);
            strcat(path, fileName);
            FILE* filePointer = fopen(path, "rb");
            /*check if file is open, transmit error */
            if (filePointer == 0) {
                fileSize = -99999;
                sendto(sockfd, &fileSize, sizeof(fileSize), 0, &serveraddr, serverlen);
                error("ERROR in fopen put client side");
            }

            // https://stackoverflow.com/questions/238603/how-can-i-get-a-files-size-in-c
            fseek(filePointer, 0, SEEK_END);
            fileSize = ftell(filePointer);
            fseek(filePointer, 0, SEEK_SET);

            /*send file size to server */
            sendToBytes = sendto(sockfd, &fileSize, sizeof (fileSize), 0,
                                 &serveraddr, serverlen);

            if (sendToBytes < 0)
                error("ERROR in sendto client");

            bytesSent = 0;
            while (bytesSent < fileSize) {
                /*clear the buffer */
                bzero(fileStorage, BUFSIZE);

                /* read as many bytes as possible*/
                bytesRead = fread(fileStorage, 1, BUFSIZE, filePointer);
                if(bytesRead < 0)
                    error("ERROR in fread put on client side");

                /*send as many as read */
                sendToBytes = sendto(sockfd, fileStorage, bytesRead, 0,
                                     (struct sockaddr*) &serveraddr, serverlen);
                if (sendToBytes < 0)
                    error("ERROR in sendto put on client side");
                /* read bytes till file size */
                bytesSent = bytesSent + sendToBytes;

            }
            printf("Client Sent %d bytes\n", bytesSent);

            /*ack file name as success */
            s = recvfrom(sockfd, fileName, BUFSIZE, 0, (struct sockaddr *) &serveraddr, &serverlen);
            if(s < 0)
                error("ERROR in ack file name");
            printf("File received %s successfully\n", fileName);
            bzero(fileStorage, BUFSIZE);
            bzero(command, BUFSIZE);
            bzero(fileName, BUFSIZE);
            fclose(filePointer);
        }
//        https://www.tutorialkart.com/c-programming/c-delete-file/
        else if (strcmp(command, "delete") == 0) {
            char path[] = "./client/";
            j = recvfrom(sockfd, command, BUFSIZE, 0, (struct sockaddr *) &serveraddr,& serverlen);
            if(j < 0)
                error("ERROR in ack command");
            s = recvfrom(sockfd, fileName, BUFSIZE, 0, (struct sockaddr *) &serveraddr, &serverlen);

            if(s < 0)
                error("ERROR in ack file name");

            printf("server received command: %s\n", command);
            printf("server received file name: %s\n", fileName);

            s = recvfrom(sockfd, fileName, BUFSIZE, 0, (struct sockaddr *) &serveraddr, &serverlen);

            if(s < 0)
                error("ERROR in ack file name");
            printf("File %s deleted \n", fileName);
            bzero(command, BUFSIZE);
            bzero(fileName, BUFSIZE);
        }
            //https://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program/17683417
        else if (strcmp(command, "ls") == 0) {
            j = recvfrom(sockfd, command, BUFSIZE, 0, (struct sockaddr *) &serveraddr,& serverlen);
            if(j < 0)
                error("ERROR in ack command");
            printf("server received command: %s\n", command);
            lsDirReceiver = recvfrom(sockfd, lsString, BUFSIZE, 0, &serveraddr, &serverlen);
            if(lsDirReceiver < 0){
                error("ERROR in delete message");
            }
            printf("Directory: %s\n", lsString);
            bzero(command, BUFSIZE);
            bzero(fileName, BUFSIZE);
            bzero(lsString, BUFSIZE);
        }
        else if (strcmp(command, "exit") == 0) {
            j = recvfrom(sockfd, command, BUFSIZE, 0, (struct sockaddr *) &serveraddr, &serverlen);
            if(j < 0)
                error("ERROR in ack command");
            printf("server received command: %s\n", command);
            printf("Client is exiting\n");
            bzero(command, BUFSIZE);
            bzero(fileName, BUFSIZE);
            exit(0);
        } else {
            /* recv ack of invalid command */
            j = recvfrom(sockfd, command, BUFSIZE, 0, (struct sockaddr *) &serveraddr, &serverlen);
            if(j < 0)
                error("ERROR in ack command");
            /* print the command back to user */
            printf("command \"%s\" is invalid\n", command);
            bzero(command, BUFSIZE);
            bzero(fileName, BUFSIZE);
        }
    }
}
