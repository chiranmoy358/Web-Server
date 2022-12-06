CC = gcc -pthread
SERVER_FILE = server.c
HTTP_SERVER_FILE = libhserver.c

all: server

server: $(SERVER_FILE) $(HTTP_SERVER_FILE)
	$(CC) $(SERVER_FILE) $(HTTP_SERVER_FILE) -o server

clean:
	rm -f server
