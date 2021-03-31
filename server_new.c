/*
Compile:
gcc -o server_new  server_new.c -lssl -lcrypto -l pthread
 */

#include <stdio.h>
#include <string.h>	//strlen
#include <stdlib.h>	
#include <sys/socket.h>
#include <arpa/inet.h>	//inet_addr
#include <unistd.h> //write
#include <stdbool.h>
#include <pthread.h> //for threading , link with lpthread

#include <openssl/bio.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <errno.h>
#include <malloc.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <resolv.h>


#define FAIL    -1

#define MAX_NAME_SIZE 20
#define MAX_NAME_ENTER 50
//UserName#UserIP#UserPortNum
#define MAX_LIST_LEN 40 

SSL_CTX* InitServerCTX(void)
{
	SSL_CTX *ctx2;
    OpenSSL_add_all_algorithms();  /* load & register all cryptos, etc. */
    SSL_load_error_strings();   /* load all error messages */
	SSL_library_init();
    ctx2 = SSL_CTX_new(SSLv23_server_method());   /* create new context from method */
	SSL_CTX_set_options(ctx2, SSL_OP_SINGLE_DH_USE);
    if ( ctx2 == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx2;
}

void DestroySSL()
{
	// frees all previously loaded error strings.
    ERR_free_strings();
    EVP_cleanup();
}

void ShutdownSSL(SSL *ssl)
{
	// shuts down an active TLS/SSL connection.
    SSL_shutdown(ssl);
    SSL_free(ssl);
}

void LoadCertificates(SSL_CTX* ctx, char* CertFile, char* KeyFile)
{
    /* set the local certificate from CertFile */
    if ( SSL_CTX_use_certificate_file(ctx, CertFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* set the private key from KeyFile (may be the same as CertFile) */
    if ( SSL_CTX_use_PrivateKey_file(ctx, KeyFile, SSL_FILETYPE_PEM) <= 0 )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    /* verify private key */
    if ( !SSL_CTX_check_private_key(ctx) )
    {
        fprintf(stderr, "Private key does not match the public certificate\n");
        abort();
    }
}

void ShowCerts(SSL* ssl)
{
    X509 *cert;
    char *line;
    cert = SSL_get_peer_certificate(ssl); /* Get certificates (if available) */
    if ( cert != NULL )
    {
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: %s\n", line);
        free(line);
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);
        free(line);
        X509_free(cert);
    }
    else
        printf("No certificates.\n");
}

struct arg_struct {
    int* arg1_socket;
    int* arg2_ncount;
    char (*arg3_narr)[MAX_NAME_SIZE];
    int* arg4_online;
    char* arg5_clientIP;
    char (*arg6_online_list)[MAX_LIST_LEN];
    int* arg7_balances;
    SSL* arg7_ssl;
};

void *connection_handler(void *arguments);

int main(int argc , char *argv[])
{
	SSL_CTX *ctx;
	SSL *ssl;
    ctx = InitServerCTX();        /* initialize SSL */
    LoadCertificates(ctx, "mycert.pem", "mycert.pem"); /* load certs */
	
	// SSL_CTX_use_certificate_file() loads the first certificate stored in file into ctx. 
	// The formatting type of the certificate must be specified from the known types SSL_FILETYPE_PEM
	//int use_cert = SSL_CTX_use_certificate_file(sslctx, "/cert.pem" , SSL_FILETYPE_PEM);

	// SSL_CTX_use_PrivateKey_file() adds the first private key found in file to ctx. 
	// The formatting type of the certificate must be specified from the known types SSL_FILETYPE_PEM
	//int use_prv = SSL_CTX_use_PrivateKey_file(sslctx, "/cert.pem", SSL_FILETYPE_PEM);
	ssl = SSL_new(ctx);
	int socket_desc , new_socket , c;
	struct sockaddr_in server , client;
	char *message = NULL;
	
	int port_num;
	//Here enter port manually
	printf("Port number: ");
	scanf("%d", &port_num);
	
	//Create socket
	socket_desc = socket(AF_INET , SOCK_STREAM , 0);
	if (socket_desc == -1)
	{
		printf("Could not create socket");
	}
	
	//Prepare the sockaddr_in structure
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = INADDR_ANY;
	server.sin_port = htons( port_num );
	
	//Bind
	if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
	{
		puts("bind failed");
		return 1;
	}
	
	//Listen
	listen(socket_desc , 3);
	
	//initialize account money 
	int balances[MAX_NAME_ENTER];
	
	//use a string array to store users who want to REGISTER
	char name_arr[MAX_NAME_ENTER][MAX_NAME_SIZE];
	//Store online list
	char online_list[MAX_NAME_ENTER][MAX_LIST_LEN];
	//Initialize them
	for(int i = 0; i < MAX_NAME_ENTER; i++)
	{
		balances[i] = 100;
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
		
		
		//=========================================================
		// SSL_set_fd() sets the file descriptor fd as the input/output facility for the TLS/SSL (encrypted) side of ssl. 
		// fd will typically be the socket file descriptor of a network connection.
		ssl = SSL_new(ctx);              /* get new SSL state with context */
        SSL_set_fd(ssl, new_socket);      /* set connection socket to SSL state */
		
		puts("after SSL_set_fd(cSSL, new_socket)\n");
		
		// Here is the SSL Accept portion. Now all reads and writes must use SSL
		// SSL_accept() waits for a TLS/SSL client to initiate the TLS/SSL handshake.
		int ssl_err = SSL_accept(ssl);
		
		printf("ssl_err: %d\n", ssl_err);
		
		if(ssl_err <= 0)
		{
			printf("Handshake error %d\n", SSL_get_error(ssl, ssl_err));
			//Error occurred, log and close down ssl
			ShutdownSSL(ssl);
			return -1;
		}

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
		args.arg7_balances = balances;
		args.arg7_ssl = ssl;


		//Reply to the client
		message = "SSL Connection accepted\nHello from the server!\nThe server will assign a handler to you soon\n";
		SSL_write(args.arg7_ssl, message, strlen(message));	
		
		if( pthread_create( &sniffer_thread , NULL ,  connection_handler, (void *)&args) < 0)
		{
			perror("could not create thread");
			return 1;
		}
		puts("Handler assigned");	
	}
	
	if (new_socket < 0)
	{
		perror("accept failed");
		return 1;
	}

	SSL_CTX_free(ctx);         /* release context */
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
	int port_num = 0;

	//balance
	char balance_left[10] = {0};

	//Receive a message from client
	while(SSL_read(args->arg7_ssl, client_message , 50) > 0)
	{
		printf("recv success! --> %s\n", client_message);
		
		//Find "#" position in client_message
		char *seperate;
		size_t index;
		seperate = strchr(client_message, '#');
		int tempBalance = 0;
		
		//Send the message according to client_message
		//1.REGISTER#
		if(strncmp(client_message, "REGISTER#", 9) == 0)
		{
			char new_name_buf[MAX_NAME_SIZE];
			
			//REGISTER#sara#100 --> 14
			char *sep = strrchr(client_message, '#');
			int index2 = (int)(sep-client_message);
			int name_len = index2 - 9;
			char money[10] = {0};
			for(int i = index2+1; i < strlen(client_message); i++){
				for(int j = 0; j < strlen(money); j++){
					money[j] = client_message[i];
				}
			}
			tempBalance = (atoi)(money);

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
					SSL_write(args->arg7_ssl , message , strlen(message));
				}
				else{
					strcpy(message, "100 OK\n");
					SSL_write(args->arg7_ssl , message , strlen(message));
					strcpy(args->arg3_narr[*args->arg2_ncount], new_name_buf);
					*args->arg2_ncount += 1;
					args->arg7_balances += tempBalance;
				}
			}
			else
			{
				//this name has already been registered
				strcpy(message, "210 FAIL\n");
				SSL_write(args->arg7_ssl , message , strlen(message));
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
			
		
			//sara#1234 index = 4, len = 11
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
				port_num = atoi(port_buf);
				if((port_num > 65535) || (port_num < 1024))
				{
					port_range = false;
				}
			}
			
			for(int j = 0; j <= MAX_NAME_ENTER; j++)
			{
				if(strcmp(args->arg3_narr[j], name_buf) == 0)
				{
					client_index = j;
					has_regis = true;
					break;
				}
			}
			
			printf("client_index: %d\n", client_index);
			
			
			if(has_regis)
			{
				if(!has_log && port_valid && port_range)
				{
					//Number of online account increase by 1
					*args->arg4_online += 1;
					//Show List here
					char online_count[5];
					sprintf(online_count, "%d", *args->arg4_online);
					
					sprintf(balance_left, "%d", args->arg7_balances[client_index]);
					strcpy(message, balance_left);
					strcat(message, "\n");
					strcat(message, "Number of accounts online: ");
					strcat(message, online_count);
					strcat(message, "\n");
					
					//1.Find client_index of name_buf in args->arg3_narr[j]
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
					
					SSL_write(args->arg7_ssl , message , strlen(message));
					
					has_log = true;
				}
				else if(!has_log && !port_valid && port_range)
				{
					strcpy(message, "Port number is not valid!\n(contains non-integer character)\n");
					SSL_write(args->arg7_ssl , message , strlen(message));
				}
				else if(!has_log && port_valid && !port_range)
				{
					strcpy(message, "Port number is not between 1024 and 65535\n");
					SSL_write(args->arg7_ssl , message , strlen(message));
				}
				else
				{	
					//Can't log in twice
					strcpy(message, "Please type the right option number!\n(Only “List” and “Exit” command after entering an account)\n");
					SSL_write(args->arg7_ssl , message , strlen(message));
				}
			}
			else
			{	
				//If the client hasn's registered, then he/she can't logged in
				strcpy(message, "220 AUTH_FAIL\n");
				SSL_write(args->arg7_ssl, message , strlen(message));
			}
		}
		
		//3.Micropayment transaction: sarah#10#tina
		else if( (strncmp(client_message, "Micropayment", 12) == 0) && has_regis && has_log)
		{
			strcpy(message, "Enter your name:");
			SSL_write(args->arg7_ssl, message, strlen(message));
			SSL_read(args->arg7_ssl, client_message, strlen(client_message));

			//char *sep1 = strchr(client_message, '#');
			//int name_len = (int)(sep1-client_message);
			//char *sep2 = strrchr(client_message, '#');
			//int payAmount_len = (int)(sep2-client_message)-name_len;

			bool find_client = false;
			char name_enter1[MAX_NAME_SIZE];
			char name_enter2[MAX_NAME_SIZE];
			
			int i = 0;
			
			//Extract user name from client_message
			while(i < strlen(client_message))
			{
				name_enter1[i] = client_message[i];
				i++;
			}

			strcpy(message, "Enter the pay amount:");
			SSL_write(args->arg7_ssl, message, strlen(message));
			SSL_read(args->arg7_ssl, client_message, strlen(client_message));

			int payAmount = (atoi)(client_message);
			
			strcpy(message, "Enter payee's name:");
			SSL_write(args->arg7_ssl, message, strlen(message));
			SSL_read(args->arg7_ssl, client_message, strlen(client_message));

			int j = 0;

			while(j < strlen(client_message))
			{
				name_enter2[i] = client_message[i];
				j++;
			}
			
			
			// Find the client who this client wants to connect to
			
			char name_extract[MAX_NAME_SIZE];
			int target_port;
			char port[10];
			int payer_index = client_index;
			int payee_index = -1;
			for(int i = 0; i < MAX_NAME_ENTER; i++)
			{
				
				if(strlen(args->arg6_online_list[i]) > 0)
				{	
											
					//~ strcat(message, args->arg6_online_list[i]);
					//~ strcat(message, "\n");
					memset(name_extract, '\0', sizeof(name_extract));
					char *name_pos = strstr(args->arg6_online_list[i], "#");
					int position = name_pos - args->arg6_online_list[i];
					
					strncpy(name_extract, args->arg6_online_list[i], position);
					
					if(strcmp(name_enter2, name_extract) == 0)
					{
						payee_index = i;
						char *port_pos = strstr(&args->arg6_online_list[i][position + 1], "#");
						
						int port_position = port_pos - args->arg6_online_list[i];
						
						
						int j = 0;
						while(j < strlen(args->arg6_online_list[i]) - port_position -1)
						{
							port[j] = args->arg6_online_list[i][port_position + j + 1];
							j++;
						}
						target_port = atoi(port);
						find_client = true;
						printf("Target port: %d\n",target_port);
						puts("-------FOUND-------");
					}
				}
			}
			
			if(find_client)
			{
				puts("Give IP and port to whom wants to communicate~");
				
				char payer_index_temp[5];
				char payee_index_temp[5];
				sprintf(payer_index_temp, "%d", payer_index);
				sprintf(payee_index_temp, "%d", payee_index);
				
				strcpy(message, "Client Found!\n");
				strcat(message, "Payer index(me): -");
				strcat(message, payer_index_temp);
				strcat(message, "\n");
				strcat(message, "Payee index: +");
				strcat(message, payee_index_temp);
				strcat(message, "\n");
				strcat(message, "Name: ");
				strcat(message, name_extract);
				strcat(message, "\n");
				strcat(message, "Port: #");
				strcat(message, port);
				strcat(message, "\n");
				
				
				SSL_write(args->arg7_ssl , message , strlen(message));
				//--------------------------------------------------
					
			}
			else
			{
				SSL_write(args->arg7_ssl , "No such client!\n" , strlen("No such client!\n"));
			}
			
			
		}
		
		//4.5 Transfer informantion
		// undone
		
		//5.List
		else if(strcmp(client_message, "List") == 0)
		{
			if(has_regis && has_log)
			{
				char online_count[5];
				sprintf(online_count, "%d", *args->arg4_online);
				
				char balance_left[10];
				sprintf(balance_left, "%d", args->arg7_balances[client_index]);
				strcpy(message, balance_left);
				strcat(message, "\n");
				strcat(message, "Number of accounts online: ");
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
				
				
				SSL_write(args->arg7_ssl , message , strlen(message));
			}
		
			else
			{
				//If the client hasn's registered and logged then he/she can't use "List"
				strcpy(message, "220 AUTH_FAIL\n");
				SSL_write(args->arg7_ssl , message , strlen(message));
			}
			
		}
		
		//6.Exit
		else if(strcmp(client_message, "Exit") == 0)
		{
			*args->arg4_online -= 1;
			
			//Delete Name#IP#port as a string at arg6_online_list[client_index]
			strcpy(args->arg6_online_list[client_index], "\0");
			
			strcpy(message, "Bye\n");
			SSL_write(args->arg7_ssl , message , strlen(message));
			break;
		}

		//7.Wrong Command before and after registering
		else
		{
			if(has_regis && has_log)
			{
				strcpy(message, "Please type the right option number!\n(Only “List” and “Exit” command after entering an account)\n");
				SSL_write(args->arg7_ssl , message , strlen(message));
			}
			else
			{
				strcpy(message, "220 AUTH_FAIL\n");
				SSL_write(args->arg7_ssl , message , strlen(message));
			}
		}
		
		memset(&client_message[0], 0, 50);
		
	}

	return 0;
}