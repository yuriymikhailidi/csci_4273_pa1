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
#define MAXBUFLEN 64000
#define nofile "File Not Found!"


const char * creatFilePath(char *const *fileName);

/*
 * error - wrapper for perror
 */
void error(char *msg) {
    perror(msg);
    exit(0);
}

void writeBufToFile(char* path, char* buf, int bufSize){
    /* Write */
    FILE* pFile = NULL;
    pFile = fopen(path,"wb");

    if (pFile){
        fwrite(buf,1, bufSize, pFile);
        printf("Client wrote to buf\n");
    }
    else{
        printf("Client could not write buf to File\n");
    }
    fclose(pFile);

}

/*from butter to FILE* */
/*https://stackoverflow.com/questions/27213419/fread-in-c-not-reading-the-complete-file?rq=1*/
FILE* readFileToBuf(char* path, char* buf, int bufSize){
    /* Read */
    FILE* pFile = NULL;
    pFile = fopen(path, "rb");
    int nread;
    /* write as many as you read */
    while(nread = fread(buf, 100, bufSize, pFile) > 0) {
        fwrite(buf, 10, nread, pFile);
        fflush(pFile);
    }

    return pFile;

}

int main(int argc, char **argv) {
    int sockfd, portno, commandTransfer, i = 0, fileTransfer, fileNameTransfer;
    int serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE];
    char* command[BUFSIZE];/* command string */
    char* fileName[BUFSIZE];/*filename string*/
    FILE* fp = NULL;
    char* fileSourceBuffer[MAXBUFLEN];
    char path[100];

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
            buf[strcspn(buf, "\r\n")] = 0;
            buf[strcspn(buf, "\0")] = 0;

            char* ptr = strtok(buf, " ");
            char** parsedInput = NULL;
            int n_spaces = 0, i = 0;

            /*Reference code: https://stackoverflow.com/questions/11198604/c-split-string-into-an-array-of-strings */
            /* This snipet of code allows to  /

            /* split string and append tokens to 'res' */
            while(ptr){
                parsedInput = realloc(parsedInput, sizeof (char* ) * n_spaces++);

                if(parsedInput == NULL)
                    exit(-1);

                parsedInput[n_spaces - 1] = ptr;

                ptr = strtok(NULL, " ");
            }
            /* realloc one extra element for the last NULL */
            parsedInput = realloc(parsedInput, sizeof (char *) * n_spaces + 1);
            parsedInput[n_spaces] = 0;

            /*Copy command from input*/
            strcpy(command, parsedInput[0]);



            /* Copy filename if not null from input*/
            if(parsedInput[1] != NULL)
                strcpy(fileName, parsedInput[1]);

//            if((strcmp(command, "exit") == 0) || (strcmp(command, "ls") == 0))
//                strcpy(fileName, "cmd exit or ls");

        /* send the message to the server */
            serverlen = sizeof(serveraddr);
            commandTransfer = sendto(sockfd, command, strlen(command), 0, &serveraddr, serverlen);
            if (commandTransfer < 0)
                error("CommandTransfer ERROR in sendto");

            /*file name */
            fileNameTransfer = sendto(sockfd, fileName, strlen(fileName)+1, 0, &serveraddr, serverlen);
            if(fileNameTransfer < 0)
            error("FileNameTransfer ERROR in sendto");

            /* print the server's reply */
            commandTransfer = recvfrom(sockfd, command, strlen(command), 0, &serveraddr, &serverlen);
            if (commandTransfer < 0)
                error("CommandTransfer ERROR in recvfrom");

            printf("Server received the command: %s\n", command);

            if(strcmp(command, "exit") == 0) {
                free(parsedInput);
            }

            fileNameTransfer = recvfrom(sockfd, fileName, strlen(fileName) + 1, 0, &serveraddr, &serverlen);
            if(fileNameTransfer < 0)
                error("FileNameTransfer ERROR in recvfro");
//            if(strcmp((fileName, "cmd exit or ls") != 0 ) ||
//                    (strcmp(fileName, "cmd exit or ls") != 0))
            printf("Server received file name: %s\n", fileName);

            /* file */
            strcpy(path, creatFilePath(fileName));
//            sendFile(fp, (char *) fileSourceBuffer, MAXBUFLEN);
            FILE* fp = readFileToBuf(path, fileSourceBuffer, MAXBUFLEN);
            int send_left = sizeof(fileSourceBuffer);
            int send_rc;
            while (send_rc < send_left )
            {
                send_rc += sendto(sockfd, fileSourceBuffer, send_left, 0,&serveraddr, serverlen);
                if (send_rc < 0)
                    error("Error in sendto");
                send_left -= send_rc;
            }
            if (fileTransfer < 0)
                error("FileTransfer ERROR in sendto");
            fclose(fp);



    }
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