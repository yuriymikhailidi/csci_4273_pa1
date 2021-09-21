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
#define MAXBUFLEN 64000

bool getBoolean(char *command);

void clientRequestAckAndInitFileCommand(int sockfd, const char *command, char *file, const char *err, int commandReceiver, int fileReceiver, bool ack,
                                        int *clientlen, struct sockaddr_in *clientaddr, const char *filenameStorage, int fileNameStorageReciever, const char* path);

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

/*from FILE* to buffer */
void writeBufToFile(char* path, char buf[],int bufSize);

/*from butter to FILE* */
void readFileToBuf(char* path, char buf[], int bufSize);



int clientRequestType(char *command, int s){
    /* switch statment to ack the command, and preform action on file */
    int ackDigit = 0;
    if(strcmp(command, "get") == 0) {
        ackDigit = 1;
    }
    if(strcmp(command, "put") == 0) {
        ackDigit = 2;
    }
    if(strcmp(command, "delete") == 0) {
        ackDigit = 3;
    }
    if(strcmp(command, "ls") == 0) {
        ackDigit = 4;
    }
    if(strcmp(command, "exit") == 0) {
        ackDigit = 5;
    }
    return ackDigit;
}
int main(int argc, char **argv) {
    int i = 0; /* iterator */
    int sockfd; /* socket */
    int portno; /* port to listen on */
    int clientlen; /* byte size of client's address */
    struct sockaddr_in serveraddr; /* server's addr */
    struct sockaddr_in clientaddr; /* client addr */
    struct hostent *hostp; /* client host info */
    char command[BUFSIZE]; /* message command */
    char file[MAXBUFLEN];
    char path[BUFSIZE];
    char filenameStorage[BUFSIZE];
    char err[] = "Invalid command was used"; /* error message */
    FILE* fp = NULL; /*file buffer */
    char *hostaddrp; /* dotted decimal host addr string */
    int optval; /* flag value for setsockopt */
    int commandReceiver, fileReceiver = 0, fileNameStorageReciever = 0; /* message byte size */

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
        bzero(command, BUFSIZE);
        commandReceiver = recvfrom(sockfd, command, BUFSIZE, 0,
                                   (struct sockaddr *) &clientaddr, &clientlen);
        /*file name */
        fileNameStorageReciever = recvfrom(sockfd, filenameStorage, BUFSIZE, 0,
                                           (struct sockaddr *) &clientaddr, &clientlen);

        if(fileNameStorageReciever < 0)
            error("fileNameStorageReciever ERROR in recvfrom");

        if (commandReceiver < 0)
            error(" commandReceiver ERROR in recvfrom");
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
        bool ack = false;
        ack = getBoolean(command);
        clientRequestAckAndInitFileCommand(sockfd, command, file, err, commandReceiver, fileReceiver, ack, &clientlen, &clientaddr, filenameStorage, fileNameStorageReciever, path);

    }
}

/* Method responsible for creating the file command calls */
void clientRequestAckAndInitFileCommand(int sockfd, const char *command, char *file, const char *err, int commandReceiver, int fileReceiver, bool ack,
                                        int *clientlen, struct sockaddr_in *clientaddr,const char *filenameStorage, int fileNameStorageReciever, const char *path) {
        /*sending back command response */
        commandReceiver = sendto(sockfd, command, strlen(command), 0,
                                 (struct sockaddr *) clientaddr, (*clientlen));

        if (strcmp(command, "exit") == 0){
            printf("INFO: Server is exiting\n");
            exit(0);
        }

        if (commandReceiver < 0)
            error("commandReceiver ERROR in sendto");


        strcpy(path, creatFilePath(filenameStorage));

        fileNameStorageReciever = sendto(sockfd, filenameStorage, BUFSIZE, 0,
                                           (struct sockaddr *) clientaddr, (*clientlen));

        if(fileNameStorageReciever < 0)
            error("fileNameStorageReciever ERROR in sendto");


        /* file*/
        fileReceiver = recvfrom(sockfd, file, MAXBUFLEN, 0,
                                (struct sockaddr *) &clientaddr, &clientlen);
        if (fileReceiver < 0)
            error("fileReceiver ERROR in recvfrom");

        writeBufToFile(path, file, MAXBUFLEN);

    } else {
        commandReceiver = sendto(sockfd, err, strlen(err), 0,
                                 (struct sockaddr *) clientaddr, (*clientlen));
        if (commandReceiver < 0)
            error("ERROR in sendto");
}

/* returns ack check of the command */
bool getBoolean(char *command) {
    bool ack = false;
    switch (clientRequestType(command, strlen(command))) {
        case 0:
            printf("ERROR: Invalid command used\n");
            ack = false;
            break;
        case 1:
            //get
            ack = true;
            break;
        case 2:
            //put
            ack = true;
            break;
        case 3:
            //delete
            ack = true;
            break;
        case 4:
            //ls
            ack = true;
            break;
        case 5:
            //exit
            ack = true;
    }
    return ack;
}
/* This function builds the path to open file */
const char * creatFilePath(char *const *fileName) {
    char* TARGETDIR = "./server/";
    char tempPath[100];
    strcpy(tempPath, TARGETDIR);
    strcat(tempPath, fileName);
    return tempPath;
}

/*from FILE* to buffer */
void writeBufToFile(char* path, char* buf, int bufSize){
    /* Write */
    FILE* pFile = NULL;
    printf("%s\n", buf);
    pFile = fopen(path,"wb");
    if (pFile){
        fwrite(buf, 10, 45000, pFile);
    }
    else{
        printf("Server could not write buf to File\n");
    }

}

/*from butter to FILE* */
void readFileToBuf(char* path, char* buf, int bufSize){
    /* Read */
    FILE* pFile = NULL;
    pFile = fopen(path, "rb");
    size_t nread;
    /* write as many as you read */
    while(nread = fread(buf, 10, bufSize, pFile) > 0) {
        fwrite(buf, 10, nread, pFile);
        fflush(pFile);
    }

    fclose(pFile);

}

#pragma clang diagnostic pop