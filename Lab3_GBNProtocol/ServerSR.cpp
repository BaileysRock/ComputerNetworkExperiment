#include <stdlib.h>
#include <time.h>
#include <WinSock2.h>
#include <fstream>

#pragma comment(lib,"ws2_32.lib")

#define SERVER_PORT  12340  //端口号
#define SERVER_IP  "0.0.0.0" //IP 地址
const int BUFFER_LENGTH = 1026; //缓冲区大小， （以太网中 UDP 的数据帧中包长度应小于 1480 字节）
const int SEND_WIND_SIZE = 10;//发送窗口大小为 10，GBN 中应满足 W + 1 <=N（W 为发送窗口大小，N 为序列号个数）

//本例取序列号 0...19 共 20 个
//如果将窗口大小设为 1，则为停-等协议
const int SEQ_SIZE = 20; //序列号的个数，从 0~19 共计 20 个
const int SEQ_NUMBER = 9;

//由于发送数据第一个字节如果值为 0， 则数据会发送失败
//因此接收端序列号为 1~20，与发送端一一对应
BOOL ack[SEQ_SIZE];//收到 ack 情况，对应 0~19 的 ack
char dataBuffer[SEQ_SIZE][BUFFER_LENGTH];
int curSeq;      //当前数据包的 seq
int curAck;      //当前等待确认的 ack
int totalPacket; //需要发送的包总数

int totalSeq;     //已发送的包的总数
int totalAck;     //确认收到（ack）的包的总数
int finish;       //标志位：数据传输是否完成（finish=1->数据传输已完成）
int finish_S;

// 获取时间
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


// 判断当前序列号 curSeq 是否可用
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


// 超时重传处理函数，
void timeoutHandler(){
    printf("Timer out error!\n");
    int index;

    if(totalSeq==totalPacket){//之前发送到了最后一个数据包
        if(curSeq>curAck){
            totalSeq -= (curSeq-curAck);
        }
        else if(curSeq<curAck){
            totalSeq -= (curSeq-curAck+20);
        }
    }
    else{//之前没发送到最后一个数据包
        totalSeq -= SEND_WIND_SIZE;
    }

    curSeq = curAck;
}

// 处理 ack
void ackHandler(char c){
    unsigned char index = (unsigned char)c - 1;    //序列号减一
    printf("Recv a ack of seq %d \n",index + 1);   //从接收方收到的确认收到的序列号
    int all;
    int next;
    int add;
    all = 1;

    if(curAck == index){
        add = 1;
        totalAck += 1;
        ack[index] = FALSE;
        curAck = (index + 1) % SEQ_SIZE;
        for(int i = 1;i<SEQ_SIZE;i++){
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
    else if(curAck < index && index-curAck + 1 <= SEND_WIND_SIZE){//要保证是要接受的消息（在滑动窗口内）
        if(!ack[index]){
            // printf("\t\tcurAck < index , totalAck += %d\n",1);
            totalAck += 1;
            ack[index] = TRUE;
        }
    }else if(SEQ_SIZE - curAck + index + 1 <= SEND_WIND_SIZE && curAck > index){ //要保证是要接受的消息（在滑动窗口内）
        if(!ack[index]){
            // printf("\t\tcurAck > index , totalAck += %d\n",1);
            totalAck += 1;
            ack[index] = TRUE;
        }

    }
}

// 模拟丢包
BOOL lossInLossRatio(float lossRatio) {
    int lossBound = (int) (lossRatio * 100);
    int r = rand() % 101;
    if(r <= lossBound){
        return TRUE;
    }
    return FALSE;
}


// 判断当前收到的序列号 recvSeq 是否在可收范围内
BOOL seqRecvAvailable(int recvSeq) {
    int step;
    int index;
    index = recvSeq - 1;
    step = index - curAck;
    step = step >= 0 ? step : step + SEQ_SIZE;
    //序列号是否在当前发送窗口之内
    if(step >= SEND_WIND_SIZE){
        return FALSE;
    }
    return TRUE;
}

//主函数
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
        printf("WSAStartup failed with error: %d\n", err);
        return -1;
    }
    if(LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) !=2)
    {
        printf("Could not find a usable version of Winsock.dll\n");
        WSACleanup();
    }else{
        printf("The Winsock 2.2 dll was found okay\n");
    }

    SOCKET sockServer = socket(AF_INET, SOCK_DGRAM,IPPROTO_UDP);
    //设置套接字为非阻塞模式
    int iMode = 1; //1：非阻塞，0：阻塞
    ioctlsocket(sockServer,FIONBIO, (u_long FAR*) &iMode);//非阻塞设置
    SOCKADDR_IN addrServer; //服务器地址
    //addrServer.sin_addr.S_un.S_addr = inet_addr(SERVER_IP);
    addrServer.sin_addr.S_un.S_addr = htonl(INADDR_ANY);//两者均可
    addrServer.sin_family = AF_INET;
    addrServer.sin_port = htons(SERVER_PORT);

    err = bind(sockServer,(SOCKADDR*)&addrServer, sizeof(SOCKADDR));
    if(err){
        err = GetLastError();
        printf("Could  not  bind  the  port  %d  for  socket.Error  code   is %d\n",SERVER_PORT,err);
        WSACleanup();
        return -1;
    }

    float packetLossRatio = 0.2;    //默认包丢失率 0.2
    float ackLossRatio = 0.2;       //默认 ACK 丢失率 0.2
    //用时间作为随机种子，放在循环的最外面
    srand((unsigned)time(NULL));

    SOCKADDR_IN addrClient;         //客户端地址
    int length = sizeof(SOCKADDR);
    char buffer[BUFFER_LENGTH];     //数据发送接收缓冲区
    ZeroMemory(buffer,sizeof(buffer));

    //将测试数据读入内存
    std::ifstream icin;
    icin.open("SRServer.txt");
    char data[1024 * SEQ_NUMBER];
    ZeroMemory(data,sizeof(data));
    icin.read(data,1024 * SEQ_NUMBER);
    icin.close();

    totalPacket = sizeof(data) / 1024;
    // printf("totalPacket is ：%d\n\n",totalPacket);

    int recvSize ;
    finish = 0;
    finish_S = 0;
    for(int i=0; i < SEQ_SIZE; ++i){
        ack[i] = FALSE;
    }

    while(true){
        //非阻塞接收，若没有收到数据，返回值为-1
        recvSize  = recvfrom(sockServer,buffer,BUFFER_LENGTH,0,((SOCKADDR*)&addrClient),&length);
        if(recvSize < 0){
            Sleep(200);
            continue;
        }

        printf("recv from client: %s\n",buffer);

        if(strcmp(buffer,"-time") == 0){
            printf("收到 -time");
            getCurTime(buffer);
        }
        else if(strcmp(buffer,"-quit") == 0){
            strcpy(buffer,"Good bye!");
            sendto(sockServer, buffer, strlen(buffer)+1, 0, (SOCKADDR*)&addrClient, sizeof(SOCKADDR));
            break;
        }
        else if(strcmp(buffer,"-test_sr") == 0){

            for(int i=0; i < SEQ_SIZE; ++i){
                ack[i] = FALSE;
            }
            ZeroMemory(buffer,sizeof(buffer));
            int recvSize;
            int waitCount = 0;
            finish=0;
            printf("Begain to test SR protocol......\n");

            printf("Shake hands stage\n");
            int stage = 0;
            bool runFlag = true;
            while(runFlag){
                switch(stage){
                    case 0://发送 205 阶段
                        buffer[0] = 205;
                        sendto(sockServer,  buffer,  strlen(buffer)+1,  0,
                               (SOCKADDR*)&addrClient, sizeof(SOCKADDR));
                        Sleep(100);
                        stage = 1;
                        break;
                    case 1://等待接收 200 阶段，没有收到则计数器+1，超时则放弃此次“连接”，等待从第一步开始
                        recvSize  =  recvfrom(sockServer,buffer,BUFFER_LENGTH,0,((SOCKADDR*)&addrClient),&length);
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
                                finish=0;
                                stage = 2;
                            }
                        }
                        break;
                    case 2://数据传输阶段


                        if(seqIsAvailable()==1 && totalSeq<=(totalPacket-1)){//totalSeq<=(totalPacket-1)：未传到最后一个数据包

                            //发送给客户端的序列号从 1 开始
                            buffer[0] = curSeq + 1;
                            ack[curSeq] = FALSE;
                            //数据发送的过程中应该判断是否传输完成->现在此代码已经实现了ok
                            //为简化过程此处并未实现->现在此代码已经实现了ok
                            memcpy(&buffer[1],data + 1024 * totalSeq,1024);
                            printf("send a packet with a seq of : %d\n",curSeq+1);
                            sendto(sockServer, buffer, BUFFER_LENGTH, 0,
                                   (SOCKADDR*)&addrClient, sizeof(SOCKADDR));
                            ++curSeq;
                            curSeq %= SEQ_SIZE;
                            ++totalSeq;
                            Sleep(500);
                        }
                        else if(seqIsAvailable() == 2 && totalSeq<=(totalPacket-1)){
                            ++curSeq;
                            curSeq %= SEQ_SIZE;
                            ++totalSeq;
                            break;
                        }

                        //等待 Ack，若没有收到，则返回值为-1，计数器+1
                        recvSize  =  recvfrom(sockServer,buffer,BUFFER_LENGTH,0,((SOCKADDR*)&addrClient),&length);
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
                            // printf("\t\t----totalAck Now is : %d\n",totalAck);
                            waitCount = 0;

                            if(totalAck==totalPacket){//数据传输完成
                                finish=1;
                                break;
                            }

                        }
                        Sleep(500);
                        break;
                }


                if(finish==1){
                    printf("finish transfer all the data!\n");
                    strcpy(buffer,"finish transfer all the data!\n");
                    sendto(sockServer, buffer, strlen(buffer)+1, 0, (SOCKADDR*)&addrClient,sizeof(SOCKADDR));
                    break;
                }
            }
        }
        else if(strcmp(buffer,"-test_sr_send") == 0){
            iMode = 0; //1：非阻塞，0：阻塞
            ioctlsocket(sockServer,FIONBIO, (u_long FAR*) &iMode);//非阻塞设置
            printf("%s\n","Begin to test SR protocol......");
            printf("The loss ratio of packet is %.2f,the loss ratio of ack  is %.2f\n",packetLossRatio,ackLossRatio);

            int waitCount = 0;
            int stage = 0;
            finish_S=0;
            BOOL b;
            curAck=0;

            for(int i=0; i < SEQ_SIZE; ++i){
                ack[i] = FALSE;
            }

            unsigned char u_code;//状态码
            unsigned short seq;//包的序列号
            unsigned short recvSeq;//接收窗口大小为 1，已确认的序列号
            unsigned short waitSeq;//等待的序列号
            int next;
            sendto(sockServer,  "-test_sr_send",  strlen("-test_sr_send")+1,  0,
                   (SOCKADDR*)&addrClient, sizeof(SOCKADDR));
            while (true)
            {
                //等待 server 回复设置 UDP 为阻塞模式
                recvfrom(sockServer,buffer,BUFFER_LENGTH,0,(SOCKADDR*)&addrClient, &length);

                if(!strcmp(buffer,"finish transfer all the data!\n")){
                    finish_S=1;
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
                            sendto(sockServer,  buffer,  2,  0, (SOCKADDR*)&addrClient, sizeof(SOCKADDR));
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
                        //如果是期待的包，正确接收，正常确认即可
                        if(seqRecvAvailable(seq)){
                            recvSeq = seq;
                            ack[seq-1]=TRUE;
                            ZeroMemory(dataBuffer[seq-1],sizeof(dataBuffer[seq-1]));
                            strcpy(dataBuffer[seq-1],&buffer[1]);
                            buffer[0] = recvSeq;
                            buffer[1] = '\0';
                            int tempt=curAck;
                            if(seq-1==curAck){
                                for(int i=0;i<SEQ_SIZE;i++){
                                    next=(tempt+i)%SEQ_SIZE;
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
                        sendto(sockServer,  buffer,  2,  0, (SOCKADDR*)&addrClient, sizeof(SOCKADDR));
                        printf("send a ack of %d\n",(unsigned char)buffer[0]);
                        break;
                }
                Sleep(500);
            }
            iMode = 1; //1：非阻塞，0：阻塞
            ioctlsocket(sockServer,FIONBIO, (u_long FAR*) &iMode);//非阻塞设置
        }
        if(finish_S==1){
            printf("finish transfer all the data!\n");
            finish_S=0;
            continue;
        }
        if(finish==1){
            finish=0;
            continue;
        }
        sendto(sockServer, buffer, strlen(buffer)+1, 0, (SOCKADDR*)&addrClient,sizeof(SOCKADDR));
        Sleep(500);
    }
    //关闭套接字，卸载库
    closesocket(sockServer);
    WSACleanup();
    return 0;
}