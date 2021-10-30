//
// Created by Alienware on 2021/10/25.
//

#include "Client.h"

Client::Client() {


}

/**
 * 初始化Client客户端
 * @param host 服务器地址
 */
void Client::InitClient(char *host) {
//    cout << "初始化客户端中..." << endl;
    // 创建套接字
    ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ClientSocket == INVALID_SOCKET) {
        cout << "创建套接字失败!" << "错误代码为" << WSAGetLastError() << endl;
        CreateClient = false;
    } else {
        LoadResource();
        hostent *hostent = gethostbyname(host);
        in_addr Inaddr = *((in_addr *) *hostent->h_addr_list);
        sockaddr_in serverAddr;
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(ClientDefaultPort);
        serverAddr.sin_addr.s_addr = inet_addr(inet_ntoa(Inaddr));
        int Connect = connect(ClientSocket, (SOCKADDR *) &serverAddr, sizeof(serverAddr));
        if (Connect == SOCKET_ERROR) {
            cout << "连接远程服务器失败!" << endl;
            CreateClient = false;
        } else {
            CreateClient = true;
        }
    }
    if (CreateClient) {
//        cout << "Client启动成功!" << endl;
    } else {
        cout << "Client启动失败!" << endl;
    }
}

/**
 * 向服务器发送数据
 * @param sendBuff
 * @param recvBuff
 * @param recvFromHttpServerSize
 */
void Client::SendDataToServer(char *sendBuff, char *recvBuff,int *recvFromHttpServerSize) {
    int SendError = send(ClientSocket, sendBuff, strlen(sendBuff) + 1, 0);
    int RecvError = 0;
    if (SendError == SOCKET_ERROR) {
        cout << "From proxy to httpServer.发送错误:" << WSAGetLastError() << endl;
    } else {
        while (1) {
            RecvError= recv(ClientSocket, recvBuff, MAX_MESSAGE_LENGTH, 0);
            if (RecvError == SOCKET_ERROR) {
                cout << "From httpServer to proxy.接收错误:" << WSAGetLastError() << endl;
            } else if (RecvError == 0) {
                cout << "From httpServer to proxy.接收完成!" << endl;
                break;
            }
            *recvFromHttpServerSize = RecvError;
            cout << "From httpServer to proxy.正在接收!" << endl;

//            cout << "接收到的报文为" << endl;
//            cout << recvBuff << endl;
        }
    }

}


/**
 * 加载dll资源
 */
void Client::LoadResource() {
// 存储版本
    WORD wVersionRequested;
    // 存储被WSAStartup函数调用后
    // 返回的Windows sockets数据
    WSADATA wsaData;
    // 使用WinSock 2.2版本
    wVersionRequested = MAKEWORD(2, 2);
    int error;
    // 初始化DLL
    error = WSAStartup(wVersionRequested, &wsaData);
    if (error != 0) {
        cout << "客户端加载WinSock失败!" << endl;
        CreateClient = false;
    }
}


/**
 * 析构函数
 */
Client::~Client() {
    closesocket(ClientSocket);
    WSACleanup();
    cout << "Client已释放";
}


bool Client::isCreateClient() {
    return CreateClient;
}