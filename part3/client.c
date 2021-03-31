//Compile: gcc -o client client.c -lssl -lcrypto

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
#include <sys/socket.h>
#include<arpa/inet.h>	//inet_addr
#include <resolv.h>
#include <netdb.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#define FAIL -1

SSL_CTX* InitCTX(void)
{
    SSL_CTX *ctx;
    OpenSSL_add_all_algorithms();  /* Load cryptos, et.al. */
    SSL_load_error_strings();   /* Bring in and register error messages */
    SSL_library_init();
    ctx = SSL_CTX_new(SSLv23_client_method());   /* Create new context */
    if ( ctx == NULL )
    {
        ERR_print_errors_fp(stderr);
        abort();
    }
    return ctx;
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

void ShowCerts(SSL* ssl)
{
    X509 *cert;
    char *line;
    cert = SSL_get_peer_certificate(ssl); /* get the server's certificate */
    if ( cert != NULL )
    {
        printf("Server certificates:\n");
        line = X509_NAME_oneline(X509_get_subject_name(cert), 0, 0);
        printf("Subject: %s\n", line);
        free(line);       /* free the malloc'ed string */
        line = X509_NAME_oneline(X509_get_issuer_name(cert), 0, 0);
        printf("Issuer: %s\n", line);
        free(line);       /* free the malloc'ed string */
        X509_free(cert);     /* free the malloc'ed certificate copy */
    }
    else
        printf("Info: No client certificates configured.\n");
}

int main(int argc , char *argv[])
{
	int socket_desc;
    struct sockaddr_in server;
    bzero(&server,sizeof(server)); //initiation
    char server_reply[2000];

    //CTX
    SSL_CTX *ctx;
    SSL *ssl;
    char buf[1024];
    char acClientRequest[1024] = {0};
    int bytes;
	
	ctx = InitCTX();

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
        
    ssl = SSL_new(ctx);      // create new SSL connection state 
    SSL_set_fd(ssl, socket_desc);    // attach the socket descriptor 
    int ssl_err = SSL_connect(ssl);
	printf("ssl_err: %d\n", ssl_err);
	if(ssl_err <= 0)
	{
		printf("Handshake error %d\n", SSL_get_error(ssl, ssl_err));
		//Error occurred, log and close down ssl
		ShutdownSSL(ssl);
		return -1;
	}
    else ShowCerts(ssl);

    //receive a reply from the server
    if(SSL_read(ssl, server_reply, 2000) <= 0){
        puts("receive failed");
    }
        
    puts(server_reply);
    memset(&server_reply[0], 0, sizeof(server_reply));

    char message[20];
    printf("———————————————————————————————————————————————\n");
    printf("Hello, what would you like to do now?\n");
	while(strcmp(message, "Exit") != 0){
		
	    memset(&message[0], 0, sizeof(message));
	    scanf("%s", message);

	    //Send some data
	    if(SSL_write(ssl, message, strlen(message)) <= 0)
	    {
	    	puts("send failed");
	    	return 1;
	    }
		
	    //Receive a reply from the server
	    if(SSL_read(ssl, server_reply, 1000) <= 0)
	    { 
	    	puts("recv failed");
	    }

	    puts(server_reply);
	    memset(&server_reply[0], 0, sizeof(server_reply));	
        
	}
    SSL_free(ssl);        // release connection state 
    close(socket_desc);
    SSL_CTX_free(ctx);        /* release context */
	return 0;
}