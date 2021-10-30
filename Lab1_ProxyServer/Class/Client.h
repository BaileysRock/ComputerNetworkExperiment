//
// Created by Alienware on 2021/10/25.
//

#ifndef LAB1_CLIENT_H
#define LAB1_CLIENT_H
#include <winsock2.h>
#include <iostream>
#include<Ws2tcpip.h>
using namespace std;

class Client {

private:
    // 是否成功启动Client
    bool CreateClient = false;
    // 默认端口
    unsigned ClientDefaultPort = 80;

    // 服务器信息
    SOCKADDR_IN ClientAddrIn;
    // 设置发送报文最大长度
    static const unsigned MAX_MESSAGE_LENGTH = 65507;
public:
    // 响应客户端链接的socket
    SOCKET ClientSocket;
    //构造函数
    Client();
    // 析构函数
    ~Client();
    // 加载资源
    void LoadResource();
    // 使用给定服务器地址初始化Client
    void InitClient(char* hostname);
    // 是否成功构造了Client
    bool isCreateClient();
    // 发送数据
    void SendDataToServer(char*sendBuff,char *recvBuff,int *recvFromHttpServerSize);
};


#endif //LAB1_CLIENT_H
