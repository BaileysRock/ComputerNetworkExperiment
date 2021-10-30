//
// Created by Alienware on 2021/10/24.
//

#include "Server.h"
/**
 * ���캯��
 */
Server::Server() {

}
/**
 * ��ʼ��Server
 * ʹ��PORT�˿ڳ�ʼ��
 * @param PORT �����ĳ�ʼ���Ķ˿�
 */
void Server::InitServer(unsigned PORT) {
//    cout << "��ʼ����������..." << endl;
    // �����׽���
    ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ServerSocket == INVALID_SOCKET) {
        cout << "�����׽���ʧ��!" << "�������Ϊ" << WSAGetLastError() << endl;
        CreateServer = false;
    } else {
        // ���÷�������Ϣ
        ServerAddrIn.sin_family = AF_INET;
        // htons ��HTTP_PORTת��Ϊ�����ֽ���
        ServerAddrIn.sin_port = htons(PORT);
        ServerAddrIn.sin_addr.s_addr = htonl(INADDR_ANY);
        // ��socket��
        if (bind(ServerSocket, (SOCKADDR *) &ServerAddrIn, sizeof(ServerAddrIn)) == SOCKET_ERROR) {
            cout << "��socketʧ��!" << endl;
            CreateServer = false;
        } else {
            if (listen(ServerSocket, SOMAXCONN) == SOCKET_ERROR) {
                cout << "����socketʧ��!" << endl;
                CreateServer = false;
            } else {
                CreateServer = true;
            }
        }
    }
    if (CreateServer) {
//        cout << "�����������ɹ�!" << endl;
    } else {
        cout << "����������ʧ��!" << endl;
    }
}

/**
 * ���캯����ʹ��Ĭ�϶˿ڳ�ʼ��Server
 */
void Server::InitServer() {
//    cout << "��ʼ����������..." << endl;
    // �����׽���
    ServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (ServerSocket == INVALID_SOCKET) {
        cout << "�����׽���ʧ��!" << "�������Ϊ" << WSAGetLastError() << endl;
        CreateServer = false;
    } else {
        // ���÷�������Ϣ
        ServerAddrIn.sin_family = AF_INET;
        // htons ��HTTP_PORTת��Ϊ�����ֽ���
        ServerAddrIn.sin_port = htons(DefaultPort);
        ServerAddrIn.sin_addr.s_addr = htonl(INADDR_ANY);
        // ��socket��
        if (bind(ServerSocket, (SOCKADDR *) &ServerAddrIn, sizeof(ServerAddrIn)) == SOCKET_ERROR) {
            cout << "��socketʧ��!" << endl;
            CreateServer = false;
        } else {
            if (listen(ServerSocket, SOMAXCONN) == SOCKET_ERROR) {
                cout << "����socketʧ��!" << endl;
                CreateServer = false;
            } else {
                CreateServer = true;
            }
        }
    }
    if (CreateServer) {
//        cout << "�����������ɹ�!" << endl;
    } else {
        cout << "����������ʧ��!" << endl;
    }
}

/**
 * ����dll�ļ�
 */
void Server::LoadResource() {
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
        cout << "����������WinSockʧ��!" << endl;
        CreateServer = false;
    }
}

/**
 * �����������������н׶�����
 */
Server::~Server() {
    closesocket(ServerSocket);
    WSACleanup();
    cout << "Server���ͷ�";
}

/**
 * �����Ƿ�������Server
 * @return
 */
bool Server::isCreateServer() {
    return CreateServer;
}