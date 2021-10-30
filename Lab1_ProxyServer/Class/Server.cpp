//
// Created by Alienware on 2021/10/24.
//

#include "Server.h"
/**
 * 构造函数
 */
Server::Server() {

}
/**
 * 初始化Server
 * 使用PORT端口初始化
 * @param PORT 给定的初始化的端口
 */
void Server::InitServer(unsigned PORT) {
//    cout << "初始化服务器中..." << endl;
    // 创建套接字
    ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ServerSocket == INVALID_SOCKET) {
        cout << "创建套接字失败!" << "错误代码为" << WSAGetLastError() << endl;
        CreateServer = false;
    } else {
        // 设置服务器信息
        ServerAddrIn.sin_family = AF_INET;
        // htons 将HTTP_PORT转换为网络字节序
        ServerAddrIn.sin_port = htons(PORT);
        ServerAddrIn.sin_addr.s_addr = htonl(INADDR_ANY);
        // 对socket绑定
        if (bind(ServerSocket, (SOCKADDR *) &ServerAddrIn, sizeof(ServerAddrIn)) == SOCKET_ERROR) {
            cout << "绑定socket失败!" << endl;
            CreateServer = false;
        } else {
            if (listen(ServerSocket, SOMAXCONN) == SOCKET_ERROR) {
                cout << "监听socket失败!" << endl;
                CreateServer = false;
            } else {
                CreateServer = true;
            }
        }
    }
    if (CreateServer) {
//        cout << "服务器启动成功!" << endl;
    } else {
        cout << "服务器启动失败!" << endl;
    }
}

/**
 * 构造函数，使用默认端口初始化Server
 */
void Server::InitServer() {
//    cout << "初始化服务器中..." << endl;
    // 创建套接字
    ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ServerSocket == INVALID_SOCKET) {
        cout << "创建套接字失败!" << "错误代码为" << WSAGetLastError() << endl;
        CreateServer = false;
    } else {
        // 设置服务器信息
        ServerAddrIn.sin_family = AF_INET;
        // htons 将HTTP_PORT转换为网络字节序
        ServerAddrIn.sin_port = htons(DefaultPort);
        ServerAddrIn.sin_addr.s_addr = htonl(INADDR_ANY);
        // 对socket绑定
        if (bind(ServerSocket, (SOCKADDR *) &ServerAddrIn, sizeof(ServerAddrIn)) == SOCKET_ERROR) {
            cout << "绑定socket失败!" << endl;
            CreateServer = false;
        } else {
            if (listen(ServerSocket, SOMAXCONN) == SOCKET_ERROR) {
                cout << "监听socket失败!" << endl;
                CreateServer = false;
            } else {
                CreateServer = true;
            }
        }
    }
    if (CreateServer) {
//        cout << "服务器启动成功!" << endl;
    } else {
        cout << "服务器启动失败!" << endl;
    }
}

/**
 * 加载dll文件
 */
void Server::LoadResource() {
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
        cout << "服务器加载WinSock失败!" << endl;
        CreateServer = false;
    }
}

/**
 * 析构函数，结束运行阶段运行
 */
Server::~Server() {
    closesocket(ServerSocket);
    WSACleanup();
    cout << "Server已释放";
}

/**
 * 返回是否生成了Server
 * @return
 */
bool Server::isCreateServer() {
    return CreateServer;
}