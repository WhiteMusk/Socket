#include<stdio.h>
#include<stdlib.h>
#include<string.h>       //strlen
#include<sys/socket.h> 
#include<arpa/inet.h>	//inet_addr
#include<unistd.h>      //close

int main(int argc , char *argv[])
{
	int socket_desc;
    struct sockaddr_in server;
    bzero(&server,sizeof(server)); //initiation
    char server_reply[2000];

    //create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1)
	{
		printf("Could not create socket");
	}

    //inputs
    int port_num;
	char server_ip[20];
    printf("———————————————————————————————————————————————\nWelcome!\nWhich server do you want to connect?\n\nIP address: ");
	scanf("%s", server_ip);	
	printf("Port number: ");
    scanf("%d", &port_num);
	printf("———————————————————————————————————————————————\n");

    server.sin_addr.s_addr = inet_addr(server_ip); 
    server.sin_family = AF_INET;
    server.sin_port = htons(port_num);

    //connect to remote server
    if(connect(socket_desc,(struct sockaddr*)&server, sizeof(server)) < 0){
        puts("connect error");
        return 1;
    }

    //receive a reply from the server
    if(recv(socket_desc, server_reply, 2000, 0) < 0){
        puts("receive failed");
    }

    puts(server_reply);
    memset(&server_reply[0], 0, sizeof(server_reply));

    char message[20];
	printf("———————————————————————————————————————————————\nHello, what would you like to do ?\n");
	while(strcmp(message, "Exit") != 0){
		
		memset(&message[0], 0, sizeof(message));
		scanf("%s", message);
		
		//Send some data
		if( send(socket_desc , message , strlen(message) , 0) < 0)
		{
			puts("send failed");
			return 1;
		}
		
		//Receive a reply from the server
		if( recv(socket_desc, server_reply , 500 , 0) < 0)
		{
			puts("recv failed");
		}
		puts(server_reply);
		memset(&server_reply[0], 0, sizeof(server_reply));	
	}

    close(socket_desc);
	return 0;
}