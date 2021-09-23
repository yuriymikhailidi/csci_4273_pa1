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
#include <readline/readline.h>
#include <dirent.h>
#include <stdbool.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
#define BUFSIZE 1024
#define MAXBUFLEN 6000
#define nofile "File Not Found!"

/*
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

const char * creatFilePath(char *const *fileName);

void sendFileBuffer(int sockfd, const char *buf, char **fileSourceBuffer, const char *path, int *serverlen,
                    struct sockaddr_in *serveraddr);

void recieveBufFile(int sockfd, char *buf, const char *path, int *clientLen, struct sockaddr_in *clientAddr);

int main(int argc, char **argv) {
    int sockfd, portno, bufTransfer, i = 0, fileTransfer, fileNameTransfer;
    int serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE];
    char* command[BUFSIZE];/* command string */
    char* fileName[BUFSIZE];/*filename string*/
    char* err[BUFSIZE];/*err string */
    FILE* filePointer = NULL;
    char* fileSourceBuffer[BUFSIZE];
    char path[100];
    int fileSize, sizeLenSent, bytes_sent = 0, bytes_read;


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

    while(1) {

            /* get a message from the user */
            bzero(buf, BUFSIZE);
            printf("Enter get <filename>, put <filename>, delete <filename>, ls or exit: ");
            fgets(buf, BUFSIZE, stdin);
            sscanf(buf, "%s %s",command, fileName);

        /* send the message to the server */
            serverlen = sizeof(serveraddr);

            bufTransfer = sendto(sockfd,buf, strlen(buf), 0, &serveraddr, serverlen);

            if (bufTransfer < 0)
                error("CommandTransfer ERROR in sendto");

            /* file path creation*/
            strcpy(path, creatFilePath(fileName));

            if(strcmp(command, "get") == 0) {
                /* print the server's reply */

                bufTransfer = recvfrom(sockfd, command, strlen(command), 0, &serveraddr, &serverlen);
                if (bufTransfer < 0)
                    error("CommandTransfer ERROR in recvfrom");

                printf("Server received the command: %s\n", command);

                fileNameTransfer = recvfrom(sockfd, fileName, strlen(fileName) + 1, 0, &serveraddr, &serverlen);
                if(fileNameTransfer < 0)
                    error("FileNameTransfer ERROR in recvfro");

                printf("Server received file name: %s\n", fileName);


                //recieveBufFile(sockfd, buf, fp, path, &serverlen, &serveraddr);

                bzero(command, sizeof (command));
                bzero(fileName, sizeof (fileName));
            }
            if(strcmp(command, "put") == 0) {
                /* print the server's reply */
                bufTransfer = recvfrom(sockfd, command, strlen(command), 0, &serveraddr, &serverlen);
                if (bufTransfer < 0)
                    error("CommandTransfer ERROR in recvfrom");

                printf("Server received the command: %s\n", command);

                fileNameTransfer = recvfrom(sockfd, fileName, strlen(fileName) + 1, 0, &serveraddr, &serverlen);

                if(fileNameTransfer < 0)
                    error("FileNameTransfer ERROR in recvfro");
                printf("Server received filePointer name: %s\n", fileName);

                //sendFileBuffer(sockfd, buf, fileSourceBuffer, path, &serverlen, &serveraddr);
                strcpy(path , creatFilePath(fileName));
                filePointer= fopen(path, "rb+");
                if(!filePointer){
                    /* send a size of -1 to the client to inform them the filePointer could not be opened */
                    fileSize = -1;
                    sendto(sockfd, &fileSize, sizeof(fileSize), 0, (struct sockaddr*)&serveraddr, serverlen);
                    error("ERROR in fopen (GET)");
                }

                //get filePointer size == source: https://stackoverflow.com/questions/238603/how-can-i-get-a-files-size-in-c
                fseek(filePointer, 0L, SEEK_END);
                fileSize = ftell(filePointer);
                rewind(filePointer);
                //send size of filePointer to client
                if(sendto(sockfd, &fileSize, sizeof(fileSize), 0, (struct sockaddr *) &serveraddr, serverlen) <= 0){
                    error("ERROR in sendto size of file");
                }

                bzero(fileSourceBuffer, sizeof (fileSourceBuffer));
                //send entire filePointer over now
                while (bytes_sent < fileSize){
                    bzero(fileSourceBuffer,BUFSIZE);
                    fread(fileSourceBuffer,1, BUFSIZE, filePointer);
                    bytes_sent = sendto(sockfd, fileSourceBuffer, sizeof (fileSourceBuffer), 0, (struct sockaddr *) &serveraddr, serverlen);
                    if (bytes_sent < 0)
                        error("ERROR in sendto server");
                    bytes_sent = bytes_sent + sizeof (fileSourceBuffer);
                   // printf("CLIENT: %d %d\n", fileSize, bytes_sent);
                }

                //close filePointer and zero out the buffer
                fclose(filePointer);

                bzero(command, sizeof (command));
                bzero(fileName, sizeof (fileName));
            }
            if(strcmp(command, "delete") == 0) {
                /* print the server's reply */
                bufTransfer = recvfrom(sockfd, command, strlen(command), 0, &serveraddr, &serverlen);
                if (bufTransfer < 0)
                    error("CommandTransfer ERROR in recvfrom");

                printf("Server received the command: %s\n", command);

                fileNameTransfer = recvfrom(sockfd, fileName, strlen(fileName) + 1, 0, &serveraddr, &serverlen);
                if(fileNameTransfer < 0)
                    error("FileNameTransfer ERROR in recvfro");
                printf("Server received file name: %s\n", fileName);

                bzero(command, sizeof (command));
                bzero(fileName, sizeof (fileName));
            }
            if(strcmp(command, "ls") == 0) {
                /* print the server's reply */
                bufTransfer = recvfrom(sockfd, command, strlen(command), 0, &serveraddr, &serverlen);
                if (bufTransfer < 0)
                    error("CommandTransfer ERROR in recvfrom");

                printf("Server received the command: %s\n", command);

                fileNameTransfer = recvfrom(sockfd, fileName, strlen(fileName) + 1, 0, &serveraddr, &serverlen);
                if(fileNameTransfer < 0)
                    error("FileNameTransfer ERROR in recvfro");
                printf("Server received file name: %s\n", fileName);

                bzero(command, sizeof (command));
                bzero(fileName, sizeof (fileName));
            }
            if(strcmp(command, "exit") == 0){
                /* print the server's reply */
                bufTransfer = recvfrom(sockfd, command, strlen(command), 0, &serveraddr, &serverlen);

                if (bufTransfer < 0)
                    error("CommandTransfer ERROR in recvfrom");

                bzero(command, sizeof (command));
                bzero(fileName, sizeof (fileName));
                break;

            }

    }
}

void recieveBufFile(int sockfd, char *buf, const char *path, int *serverlen, struct sockaddr_in *serveraddr) {
    FILE* filePointer = fopen(path, "rb");
    fseek(filePointer, 0, SEEK_END);
    size_t fileSize = ftell(filePointer);
    rewind(filePointer);

    int bytes_recieved = 0, n = 0;
    while(bytes_recieved < fileSize){
        bzero(buf, BUFSIZE);
        n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr*) serveraddr, serverlen);
        if (n < 0)
            error("ERROR in recvfrom (GET)");
        bytes_recieved += fwrite(buf, 1, n, filePointer);
    }

    //now we close it, alert user
    fclose(filePointer);
}

void sendFileBuffer(int sockfd, const char *buf, char **fileSourceBuffer, const char *path, int *serverlen,
                    struct sockaddr_in *serveraddr) {
    FILE* filePointer = fopen(path, "rb");
    fseek(filePointer, 0, SEEK_END);
    size_t fileSize = ftell(filePointer);
    rewind(filePointer);

    //send entire file over now
    int bytes_sent = 0, n = 0;
    while (bytes_sent < fileSize){
        bzero(fileSourceBuffer, MAXBUFLEN);
        fread(fileSourceBuffer, 1, BUFSIZE, filePointer);
        n = sendto(sockfd, fileSourceBuffer, sizeof(fileSourceBuffer), 0, (struct sockaddr *) serveraddr, serverlen);
        if (n < 0)
            error("ERROR in sendto (GET)");

        bytes_sent = bytes_sent + sizeof(fileSourceBuffer);

    }
    //close file and zero out the buffer
    fclose(filePointer);
}

/* This function builds the path to open file */
const char * creatFilePath(char *const *fileName) {
    char* TARGETDIR = "./client/";
    char tempPath[100];
    strcpy(tempPath, TARGETDIR);
    strcat(tempPath, fileName);
    return tempPath;
}


#pragma clang diagnostic pop