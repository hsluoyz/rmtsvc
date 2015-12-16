/*******************************************************************
   *	socketdef.h
   *    DESCRIPTION:socket类所用到的常量、结构以及enum的定义
   *				
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2004-10-10
   *	Last modify: 2005-09-01
   *	net4cpp 2.1
   *******************************************************************/

#ifndef __YY_CSOCKETDEF_H__
#define __YY_CSOCKETDEF_H__

//#define IPPROTO_IPV6

#ifdef WIN32 //windows系统平台
	//socket errno
	#define        ENOTSOCK        WSAEOPNOTSUPP
	#define        ECONNRESET      WSAECONNRESET
	#define        ENOTCONN        WSAENOTCONN
	#define	       EINPROGRESS	WSAEINPROGRESS
	//#define      EBADF           WSAENOTSOCK
	//#define      EPIPE            WSAESHUTDOWN
	//#define	       MSG_NOSIGNAL    0  //windows下没有此定义  //在sysconfig.h中定义
	#define        SOCK_M_GETERROR WSAGetLastError() //get wrong code
			
	#define socklen_t int
	#pragma comment(lib,"ws2_32") //静态连接ws2_32.dll动态库的lib库
#elif defined MAC //暂时不支持
	//....
#else  //unix/linux平台
	extern "C"
	{
		//包含socket通讯库函数
		#include <unistd.h>  //::close(fd) --- close socket
		#include <sys/types.h>
		#include <sys/socket.h>
		#include <netinet/in.h>
		#include <arpa/inet.h>
		#include <netdb.h>
	}//?extern "C"
	//在Sun OS5.8下编译会报找不到INADDR_NONE的定义(未包含相应头文件)，因此手工定义
	#ifndef INADDR_NONE
		#define INADDR_NONE             ((unsigned long int) 0xffffffff)
	#endif
	
	#define closesocket close
	#define ioctlsocket ioctl	
	#define SOCK_M_GETERROR errno //get wrong code
	#define WSAEACCES     EACCES		
	#define WSADATA long
#endif

#ifndef SD_SEND
#define SD_RECEIVE 0x00		//不允许继续接收数据 
#define SD_SEND 0x01		//不允许继续发送数据 
#define SD_BOTH 0x02
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

#ifndef SO_EXCLUSIVEADDRUSE
#define SO_EXCLUSIVEADDRUSE   (u_int)(~SO_REUSEADDR)
#endif

#define SSENDBUFFERSIZE 1460 //最佳发送缓冲大小
							 //在SOCK_STREAM方式下，如果单次发送数据超过1460，系统将分成多个数据报传送，
							//在对方接受到的将是一个数据流，应用程序需要增加断帧的判断。当然可以采用修改
							//注册表的方式改变1460的大小，但MicrcoSoft认为1460是最佳效率的参数，不建议修改。 

#define MAXRATIOTIMEOUT 600000 //us 流量超限制延迟 600ms
#define SCHECKTIMEOUT 200000 //us,select的超时时间 200ms

#define SOCKSRESULT int
#define SOCKSERR_OK 0
#define SOCKSERR_ERROR -1 //发生系统错误，通过getErrcode获得错误代码
#define SOCKSERR_INVALID -2 //无效的socket句柄
#define SOCKSERR_CLOSED -3 //对方已经关闭了连接
#define SOCKSERR_HOST -4 //无效的主机名，或主机名无法解析
#define SOCKSERR_BIND -5 //绑定错误
#define SOCKSERR_SETOPT -6 //调用setsockopt发生错误
#define SOCKSERR_CONN -7 //连接失败
#define SOCKSERR_LISTEN -8 //侦听失败
#define SOCKSERR_SNIFF -9 //sniffer失败
#define SOCKSERR_ZEROLEN -10 //发送或接收数据返回0

#define SOCKSERR_PARAM -11 //无效的参数
#define SOCKSERR_BUFFER -12 //buffer缓冲错误
#define SOCKSERR_TIMEOUT -13 //超时
#define SOCKSERR_EACCES	-14 //指定的地址是一个广播地址，但没有设置广播标志
#define SOCKSERR_THREAD -15 //创建线程执行任务失败
#define SOCKSERR_NOTSURPPORT -16 //不支持此功能
#define SOCKSERR_MEMORY -17 //内存分配错误
#define SOCKSERR_SSLASSCIATE -18 //SSL协商失败
#define SOCKSERR_PARENT -19 //父socket关闭或错误

#define SOCKS_TCP_IN 1    //指示TCP的连接方向

#include <vector>
#include <string>
#include <ctime>

namespace net4cpp21
{
	typedef enum //socket句柄的类型
	{
		SOCKS_NONE,//未创建，无效的socket句柄
		SOCKS_TCP,
		SOCKS_UDP,
		SOCKS_RAW //原始socket
	}SOCKETTYPE;
	typedef enum //socket句柄的连接状态
	{
		SOCKS_ERROR, //TCP连接异常，有问题
		SOCKS_CLOSED, //已关闭
		SOCKS_LISTEN, //TCP 侦听状态
		SOCKS_CONNECTED,//已建立TCP连接
		SOCKS_OPENED//udp或原始套接字已打开
	}SOCKETSTATUS;
	typedef enum
	{
		SOCKS_OP_PEEK,
		SOCKS_OP_READ,
		SOCKS_OP_WRITE,
		SOCKS_OP_ROOB,//读OOB数据
		SOCKS_OP_WOOB //写OOB数局
	}SOCKETOPMODE; //对socket的操作模式
	typedef enum //SSL初始化类型
	{
		SSL_INIT_NONE=0, //未初始化SSL
		SSL_INIT_SERV, //服务端
		SSL_INIT_CLNT  //客户端
	}SSL_INIT_TYPE;
	//阻塞处理时的判断函数定义,如果此函数返回假则退出等待，超时
	typedef bool (FUNC_BLOCK_HANDLER)(void *);

}//?namespace net4cpp21

#endif
