/* 
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 */
#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
#define BUFSIZE 1024

/* 
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

/* This function builds the path to open file */
const char *creatFilePath(char *fileName) {
    char *TARGETDIR = "./client/";
    char tempPath[100];
    strcpy(tempPath, TARGETDIR);
    strcat(tempPath, fileName);
    return tempPath;
}

int main(int argc, char **argv) {
    int sockfd, portno, sendToBytes, bytesSent, bytesRead,
            j, s, bytesReceived, fileBufferBytes, fileSizeReceiver, lsDirReceiver, deleteIndicator;
    int serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE], command[BUFSIZE], fileName[BUFSIZE],
                path[BUFSIZE], fileStorage[BUFSIZE], lsString[BUFSIZE], deleteMessage[BUFSIZE], exitMessage[BUFSIZE];
    FILE *filePointer = NULL;
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
        buf[strcspn(buf, "\r\n")] = 0;
        sscanf(buf, "%s %s", command, fileName);

        /* send the message to the server */
        serverlen = sizeof(serveraddr);

        sendToBytes = sendto(sockfd, buf, strlen(buf), 0,
                             &serveraddr, serverlen);
        /* create file path */
        strcpy(path, creatFilePath(fileName));

        if (sendToBytes < 0)
            error("ERROR in sendto client side");

        if (strcmp(command, "get") == 0) {
            j = recvfrom(sockfd, command, BUFSIZE, 0, (struct sockaddr *) &serveraddr,& serverlen);
            if(j < 0)
                error("ERROR in ack command");
            s = recvfrom(sockfd, fileName, BUFSIZE, 0, (struct sockaddr *) &serveraddr, &serverlen);

            if(s < 0)
                error("ERROR in ack file name");

            printf("server received command: %s\n", command);
            printf("server received file name: %s\n", fileName);
/*get file size from client */
            fileSizeReceiver = recvfrom(sockfd, &fileSize, sizeof(fileSize), 0, (struct sockaddr *) &serveraddr, &serverlen);
            if (fileSizeReceiver < 0)
                error("ERROR in recvfrom on server side for file size");


            strcpy(path, creatFilePath(fileName));
            filePointer = fopen(path, "wb");
            if (!filePointer)
                error("ERROR in fopen on server side");

            //file opened, so receive the file
            bytesReceived = 0;
            while (bytesReceived < fileSize) {
                bzero(fileStorage, BUFSIZE);
                fileStorage[strcspn(fileStorage, "\r\n")] = 0;
                fileBufferBytes = recvfrom(sockfd, fileStorage, BUFSIZE, 0, (struct sockaddr *) &serveraddr, &serverlen);
                if (fileBufferBytes < 0)
                    error("ERROR in recvfrom get on client side");
                bytesReceived += fwrite(fileStorage, 1, fileBufferBytes, filePointer);
            }
            printf("Client Received %d bytes \n", bytesReceived);
            fclose(filePointer);
            s = recvfrom(sockfd, fileName, BUFSIZE, 0, (struct sockaddr *) &serveraddr, &serverlen);
            if(s < 0)
                error("ERROR in ack file name");
            printf("File received %s successfully\n", fileName);
            bzero(command, BUFSIZE);
            bzero(fileName, BUFSIZE);
        }
        if (strcmp(command, "put") == 0) {
            /* ack the command and file name */
            j = recvfrom(sockfd, command, BUFSIZE, 0, (struct sockaddr *) &serveraddr,& serverlen);
            if(j < 0)
                error("ERROR in ack command");
            s = recvfrom(sockfd, fileName, BUFSIZE, 0, (struct sockaddr *) &serveraddr, &serverlen);

            if(s < 0)
                error("ERROR in ack file name");

            printf("server received command: %s\n", command);
            printf("server received file name: %s\n", fileName);

            filePointer = fopen(path, "rb");

            /*send file size to server */
            if (!filePointer) {
                fileSize = -1;
                sendto(sockfd, &fileSize, sizeof(fileSize), 0, &serveraddr, serverlen);
                error("ERROR in fopen put client side)");
            }

            // https://stackoverflow.com/questions/238603/how-can-i-get-a-files-size-in-c
            fseek(filePointer, 0, SEEK_END);
            fileSize = ftell(filePointer);
            rewind(filePointer);

            sendToBytes = sendto(sockfd, &fileSize, sizeof (fileSize), 0,
                                 &serveraddr, serverlen);

            if (sendToBytes < 0)
                error("ERROR in sendto client");

            bytesSent = 0;
            while (bytesSent < fileSize) {
                bzero(fileStorage, BUFSIZE);
                bytesRead = fread(fileStorage, 1, BUFSIZE, filePointer);
                if(bytesRead < 0)
                    error("ERROR in fread put on client side");
                sendToBytes = sendto(sockfd, fileStorage, bytesRead, 0,
                                        (struct sockaddr*) &serveraddr, serverlen);
                if (sendToBytes < 0)
                    error("ERROR in sendto put on client side");
                bytesSent = bytesSent + sendToBytes;

            }
            printf("Client Sent %d bytes\n", bytesSent);

            fclose(filePointer);
            s = recvfrom(sockfd, fileName, BUFSIZE, 0, (struct sockaddr *) &serveraddr, &serverlen);
            if(s < 0)
                error("ERROR in ack file name");
            printf("File received %s successfully\n", fileName);
            bzero(fileStorage, BUFSIZE);
            bzero(command, BUFSIZE);
            bzero(fileName, BUFSIZE);

        }
//        https://www.tutorialkart.com/c-programming/c-delete-file/
        if (strcmp(command, "delete") == 0) {
            j = recvfrom(sockfd, command, BUFSIZE, 0, (struct sockaddr *) &serveraddr,& serverlen);
            if(j < 0)
                error("ERROR in ack command");
            s = recvfrom(sockfd, fileName, BUFSIZE, 0, (struct sockaddr *) &serveraddr, &serverlen);

            if(s < 0)
                error("ERROR in ack file name");

            printf("server received command: %s\n", command);
            printf("server received file name: %s\n", fileName);

            deleteIndicator = recvfrom(sockfd, deleteMessage, BUFSIZE, 0, &serveraddr, &serverlen);
            if(deleteIndicator < 0){
                error("ERROR in delete message");
            }
            printf("%s", deleteMessage);

            bzero(command, BUFSIZE);
            bzero(fileName, BUFSIZE);
        }
        //https://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program/17683417
        if (strcmp(command, "ls") == 0) {
            j = recvfrom(sockfd, command, BUFSIZE, 0, (struct sockaddr *) &serveraddr,& serverlen);
            if(j < 0)
                error("ERROR in ack command");
            printf("server received command: %s\n", command);

            lsDirReceiver = recvfrom(sockfd, lsString, BUFSIZE, 0, (struct sockaddr *) &serveraddr, & serverlen);
            if(lsDirReceiver < 0)
                error("ERROR in ack command");

            printf("server printed directory: %s\n", lsString);

            bzero(command, BUFSIZE);
            bzero(fileName, BUFSIZE);
            bzero(lsString, BUFSIZE);

        }
        if (strcmp(command, "exit") == 0) {
            j = recvfrom(sockfd, command, BUFSIZE, 0, (struct sockaddr *) &serveraddr, &serverlen);
            if(j < 0)
                error("ERROR in ack command");
            printf("server received command: %s\n", command);

            bzero(command, BUFSIZE);
            bzero(fileName, BUFSIZE);
        }
    }
}

#pragma clang diagnostic pop
#pragma clang diagnostic pop