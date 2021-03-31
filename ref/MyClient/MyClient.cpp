#pragma comment(lib, "Ws2_32.lib")
#pragma warning(disable:4996) 

#include <WinSock2.h>
#include <iostream>
#include <string>

using namespace std;

int main(int argc, char* argv[])
{
    char server_reply[2000];

    //開始 Winsock-DLL
    WSAData wsaData;
    WORD DLLVersion;
    DLLVersion = MAKEWORD(2, 1);
    int r = WSAStartup(DLLVersion, &wsaData);
    if (r != 0) 
    {
        // 初始化Winsock 失敗
        cout << "Init socket error!";
        return 1;
    }

    //宣告給 socket 使用的 sockadder_in 結構
    SOCKADDR_IN server;
    int addlen = sizeof(server);

    //設定 socket
    SOCKET socket_desc = INVALID_SOCKET;
    //AF_INET: internet-family
    //SOCKET_STREAM: connection-oriented socket
    //IPPROTO_TCP
    // sConnect = socket(AF_INET, SOCK_STREAM, 0);
    socket_desc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_desc == INVALID_SOCKET)
    {
        // 建立失敗
        cout << "Could not create socket!";
        return 1;
    }

    //設定 server 資料
    char server_ip[20];
    int port_num;
    printf("———————————————————————————————————————————————\n");
    printf("Welcome!\nWhich server do you want to connect ? \n\nIP address : ");
    scanf("%s", server_ip);
    printf("Port number: ");
    scanf("%d", &port_num);
    printf("———————————————————————————————————————————————\n");
    
    server.sin_addr.s_addr = inet_addr(server_ip);
    server.sin_family = AF_INET;
    server.sin_port = htons(port_num);

    //connect to remote server
    // connect(sConnect, (SOCKADDR*)&addr, sizeof(addr));
    if (connect(socket_desc, (SOCKADDR*)&server, sizeof(server)) < 0)
    {
        cout << "connect error !";
        return 1;
    }

    //receive a reply from the server
    memset(&server_reply[0], 0, sizeof(server_reply));
    if (recv(socket_desc, server_reply, 2000, 0) < 0)
    {
        cout << "receive failed !";
        return 1;
    }

    cout << server_reply << endl;
    memset(&server_reply[0], 0, sizeof(server_reply));
    printf("———————————————————————————————————————————————\nWhat do you want to do?\n");

    char message[20];
    memset(&message[0], 0, sizeof(message));
    while (strcmp(message, "Exit") != 0)
    {
        memset(&message[0], 0, sizeof(message));
        scanf("%s", message);

        //Send some data
        if (send(socket_desc, message, strlen(message), 0) < 0)
        {
            cout << "send failed !";
            return 1;
        }

        //Receive a reply from the server
        if (recv(socket_desc, server_reply, 500, 0) < 0)
        {
            cout << "recv failed !" << endl;
        }
        cout << server_reply << endl;
        memset(&server_reply[0], 0, sizeof(server_reply));
    }

    //不再使用，可用 closesocket 關閉連線
    closesocket(socket_desc);
    return 0;
}

