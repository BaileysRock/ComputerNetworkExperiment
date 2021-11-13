#include <stdlib.h>
#include <WinSock2.h>
#include <time.h>
#include <fstream>

#pragma comment(lib,"ws2_32.lib")

#define SERVER_PORT  12340     //接收数据的端口号
#define SERVER_IP  "127.0.0.1" // 服务器的 IP 地址

const int BUFFER_LENGTH = 1026;
const int SEND_WIND_SIZE = 10; // 发送窗口大小为10
const int SEQ_SIZE = 20; //序列号的个数，从 0~19 共计 20 个
const int SEQ_NUMBER = 9;
//由于发送数据第一个字节如果值为 0， 则数据会发送失败
//因此接收端序列号为 1~20，与发送端一一对应

BOOL ack[SEQ_SIZE];//收到 ack 情况，对应 0~19 的 ack
char dataBuffer[SEQ_SIZE][BUFFER_LENGTH];

int curSeq;//当前数据包的 seq
int curAck;//当前等待确认的 ack
int totalPacket;//需要发送的包总数

int totalSeq;//已发送的包的总数
int totalAck;//确认收到（ack）的包的总数
int finish;//标志位：数据传输是否完成（finish=1->数据传输已完成）
int finish_S;

// 打印
//void printTips(){
//    printf("*****************************************\n");
//    printf("| -time to get current time              \n");
//    printf("| -quit to exit client                   \n");
//    printf("| -test_sr [X] [Y]                       \n");
//    printf("| -test_sr_send [X] [Y]                  \n");
//    printf("*****************************************\n");
//}

// 获取当前时间
void getCurTime(char *ptime){
    char buffer[128];
    memset(buffer,0,sizeof(buffer));
    time_t c_time;
    struct tm *p;
    time(&c_time);
    p = localtime(&c_time);
    sprintf(buffer,"%d/%d/%d %d:%d:%d",
            p->tm_year + 1900,
            p->tm_mon + 1,
            p->tm_mday,
            p->tm_hour,
            p->tm_min,
            p->tm_sec);
    strcpy(ptime,buffer);
}

// 算概率 模拟丢包
BOOL lossInLossRatio(float lossRatio){
    int lossBound = (int) (lossRatio * 100);
    int r = rand() % 101;
    if(r <= lossBound){
        return TRUE;
    }
    return FALSE;
}


//当前收到的序列号 recvSeq 是否在可接收的范围内
BOOL seqRecvAvailable(int recvSeq){
    int step;
    int index;
    index= recvSeq - 1; // 获取真实的序列号
    step = index - curAck;
    step = step >= 0 ? step : step + SEQ_SIZE;
    //序列号是否在当前发送窗口之内
    if(step >= SEND_WIND_SIZE){
        return FALSE;
    }
    return TRUE;
}


// 当前序列号是否可用
int seqIsAvailable(){
    int step;
    step = curSeq - curAck;
    step = step >= 0 ? step : step + SEQ_SIZE;
    //序列号是否在当前发送窗口之内
    if(step >= SEND_WIND_SIZE){
        return 0;
    }
    if(!ack[curSeq]){ //ack[curSeq]==FALSE
        return 1;
    }
    return 2;
}


// Qualifier: 超时重传处理函数，哪个没收到 ack ，就要重传哪个
void timeoutHandler(){
    printf("Timer out error!\n");

    int index;

    if(totalSeq == totalPacket){//之前发送到了最后一个数据包
        if(curSeq>curAck){
            totalSeq -= (curSeq-curAck);
        }
        else if(curSeq<curAck){
            totalSeq -= (curSeq-curAck+SEQ_SIZE);
        }
    }
    else{//之前没发送到最后一个数据包
        totalSeq -= SEND_WIND_SIZE;
    }

    curSeq = curAck;
}


// ack处理函数
void ackHandler(char c){
    unsigned char index = (unsigned char)c - 1; //序列号减一
    printf("Recv a ack of seq: %d\n",index+1);//从接收方收到的确认收到的序列号

    int all;
    int next;
    int add;
    all=1;

    // 如果这次收到的ack就是index 就要往前挪动窗口了
    if(curAck == index){
        add = 1;
        totalAck += 1;
        ack[index]=FALSE;
        curAck = (index + 1) % SEQ_SIZE;
        for(int i= 1;i < SEQ_SIZE;i++){
            next=(i+index)%SEQ_SIZE;
            if(ack[next]==TRUE){
                ack[next]=FALSE;
                curAck = (next + 1) % SEQ_SIZE;
                totalSeq++;
                ++curSeq;
                curSeq %= SEQ_SIZE;
            }
            else{
                break;
            }
        }
        // printf("\t\tcurAck == index , totalAck += %d\n",add);
    }
        // 不是窗口的下界 是中间元素
    else if(curAck < index && index-curAck+1 <= SEND_WIND_SIZE){          //要保证是要接受的消息（在滑动窗口内）
        if(!ack[index]){
            // printf("\t\tcurAck < index , totalAck += %d\n",1);
            totalAck += 1;
            ack[index] = TRUE;
        }
    }
        // 一左一右的分布
    else if(SEQ_SIZE-curAck+index+1 <= SEND_WIND_SIZE && curAck > index){ //要保证是要接受的消息（在滑动窗口内）
        if(!ack[index]){
            // printf("\t\tcurAck > index , totalAck += %d\n",1);
            totalAck += 1;
            ack[index] = TRUE;
        }
    }
}

int main(int argc, char* argv[])
{
    //加载套接字库（必须）
    WORD wVersionRequested;
    WSADATA wsaData;
    //套接字加载时错误提示
    int err;
    //版本 2.2
    wVersionRequested = MAKEWORD(2, 2);
    //加载 dll 文件 Scoket 库
    err = WSAStartup(wVersionRequested, &wsaData);
    if(err != 0){
        //找不到 winsock.dll
        printf("load winsock fail: %d\n", err);
        return 1;
    }
    if(LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) !=2)
    {
        printf("cannot find winsock we want......\n");
        WSACleanup();
    }else{
        printf("The Winsock 2.2 dll is ready......\n");
    }

    SOCKET socketClient = socket(AF_INET, SOCK_DGRAM, 0);
    SOCKADDR_IN addrServer;
    addrServer.sin_addr.S_un.S_addr = inet_addr(SERVER_IP);
    addrServer.sin_family = AF_INET;
    addrServer.sin_port = htons(SERVER_PORT);

    //接收缓冲区
    char buffer[BUFFER_LENGTH];
    ZeroMemory(buffer,sizeof(buffer));
    int len = sizeof(SOCKADDR);

    // 显示菜单
//    printTips();

    int ret;
    int interval = 1;//收到数据包之后返回 ack 的间隔，默认为 1 表示每个都返回 ack，0 或者负数均表示所有的都不返回 ack
    char cmd[128];

    float packetLossRatio = 0.2;  //默认包丢失率 0.2
    float ackLossRatio = 0.2;     //默认 ACK 丢失率 0.2

    //用时间作为随机种子，放在循环的最外面
    srand((unsigned)time(NULL));

    //将测试数据读入data中
    std::ifstream icin;
    icin.open("test_client_sr.txt");
    char data[1024 * SEQ_NUMBER];
    ZeroMemory(data,sizeof(data));
    icin.read(data,1024 * SEQ_NUMBER);
    icin.close();

    totalPacket = sizeof(data) / 1024;
    // printf("totalPacket is ：%d\n\n",totalPacket);

    int recvSize ;
    finish=0;

    for(int i=0; i < SEQ_SIZE; ++i){
        ack[i] = FALSE;
    }
    finish=0;
    finish_S=0;

    while(true){

        fgets(buffer, BUFFER_LENGTH ,stdin);
        ret  = sscanf(buffer,"%s%f%f",&cmd,&packetLossRatio,&ackLossRatio);
        //开始 SR 测试，使用 SR 协议实现 UDP 可靠文件传输
        if(!strcmp(cmd,"-testsr")){ // 现在是作为接收端 发送ack

            for(int i=0; i < SEQ_SIZE; ++i){
                ack[i] = FALSE;
            }

            printf("%s\n","Begin to test SR protocol......");
            printf("The loss ratio of packet is %.2f,the loss ratio of ack  is %.2f\n",packetLossRatio,ackLossRatio);

            int waitCount = 0;
            int stage = 0;
            finish=0;
            BOOL b;
            curAck=0;

            unsigned char u_code;   //状态码
            unsigned short seq;     //包的序列号
            unsigned short recvSeq; //接收窗口大小为 1，已确认的序列号
            unsigned short waitSeq; //等待的序列号
            int next;

            sendto(socketClient, "-test_sr", strlen("-test_sr")+1, 0, (SOCKADDR*)&addrServer, sizeof(SOCKADDR));
            while(true) {
                //等待 server 回复设置 UDP 为阻塞模式
                recvfrom(socketClient,buffer,BUFFER_LENGTH,0,(SOCKADDR*)&addrServer, &len);


                if(!strcmp(buffer,"finish transfer all the data!\n")){
                    finish=1;
                    break;
                }


                switch(stage){
                    case 0://等待握手阶段
                        u_code = (unsigned char)buffer[0];
                        if ((unsigned char)buffer[0] == 205)
                        {
                            printf("Ready for file transmission\n");
                            buffer[0] = 200;
                            buffer[1] = '\0';
                            sendto(socketClient,  buffer,  2,  0, (SOCKADDR*)&addrServer, sizeof(SOCKADDR));
                            stage = 1;
                            recvSeq = 0;
                            waitSeq = 1;
                        }
                        break;
                    case 1://等待接收数据阶段
                        seq = (unsigned short)buffer[0];
                        //随机法模拟包是否丢失
                        b = lossInLossRatio(packetLossRatio);
                        if(b){
                            printf("The packet with a seq of %d loss\n",seq);
                            continue;
                        }

                        printf("recv a packet with a seq of %d\n",seq);

                        // seq在可接受的范围
                        if(seqRecvAvailable(seq)){
                            recvSeq = seq;
                            ack[seq-1]=TRUE;
                            ZeroMemory(dataBuffer[seq-1],sizeof(dataBuffer[seq-1]));
                            strcpy(dataBuffer[seq-1],&buffer[1]);

                            buffer[0] = recvSeq;
                            buffer[1] = '\0';

                            int temp = curAck;
                            if(seq-1 == curAck){
                                for(int i=0;i<SEQ_SIZE;i++){

                                    next=(temp+i)%SEQ_SIZE;
                                    if(ack[next]){
                                        curAck=(next+1)%SEQ_SIZE;
                                        ack[next]=FALSE;
                                    }

                                    else{
                                        break;
                                    }
                                }
                            }

                        }
                        else{
                            recvSeq = seq;
                            buffer[0] = recvSeq;
                            buffer[1] = '\0';
                        }

                        b = lossInLossRatio(ackLossRatio);
                        if(b){
                            printf("The  ack  of  %d  loss\n",(unsigned char)buffer[0]);
                            continue;
                        }
                        sendto(socketClient,  buffer,  2,  0, (SOCKADDR*)&addrServer, sizeof(SOCKADDR));
                        printf("send a ack of %d\n",(unsigned char)buffer[0]);
                        break;
                }
                Sleep(500);
            }
        }
        else if(strcmp(cmd,"-time") == 0){
            getCurTime(buffer);
        }
        else if(!strcmp(cmd,"-test_sr_send")){
            //进入 gbn 测试阶段
            //首先 server（server 处于 0 状态）向 client 发送 205 状态码（server进入 1 状态）
            //server 等待 client 回复 200 状态码， 如果收到 （server 进入 2 状态） ，则开始传输文件，否则延时等待直至超时\
			//在文件传输阶段，server 发送窗口大小设为
            for(int i=0; i < SEQ_SIZE; ++i){
                ack[i] = FALSE;
            }

            ZeroMemory(buffer,sizeof(buffer));
            int recvSize;
            int waitCount = 0;
            finish_S=0;
            printf("Begin to test SR protocol......\n");
            //加入了一个握手阶段
            //首先服务器向客户端发送一个 205 大小的状态码（我自己定义的）表示服务器准备好了，可以发送数据
            //客户端收到 205 之后回复一个 200 大小的状态码，表示客户端备好了，可以接收数据了
            //服务器收到 200 状态码之后，就开始使用 GBN 发送数据了
            printf("Shake hands stage\n");
            int stage = 0;
            bool runFlag = true;

            sendto(socketClient,  "-test_sr_send",  strlen("-test_sr_send")+1,  0,
                   (SOCKADDR*)&addrServer, sizeof(SOCKADDR));
            Sleep(100);
            recvfrom(socketClient,buffer,BUFFER_LENGTH,0,((SOCKADDR*)&addrServer),&len);

            ZeroMemory(buffer,sizeof(buffer));
            int iMode = 1; //1：非阻塞，0：阻塞
            ioctlsocket(socketClient,FIONBIO, (u_long FAR*) &iMode);//非阻塞设置

            while(runFlag){
                switch(stage){
                    case 0://发送 205 阶段
                        buffer[0] = 205;
                        sendto(socketClient,  buffer,  strlen(buffer)+1,  0,
                               (SOCKADDR*)&addrServer, sizeof(SOCKADDR));
                        Sleep(100);
                        stage = 1;
                        break;
                    case 1://等待接收 200 阶段，没有收到则计数器+1，超时则放弃此次“连接”，等待从第一步开始
                        recvSize  =  recvfrom(socketClient,buffer,BUFFER_LENGTH,0,((SOCKADDR*)&addrServer),&len);
                        if(recvSize < 0){
                            ++waitCount;
                            if(waitCount > 20){
                                runFlag = false;
                                printf("Timeout error!\n");
                                break;
                            }
                            Sleep(500);
                            continue;
                        }else{
                            if((unsigned char)buffer[0] == 200){
                                printf("Begin a file transfer\n");
                                printf("File size is %dB, each packet is 1024B  and packet total num is %d\n",sizeof(data),totalPacket);
                                curSeq = 0;
                                curAck = 0;
                                totalSeq = 0;
                                waitCount = 0;
                                totalAck=0;
                                finish_S=0;
                                stage = 2;
                            }
                        }
                        break;

                    case 2://数据传输阶段

                        if(seqIsAvailable() == 1 && totalSeq<=(totalPacket-1)){//totalSeq<=(totalPacket-1)：未传到最后一个数据包

                            buffer[0] = curSeq + 1;
                            ack[curSeq] = FALSE;

                            memcpy(&buffer[1],data + 1024 * totalSeq,1024);
                            printf("send a packet with a seq of : %d\n",curSeq+1);
                            sendto(socketClient, buffer, BUFFER_LENGTH, 0,
                                   (SOCKADDR*)&addrServer, sizeof(SOCKADDR));
                            ++curSeq;
                            curSeq %= SEQ_SIZE;
                            ++totalSeq;
                            Sleep(500);
                        }
                        else if(seqIsAvailable() == 2 && totalSeq <= totalPacket-1) {
                            ++curSeq;
                            curSeq %= SEQ_SIZE;
                            ++totalSeq;
                            break;
                        }

                        //等待 Ack，若没有收到，则返回值为-1，计数器+1
                        recvSize  =  recvfrom(socketClient,buffer,BUFFER_LENGTH,0,((SOCKADDR*)&addrServer),&len);
                        if(recvSize < 0){
                            waitCount++;
                            //20 次等待 ack 则超时重传
                            if (waitCount > 20)
                            {
                                timeoutHandler();
                                // printf("\t----totalSeq Now is : %d\n",totalSeq);
                                waitCount = 0;
                            }
                        }else{
                            //收到 ack
                            ackHandler(buffer[0]);
                            printf("totalAck Now is : %d\n",totalAck);
                            waitCount = 0;


                            if(totalAck==totalPacket){//数据传输完成
                                finish_S = 1;
                                break;
                            }

                        }
                        Sleep(500);
                        break;
                }


                if(finish_S==1){
                    printf("finish transfer all the data!\n");
                    strcpy(buffer,"finish transfer all the data!\n");
                    sendto(socketClient, buffer, strlen(buffer)+1, 0, (SOCKADDR*)&addrServer,sizeof(SOCKADDR));
                    break;
                }

            }
            iMode = 0; //1：非阻塞，0：阻塞
            ioctlsocket(socketClient,FIONBIO, (u_long FAR*) &iMode);//非阻塞设置
        }

        /*判断数据传输是否完成添加或修改*/
        if(finish==1){
            printf("finish transfer all the data!\n");
//            printTips();
            finish=0;
            continue;
        }
        if(finish_S==1){
//            printTips();
            finish_S=0;
            continue;
        }
        /*判断数据传输是否完成添加或修改*/

        sendto(socketClient,  buffer,  strlen(buffer)+1,  0, (SOCKADDR*)&addrServer, sizeof(SOCKADDR));
        ret = recvfrom(socketClient,buffer,BUFFER_LENGTH,0,(SOCKADDR*)&addrServer,&len);
        printf("%s\n",buffer);
        if(!strcmp(buffer,"Good bye!")){
            break;
        }
        //printTips();
    }
    //关闭套接字
    closesocket(socketClient);
    WSACleanup();
    return 0;
}