
CC = gcc
CFLAGS  = -g -Wall
SERVER_TARGET = serverapp
CLIENT_TARGET = clientapp
all:
	$(CC) $(CFLAGS) server/udp_server.c -o serverapp; $(CC) $(CFLAGS) client/udp_client.c -o clientapp

clean:
	$(RM) serverapp clientapp;