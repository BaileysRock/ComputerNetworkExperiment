//
// Created by Alienware on 2021/10/29.
//


#include "HttpProxyServer.h"

/**
 * 无参构造函数
 * 使用默认端口构造Server
 */
HttpProxyServer::HttpProxyServer() : Server() {
    LoadResource();
    this->InitServer(this->HttpPort);

}


void HttpProxyServer::HttpProxyServerStart() {
    if (CreateServer == false) {
        cout << "服务器启动失败，当前无法启动HTTP代理服务!" << endl;
    } else {
        cout << "代理服务正在运行，当前代理端口为" << HttpPort << endl;
        MultiThreading();
    }
}


/**
 * false为不过滤
 * true为过滤
 * @param ClientAddr
 * @param ip
 * @return
 */
bool HttpProxyServer::UserFilters(SOCKADDR_IN ClientAddr,char * ip) {

    char *UserIP = inet_ntoa(ClientAddr.sin_addr);
    for(int i=0;i<strlen(ip);i++)
    {
        if(UserIP[i]!=ip[i])
        {
            return false;
        }
    }
    return true;

}

void HttpProxyServer::MultiThreading() {
    SOCKADDR_IN ClientAddr;
    int len = sizeof(SOCKADDR);
    for (int i = 0; i < MAX_THREAD_HANDLE; i++) {
        SOCKET AcceptSocket = INVALID_SOCKET;
        AcceptSocket = accept(ServerSocket, (SOCKADDR *) &ClientAddr, &len);
        if (AcceptSocket == INVALID_SOCKET) {
            cout << "HTTP客户端尚未连接!" << endl;
            continue;
        }

        char * ProhibitIp = " ";
//        char * ProhibitIp = "127.0.0.1";


        bool userFilters = UserFilters(ClientAddr,ProhibitIp);
        if(userFilters)
        {
            cout<<"当前用户禁止访问外部网站!"<<endl;
        }
        else {


            cout << "HTTP客户端已连接!" << endl;
            HANDLE hThread = (HANDLE) _beginthreadex(NULL, 0, &ChildThread, &AcceptSocket, 0, NULL);
            CloseHandle(hThread);
            sleep(1);
        }
    }

}

unsigned int __stdcall HttpProxyServer::ChildThread(PVOID PM) {
    HttpHeader *httpHeader = new HttpHeader;
    char buffer[MAX_MESSAGE_LENGTH] = {0};
    int recvSize;
    cout << "当前线程ID=" << GetCurrentThreadId() << endl;
    if ((recvSize = recv(*(SOCKET *) PM, buffer, MAX_MESSAGE_LENGTH, 0)) != -1) {
        cout << "Client Said:" << endl;
        cout << buffer << endl;
        cout << "From client to proxy.发送完成!" << endl;
        char *RecvBuffer = new char[recvSize];
        char *FileName = new char[MAX_URL_LENGTH + 13];
        char *HostName = new char[MAX_URL_LENGTH];
        ZeroMemory(RecvBuffer, recvSize);
        ZeroMemory(FileName, MAX_URL_LENGTH);
        memcpy(RecvBuffer, buffer, recvSize);

//        char *ProhibitAccessToWebsites = "http://www.gov.cn/";
        char *ProhibitAccessToWebsites = " ";
        if (ParseHttpHead(RecvBuffer, httpHeader)) {// 客户端

//            char * PhishingSite = "http://jwts.hit.edu.cn";
            //是否钓鱼
//            GuidePhishingSite(PhishingSite,httpHeader->Host,httpHeader->RequestUrl);
            if(AccessWebsite(httpHeader->RequestUrl, ProhibitAccessToWebsites)) {
                Client client;
                client.InitClient(httpHeader->Host);
                char *copyText = new char[MAX_MESSAGE_LENGTH];
                memcpy(copyText,buffer,recvSize);
                char *changedText = new char[MAX_MESSAGE_LENGTH];
                ChangeText(copyText,changedText,httpHeader->RequestUrl,httpHeader->Host);

                int *recvFromHttpServerSize = new int;
                *recvFromHttpServerSize = 0;
                GenerateFileName(FileName, httpHeader->RequestUrl);
                // test1
                SendToClient(buffer, *(SOCKET *) PM, client.ClientSocket, FileName);
            }
        } else {
//            cout << "报文段解析失败!" << endl;
        }
    }
//    closesocket((*(SOCKET *) PM));

}


void HttpProxyServer::ChangeText(char *Text, char * changedText,char *url, char *host) {

    int iterNum;
    char *iterchangedText = changedText;

    for(int i =0;Text[i]!=' ';i++)
    {
        *iterchangedText=Text[i];
        iterchangedText++;
    }
    *iterchangedText = ' ';
    iterchangedText++;

    for(iterNum =0;iterNum<strlen(url);iterNum++)
    {
        *iterchangedText=url[iterNum];
        iterchangedText++;
    }

    *iterchangedText = ' ';
    iterchangedText++;
    *iterchangedText = 'H';
    iterchangedText++;
    *iterchangedText = 'T';
    iterchangedText++;
    *iterchangedText = 'T';
    iterchangedText++;
    *iterchangedText = 'P';
    iterchangedText++;
    *iterchangedText = '/';
    iterchangedText++;
    *iterchangedText = '1';
    iterchangedText++;
    *iterchangedText = '.';
    iterchangedText++;
    *iterchangedText = '1';
    iterchangedText++;
    *iterchangedText = '\r';
    iterchangedText++;
    *iterchangedText = '\n';
    iterchangedText++;
    *iterchangedText = 'H';
    iterchangedText++;
    *iterchangedText = 'o';
    iterchangedText++;
    *iterchangedText = 's';
    iterchangedText++;
    *iterchangedText = 't';
    iterchangedText++;
    *iterchangedText = ':';
    iterchangedText++;
    *iterchangedText = ' ';
    iterchangedText++;
    for(iterNum =0;iterNum<strlen(host);iterNum++)
    {
        *iterchangedText=host[iterNum];
        iterchangedText++;
    }
    *iterchangedText = '\r';
    iterchangedText++;
    *iterchangedText = '\n';
    iterchangedText++;
    int count =0;
    for(int i=0;i<strlen(Text);i++)
    {
        if(count!=2)
        {
            if(Text[i]=='\n')
                count++;
        }
        else
        {
            *iterchangedText = Text[i];
            iterchangedText++;
        }
    }



}


/**
 * true为可以访问
 * false 为不可访问
 * @param website
 * @param ProhibitAccessToWebsites
 * @return
 */
bool HttpProxyServer:: AccessWebsite(char *website,char*ProhibitAccessToWebsites)
{
    for(int i =0;i<strlen(ProhibitAccessToWebsites);i++)
    {
        if(website[i]!=ProhibitAccessToWebsites[i])
        {
            return true;
        }
    }
    cout<<"当前网页禁止访问!"<<endl;
    return false;

}

void HttpProxyServer::cacheFromFile(char *filename, char *cache) {
    ifstream inFile(filename);
    if (!filename) {
        cout << "Read Cache Failed!" << endl;
    } else {
        inFile.getline(cache, MAX_MESSAGE_LENGTH);
    }
}

void HttpProxyServer::WriteToFile(char *filename, char *lastModified, char *cachebuff) {
    ofstream outfile;
    outfile.open(filename, ios::out | ios::app);
    if (*lastModified == '\0') {
        outfile << cachebuff << endl;
    } else {
        outfile << lastModified << endl;
        outfile << cachebuff << endl;
    }
    outfile.close();
}

/**
 * 返回是否文件已更改
 * true为已更改
 * false为未更改
 * @param filename
 * @param lastmodifiedFromHead
 * @return
 */
bool HttpProxyServer::LastModified(char *filename, char *lastmodifiedFromHead) {
    ifstream inFile(filename);
    if (!filename) {
//        cout<<"No File!"<<endl;
        return false;
    }
    if (*lastmodifiedFromHead == '\0')
        return false;
    else if(*lastmodifiedFromHead =='1')
        return false;
    char *lastmodified = new char[100];
    inFile.getline(lastmodified, 100);
    for (int i = 0; lastmodified[i] != '\0'; i++) {
        if (lastmodified[i] != lastmodifiedFromHead[i]) {
            delete[]lastmodified;
            return true;
        }
    }
    return false;
}

/**
 * 检查是否存在filename的缓存文件
 * @param filename
 * @return
 */
bool HttpProxyServer::IsCache(char *filename) {
    __int64 Handle;
    char *path = "../CacheList";
    vector<string> files;
    struct __finddata64_t FileInfo;
    string p;
    string suffix2 = "\\*.*";
    if ((Handle = _findfirst64(p.assign(path).append(suffix2).c_str(), &FileInfo)) != -1) {
        do {
            files.push_back(p.assign(path).append("/").append(FileInfo.name));
        } while (_findnext64(Handle, &FileInfo) == 0);
        _findclose(Handle);
    }
    int size = files.size();
    int flag = 0;
    for (int i = 0; i < size; i++) {
        const char *strCompare = files[i].c_str();
        flag = 0;
        if(strlen(strCompare )!=strlen(filename))
            continue;
        for (int j = 0; strCompare[j] != '\0' && filename[j] != '\0'; j++) {
            if (strCompare[j] != filename[j]) {
                flag = 1;
                break;
            }
        }
        if (flag == 0) {
            return true;
        }
    }
    return false;
}





void HttpProxyServer::SendToClient(char *sendBuff, SOCKET socket,SOCKET ClientSocket,char*FileName) {
    int SendError = send(ClientSocket, sendBuff, strlen(sendBuff) + 1, 0);
    int RecvError = 0;
    if (SendError == SOCKET_ERROR) {
        cout << "From proxy to httpServer.发送错误:" << WSAGetLastError() << endl;
    } else {
        char *recvBuff = new char[MAX_MESSAGE_LENGTH];
        while (1) {
            RecvError= recv(ClientSocket, recvBuff, MAX_MESSAGE_LENGTH, 0);
            if (RecvError == SOCKET_ERROR) {
                cout << "From httpServer to proxy.接收错误:" << WSAGetLastError() << endl;
            } else if (RecvError == 0) {
                cout << "From httpServer to proxy.接收完成!" << endl;
                break;
            }

            cout << "From httpServer to proxy.正在接收!" << endl;

            bool isCache = IsCache(FileName);
            char *lastModified = new char[100];
            // 检查是否存在对应的cache缓存
            //            检查是否存在该文件的cache
//            若存在，则检查Last-Modified，若没有，则直接发送，若存在，则需要进一步验证
//            若不存在，则写入
            if (isCache == true) {
//                检查lastmodified
//                若有lastmodified则为true
//                若没有lastmodified则为false

                char *RecvBuffer = new char[RecvError];
                memcpy(RecvBuffer, recvBuff, RecvError);
                bool ParseLastModified = ParseResponseMessage(RecvBuffer, lastModified);
                // 若有lastmodified 则进一步检查
                if (ParseLastModified == true) {
                    // 从httpserver获取recvbuff
                    bool isModified = LastModified(FileName,lastModified);
//                    如果被修改了
                    if(isModified)
                    {
                        cout << "写入cache并向clent发送cache缓存!" << endl;
                        int sendSize = send(socket, recvBuff, RecvError, 0);
                        if(sendSize==SOCKET_ERROR)
                        {
                            cout<<"From proxy to client.发送错误:"<<WSAGetLastError()<<endl;
                        }
                        else{
                            cout<<"From proxy to client.发送成功!"<<endl;
                        }
                        bool isModified = LastModified(FileName, lastModified);
                        if (isModified == false) {
                            *lastModified = '\0';
                        }
                        // 写入并发送
                        WriteToFile(FileName, lastModified, recvBuff);
                    }
//                    如果没有被修改
                    else
                    {
                        cout<<"check Last-Modified! Result: not modified!"<<endl;
                        cout << "向clent发送cache缓存!" << endl;
                        cacheFromFile(FileName,RecvBuffer);
//                    memcpy(recvBuff,RecvBuffer,RecvError);
//                    client.SendDataToServer(buffer, recvBuff, recvFromHttpServerSize);
                        int sendSize = send(socket, recvBuff, RecvError, 0);
                        if(sendSize==SOCKET_ERROR)
                        {
                            cout<<"From proxy to client.发送错误:"<<WSAGetLastError()<<endl;
                        }
                        else{
                            cout<<"From proxy to client.发送成功!"<<endl;
                        }
                    }

                } else {
                    cout << "向clent发送cache缓存!" << endl;
                    cacheFromFile(FileName,RecvBuffer);
//                    memcpy(recvBuff,RecvBuffer,RecvError);
//                    client.SendDataToServer(buffer, recvBuff, recvFromHttpServerSize);
                    int sendSize = send(socket, recvBuff, RecvError, 0);
                    if(sendSize==SOCKET_ERROR)
                    {
                        cout<<"From proxy to client.发送错误:"<<WSAGetLastError()<<endl;
                    }
                    else{
                        cout<<"From proxy to client.发送成功!"<<endl;
                    }
                }

            }
            else {
                cout << "写入cache并向clent发送cache缓存!" << endl;
                int sendSize = send(socket, recvBuff, RecvError, 0);
                if(sendSize==SOCKET_ERROR)
                {
                    cout<<"From proxy to client.发送错误:"<<WSAGetLastError()<<endl;
                }
                else{
                    cout<<"From proxy to client.发送成功!"<<endl;
                }
//                bool isModified = LastModified(FileName, lastModified);
                char *RecvBuffer = new char[RecvError];
                memcpy(RecvBuffer, recvBuff, RecvError);
                bool isModified = ParseResponseMessage(RecvBuffer,lastModified);
                if (isModified == false) {
                    *lastModified = '\0';
                }
                // 写入并发送
                WriteToFile(FileName, lastModified, recvBuff);
            }

//            int sendSize = send(socket, recvBuff, RecvError, 0);
//            if(sendSize==SOCKET_ERROR)
//            {
//                cout<<"From proxy to client.发送错误:"<<WSAGetLastError()<<endl;
//            }
//            else{
//                cout<<"From proxy to client.发送成功!"<<endl;
//            }

//            cout << "接收到的报文为" << endl;
//            cout << recvBuff << endl;
        }
    }


}

void HttpProxyServer::GuidePhishingSite(char *PhishingSite, char *HttpHeaderHost,char *HttpHeaderUrl) {

    for (int i =0;i<=strlen(PhishingSite);i++)
    {
        HttpHeaderHost[i]=PhishingSite[i+7];
    }
    for (int i =0;i<=strlen(PhishingSite);i++)
    {
        HttpHeaderUrl[i]=PhishingSite[i];
    }



}





/**
 * 检测lastmodified
 * 若没有则返回false
 * 否则返回true
 * @param buffer
 * @param lastModified
 * @return
 */
bool HttpProxyServer::ParseResponseMessage(char *buffer, char *lastModified) {
    char *line;
    const char *delim = "\r\n";
    // 第一次调用，第一个参数为被分解的字符串
    // 即第一行
    line = strtok(buffer, delim);
    if(line!=NULL)
    {
        if (strlen(line)>12)
        {
            if(line[9] == '3'&& line[10]=='0'&&line[11]=='4')
            {
                *lastModified = '1';
                return true;
            }

        }
    }

    while (line) {
        if (line[0] == 'L' && line[1] == 'a' && line[2] == 's' &&
            line[3] == 't' && line[4] == '-' && line[5] == 'M' &&
            line[6] == 'o' && line[7] == 'd' && line[8] == 'i' &&
            line[9] == 'f' && line[10] == 'i' && line[11] == 'e' &&
            line[12] == 'd') {
            memcpy(lastModified, line, strlen(line));
            return true;
        }
        line = strtok(NULL, delim);
    }
    return false;
}

/**
 * 解析HTTP报文的头部
 * @param buffer 待解析的报文段
 * @param httpHeader http报文头部
 */
bool HttpProxyServer::ParseHttpHead(char *buffer, HttpHeader *httpHeader) {
    char *line;
    const char *delim = "\r\n";
    // 第一次调用，第一个参数为被分解的字符串
    // 即第一行
    line = strtok(buffer, delim);
    if (line == NULL) {
//        cout<<"报文段为空"<<endl;
        return false;
    }
    // line举例:
    // POST /chapter17/user.html HTTP/1.1
    // GET /chapter17/user.html HTTP/1.1
    if (line[0] == 'G') {
        //GET 方式
        memcpy(httpHeader->RequestMethod, "GET", 3);
        //'Get' 和 'HTTP/1.1' 各占 3 和 8 个，再加上俩空格，一共13个
        memcpy(httpHeader->RequestUrl, &line[4], strlen(line) - 13);
    } else if (line[0] == 'P') {
        //POST 方式
        memcpy(httpHeader->RequestMethod, "POST", 4);
        //'Post' 和 'HTTP/1.1' 各占 4 和 8 个，再加上俩空格，一共14个
        memcpy(httpHeader->RequestUrl, &line[5], strlen(line) - 14);
    } else {
        cout << "当前报文头为:" << line << "不符合实验设定!" << endl;
        return false;
    }
    cout << "当前访问的url是:" << httpHeader->RequestUrl << endl;
    // 此时第二次调用
    // 第二次调用时需要将buff设置为NULL
    line = strtok(NULL, delim);
    while (line) {
        switch (line[0]) {
            //Host
            case 'H':
                // 首部行示例
                // Host: localhost:8088
                // host和冒号和空格共占六个字节
                // 因此从第七个字节开始为host
                if (strlen(line) > 6) {
                    char header[6];
                    ZeroMemory(header, sizeof(header));
                    memcpy(header, line, 4);
                    if (!strcmp(header, "Host")) {
                        memcpy(httpHeader->Host, &line[6], strlen(line) - 6);
                    }
                }
                break;
            case 'C'://Cookie
                // 首部行示例
                // Cookie： JSESSIONID=24DF2688E37EE4F66D9669D2542AC17B
                // Cookie和冒号和空格共占8个字节
                // 因此从底9个字节开始为cookie
                if (strlen(line) > 8) {
                    char header[8];
                    ZeroMemory(header, sizeof(header));
                    memcpy(header, line, 6);
                    if (!strcmp(header, "Cookie")) {
                        memcpy(httpHeader->Cookie, &line[8], strlen(line) - 8);
                    }
                }
                break;
            default:
                break;
        }
        line = strtok(NULL, delim);
    }
    return true;
}

/**
 * 根据url生成文件名称
 * @param filename
 * @param url
 */
void HttpProxyServer::GenerateFileName(char *filename, char *url) {
    char *filepath = "../CacheList/";
    for (int i = 0; i < strlen(filepath); i++)
        filename[i] = filepath[i];
    char *filenameIter = filename;
    while (*filenameIter != '\0')
        filenameIter++;
    int length = 0;
    while (url[length] != '\0')
        length++;
    int iter;
    for (iter = 0; iter < length; iter++)
        filenameIter[iter] = url[iter];
    filenameIter[iter] = '\0';
    while (*filenameIter != '\0') {
        if (*filenameIter == '/') {
            *filenameIter = '_';
        } else if (*filenameIter == '.') {
            *filenameIter = '_';
        } else if (*filenameIter == ':') {
            *filenameIter = '_';
        }
        filenameIter++;
    }
}


void HttpProxyServer::LoadResource() {
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
        cout << "Proxy加载WinSock失败!" << endl;
    } else {
        cout << "Proxy加载WinSock成功!" << endl;
    }
}