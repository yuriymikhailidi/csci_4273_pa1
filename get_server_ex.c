//The server transmits the requested file to the client

file = fopen(file_name, "rb");
if(!file){
/* send a size of -1 to the client to inform them the file could not be opened */
size = -1;
sendto(sockfd, &size, sizeof(size), 0, (struct sockaddr*)&clientaddr, clientlen);
    error("ERROR in fopen (GET)");
}

//get file size == source: https://stackoverflow.com/questions/238603/how-can-i-get-a-files-size-in-c
fseek(file, 0L, SEEK_END);
size = ftell(file);
rewind(file);
//send size of file to client
n = sendto(sockfd, &size, sizeof(size), 0, (struct sockaddr *) &clientaddr, clientlen);
if (n < 0)
    error("ERROR in sendto (GET)");

//send entire file over now
bytes_sent = 0;
while (bytes_sent < size){
    bzero(buf,BUFSIZE);
    fread(buf,1,BUFSIZE,file);
    n = sendto(sockfd, buf, sizeof(buf), 0, (struct sockaddr *) &clientaddr, clientlen);
    if (n < 0)
    error("ERROR in sendto (GET)");
    bytes_sent = bytes_sent + sizeof(buf);

}
//close file and zero out the buffer
fclose(file);
bzero(buf,BUFSIZE);
