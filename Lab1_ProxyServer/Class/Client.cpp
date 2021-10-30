//
// Created by Alienware on 2021/10/25.
//

#include "Client.h"

Client::Client() {


}

/**
 * ��ʼ��Client�ͻ���
 * @param host ��������ַ
 */
void Client::InitClient(char *host) {
//    cout << "��ʼ���ͻ�����..." << endl;
    // �����׽���
    ClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ClientSocket == INVALID_SOCKET) {
        cout << "�����׽���ʧ��!" << "�������Ϊ" << WSAGetLastError() << endl;
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
            cout << "����Զ�̷�����ʧ��!" << endl;
            CreateClient = false;
        } else {
            CreateClient = true;
        }
    }
    if (CreateClient) {
//        cout << "Client�����ɹ�!" << endl;
    } else {
        cout << "Client����ʧ��!" << endl;
    }
}

/**
 * ���������������
 * @param sendBuff
 * @param recvBuff
 * @param recvFromHttpServerSize
 */
void Client::SendDataToServer(char *sendBuff, char *recvBuff,int *recvFromHttpServerSize) {
    int SendError = send(ClientSocket, sendBuff, strlen(sendBuff) + 1, 0);
    int RecvError = 0;
    if (SendError == SOCKET_ERROR) {
        cout << "From proxy to httpServer.���ʹ���:" << WSAGetLastError() << endl;
    } else {
        while (1) {
            RecvError= recv(ClientSocket, recvBuff, MAX_MESSAGE_LENGTH, 0);
            if (RecvError == SOCKET_ERROR) {
                cout << "From httpServer to proxy.���մ���:" << WSAGetLastError() << endl;
            } else if (RecvError == 0) {
                cout << "From httpServer to proxy.�������!" << endl;
                break;
            }
            *recvFromHttpServerSize = RecvError;
            cout << "From httpServer to proxy.���ڽ���!" << endl;

//            cout << "���յ��ı���Ϊ" << endl;
//            cout << recvBuff << endl;
        }
    }

}


/**
 * ����dll��Դ
 */
void Client::LoadResource() {
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
        cout << "�ͻ��˼���WinSockʧ��!" << endl;
        CreateClient = false;
    }
}


/**
 * ��������
 */
Client::~Client() {
    closesocket(ClientSocket);
    WSACleanup();
    cout << "Client���ͷ�";
}


bool Client::isCreateClient() {
    return CreateClient;
}