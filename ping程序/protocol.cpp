#include "protocol.h"

static volatile int keepping = 1;
//捕获终止信号函数,专门处理无限ping时的操作
void get_ctrl_stop(int signal)
{
	if (signal == SIGINT)
	{
		keepping = 0;
	}
}


uint16_t getCheckSum(void * protocol, char * type)
{
	uint32_t checkSum = 0;
	uint16_t* word = (uint16_t*)protocol;
	uint32_t size = 0;
	if (type == "ICMP") {//计算有多少个字节
		size = (sizeof(ICMPrecvReq));
	}
	while (size >1)//用32位变量来存是因为要存储16位数相加可能发生的溢出情况，将溢出的最高位最后加到16位的最低位上
	{
		checkSum += *word++;
		size -=2;
	}
	if (size == 1) 
	{
		checkSum += *(uint8_t*)word;
	}
	//二进制反码求和运算，先取反在相加和先相加在取反的结果是一样的，所以先全部相加在取反
	//计算加上溢出后的结果
	while (checkSum >> 16) checkSum = (checkSum >> 16) + (checkSum & 0xffff);
	//取反
	return (~checkSum);
}

bool ping(const char * dstIPaddr,const char* param)
{
	SOCKET		rawSocket;//socket
	sockaddr_in srcAddr;//socket源地址
	sockaddr_in dstAddr;//socket目的地址
	int			Ret;//捕获状态值
	char		TTL = '0';//跳数
	
	//生成一个套接字
	//TCP/IP协议族,RAW模式，ICMP协议
	//RAW创建的是一个原始套接字，最低可以访问到数据链路层的数据，也就是说在网络层的IP头的数据也可以拿到了。
	rawSocket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	//设置目标IP地址
	dstAddr.sin_addr.S_un.S_addr = inet_addr(dstIPaddr);
	//端口
	dstAddr.sin_port = htons(0);
	//协议族
	dstAddr.sin_family = AF_INET;

	//提示信息
	std::cout << "正在 Ping " << inet_ntoa(dstAddr.sin_addr) << " 具有 " << sizeof(ICMPrecvReq::data) << " 字节的数据:" << std::endl;
	if(!strcmp(param,"-t"))
	{
		uint32_t i=0;
		
		while (keepping)
		{
			//无限执行ping操作
			doPing(rawSocket, srcAddr, dstAddr,i);
			//睡眠1ms
			Sleep(1000);
			i++;
		}
		keepping = 1;
	}
	else if(!strcmp(param,"no_param"))
	{	//执行4次ping
		for (int i = 0; i < 4; i++)
		{
			doPing(rawSocket, srcAddr, dstAddr, i);
			Sleep(1000);
		 }
	}
	
	Ret = closesocket(rawSocket);
	if (Ret == SOCKET_ERROR)
	{
		std::cerr << "socket关闭时发生错误:" << WSAGetLastError() << std::endl;
	}
	
	return true;
}

bool sendICMPReq(SOCKET &mysocket,sockaddr_in &dstAddr,unsigned int num)
{
	//创建ICMP请求回显报文
	//设置回显请求
	ICMPrecvReq myIcmp;//ICMP请求报文
	myIcmp.icmphead.type = 8;
	myIcmp.icmphead.code = 0;
	//设置初始检验和为0
	myIcmp.icmphead.checkSum = 0;
	//获得一个进程标识，这样就能够根据校验来识别接收到的是不是这个进程发出去的ICMP报文
	myIcmp.icmphead.ident = (uint16_t)GetCurrentProcessId();
	//设置当前序号为0
	myIcmp.icmphead.seqNum = ++num;
	//保存发送时间
	myIcmp.timeStamp = GetTickCount();
	//计算并且保存校验和
	myIcmp.icmphead.checkSum = getCheckSum((void*)&myIcmp, "ICMP");
	//发送报文
	int Ret = sendto(mysocket, (char*)&myIcmp, sizeof(ICMPrecvReq), 0, (sockaddr*)&dstAddr, sizeof(sockaddr_in));
	
	if (Ret == SOCKET_ERROR)
	{
		std::cerr << "socket 发送错误:" <<WSAGetLastError()<< std::endl;
		return false;
	}
	return true;
}

uint32_t readICMPanswer(SOCKET &mysocket, sockaddr_in &srcAddr, char &TTL)
{
	ICMPansReply icmpReply;//接收应答报文
	int addrLen = sizeof(sockaddr_in);
	//接收应答
	int Ret = recvfrom(mysocket, (char*)&icmpReply, sizeof(ICMPansReply), 0, (sockaddr*)&srcAddr, &addrLen);
	if (Ret == SOCKET_ERROR)
	{
		std::cerr << "socket 接收错误:" << WSAGetLastError() << std::endl;
		return false;
	}
	//读取校验并重新计算对比
	uint16_t checkSum = icmpReply.icmpanswer.icmphead.checkSum;
	//因为发出去的时候计算的校验和是0
	icmpReply.icmpanswer.icmphead.checkSum = 0;
	//重新计算
	if (checkSum== getCheckSum((void*)&icmpReply.icmpanswer, "ICMP")) {
		//获取TTL值
		TTL = icmpReply.iphead.timetoLive;
		return icmpReply.icmpanswer.timeStamp;
	}
	
	return -1;
}

int waitForSocket(SOCKET &mysocket)
{
	//5S 等待套接字是否由数据
	timeval timeOut;
	fd_set	readfd;
	readfd.fd_count = 1;
	readfd.fd_array[0] = mysocket;
	timeOut.tv_sec = 5;
	timeOut.tv_usec = 0;
	return (select(1, &readfd, NULL, NULL, &timeOut));
}

void doPing(SOCKET & mysocket, sockaddr_in & srcAddr, sockaddr_in & dstAddr, int num)
{
	uint32_t	timeSent;//发送时的时间
	uint32_t	timeElapsed;//延迟时间
	char		TTL;//跳数
	//发送ICMP回显请求
	sendICMPReq(mysocket, dstAddr, num);
	//等待数据
	int Ret = waitForSocket(mysocket);
	if (Ret == SOCKET_ERROR)
	{
		std::cerr << "socket发生错误:" << WSAGetLastError() << std::endl;
		return;
	}
	if (!Ret)
	{
		std::cout << "请求超时:" << std::endl;
		return;
	}
	timeSent = readICMPanswer(mysocket, srcAddr, TTL);
	if (timeSent != -1) {
		timeElapsed = GetTickCount() - timeSent;
		//输出信息，注意TTL值是ASCII码，要进行转换
		std::cout << "来自 " << inet_ntoa(srcAddr.sin_addr) << " 的回复: 字节= " << sizeof(ICMPrecvReq::data) << " 时间= " << timeElapsed << "ms TTL= " << fabs((int)TTL) << std::endl;
	}
	else {
		std::cout << "请求超时" << std::endl;
	}
	
}

char *isParamEmpty(char *buffer, char *param)
{
	char *temp = NULL;
	temp = buffer;
	while (*temp != '\0')
	{
		if (*temp == ' ')
		{
			*temp = '\0';
			param = ++temp;
		}
		temp++;
	}
	return param;
}


