#include<stdio.h>
#include<string.h>	//strlen
#include<stdlib.h>	//strlen
#include<sys/socket.h>
#include<arpa/inet.h>	//inet_addr
#include<unistd.h>	//write

#include<pthread.h> //for threading , link with lpthread

#define MAX_NAME_SIZE 20
#define MAX_NAME_ENTER 50
//UserName#UserIP#UserPortNum
#define MAX_LIST_LEN 40 

struct arg_struct {
    int* arg1_socket;
    int* arg2_ncount;
    char (*arg3_narr)[MAX_NAME_SIZE];
    int* arg4_online;
    char* arg5_clientIP;
    char (*arg6_online_list)[MAX_LIST_LEN];
};

#ifndef __cplusplus
typedef unsigned char bool;
static const bool false = 0;
static const bool true = 1;
#endif

void *connection_handler(void *arguments);

int main(int argc , char *argv[])
{
	int socket_desc , new_socket ,c , *new_sock;
	struct sockaddr_in server , client;
	char *message = NULL;
	
	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1)
	{
		printf("Could not create socket");
	}
	
	int port_num;
	//Here enter port manually
	printf("Port number: ");
	scanf("%d", &port_num);

	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons(port_num);
	
	//Bind
	if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
	{
		puts("bind failed");
		return 1;
	}
	puts("bind done");
	
	//Listen
	listen(socket_desc , 3);

	//use a string array to store users who want to REGISTER
	char name_arr[MAX_NAME_ENTER][MAX_NAME_SIZE];
	//Store online list
	char online_list[MAX_NAME_ENTER][MAX_LIST_LEN];
	//Initialize them
	for(int i = 0; i < MAX_NAME_ENTER; i++)
	{
		memset(&name_arr[i], 0, MAX_NAME_SIZE);
		memset(&online_list[i], 0, MAX_LIST_LEN);
	}
	
	int name_count = 0;
	int online_count = 0;
	
	//Accept and incoming connection
	puts("Waiting for incoming connections...");
	c = sizeof(struct sockaddr_in);
	while( (new_socket = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
	{
		puts("Connection accepted");
		
		//Get client'IP address
		struct sockaddr_in* pV4Addr = (struct sockaddr_in*)&client;
		struct in_addr ipAddr = pV4Addr->sin_addr;
		char client_ip[INET_ADDRSTRLEN];
		inet_ntop( AF_INET, &ipAddr, client_ip, INET_ADDRSTRLEN );
		
		printf("\nClient IP: %s\n\n", client_ip);

		//Reply to the client
		message = (char*)"Hello Client , I have received your connection. And now I will assign a handler for you.\n";
		write(new_socket , message , strlen(message));
		
		pthread_t sniffer_thread;
		//Pass multiple arguments to the thread
		struct arg_struct args;
		args.arg1_socket = (int*)malloc(1);
		*args.arg1_socket = new_socket;
		args.arg2_ncount = &name_count;
		args.arg3_narr = name_arr;
		args.arg4_online = &online_count;
		args.arg5_clientIP = client_ip;
		args.arg6_online_list = online_list;
		
		if( pthread_create( &sniffer_thread , NULL ,  connection_handler ,  (void *)&args) < 0)
		{
			perror("could not create thread");
			return 1;
		}
		
		//Now join the thread , so that we dont terminate before the thread
		//pthread_join( sniffer_thread , NULL);
		puts("Handler assigned");
	}
	
	if (new_socket<0)
	{
		perror("accept failed");
		return 1;
	}
	
	return 0;
}

/*
 * This will handle connection for each client
 * */
void *connection_handler(void *arguments)
{
	struct arg_struct *args = (struct arg_struct *)arguments;
	
	//Get the socket descriptor
	int sock = *(int*)args->arg1_socket;
	char message[500] , client_message[50] = "\0";
	
	//List components: number of accounts online, client_ip_addr , name_buf and port_buf
	char name_buf[MAX_NAME_SIZE];
	char port_buf[MAX_NAME_SIZE];

	bool has_regis = false;
	bool has_log = false;
	
	int client_index = -1;
	
	//Receive a message from client
	while(recv(sock , client_message , 50 , 0) > 0)
	{
		printf("recv success! --> %s\n", client_message);
		
		//Find "#" position in client_message
		char *seperate;
		size_t index;
		seperate = strchr(client_message, '#');

		//Send the message according to client_message
		//1.REGISTER#
		if(strncmp(client_message, "REGISTER#", 9) == 0)
		{
			char new_name_buf[MAX_NAME_SIZE];
			
			//REGISTER#sara#100 --> 19
			int name_len = strlen(client_message) - 9 -4;
			int i = 0;
			
			//Extract user name from client_message
			while(i < name_len)
			{
				new_name_buf[i] = client_message[9+i];
				i++;
			}

			new_name_buf[name_len] = '\0';
			
			//Check if it is in name_arr:
			//If it already exists, then return "210 FAIL\n" to client
			//If not, add it to the name_arr
			
			bool new_name = true;
			
			for(int j = 0; j <= MAX_NAME_ENTER; j++)
			{
				if(strcmp(args->arg3_narr[j], new_name_buf) == 0)
				{
					new_name = false;
					break;
				}
			}
			if(new_name)
			{
				if(*args->arg2_ncount == MAX_NAME_ENTER)
				{	
					//The name array arg3_narr has already full
					//Ask the client to log in of the ten accounts
					strcpy(message, "You have registered too many accounts, please try to enter one of the accouts.\n");
					write(sock , message , strlen(message));
				}
				else{
					strcpy(message, "100 OK\n");
					write(sock , message , strlen(message));
					strcpy(args->arg3_narr[*args->arg2_ncount], new_name_buf);
					*args->arg2_ncount += 1;
				}
			}
			else
			{
				//this name has already been registered
				strcpy(message, "210 FAIL\n");
				write(sock , message , strlen(message));
			}
		}
		//2.UserAccountName#portNum
		else if (seperate) 
		{

			index = (int)(seperate - client_message);
			//client_message[0]~[index-1] is UserAccountName
			//client_message[index + 1]~[strlen(client_message)-1] is portNum
			
			for(int i = 0; i < index; i++)
				name_buf[i] = client_message[i];
			name_buf[index] = '\0';
			
		
			//sara#1234 index = 4, len = 10
			for(int i = index+1; i < strlen(client_message); i++)
				port_buf[i-index-1] = client_message[i];
			port_buf[strlen(client_message)-1-index] = '\0';
			
			//Check if port_buf is valid
			bool port_valid = true;
			bool port_range = true;
			for(int i = 0; i < strlen(port_buf); i++)
			{
				if(((int)port_buf[i] < 48) || ((int)port_buf[i] > 57))
				{
					port_valid = false;
					break;
				}
			}
			if(port_valid)
			{
				int port_num = atoi(port_buf);
				if((port_num > 65535) || (port_num < 1024))
				{
					port_range = false;
				}
			}
			
			for(int j = 0; j <= MAX_NAME_ENTER; j++)
			{
				if(strcmp(args->arg3_narr[j], name_buf) == 0)
				{
					has_regis = true;
					break;
				}
			}
			
			if(has_regis)
			{
				if(!has_log && port_valid && port_range)
				{
					//Number of online account increase by 1
					*args->arg4_online += 1;
					//Show List here
					char online_count[5];
					sprintf(online_count, "%d", *args->arg4_online);
					
					strcpy(message, "100\nNumber of accounts online: ");
					strcat(message, online_count);
					strcat(message, "\n");
					
					//1.Find client_index of name_buf in args->arg3_narr[j]
					
					client_index = *args->arg2_ncount - 1;
					//2.Store Name#IP#port as a string at arg6_online_list[client_index]
					char temp[MAX_LIST_LEN];
					strcpy(temp, name_buf);
					strcat(temp, "#");
					strcat(temp, args->arg5_clientIP);
					strcat(temp, "#");
					strcat(temp, port_buf);
					strcpy(args->arg6_online_list[client_index], temp);
					
					//3.Append all the active list data on message,
					//and write to client
					for(int i = 0; i < MAX_NAME_ENTER; i++)
					{
						if(strlen(args->arg6_online_list[i]) > 0)
						{
							strcat(message, args->arg6_online_list[i]);
							strcat(message, "\n");
						}
					}
					
					write(sock , message , strlen(message));
					
					has_log = true;
				}
				else if(!has_log && !port_valid && port_range)
				{
					strcpy(message, "Port number is not valid!\n(contains non-integer character)\n");
					write(sock , message , strlen(message));
				}
				else if(!has_log && port_valid && !port_range)
				{
					strcpy(message, "Port number is not between 1024 and 65535\n");
					write(sock , message , strlen(message));
				}
				else
				{	
					//Can't log in twice
					strcpy(message, "Please type the right option number!\n(Only “List” and “Exit” command after entering an account)\n");
					write(sock , message , strlen(message));
				}
			}
			else
			{	
				//If the client hasn's registered, then he/she can't logged in
				strcpy(message, "220 AUTH_FAIL\n");
				write(sock , message , strlen(message));
			}
		}
		
		//3.List
		else if(strcmp(client_message, "List") == 0)
		{
			if(has_regis && has_log)
			{
				char online_count[5];
				sprintf(online_count, "%d", *args->arg4_online);
				
				strcpy(message, "100\nNumber of accounts online: ");
				strcat(message, online_count);
				strcat(message, "\n");
				
				for(int i = 0; i < MAX_NAME_ENTER; i++)
				{
					if(strlen(args->arg6_online_list[i]) > 0)
					{
						strcat(message, args->arg6_online_list[i]);
						strcat(message, "\n");
					}
				}
				
				
				write(sock , message , strlen(message));
			}
		
			else
			{
				//If the client hasn's registered and logged then he/she can't use "List"
				strcpy(message, "220 AUTH_FAIL\n");
				write(sock , message , strlen(message));
			}
			
		}
		
		//4.Exit
		else if(strcmp(client_message, "Exit") == 0)
		{
			*args->arg4_online -= 1;
			
			//Delete Name#IP#port as a string at arg6_online_list[client_index]
			strcpy(args->arg6_online_list[client_index], "\0");
			
			strcpy(message, "Bye\n");
			write(sock , message , strlen(message));
			puts("Client Disconnected!");
			break;
		}
		
		//5.Wrong Command before and after registering
		else
		{
			if(has_regis && has_log)
			{
				strcpy(message, "Please type the right option number!\n(Only “List” and “Exit” command after entering an account)\n");
				write(sock , message , strlen(message));
			}
			else
			{
				strcpy(message, "220 AUTH_FAIL\n");
				write(sock , message , strlen(message));
			}
		}
		
		memset(&client_message[0], 0, 50);
		
	}
	return 0;
}
