#pragma once
#include<cstdint>
#include <string>
#include<WinSock2.h>
#include<WS2tcpip.h>
#pragma comment(lib,"ws2_32.lib")
#include<Windows.h>
#include<iostream>
#include<csignal>
#define _WINSOCK_DEPRECATED_NO_WARNINGS
//本文件定义了符合ICMP协议还有IP协议的一些数据结构类型
//以及一些Ping程序处理的函数
/*                       IP报文格式
0            8           16    19                   31
+------------+------------+-------------------------+
| ver + hlen |  服务类型  |          总长度         |
+------------+------------+----+--------------------+
|           标识位        |flag|    分片偏移(13位)  |
+------------+------------+----+--------------------+
|   生存时间 | 高层协议号 |        首部校验和       |
+------------+------------+-------------------------+
|                   源 IP 地址                      |
+---------------------------------------------------+
|                  目的 IP 地址                     |
+---------------------------------------------------+
*/
//IP头
struct IPhead
{
	//这里使用了C语言的位域，也就是说像version变量它的大小在内存中是占4bit，而不是8bit
	uint8_t		version : 4; //IP协议版本
	uint8_t		headLength : 4;//首部长度
	uint8_t		serverce;//区分服务
	uint16_t	totalLength;//总长度
	uint16_t	flagbit;//标识
	uint16_t	flag : 3;//标志
	uint16_t	fragmentOffset : 13;//片偏移
	char		timetoLive;//生存时间（跳数）
	uint8_t		protocol;//使用协议
	uint16_t	headcheckSum;//首部校验和
	uint32_t	srcIPadd;//源IP
	uint32_t	dstIPadd;//目的IP
	//可选项和填充我就不定义了
};
/*
ICMP 请求报文
+--------+--------+---------------+
|  类型  |  代码   |      描述    |
+--------+--------+---------------+
|    8   |    0   |    回显请求   |
+--------+--------+---------------+
|   10   |    0   |   路由器请求  |
+--------+--------+---------------+
|   13   |    0   |   时间戳请求  |
+--------+--------+---------------+
|   15   |    0   | 信息请求(废弃)|
+--------+--------+---------------+
|   17   |    0   | 地址掩码请求  |
+--------+--------+---------------+

ICMP 应答报文
+--------+--------+---------------+
|  类型  |  代码  |      描述     |
+--------+--------+---------------+
|    0   |    0   |    回显回答   |
+--------+--------+---------------+
|    9   |    0   |   路由器回答  |
+--------+--------+---------------+
|   14   |    0   |   时间戳回答  |
+--------+--------+---------------+
|   16   |    0   | 信息回答(废弃)|
+--------+--------+---------------+
|   18   |    0   | 地址掩码回答  |
+--------+--------+---------------+

ICMP协议请求/回答报文格式
0        8       16                32
+--------+--------+-----------------+
|  类型   |  代码   |     校验和    |
+--------+--------+-----------------+
|      标识符      |      序号      |
+-----------------+-----------------+
|             回显数据              |
+-----------------+-----------------+

*/
//ICMP协议我只定义了请求/回答功能的报文格式。因为不同的类型和代码报文的内容组成不一样,ICMP分请求报文，回答报文，差错报文。差错报文我没写出来，因为用不着
//ICMP内容
//ICMP头
struct ICMPhead
{
	uint8_t type;//类型
	uint8_t code;//代码
	uint16_t checkSum;//校验和
	uint16_t ident;//进程标识符
	uint16_t seqNum;//序号
};
//ICMP回显请求报文(发送用)
struct ICMPrecvReq
{
	ICMPhead icmphead;//头部
	uint32_t timeStamp;//时间戳
	char	 data[32];//数据
};
//ICMP应答报文(接收用)
struct ICMPansReply
{
	IPhead iphead;//IP头
	ICMPrecvReq icmpanswer;//ICMP报文
	char data[1024];//应答报文携带的数据缓冲区
};
//计算校验和，参数1：协议头的指针，需要转换成空指针。参数2：计算的协议类型。返回校验和
uint16_t getCheckSum(void* protocol,char* type);
//ping程序，参数1：为目的IP地址。返回是否发送成功.参数2：只能写-t,或者不写
bool ping(const char* dstIPaddr,const char* param='\0');
//发送ICMP请求回答报文，参数1：套接字.参数2：目的地址.参数3：第num次发送
bool sendICMPReq(SOCKET &mysocket, sockaddr_in &dstAddr, unsigned int num);
//读取ICMP应答报文，参数1：套接字.参数2：接收方地址.参数3：跳数
uint32_t readICMPanswer(SOCKET &mysocket, sockaddr_in &srcAddr, char &TTL);
//等待socket是否由数据需要读取
int waitForSocket(SOCKET &mysocket);
//执行一次ping操作,参数1：套接字.参数2：源地址.参数3：目的地址序号ping报文序号
void doPing(SOCKET &mysocket,sockaddr_in &srcAddr,sockaddr_in &dstAddr,int num);

char* isParamEmpty(char *buffer, char *param);
//捕获终止信号函数
void get_ctrl_stop(int signal);

