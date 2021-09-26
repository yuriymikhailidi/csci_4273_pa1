/*
 * udpserver.c - A simple UDP echo server
 * usage: udpserver <port>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>

#define BUFSIZE 1024

/*
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char **argv) {
    int sockfd; /* socket */
    int portno; /* port to listen on */
    int clientlen; /* byte size of client's address */
    struct sockaddr_in serveraddr; /* server's addr */
    struct sockaddr_in clientaddr; /* client addr */
    struct hostent *hostp; /* client host info */
    char buf[BUFSIZE], command[BUFSIZE],
            fileName[BUFSIZE], fileStorage[BUFSIZE], dirString[]= "./server";
    /* message buf */
    char *hostaddrp; /* dotted decimal host addr string */
    int optval; /* flag value for setsockopt */
    int bufferBytes, sendToBytes, bytesSent, bytesReceived, bytesRead,
            fileBufferBytes, j, k, fileSizeReceiver, lsStringSender; /* message byte size */
    long fileSize;
    DIR* directory = NULL; /*directory read */
    struct dirent *dir; /* directory struct */
    char deleteMessage[] = "File deleted\n"; /*delete conformation message */
    /*
     * check command line arguments
     */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        exit(1);
    }
    portno = atoi(argv[1]);

    /*
     * socket: create the parent socket
     */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    /* setsockopt: Handy debugging trick that lets
     * us rerun the server immediately after we kill it;
     * otherwise we have to wait about 20 secs.
     * Eliminates "ERROR on binding: Address already in use" error.
     */
    optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,
               (const void *) &optval, sizeof(int));

    /*
     * build the server's Internet address
     */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short) portno);
    /*
     * bind: associate the parent socket with a port
     */
    if (bind(sockfd, (struct sockaddr *) &serveraddr,
             sizeof(serveraddr)) < 0)
        error("ERROR on binding");
    /*
     * main loop: wait for a datagram, then echo it
     */
    clientlen = sizeof(clientaddr);
    while (1) {
        /*
         * recvfrom: receive a UDP datagram from a client
         */
        /* clear buffers to be used */
        bzero(buf, BUFSIZE);

        /* receive user input from the client */
        bufferBytes = recvfrom(sockfd, buf, BUFSIZE, 0,
                               (struct sockaddr *) &clientaddr, &clientlen);

        /*parse*/
        sscanf(buf, "%s %s", &command, &fileName);

        if (bufferBytes < 0)
            error("ERROR in recvfrom server side");
        /*
         * gethostbyaddr: determine who sent the datagram
         */
        hostp = gethostbyaddr((const char *) &clientaddr.sin_addr.s_addr,
                              sizeof(clientaddr.sin_addr.s_addr), AF_INET);
        if (hostp == NULL)
            error("ERROR on gethostbyaddr");
        hostaddrp = inet_ntoa(clientaddr.sin_addr
        );
        if (hostaddrp == NULL)
            error("ERROR on inet_ntoa\n");


        /* check command if else block */
        if (strcmp(command, "get") == 0) {
            char path[] = "./server/";
            printf("server received datagram from %s (%s)\n",
                   hostp->h_name, hostaddrp);

            /*ack the command and file name */
            j = sendto(sockfd, command, strlen(command), 0, &clientaddr, clientlen);
            if(j < 0)
                error("ERROR in send ack command");
            k = sendto(sockfd, fileName, strlen(fileName), 0, &clientaddr, clientlen);
            if(k < 0)
                error("ERROR in send ack file name");

            strcat(path, fileName);
            FILE*  filePointer = fopen(path, "rb");

            /*check if file is readbale if not send error size */
            if (!filePointer) {
                fileSize = -99999;
                sendto(sockfd, &fileSize, sizeof(fileSize), 0, &clientaddr, clientlen);
                error("ERROR in fopen get server side");
            }

            // https://stackoverflow.com/questions/238603/how-can-i-get-a-files-size-in-c
            fseek(filePointer, 0, SEEK_END);
            fileSize = ftell(filePointer);
            fseek(filePointer, 0, SEEK_SET);

            sendToBytes = sendto(sockfd, &fileSize, sizeof (fileSize), 0,
                                 &clientaddr, clientlen);

            if (sendToBytes < 0)
                error("ERROR in sendto get on server side");

            //send entire file over now
            bytesSent = 0;
            while (bytesSent < fileSize) {
                bzero(fileStorage, BUFSIZE);
                /* read bytes to buffer */
                bytesRead = fread(fileStorage, 1, BUFSIZE, filePointer);
                if(bytesRead < 0){
                    error("ERROR in fread get on server side");
                }

                /*send as many as read */
                sendToBytes = sendto(sockfd, fileStorage, bytesRead, 0,(struct sockaddr*) &clientaddr, clientlen);
                if (sendToBytes < 0)
                    error("ERROR in sendto get on server side");

                /* iterate till file size */
                bytesSent = bytesSent + sendToBytes;

            }
            printf("Server Sent %d bytes \n", bytesSent);
            /*send file ack as success */
            k = sendto(sockfd, fileName, strlen(fileName), 0, &clientaddr, clientlen);
            if(k < 0)
                error("ERROR in send ack file name");
            bzero(fileStorage, BUFSIZE);
            bzero(command, BUFSIZE);
            bzero(fileName, BUFSIZE);
            fclose(filePointer);
        }
        else if (strcmp(command, "put") == 0) {
            char path[] = "./server/";
            printf("server received datagram from %s (%s)\n",
                   hostp->h_name, hostaddrp);
            /* ack the command and file name */
            j = sendto(sockfd, command, strlen(command), 0, &clientaddr, clientlen);
            if (j < 0)
                error("ERROR in send ack command");
            k = sendto(sockfd, fileName, strlen(fileName), 0, &clientaddr, clientlen);
            if (k < 0)
                error("ERROR in send ack file name");

            /*receive file size from client */
            fileSizeReceiver = recvfrom(sockfd, &fileSize, sizeof(fileSize), 0,
                                        (struct sockaddr *) &clientaddr, &clientlen);
            if (fileSizeReceiver < 0)
                error("ERROR in recvfrom on server side for file size");

            strcat(path, fileName);
            FILE* filePointer = fopen(path, "wb");
            if (!filePointer)
                error("ERROR in fopen on server side");

            bytesReceived = 0;

            while (bytesReceived < fileSize) {
                bzero(fileStorage, BUFSIZE);
                /* recive as many bytes as you can */
                fileBufferBytes = recvfrom(sockfd, fileStorage, BUFSIZE, 0, (struct sockaddr *) &clientaddr,
                                           &clientlen);
                if (fileBufferBytes < 0)
                    error("ERROR in recvfrom put on server side");

                /* write the bytes to file till file size */
                bytesReceived += fwrite(fileStorage, 1, fileBufferBytes, filePointer);
            }

            printf("Server Received %d bytes\n", bytesReceived);

            /*send ack of file name as success */
            fclose(filePointer);
            k = sendto(sockfd, fileName, strlen(fileName), 0, &clientaddr, clientlen);
            if (k < 0)
                error("ERROR in send ack file name");
            bzero(command, BUFSIZE);
            bzero(fileName, BUFSIZE);
        }
            //        https://www.tutorialkart.com/c-programming/c-delete-file/
        else if (strcmp(command, "delete") == 0) {
            char path[] = "./server/";
            printf("server received datagram from %s (%s)\n",
                   hostp->h_name, hostaddrp);

            /*ack the command and file name */
            j = sendto(sockfd, command, strlen(command), 0,
                       &clientaddr, clientlen);
            if(j < 0)
                error("ERROR in send ack command");
            k = sendto(sockfd, fileName, strlen(fileName), 0,
                       &clientaddr, clientlen);
            if(k < 0)
                error("ERROR in send ack file name");
            strcat(path, fileName);

            FILE* filePointer = fopen(path, "wb");

            if(filePointer == NULL)
                error("ERROR fopen delete on server side");

            fclose(filePointer);
            /*invoke the remove command, if success, send delete ack message */
            if(remove(path) == 0){
                sendto(sockfd, fileName, sizeof (fileName), 0,
                       &clientaddr, clientlen);
            } else {
                printf("File failed to delete %s\n", path);
            }
            bzero(command, BUFSIZE);
            bzero(fileName, BUFSIZE);
        }
        else if (strcmp(command, "ls") == 0) {
            printf("server received datagram from %s (%s)\n",
                   hostp->h_name, hostaddrp);
            /*ack the command and file name */
            j = sendto(sockfd, command, strlen(command), 0,
                       &clientaddr, clientlen);
            if(j < 0)
                error("ERROR in send ack command");

            /*sets the server directory */
            directory = opendir("./server");

            /*loop prints the directory */
            if(directory){
                /*reads dir and adds to struct till null*/
                while ((dir = readdir(directory)) != NULL){
                    /*print*/
                    printf("%s\n", dir->d_name);
                }
                /*close */
                closedir(directory);
            }
            /* send ack after print */
            sendto(sockfd, dirString, sizeof (dirString), 0,
                   &clientaddr, clientlen);
            bzero(command, BUFSIZE);
            bzero(fileName, BUFSIZE);
        }
        else if (strcmp(command, "exit") == 0) {
            printf("server received datagram from %s (%s)\n",
                   hostp->h_name, hostaddrp);

            /*ack the command and file name */
            j = sendto(sockfd, command, strlen(command), 0, &clientaddr, clientlen);
            if (j < 0)
                error("ERROR in send ack command");

            bzero(command, BUFSIZE);
            bzero(fileName, BUFSIZE);
        } else {
            printf("server received datagram from %s (%s)\n",
                   hostp->h_name, hostaddrp);

            /*ack the command and file name */
            j = sendto(sockfd, command, strlen(command), 0, &clientaddr, clientlen);
            if (j < 0)
                error("ERROR in send ack command");
            bzero(command, BUFSIZE);
            bzero(fileName, BUFSIZE);
        }
    }
}