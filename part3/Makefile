all: server client

server: server
	gcc -o server server.c -lssl -lcrypto -l pthread

client: client
	gcc -o client client.c -lssl -lcrypto