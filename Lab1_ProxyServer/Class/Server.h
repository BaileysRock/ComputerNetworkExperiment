//
// Created by Alienware on 2021/10/24.
//

#ifndef LAB1_SERVER_H
#define LAB1_SERVER_H
#include <winsock2.h>
#include <iostream>
using namespace std;
class Server {
protected:
    // 是否成功启动Server
    bool CreateServer = false;
    // 默认端口
    unsigned DefaultPort = 8080;
    // 响应客户端链接的socket
    SOCKET ServerSocket;
    // 服务器信息
    SOCKADDR_IN ServerAddrIn;
public:
    // 构造函数
    Server();
    // 析构函数
    ~Server();
    // 加载资源
    void LoadResource();
    // 使用默认端口初始化服务器
    void InitServer();
    // 使用给定端口初始化服务器
    void InitServer(unsigned PORT);
    // 是否成功构造了Server
    bool isCreateServer();
};
#endif //LAB1_SERVER_H
