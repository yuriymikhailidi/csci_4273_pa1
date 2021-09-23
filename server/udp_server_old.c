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
#include <stdbool.h>

#pragma clang diagnostic push
#pragma ide diagnostic ignored "EndlessLoop"
#define BUFSIZE 1024
#define MAXBUFLEN 50000

bool getBoolean(char *command);

void clientRequestAckAndInitFileCommand(int sockfd, const char *command, char *file, const char *err, int commandReceiver, int fileReceiver,
                                        int *clientlen, struct sockaddr_in *clientaddr, const char *filenameStorage, int fileNameStorageReciever, const char* path, int fileSizeReceived, int bReceived);

const char * creatFilePath(char *const *fileName);

/*
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(1);
}
/*basic File commands */
int lsFile(FILE* fileArr);
int deleteFile(FILE* fileArr);
FILE* getFile(FILE* fileArr);
int putFile(FILE* fileArr[]);

void recieveBufFile(int sockfd, char *buf, const char *path, int *clientLen, struct sockaddr_in *clientAddr);

void sendFileBuffer(int sockfd, const char *buf, const char *path, int *serverlen,
                    struct sockaddr_in *serveraddr);

int main(int argc, char **argv) {
    int i = 0; /* iterator */
    int sockfd; /* socket */
    int portno; /* port to listen on */
    int clientlen; /* byte size of client's address */
    struct sockaddr_in serveraddr; /* server's addr */
    struct sockaddr_in clientaddr; /* client addr */
    struct hostent *hostp; /* client host info */
    char bufRecieved[BUFSIZE]; /* message bufRecieved */
    char* file[MAXBUFLEN];
    char path[BUFSIZE];
    char filenameStorage[BUFSIZE];
    char commandStorage[BUFSIZE];
    char err[] = "Invalid bufRecieved was used"; /* error message */
    FILE* fp = NULL; /*file buffer */
    char *hostaddrp; /* dotted decimal host addr string */
    int optval; /* flag value for setsockopt */
    int nBufRecieved, fileReceiver = 0, fileNameStorageReciever = 0, fileSizeReceived = 0, bReceived; /* message byte size */

    /*
     * check bufRecieved line arguments
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
        bzero(bufRecieved, sizeof (bufRecieved));
        bzero(filenameStorage, sizeof(filenameStorage));
        bzero(commandStorage, sizeof(commandStorage));

        nBufRecieved = recvfrom(sockfd, bufRecieved, BUFSIZE, 0,
                                (struct sockaddr *) &clientaddr, &clientlen);
        sscanf(bufRecieved, "%s %s",commandStorage, filenameStorage);


        if (nBufRecieved < 0)
            error(" nBufRecieved ERROR in recvfrom");
        /*
         * gethostbyaddr: determine who sent the datagram
         */
        hostp = gethostbyaddr((const char *) &clientaddr.sin_addr.s_addr,
                              sizeof(clientaddr.sin_addr.s_addr), AF_INET);
        if (hostp == NULL)
            error("ERROR on gethostbyaddr");
        hostaddrp = inet_ntoa(clientaddr.sin_addr);

        if (hostaddrp == NULL)
            error("ERROR on inet_ntoa\n");

        printf("server received datagram from %s (%s)\n",
               hostp->h_name, hostaddrp);

        /*
         * sendto: echo the input back to the client
         */
        clientRequestAckAndInitFileCommand(sockfd, commandStorage, file, err, nBufRecieved, fileReceiver, &clientlen, &clientaddr, filenameStorage, fileNameStorageReciever, path, fileSizeReceived, bReceived);

    }
}

/* Method responsible for creating the file command calls */
void clientRequestAckAndInitFileCommand(int sockfd, const char *command, char *file, const char *err, int commandReceiver, int fileReceiver,
                                        int *clientlen, struct sockaddr_in *clientaddr,const char *filenameStorage,
                                                int fileNameStorageReciever, const char *path, int fileSizeReceived, int bReceived) {
        int k;
        if(strcmp(command, "get") == 0) {
            /*sending back command response */
            commandReceiver = sendto(sockfd, command, strlen(command), 0,
                                     (struct sockaddr *) clientaddr, (*clientlen));

            if (commandReceiver < 0)
                error("commandReceiver ERROR in sendto");


            strcpy(path, creatFilePath(filenameStorage));

            fileNameStorageReciever = sendto(sockfd, filenameStorage, BUFSIZE, 0,
                                             (struct sockaddr *) clientaddr, (*clientlen));

            if(fileNameStorageReciever < 0)
                error("fileNameStorageReciever ERROR in sendto");

        }
        if(strcmp(command, "put") == 0) {
            /*sending back command response */
            commandReceiver = sendto(sockfd, command, strlen(command), 0,
                                     (struct sockaddr *) clientaddr, (*clientlen));

            if (commandReceiver < 0)
                error("commandReceiver ERROR in sendto");

            fileNameStorageReciever = sendto(sockfd, filenameStorage, BUFSIZE, 0,
                                             (struct sockaddr *) clientaddr, (*clientlen));

            if(fileNameStorageReciever < 0)
                error("fileNameStorageReciever ERROR in sendto");

           // recieveBufFile(0, file, path, clientlen, clientaddr);
            //recieve incoming response from the server (size)
            int n = recvfrom(sockfd, &fileSizeReceived, sizeof(fileSizeReceived), 0, (struct sockaddr*)&clientaddr, &clientlen);
            if (n < 0)
                error("ERROR in recvfrom server file size");

            //if we recieved everything, we keep going

            //gotta make sure the file can be opened

            strcpy(path, creatFilePath(filenameStorage));
            FILE* filePointer = fopen(path, "wb");

            if(!filePointer)
                error("ERROR in fopen server)");

            //file opened, so receive the file
            bReceived = 0;

            while(bReceived < fileSizeReceived){
                bzero(file, BUFSIZE);
                k = recvfrom(sockfd, file, BUFSIZE, 0, (struct sockaddr*)&clientaddr, &clientlen);
                if (k < 0)
                    error("ERROR in recvfrom server");
                bReceived += fwrite(file, 1, k, filePointer);
                printf("SERVER: %d %d", fileSizeReceived, bReceived);
            }

            //now we close it, alert user
            printf("%s file recieved\n", filenameStorage);


            //now we close it, alert user
            fclose(filePointer);
            printf("File successfully received\n");



            bzero(command, sizeof (command));
            bzero(filenameStorage, sizeof (filenameStorage));


        }
        if(strcmp(command, "delete") == 0) {
            /*sending back command response */
            commandReceiver = sendto(sockfd, command, strlen(command), 0,
                                     (struct sockaddr *) clientaddr, (*clientlen));

            if (commandReceiver < 0)
                error("commandReceiver ERROR in sendto");


            strcpy(path, creatFilePath(filenameStorage));

            fileNameStorageReciever = sendto(sockfd, filenameStorage, BUFSIZE, 0,
                                             (struct sockaddr *) clientaddr, (*clientlen));

            if(fileNameStorageReciever < 0)
                error("fileNameStorageReciever ERROR in sendto");



            bzero(command, sizeof (command));
            bzero(filenameStorage, sizeof (filenameStorage));

        }
        if(strcmp(command, "ls") == 0) {
            /*sending back command response */
            commandReceiver = sendto(sockfd, command, strlen(command), 0,
                                     (struct sockaddr *) clientaddr, (*clientlen));

            if (commandReceiver < 0)
                error("commandReceiver ERROR in sendto");


            strcpy(path, creatFilePath(filenameStorage));

            fileNameStorageReciever = sendto(sockfd, filenameStorage, BUFSIZE, 0,
                                             (struct sockaddr *) clientaddr, (*clientlen));

            if(fileNameStorageReciever < 0)
                error("fileNameStorageReciever ERROR in sendto");

        }
        if(strcmp(command, "exit") == 0) {
            printf("INFO: Server is exiting\n");
            commandReceiver = sendto(sockfd, command, strlen(command), 0,
                                     (struct sockaddr *) clientaddr, (*clientlen));

            if (commandReceiver < 0)
                error("commandReceiver ERROR in sendto");
            exit(0);
        }
}
/* This function builds the path to open file */
const char * creatFilePath(char *const *fileName) {
    char* TARGETDIR = "./server/";
    char tempPath[100];
    strcpy(tempPath, TARGETDIR);
    strcat(tempPath, fileName);
    return tempPath;
}

void recieveBufFile(int sockfd, char *buf, const char *path, int *clientLen, struct sockaddr_in *clientAddr) {
    FILE* filePointer = fopen(path, "rb");
    fseek(filePointer, 0, SEEK_END);
    size_t fileSize = ftell(filePointer);
    rewind(filePointer);

    int bytes_recieved = 0, n = 0;
    while(bytes_recieved < fileSize){
        bzero(buf, BUFSIZE);
        n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr*) clientAddr, clientLen);
        if (n < 0)
            error("ERROR in recvfrom recieveFile");
        bytes_recieved += fwrite(buf, 1, n, filePointer);
    }

    //now we close it, alert user
    fclose(filePointer);
}

void sendFileBuffer(int sockfd, const char *buf, const char *path, int *serverlen,
                    struct sockaddr_in *serveraddr) {
    FILE* filePointer = fopen(path, "rb");
    fseek(filePointer, 0, SEEK_END);
    size_t fileSize = ftell(filePointer);
    rewind(filePointer);

    //send entire file over now
    int bytes_sent = 0, n = 0;
    while (bytes_sent < fileSize){
        bzero(buf, MAXBUFLEN);
        fread(buf, 1, BUFSIZE, filePointer);
        n = sendto(sockfd, buf, sizeof(buf), 0, (struct sockaddr *) serveraddr, serverlen);
        if (n < 0)
            error("ERROR in sendto client");

        bytes_sent = bytes_sent + sizeof(buf);

    }
    //close file and zero out the buffer
    fclose(filePointer);
}


#pragma clang diagnostic pop