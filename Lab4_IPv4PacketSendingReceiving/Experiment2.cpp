/*
* THIS FILE IS FOR IP TEST
*/
// system support
#include "sysInclude.h"

// 丢弃有错误的分组并报告错误类型
// 参数：
// pBuffer：指向被丢弃分组的指针
// type：分组被丢弃的原因，可取以下值：
// STUD_IP_TEST_CHECKSUM_ERROR //IP 校验和出错
// STUD_IP_TEST_TTL_ERROR //TTL 值出错
// STUD_IP_TEST_VERSION_ERROR //IP 版本号错
// STUD_IP_TEST_HEADLEN_ERROR //头部长度错
// STUD_IP_TEST_DESTINATION_ERROR //目的地址错
extern void ip_DiscardPkt(char* pBuffer,int type);

// 发送分组
// 参数：
// pBuffer：指向待发送的 IPv4 分组头部的指针
// length：待发送的 IPv4 分组长度
// 把分组交给下层完成发送
extern void ip_SendtoLower(char*pBuffer,int length);

// 上层接收
// 参数：
// pBuffer：指向要上交的上层协议报文头部的指针
// length：上交报文长度
extern void ip_SendtoUp(char *pBuffer,int length);

// 获取本机 IPv4 地址
// unsigned int getIpv4Address( )
// 参数：无
extern unsigned int getIpv4Address();


struct Ipv4
{
	// 版本号+首部长度
	char Version_Header;
	// 服务类型
	char TOS;
	// 总长度
	short TotalLength;
	// 标识
	short Identification;
	// 标志位+片偏移
	short Fragment_Offset;
	// 生存时间(TTL)
	char TTL;
	// 协议
	char Protocol;
	// 首部校验和
	short HeaderChecksum;
	// 源IP地址
	unsigned int SourceAddress;
	// 目的IP地址
	unsigned int DestinationAddress;
	Ipv4() {
		memset(this,0,sizeof(Ipv4));
	}

	Ipv4(unsigned int len,unsigned int srcAddr,unsigned int dstAddr,byte protocol,byte ttl) {
		memset(this,0,sizeof(Ipv4));
		// 版本号为0100 首部长度为0101
		// 因此为01000101-> 0x45
		Version_Header = 0x45;
		// 数据包长度+首部长度为总长度
		TotalLength = htons(len+20);
		// 设置ttl
		TTL = ttl;
		// 设置协议
		Protocol = protocol;
		// 设置源IP
		SourceAddress = htonl(srcAddr);
		// 设置目的IP
		DestinationAddress = htonl(dstAddr);

		char *pBuffer = new char[sizeof(Ipv4)];
		memcpy(pBuffer,this,sizeof(Ipv4));
		// 计算校验和
		int sum = 0;
		for(int i = 0; i < 10; i++) {
			if(i != 5) {
				sum += (int)((unsigned char)pBuffer[i*2] << 8);
				sum += (int)((unsigned char)pBuffer[i*2+1]);
			}
		}
		while((sum & 0xffff0000) != 0) {
			sum = (sum & 0xffff) + ((sum >> 16) & 0xffff);
		}
		unsigned short int headerchecksum = sum;
		HeaderChecksum = htons(~headerchecksum);
	}
};



// implemented by students
// 将需要上层协议进一步处理的信息提交给上层协议
// (1)检查接收到的 IPv4 分组头部的字段，包括版本号（Version）、头部长度（IP Head length）、生存时间（Time to live）以及头校验和（Header checksum）字段。
// 对于出错的分组调用 ip_DiscardPkt( )丢弃，并说明错误类型。
// (2)检查 IPv4 分组是否应该由本机接收。
// 如果分组的目的地址是本机地址或广播地址，则说明此分组是发送给本机的；
// 否则调用ip_DiscardPkt( )丢弃，并说明错误类型。
// (3)如果 IPV4 分组应该由本机接收，则提取得到上层协议类型，调用ip_SendtoUp( )接口函数，交给系统进行后续接收处理。
int stud_ip_recv(char *pBuffer,unsigned short length)
{
	// struct Ipv4
	Ipv4 *ipv4 = (Ipv4*)pBuffer;
	int version = 0xf & ((ipv4->Version_Header)>> 4);
	// 判断是否为ipv4报文
	if(version != 4)  
	{
    	ip_DiscardPkt(pBuffer,STUD_IP_TEST_VERSION_ERROR);
    	return 1;
	}
	int headerLength = 0xf & ipv4->Version_Header;
	// 判断是否小于5，由于首部需要5*4字节，因此需要大于等于5
	if(headerLength < 5)
	{
		ip_DiscardPkt(pBuffer,STUD_IP_TEST_HEADLEN_ERROR);
    	return 1;
	}
	// 判断生命周期是否为0
	int ttl = (int)ipv4->TTL;
	if(ttl == 0) 
	{
    	ip_DiscardPkt(pBuffer,STUD_IP_TEST_TTL_ERROR);
    	return 1;
	}
	// 判断目的地址是否为本机
	int destination_address = ntohl(ipv4->DestinationAddress);  
	if(destination_address != getIpv4Address() && destination_address != 0xffffffff) 
	{
    	ip_DiscardPkt(pBuffer,STUD_IP_TEST_DESTINATION_ERROR);
    	return 1;
	}
	// 判断校验和是否正确
	int header_checksum = ntohs(ipv4->HeaderChecksum);
	int sum = 0;
  	for(int i = 0; i < headerLength*2; i++) {
    	if(i!=5)
    	{
      	sum += (int)((unsigned char)pBuffer[i*2] << 8);
      	sum += (int)((unsigned char)pBuffer[i*2+1]);
    	}
	}
	while((sum & 0xffff0000) != 0) {
    	sum = (sum & 0xffff) + ((sum >> 16) & 0xffff);
	}
	unsigned short int headerchecksum = (~sum) & 0xffff;
	if(headerchecksum != header_checksum) {
		ip_DiscardPkt(pBuffer,STUD_IP_TEST_CHECKSUM_ERROR);
		return 1;
	}
	// 无误则发送报文
	ip_SendtoUp(pBuffer,length);
	return 0;
}

// 发送接口函数
// (1)根据所传参数（如数据大小），来确定分配的存储空间的大小并申请分组的存储空间。
// (2)按照 IPv4 协议标准填写 IPv4 分组头部各字段，标识符（Identification）字段可以使用一个随机数来填写。
// （注意：部分字段内容需要转换成网络字节序）
// (3)完成 IPv4 分组的封装后，调用 ip_SendtoLower( )接口函数完成后续的发送处理工作，最终将分组发送到网络中。
int stud_ip_Upsend(char *pBuffer,unsigned short len,unsigned int srcAddr,unsigned int dstAddr,byte protocol,byte ttl)
{
	char *pack_to_sent = new char[len+20];
	memset(pack_to_sent,0,len+20);
	*((Ipv4*)pack_to_sent) = Ipv4(len,srcAddr,dstAddr,protocol,ttl);
	memcpy(pack_to_sent+20,pBuffer,len);
	ip_SendtoLower(pack_to_sent,len+20);
	delete[] pack_to_sent;
	return 0;
}
