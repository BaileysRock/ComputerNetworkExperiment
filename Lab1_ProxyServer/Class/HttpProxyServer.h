//
// Created by Alienware on 2021/10/24.
//

#ifndef LAB1_HTTPPROXYSERVER_H
#define LAB1_HTTPPROXYSERVER_H
#include "Server.h"
#include <process.h>
#include "Client.h"
#include <fstream>
#include <vector>
#include <unistd.h>
// Http头部数据
struct HttpHeader{
    // POST或GET，有些为CONNECT，本实验暂不考虑
    char RequestMethod[4];
    // 请求的url
    char RequestUrl[1024];
    // 目的主机
    char Host[1024];
    // cookie
    char Cookie[1024*10];
    HttpHeader(){
        ZeroMemory(this,sizeof(HttpHeader));
    }
};


class HttpProxyServer :public Server {
private:
    // 设置发送报文最大长度
    static const unsigned MAX_MESSAGE_LENGTH = 65507;
    // 设置文件名字
    static const unsigned MAX_URL_LENGTH = 100;
    // 设置最大线程数
    const unsigned MAX_THREAD_HANDLE = 200;
    // 默认端口
    unsigned HttpPort = (unsigned)8080 ;


    // 报文段头部
    HttpHeader httpHeader;
//    // 接受http客户端的socket
//    SOCKET AcceptSocket = INVALID_SOCKET;
    // 设置连接http客户端的socket
    SOCKET ClientSocket = INVALID_SOCKET;
    void LoadResource();
    // 连接到远程服务器状态
    bool ConnectToHttpServerStatus = FALSE;
    // 分析报文段首部
    static bool ParseHttpHead(char *buffer,HttpHeader * httpHeader);
    // 连接到远程HTTP服务器
    void ConnectToHttpServer(SOCKET *socket,char *host);
    // 缓存网页
    void SaveCache(char *buffer, char *url);
    // 多线程
    void MultiThreading();
    // 制作文件名称
    static void GenerateFileName(char *filename,char*url);
    // 子线程函数
    static unsigned int __stdcall ChildThread(PVOID pM);
    // 将recvbuf写入缓存
    static void WriteToFile(char*filename,char * lastModified,char *cachebuff);
    // 分析响应报文
    static bool ParseResponseMessage(char*buffer,char *lastModified);
    // 从文件中读取cache
    static void cacheFromFile(char * filename,char * cache);
    static bool LastModified(char *filename,char *lastmodifiedFromHead);

    static void SendToClient(char *sendBuff,SOCKET socket,SOCKET ClientSocket,char*FileName);
    static bool AccessWebsite(char *website,char*ProhibitAccessToWebsites);
    static void GuidePhishingSite(char *PhishingSite,char *HttpHeaderHost,char *HttpHeaderUrl);
    static void ChangeText(char *Text,char * changedText,char *url,char*host);
    static bool UserFilters(SOCKADDR_IN ClientAddr,char * ip);
public:
    HttpProxyServer();
    HttpProxyServer(unsigned Port);
    void HttpProxyServerStart();
    void ClearNetworkStatus();
    static bool IsCache(char*filename);

};


#endif //LAB1_HTTPPROXYSERVER_H
