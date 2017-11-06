CC = gcc
CFLAGS = -Wall -g 
 
all:
	$(CC) $(CFLAGS) client.c libnetfiles.c -o libnetfiles
	$(CC) $(CFLAGS) -pthread netfileserver.c -o netfileserver

clean:
	rm -rf *.o libnetfiles netfileserver