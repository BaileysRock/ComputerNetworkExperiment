/*
* THIS FILE IS FOR IP FORWARD TEST
*/
#include "sysInclude.h"
#include <iostream>
#include <vector>
using namespace std;
// system support
// 将 IP 分组上交本机上层协议的函数，即当分组需要上交上层函数的时候调用本函数。
// pBuffer：指向分组的 IP 头
// length：表示分组的长度
// 本函数是 IPv4 协议接收流程的上层接口函数，在对 IPv4 的分组完成解析处理之后，
// 如果分组的目的地址是本机的地址，则调用本函数将正确分组提交上层相应协议模块进一步处理。
extern void fwd_LocalRcv(char *pBuffer, int length);

// 将封装完成的 IP 分组通过链路层发送出去的函数。 
// pBuffer：指向所要发送的 IPv4 分组头部
// length：分组长度（包括分组头部）
// nexthop：转发时下一跳的地址。
// 本函数是发送流程的下层接口函数，在 IPv4 协议模块完成发送封装工作后调用该接口函数进行后续发送处理。
// 其中，后续的发送处理过程包括分片处理、IPv4 地址到 MAC 地址的映射（ARP 协议）、封装成 MAC
extern void fwd_SendtoLower(char *pBuffer, int length, unsigned int nexthop);

// 丢弃 IP 分组的函数。当需要丢弃一个 IP 分组的时候调用。 
// pBuffer：指向被丢弃的 IPV4 分组头部
// type：表示错误类型，包括 TTL 错误和找不到路由两种错误，定义
// STUD_FORWARD_TEST_TTLERROR
// STUD_FORWARD_TEST_NOROUTE
// 本函数是丢弃分组的函数，在接收流程中检查到错误时调用此函数将分组丢弃。
extern void fwd_DiscardPkt(char *pBuffer, int type);

// 获取本机的 IPv4 地址，用来判断分组地址和本机地址是否相同
// 本函数用于获取本机的 IPv4 地址，学生调用该函数即可返回本机的
// IPv4 地址，可以用来判断 IPV4 分组是否为本机接收。
// 本机 IPv4 地址。
// 除了以上的函数以外，学生可根据需要自己编写一些实验需要的函数和数据结构，包括路由表的数据结构，对路由表的搜索、初始化等操作函数。
extern unsigned int getIpv4Address( );

// implemented by students
// 将8位buffer转换为无符号数
unsigned get8(char *buffer)
{
  return (unsigned)buffer[0] & 0xFF;
}
// 将16位buffer转换为无符号数
unsigned get16(char *buffer)
{
  return (get8(buffer + 0) << 8) + get8(buffer + 1);
}
// 将32位buffer转换为无符号数
unsigned get32(char *buffer)
{
  return (get16(buffer + 0) << 16) + get16(buffer + 2);
}
// 将unsigned转换为char
char setChar(unsigned i)
{
  return (unsigned char)(i&0xFF);
}

unsigned getLow(unsigned IP, unsigned masklen)
{
  masklen = 32 - masklen;
  IP >>= masklen;
  IP <<= masklen;
  return IP;
}

unsigned getHigh(unsigned IP, unsigned masklen)
{
  masklen = 32 - masklen;
  IP |= (1 << masklen) -1;
  return IP;
}

struct route
{
  unsigned low,high;
  unsigned masklen;
  unsigned nextIP;
  route(unsigned low, unsigned high, unsigned masklen, unsigned nextIP)
  {
    this->low = low;
    this->high = high;
    this->masklen = masklen;
    this->nextIP = nextIP;
  }
};

// 对路由表进行初始化
// proute：指向需要添加路由信息的结构体头部 ，其数据结构
// stud_route_msg 的定义如下：
// typedef struct stud_route_msg
// {
// 	unsigned int dest;
// 	unsigned int masklen;
// 	unsigned int nexthop;
// } stud_route_msg;
// 本函数为路由表配置接口，系统在配置路由表时需要调用此接口。
// 此函数功能为向路由表中增加一个新的表项，将参数所传递的路由信息
// 添加到路由表中。
vector<route> vec;
void stud_Route_Init()
{
	vec.clear();
	return;
}

// 完成路由的增加
// 本函数将在系统启动的时候被调用，学生可将初始化路由表的代码写在这里。
void stud_route_add(stud_route_msg *proute)
{
  unsigned dest = htonl(proute->dest);
  unsigned masklen = htonl(proute->masklen);
  unsigned nextIP = htonl(proute->nexthop);
  unsigned low = getLow(dest, masklen);
  unsigned high = getHigh(dest, masklen);
  vec.push_back(route(low, high, masklen, nextIP));
  return;
}


bool getNextIP(unsigned destIP, unsigned &nextIP)
{
  unsigned len = 0;
  bool ret = false;
  for(unsigned i = 0; i < vec.size(); i++)
    if(vec[i].low <= destIP && vec[i].high >=destIP)
      if(vec[i].masklen >= len)
      {
        len = vec[i].masklen;
        nextIP = vec[i].nextIP;
        ret = true;
      }
  return ret;
}

// 1）查找路由表。
// 根据相应路由表项的类型来确定下一步操作：
// 错误分组调用函数 fwd_DiscardPkt ( )进行丢弃
// 上交分组调用接口函数 fwd_LocalRcv ( )提交给上层协议继续处理，转发分组进行转发处理。
// 转发分组还要从路由表项中获取下一跳的 IPv4地址。
// 2) 转发处理流程。
// 对 IPv4 头部中的 TTL 字段减 1，重新计算校验和，
// 然后调用下层接口 fwd_SendtoLower ( )进行发送处理。

// pBuffer：指向接收到的 IPv4 分组头部
// length：IPv4 分组的长度
// 返回值：0 为成功，1 为失败；
// 本函数是 IPv4 协议接收流程的下层接口函数，实验系统从网络中接收到分组后会调用本函数。调用该函数之前已完成IP报文的合法性检查，
// 因此学生在本函数中应该实现如下功能：
// a.判定是否为本机接收的分组，如果是则调用 fwd_LocalRcv( )；
// b.按照最长匹配查找路由表获取下一跳，查找失败则调用fwd_DiscardPkt( )；
// c.调用 fwd_SendtoLower( )完成报文发送；
// d.转发过程中注意 TTL 的处理及校验和的变化。
int stud_fwd_deal(char *pBuffer, int length)
{
  // dest IP
  unsigned destIP;
  destIP = get32(pBuffer + 16);
  unsigned localIP;
  localIP = getIpv4Address();
  if(destIP == 0xFFFFFFFF || destIP == localIP)
  {
    fwd_LocalRcv(pBuffer, length);
    return 0;
  }
  // dest IP end

  unsigned nextIP;
  if(getNextIP(destIP, nextIP))
  {
    // ttl
    unsigned ttl;
    ttl = get8(pBuffer + 8);
    if(ttl == 0)
    {
      fwd_DiscardPkt(pBuffer, STUD_FORWARD_TEST_TTLERROR);
      return 1;
    }
    ttl = ttl - 1;
    pBuffer[8] = setChar(ttl);
    // ttl end

    // check sum
    unsigned checkSum;
    checkSum = 0;
    int i;
    for(i = 0; i < 20; i+=2)
      if(i != 10)
        checkSum += get16(pBuffer + i);

    while(checkSum > 0xFFFF)
      checkSum = (checkSum >> 16) + (checkSum & 0xFFFF);
    checkSum = ((~checkSum) & 0xFFFF);
    pBuffer[10] = setChar(checkSum >> 8);
    pBuffer[11] = setChar(checkSum & 0xFF);
    // check sum end

    fwd_SendtoLower(pBuffer, length, nextIP);
    return 0;
  }
  else
  {
    fwd_DiscardPkt(pBuffer, STUD_FORWARD_TEST_NOROUTE);
    return 1;
  }
}
