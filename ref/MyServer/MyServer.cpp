#pragma comment(lib, "Ws2_32.lib")
#pragma warning(disable:4996) 

#include <WinSock2.h>
#include <iostream>
#include <thread>

using namespace std;

//UserName#UserIP#UserPortNum
#define MAX_NAME_SIZE 20
#define MAX_IP_SIZE 20
#define MAX_PORT_SIZE 20
#define MAX_CLIENT_NO 100

struct member_struct
{
    SOCKET user_socket;
    char user_name[MAX_NAME_SIZE];
    char user_ip[MAX_IP_SIZE];
    char user_port[MAX_PORT_SIZE];
    int account_balance;
    bool is_use;
};
struct member_struct member[MAX_CLIENT_NO];
int online_count = 0;

void connection_handler(SOCKET clientSocket);

int main(int argc, char* argv[])
{
    //開始 Winsock-DLL
    WSAData wsaData;
    WORD DLLVSERION;
    DLLVSERION = MAKEWORD(2, 1);//Winsocket-DLL 版本
    int r = WSAStartup(DLLVSERION, &wsaData);
    if (r != 0)
    {
        // 初始化Winsock 失敗
        cout << "Init socket error!";
        return 1;
    }

    //宣告 socket 位址資訊
    SOCKADDR_IN server, client;
    int addrlen = sizeof(client);

    //設定 socket
    SOCKET socket_desc = INVALID_SOCKET;
    socket_desc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_desc == INVALID_SOCKET)
    {
        // 建立失敗
        cout << "Could not create socket!";
        return 1;
    }

    //Here enter port manually
    int port_num;
    printf("Port number: ");
    scanf("%d", &port_num);

    //Prepare the sockaddr_in structure
    // server.sin_addr.s_addr = inet_addr("127.0.0.1");
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_family = AF_INET;
    server.sin_port = htons(port_num);

    //建立 socket
    SOCKET sListen; //listening for an incoming connection
    SOCKET sConnect; //operating if a connection was found

    //Bind
    sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (bind(sListen, (SOCKADDR*)&server, sizeof(server)) < 0)
    {
        cout << "Bind failed !";
        return 1;
    }
    cout << "Bind done" << endl;

    //Listen
    // listen(socket_desc, 3);
    listen(sListen, SOMAXCONN);     //SOMAXCONN: listening without any limit

    const char* message = "Hello Client , I have received your connection. And now I will assign a handler for you.\n";
    while (true)
    {
        //等待連線
        cout << "Waiting for incoming connections..." << endl;

        if (sConnect = accept(sListen, (SOCKADDR*)&client, &addrlen))
        {
            cout << "Connection accepted" << endl;
            printf("\nClient IP: %s\n\n", inet_ntoa(client.sin_addr));

            //傳送訊息給 client 端
            // write(new_socket, message, strlen(message));
            send(sConnect, message, (int)strlen(message), 0);

            thread newhandler(connection_handler, sConnect);
            newhandler.detach();
            puts("Handler assigned.");
        }
    }

    return 0;
}

// This will handle connection for each client
void connection_handler(SOCKET clientSocket)
{
    bool has_regis = false;
    bool has_log = false;
    char request[50] = "\0";
    char message[500];
    int member_id=0;

    //Receive a message from client
    while (recv((int)clientSocket, request, 50, 0) > 0)
    {
        printf("recv success --> %s\n", request);

        //Find "#" position in request
        char* seperate = strchr(request, '#');

        // Send the message according to client_message
        if (strcmp(request, "Exit") == 0)
        {
            // Exit
            if (has_log)
            {
                member[member_id].is_use = false;
                online_count--;
            }
            strcpy(message, "Bye\n");
            send(clientSocket, message, (int)strlen(message), 0);
            puts("Client disconnected.");
            break;
        }
        else if (strncmp(request, "REGISTER#", 9) == 0)
        {
            //REGISTER#<UserAccountName>#<depositAmount><CRLF>
            has_regis = false;
            char* p = strrchr(request, '#');

            char name_buffer[MAX_NAME_SIZE];
            // strncpy(name_buffer, seperate + 1, p - seperate - 1);
            for (int i = 0; i < (p-seperate-1); i++)
                name_buffer[i] = request[i+9];
            name_buffer[p - seperate-1] = '\0';

            char buffer[20];
            strcpy(buffer, p + 1);

            //Check if it is in member:
            //If it already exists, then return "210 FAIL\n" to client
            //If not, add it to the member
            bool new_name = true;
            for (int i = 0; i < MAX_CLIENT_NO; i++)
            {
                if ((member[i].is_use == true) && (strcmp(member[i].user_name, name_buffer) == 0))
                {
                    new_name = false;
                    break;
                }
            }

            if (new_name)
            {
                if (online_count == MAX_CLIENT_NO)
                {
                    //The name array has already full
                    //Ask the client to log in of the ten accounts
                    strcpy(message, "You have registered too many accounts, please try to enter one of the accouts.\n");
                    send(clientSocket, message, (int)strlen(message), 0);
                }
                else 
                {
                    for (int i = 0; i < MAX_CLIENT_NO; i++)
                    {
                        if (member[i].is_use == false)
                        {
                            member[i].user_socket = clientSocket;
                            strcpy(member[i].user_name, name_buffer);
                            strcpy(member[i].user_ip, "127.0.0.1");
                            strcpy(member[i].user_port, "00");
                            member[i].account_balance = atoi(buffer);
                            member[i].is_use = true;
                            has_regis = true;
                            break;
                        }
                    }

                    strcpy(message, "100 OK\n");
                    send(clientSocket, message, (int)strlen(message), 0);
                }
            }
            else
            {
                //this name has already been registered
                strcpy(message, "210 FAIL\n");
                send(clientSocket, message, (int)strlen(message), 0);
            }
        }
        else if (seperate)
        {
            // <UserAccountName>#<portNum><CRLF>
            has_log = false;
            char name_buffer[MAX_NAME_SIZE];
            strncpy(name_buffer, request, seperate - request);
            name_buffer[seperate - request] = '\0';

            char buffer[20];
            strcpy(buffer, seperate + 1);

            bool found_name = false;
            for (int i = 0; i < MAX_CLIENT_NO; i++)
            {
                if ((member[i].is_use == true) && (strcmp(member[i].user_name, name_buffer) == 0))
                {
                    found_name = true;
                    member_id = i;
                    strcpy(member[i].user_ip, "127.0.0.1");
                    strcpy(member[i].user_port, buffer);
                    has_log = true;
                    online_count++;
                    break;
                }
            }

            if (found_name)
            {
                sprintf(buffer, "%d\n", member[member_id].account_balance);
                strcpy(message, buffer);

                sprintf(buffer, "%d\n", online_count);
                strcat(message, buffer);

                for (int i = 0; i < MAX_CLIENT_NO; i++)
                {
                    if (member[i].is_use == true)
                    {
                        strcat(message, member[i].user_name);
                        strcat(message, "#");
                        strcat(message, member[i].user_ip);
                        strcat(message, "#");
                        strcat(message, member[i].user_port);
                        strcat(message, "\n");
                    }
                }

                send(clientSocket, message, (int)strlen(message), 0);
            }
            else
            {
                //If the client hasn's registered, then he/she can't logged in
                strcpy(message, "220 AUTH_FAIL\n");
                send(clientSocket, message, (int)strlen(message), 0);
            }
        }
        else
        {
            // if (has_regis && has_log)
            if (has_regis)
            {
                if (strcmp(request, "List") == 0)
                {
                    char buffer[20];
                    sprintf(buffer, "%d\n", member[member_id].account_balance);
                    strcpy(message, buffer);

                    sprintf(buffer, "%d\n", online_count);
                    strcat(message, buffer);

                    for (int i = 0; i < MAX_CLIENT_NO; i++)
                    {
                        if (member[i].is_use == true)
                        {
                            strcat(message, member[i].user_name);
                            strcat(message, "#");
                            strcat(message, member[i].user_ip);
                            strcat(message, "#");
                            strcat(message, member[i].user_port);
                            strcat(message, "\n");
                        }
                    }

                    send(clientSocket, message, (int)strlen(message), 0);
                }
                else
                {
                    // Wrong Command before and after registering
                    strcpy(message, "Please type the right option number!\n(Only “List” and “Exit” command after entering an account)\n");
                    send(clientSocket, message, (int)strlen(message), 0);
                }
            }
            else
            {
                //If the client hasn's registered and logged then he/she can't use "List"
                strcpy(message, "220 AUTH_FAIL\n");
                send(clientSocket, message, (int)strlen(message), 0);
            }
        }
        memset(&request[0], 0, 50);
    }
}
