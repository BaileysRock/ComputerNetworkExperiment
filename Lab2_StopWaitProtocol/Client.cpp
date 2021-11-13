#include<WINSOCK2.H>
#include<iostream>
#include <ctime>
#include <fstream>
#pragma comment(lib, "WS2_32.lib")
void testGBNSendFile(SOCKET SendSocket, sockaddr_in RecvAddr);
using namespace std;
int curSeq;//当前数据包的 seq
int curAck;//当前等待确认的 ack
int totalSeq;//收到的包的总数
int totalPacket;//需要发送的包总数
const int BUFFER_LENGTH = 1026;
void testGBNFile(SOCKET SendSocket, sockaddr_in RecvAddr);
const int SEND_WIND_SIZE = 1;//发送窗口大小为 5，GBN 中应满足 W + 1 <=N（W 为发送窗口大小，N 为序列号个数）
//本例取序列号 0...9 共 10 个
//如果将窗口大小设为 1，则为停-等协议
const int SEQ_SIZE = 2; //序列号的个数，从 0~9 共计 10 个
//由于发送数据第一个字节如果值为 0，则数据会发送失败
//因此接收端序列号为 1~10，与发送端一一对应
BOOL ack[SEQ_SIZE];//收到 ack 情况，对应 0~9 的 ack
//************************************
// Method: seqIsAvailable
// FullName: seqIsAvailable
// Access: public
// Returns: bool
// Qualifier: 当前序列号 curSeq 是否可用
//************************************
bool seqIsAvailable() {
    int step;
    step = curSeq - curAck;
    step = step >= 0 ? step : step + SEQ_SIZE;
    //序列号是否在当前发送窗口之内
    if (step >= SEND_WIND_SIZE) {
        return false;
    }
    if (ack[curSeq]) {
        return true;
    }
    return false;
}


int main() {
    WSADATA wsaData;//初始化
    SOCKET SendSocket;
    sockaddr_in RecvAddr;//服务器地址
    int Port = 4000;//服务器监听地址

    int BufLen = 1024;//缓冲区大小


    //初始化Socket
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    //创建Socket对象
    SendSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    //设置服务器地址
    RecvAddr.sin_family = AF_INET;
    RecvAddr.sin_port = htons(12340);
    RecvAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    //向服务器发送数据报
    printf("Sending a datagram to the receiver...\n");


//    testGBNFile(SendSocket, RecvAddr);
    testGBNSendFile(SendSocket, RecvAddr);





    //发送完成，关闭Socket
    printf("finished sending,close socket.\n");
    closesocket(SendSocket);
    printf("Exting.\n");
    WSACleanup();
    return 0;
}

//************************************
// Method: timeoutHandler
// FullName: timeoutHandler
// Access: public
// Returns: void
// Qualifier: 超时重传处理函数，滑动窗口内的数据帧都要重传
//************************************
void timeoutHandler() {
    printf("Timer out error.\n");
    int index;
    for (int i = 0; i < SEND_WIND_SIZE; ++i) {
        index = (i + curAck) % SEQ_SIZE;
        ack[index] = TRUE;
    }
    totalSeq -= SEND_WIND_SIZE;
    curSeq = curAck;
}

//************************************
// Method: ackHandler
// FullName: ackHandler
// Access: public
// Returns: void
// Qualifier: 收到 ack，累积确认，取数据帧的第一个字节
// 由于发送数据时，第一个字节（序列号）为 0（ASCII）时发送失败，因此加一了，此处需要减一还原
// Parameter: char c
//************************************
void ackHandler(char c) {
    unsigned char index = (unsigned char) c - 1; //序列号减一
    printf("Recv a ack of %d\n", (index+1)%10-1);
    if (curAck <= index) {
        for (int i = curAck; i <= index; ++i) {
            ack[i] = TRUE;
        }
        curAck = (index + 1) % SEQ_SIZE;
    } else {
//ack 超过了最大值，回到了 curAck 的左边
        for (int i = curAck; i < SEQ_SIZE; ++i) {
            ack[i] = TRUE;
        }
        for (int i = 0; i <= index; ++i) {
            ack[i] = TRUE;
        }
        curAck = index + 1;
    }
}



void testGBNFile(SOCKET SendSocket, sockaddr_in RecvAddr) {
    int BufLen = 1026;//缓冲区大小
    char *SendBuf = new char[BufLen];//发送数据的缓冲区
    char *RecvBuf = new char[BufLen];
    int RecvLen = sizeof(SOCKADDR);
    char data[1024 * 113];
    sendto(SendSocket, "-testgbn", strlen("-testgbn"), 0, (SOCKADDR *) &RecvAddr, sizeof(RecvAddr));
    int recvSeq;
    int waitSeq;
    int stage = 0;
    unsigned seq;
    int recvSize;

    int flag = 0;
    while (true) {
        //等待 server 回复设置 UDP 为阻塞模式

        // 从服务器端读东西 读到buffer中
        int recvSize = recvfrom(SendSocket, RecvBuf, BufLen, 0, (SOCKADDR *) &RecvAddr, &RecvLen);
        if (recvSize == -1)
            break;


        switch (stage) {
            case 0://等待握手阶段
                // 获取buffer中的状态码

                // 如果收到的服务器返回的状态码为105 说明服务器成功处理了请求 连接成功
                if (RecvBuf[0] == 105) {
                    printf("Ready for file transmission\n");
                    SendBuf[0] = 100;
                    SendBuf[1] = '\0';
                    sendto(SendSocket, SendBuf, 2, 0, (SOCKADDR *) &RecvAddr, sizeof(RecvAddr));
                    stage = 1;
                    recvSeq = 0; // 已经确认的序号 (最开始为0 啥也没确认)
                    waitSeq = 1; // 待用的序列号 这里最开始为1
                }
                break;
            case 1://等待接收数据阶段
                seq = RecvBuf[0];

                if (flag==1) {
                    flag ++;
                    cout << "seq of 2 is loss!" << endl;
                    continue;

                }


                // 数据包没丢失的话
                printf("recv a packet with a seq of %d\n", seq);
                flag ++;
                //如果是期待的包，正确接收，正常确认即可
                if (!(waitSeq - seq)) {
                    ++waitSeq;
                    if (waitSeq == 11) {
                        waitSeq = 1;
                    }
                    //输出数据
                    //printf("%s\n",&buffer[1]);
                    SendBuf[0] = seq;
                    recvSeq = seq;  // 已经确认的序列号修改为 seq
                    SendBuf[1] = '\0';
                } else { // 如果收到的不是当前想要的
                    //如果当前一个包都没有收到，则等待 Seq 为 1 的数据包，不是则不返回 ACK（因为并没有上一个正确的 ACK）
                    if (!recvSeq) {
                        continue;
                    }
                    SendBuf[0] = recvSeq;
                    SendBuf[1] = '\0';
                }
                sendto(SendSocket, SendBuf, 2, 0, (SOCKADDR *) &RecvAddr, sizeof(RecvAddr));
                printf("send a ack of %d\n", (unsigned char) SendBuf[0]); // 打印出序列号
                break;
        }
        Sleep(500);
    }
}


void testGBNSendFile(SOCKET SendSocket, sockaddr_in RecvAddr) {
    //进入 gbn 测试阶段
    //首先 server（server 处于 0 状态）向 client 发送 105 状态码（server进入 1 状态）
    //server 等待 client 回复 200 状态码，如果收到（server 进入 2 状态）， 则开始传输文件，否则延时等待直至超时\
            //在文件传输阶段，server 发送窗口大小设为
    //
    //
    //
    for (int i = 0; i < SEQ_SIZE; ++i) {
        ack[i] = TRUE;
    }
    sendto(SendSocket, "-testgbnSendFile", strlen("-testgbnSendFile"), 0, (SOCKADDR *) &RecvAddr, sizeof(RecvAddr));
    std::ifstream icin;
    std::string filepath = "../test.txt";
    icin.open(filepath.c_str());
    char data[1024 * 113];
    ZeroMemory(data, sizeof(data));
    icin.read(data, 1024 * 113);
    icin.close();
    int length = sizeof(SOCKADDR);
    char buffer[BUFFER_LENGTH]; //数据发送接收缓冲区
    ZeroMemory(buffer, sizeof(buffer));
    int recvSize;
    int waitCount = 0;
    printf("Begin to test GBN protocol,please don't abort the process\n");
    //加入了一个握手阶段
    //首先服务器向客户端发送一个 105 大小的状态码（我自己定义的）表示服务器准备好了，可以发送数据
    //客户端收到 105 之后回复一个 100 大小的状态码，表示客户端准备好了，可以接收数据了
    //服务器收到 100 状态码之后，就开始使用 GBN 发送数据了
    printf("Shake hands stage\n");
    int stage = 0;
    bool runFlag = true;
    while (runFlag) {
        switch (stage) {
            case 0://发送 105 阶段
                ZeroMemory(buffer, sizeof(buffer));
                recvSize = recvfrom(SendSocket, buffer, BUFFER_LENGTH, 0, ((SOCKADDR *) &RecvAddr), &length);
                if (recvSize < 0) {
                    ++waitCount;
                    if (waitCount > 20) {
                        runFlag = false;
                        printf("Timeout error\n");
                        break;
                    }
                    Sleep(500);
                    continue;
                } else {
                    if ((unsigned char) buffer[0] == 100) {
                        printf("Begin a file transfer\n");
                        printf("File size is %dB, each packet is 1024B and packet total num is %d\n",sizeof(data), totalPacket);
                        curSeq = 0;
                        curAck = 0;
                        totalSeq = 0;
                        waitCount = 0;
                        stage = 2;
                    }
                }
                break;
            case 2://数据传输阶段
                if (seqIsAvailable()) {//发送给客户端的序列号从 1 开始
                    buffer[0] = curSeq + 1;
                    ack[curSeq] = FALSE;
                    //数据发送的过程中应该判断是否传输完成
                    //为简化过程此处并未实现

                    memcpy(&buffer[1], data + 1024 * totalSeq, 1024);
                    buffer[1025]='\0';
                    printf("send a packet with a seq of %d\n", curSeq);

                    sendto(SendSocket, buffer, BUFFER_LENGTH, 0, (SOCKADDR *) &RecvAddr, sizeof(SOCKADDR));
//                            sendto(sockServer, buffer, BUFFER_LENGTH, 0,(SOCKADDR *) &addrClient, sizeof(SOCKADDR));
                    ++curSeq;
                    curSeq %= SEQ_SIZE;
                    ++totalSeq;
                    Sleep(500);
                }
                //等待 Ack，若没有收到，则返回值为-1，计数器+1
                recvSize =recvfrom(SendSocket, buffer, BUFFER_LENGTH, 0, ((SOCKADDR *) &RecvAddr), &length);
                if (recvSize < 0) {
                    waitCount++;
                    //20 次等待 ack 则超时重传
                    if (waitCount > 20) {
                        timeoutHandler();
                        waitCount = 0;
                    }
                } else {
                    if(buffer[0]==45)
                    {
                        ;
                    }
                    else{
                        //收到 ack
                        ackHandler(buffer[0]);
                        waitCount = 0;}
                }
                Sleep(500);
                break;
        }
    }
}


