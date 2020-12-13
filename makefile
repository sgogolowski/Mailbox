all: client server
	
client: client.c
	gcc client.c -o DUMBclient
server: server.c
	gcc server.c -o DUMBserver -lpthread -g
clean:
	rm DUMBclient; rm DUMBserver
