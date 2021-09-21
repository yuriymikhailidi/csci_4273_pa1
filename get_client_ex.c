//The server transmits the requested file to the client
//gotta send the 'get [file_name]' cmd to server
n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);

if (n < 0)
    error("ERROR in sendto (GET)");

//recieve incoming response from the server (size)
n = recvfrom(sockfd, &size, sizeof(size), 0, (struct sockaddr*)&serveraddr, &serverlen);
if (size < 0)
    error("ERROR in recvfrom (GET)");

//if we recieved everything, we keep going

//gotta make sure the file can be opened
file = fopen(file_name, "wb");
if(!file)
error("ERROR in fopen (GET)");

//file opened, so receive the file
bytes_recieved = 0;
while(bytes_recieved < size){
    bzero(buf, BUFSIZE);
    n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr*)&serveraddr, &serverlen);
    if (n < 0)
        error("ERROR in recvfrom (GET)");
    bytes_recieved += fwrite(buf, 1, n, file);
}

//now we close it, alert user
fclose(file);
printf("%s file recieved\n", file_name);