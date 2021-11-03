//
// Created by Alienware on 2021/10/29.
//


#include "HttpProxyServer.h"

/**
 * �޲ι��캯��
 * ʹ��Ĭ�϶˿ڹ���Server
 */
HttpProxyServer::HttpProxyServer() : Server() {
    LoadResource();
    this->InitServer(this->HttpPort);

}


void HttpProxyServer::HttpProxyServerStart() {
    if (CreateServer == false) {
        cout << "����������ʧ�ܣ���ǰ�޷�����HTTP�������!" << endl;
    } else {
        cout << "��������������У���ǰ����˿�Ϊ" << HttpPort << endl;
        MultiThreading();
    }
}


/**
 * falseΪ������
 * trueΪ����
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
            cout << "HTTP�ͻ�����δ����!" << endl;
            continue;
        }

        char * ProhibitIp = " ";
//        char * ProhibitIp = "127.0.0.1";


        bool userFilters = UserFilters(ClientAddr,ProhibitIp);
        if(userFilters)
        {
            cout<<"��ǰ�û���ֹ�����ⲿ��վ!"<<endl;
        }
        else {


            cout << "HTTP�ͻ���������!" << endl;
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
    cout << "��ǰ�߳�ID=" << GetCurrentThreadId() << endl;
    if ((recvSize = recv(*(SOCKET *) PM, buffer, MAX_MESSAGE_LENGTH, 0)) != -1) {
        cout << "Client Said:" << endl;
        cout << buffer << endl;
        cout << "From client to proxy.�������!" << endl;
        char *RecvBuffer = new char[recvSize];
        char *FileName = new char[MAX_URL_LENGTH + 13];
        char *HostName = new char[MAX_URL_LENGTH];
        ZeroMemory(RecvBuffer, recvSize);
        ZeroMemory(FileName, MAX_URL_LENGTH);
        memcpy(RecvBuffer, buffer, recvSize);

//        char *ProhibitAccessToWebsites = "http://www.gov.cn/";
        char *ProhibitAccessToWebsites = " ";
        if (ParseHttpHead(RecvBuffer, httpHeader)) {// �ͻ���

//            char * PhishingSite = "http://jwts.hit.edu.cn";
            //�Ƿ����
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
//            cout << "���Ķν���ʧ��!" << endl;
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
 * trueΪ���Է���
 * false Ϊ���ɷ���
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
    cout<<"��ǰ��ҳ��ֹ����!"<<endl;
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
 * �����Ƿ��ļ��Ѹ���
 * trueΪ�Ѹ���
 * falseΪδ����
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
 * ����Ƿ����filename�Ļ����ļ�
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
        cout << "From proxy to httpServer.���ʹ���:" << WSAGetLastError() << endl;
    } else {
        char *recvBuff = new char[MAX_MESSAGE_LENGTH];
        while (1) {
            RecvError= recv(ClientSocket, recvBuff, MAX_MESSAGE_LENGTH, 0);
            if (RecvError == SOCKET_ERROR) {
                cout << "From httpServer to proxy.���մ���:" << WSAGetLastError() << endl;
            } else if (RecvError == 0) {
                cout << "From httpServer to proxy.�������!" << endl;
                break;
            }

            cout << "From httpServer to proxy.���ڽ���!" << endl;

            bool isCache = IsCache(FileName);
            char *lastModified = new char[100];
            // ����Ƿ���ڶ�Ӧ��cache����
            //            ����Ƿ���ڸ��ļ���cache
//            �����ڣ�����Last-Modified����û�У���ֱ�ӷ��ͣ������ڣ�����Ҫ��һ����֤
//            �������ڣ���д��
            if (isCache == true) {
//                ���lastmodified
//                ����lastmodified��Ϊtrue
//                ��û��lastmodified��Ϊfalse

                char *RecvBuffer = new char[RecvError];
                memcpy(RecvBuffer, recvBuff, RecvError);
                bool ParseLastModified = ParseResponseMessage(RecvBuffer, lastModified);
                // ����lastmodified ���һ�����
                if (ParseLastModified == true) {
                    // ��httpserver��ȡrecvbuff
                    bool isModified = LastModified(FileName,lastModified);
//                    ������޸���
                    if(isModified)
                    {
                        cout << "д��cache����clent����cache����!" << endl;
                        int sendSize = send(socket, recvBuff, RecvError, 0);
                        if(sendSize==SOCKET_ERROR)
                        {
                            cout<<"From proxy to client.���ʹ���:"<<WSAGetLastError()<<endl;
                        }
                        else{
                            cout<<"From proxy to client.���ͳɹ�!"<<endl;
                        }
                        bool isModified = LastModified(FileName, lastModified);
                        if (isModified == false) {
                            *lastModified = '\0';
                        }
                        // д�벢����
                        WriteToFile(FileName, lastModified, recvBuff);
                    }
//                    ���û�б��޸�
                    else
                    {
                        cout<<"check Last-Modified! Result: not modified!"<<endl;
                        cout << "��clent����cache����!" << endl;
                        cacheFromFile(FileName,RecvBuffer);
//                    memcpy(recvBuff,RecvBuffer,RecvError);
//                    client.SendDataToServer(buffer, recvBuff, recvFromHttpServerSize);
                        int sendSize = send(socket, recvBuff, RecvError, 0);
                        if(sendSize==SOCKET_ERROR)
                        {
                            cout<<"From proxy to client.���ʹ���:"<<WSAGetLastError()<<endl;
                        }
                        else{
                            cout<<"From proxy to client.���ͳɹ�!"<<endl;
                        }
                    }

                } else {
                    cout << "��clent����cache����!" << endl;
                    cacheFromFile(FileName,RecvBuffer);
//                    memcpy(recvBuff,RecvBuffer,RecvError);
//                    client.SendDataToServer(buffer, recvBuff, recvFromHttpServerSize);
                    int sendSize = send(socket, recvBuff, RecvError, 0);
                    if(sendSize==SOCKET_ERROR)
                    {
                        cout<<"From proxy to client.���ʹ���:"<<WSAGetLastError()<<endl;
                    }
                    else{
                        cout<<"From proxy to client.���ͳɹ�!"<<endl;
                    }
                }

            }
            else {
                cout << "д��cache����clent����cache����!" << endl;
                int sendSize = send(socket, recvBuff, RecvError, 0);
                if(sendSize==SOCKET_ERROR)
                {
                    cout<<"From proxy to client.���ʹ���:"<<WSAGetLastError()<<endl;
                }
                else{
                    cout<<"From proxy to client.���ͳɹ�!"<<endl;
                }
//                bool isModified = LastModified(FileName, lastModified);
                char *RecvBuffer = new char[RecvError];
                memcpy(RecvBuffer, recvBuff, RecvError);
                bool isModified = ParseResponseMessage(RecvBuffer,lastModified);
                if (isModified == false) {
                    *lastModified = '\0';
                }
                // д�벢����
                WriteToFile(FileName, lastModified, recvBuff);
            }

//            int sendSize = send(socket, recvBuff, RecvError, 0);
//            if(sendSize==SOCKET_ERROR)
//            {
//                cout<<"From proxy to client.���ʹ���:"<<WSAGetLastError()<<endl;
//            }
//            else{
//                cout<<"From proxy to client.���ͳɹ�!"<<endl;
//            }

//            cout << "���յ��ı���Ϊ" << endl;
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
 * ���lastmodified
 * ��û���򷵻�false
 * ���򷵻�true
 * @param buffer
 * @param lastModified
 * @return
 */
bool HttpProxyServer::ParseResponseMessage(char *buffer, char *lastModified) {
    char *line;
    const char *delim = "\r\n";
    // ��һ�ε��ã���һ������Ϊ���ֽ���ַ���
    // ����һ��
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
 * ����HTTP���ĵ�ͷ��
 * @param buffer �������ı��Ķ�
 * @param httpHeader http����ͷ��
 */
bool HttpProxyServer::ParseHttpHead(char *buffer, HttpHeader *httpHeader) {
    char *line;
    const char *delim = "\r\n";
    // ��һ�ε��ã���һ������Ϊ���ֽ���ַ���
    // ����һ��
    line = strtok(buffer, delim);
    if (line == NULL) {
//        cout<<"���Ķ�Ϊ��"<<endl;
        return false;
    }
    // line����:
    // POST /chapter17/user.html HTTP/1.1
    // GET /chapter17/user.html HTTP/1.1
    if (line[0] == 'G') {
        //GET ��ʽ
        memcpy(httpHeader->RequestMethod, "GET", 3);
        //'Get' �� 'HTTP/1.1' ��ռ 3 �� 8 �����ټ������ո�һ��13��
        memcpy(httpHeader->RequestUrl, &line[4], strlen(line) - 13);
    } else if (line[0] == 'P') {
        //POST ��ʽ
        memcpy(httpHeader->RequestMethod, "POST", 4);
        //'Post' �� 'HTTP/1.1' ��ռ 4 �� 8 �����ټ������ո�һ��14��
        memcpy(httpHeader->RequestUrl, &line[5], strlen(line) - 14);
    } else {
        cout << "��ǰ����ͷΪ:" << line << "������ʵ���趨!" << endl;
        return false;
    }
    cout << "��ǰ���ʵ�url��:" << httpHeader->RequestUrl << endl;
    // ��ʱ�ڶ��ε���
    // �ڶ��ε���ʱ��Ҫ��buff����ΪNULL
    line = strtok(NULL, delim);
    while (line) {
        switch (line[0]) {
            //Host
            case 'H':
                // �ײ���ʾ��
                // Host: localhost:8088
                // host��ð�źͿո�ռ�����ֽ�
                // ��˴ӵ��߸��ֽڿ�ʼΪhost
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
                // �ײ���ʾ��
                // Cookie�� JSESSIONID=24DF2688E37EE4F66D9669D2542AC17B
                // Cookie��ð�źͿո�ռ8���ֽ�
                // ��˴ӵ�9���ֽڿ�ʼΪcookie
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
 * ����url�����ļ�����
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
    // �洢�汾
    WORD wVersionRequested;
    // �洢��WSAStartup�������ú�
    // ���ص�Windows sockets����
    WSADATA wsaData;
    // ʹ��WinSock 2.2�汾
    wVersionRequested = MAKEWORD(2, 2);
    int error;
    // ��ʼ��DLL
    error = WSAStartup(wVersionRequested, &wsaData);
    if (error != 0) {
        cout << "Proxy����WinSockʧ��!" << endl;
    } else {
        cout << "Proxy����WinSock�ɹ�!" << endl;
    }
}