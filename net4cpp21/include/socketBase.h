/*******************************************************************
   *	socketBase.h
   *    DESCRIPTION:socket基类封装类的定义
   *				支持IPV4和IPV6，如果要支持IPV6需定义IPPROTO_IPV6
   *				要创建IPV6 socket，需设置SetIpv6(true);
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2004-10-10
   *	Last modify: 2005-09-01
   *	net4cpp 2.1
   *******************************************************************/

#ifndef __YY_CSOCKETBASE_H__
#define __YY_CSOCKETBASE_H__

#include "socketdef.h"

namespace net4cpp21
{
	class socketBase //socket操作的基类
	{
	public:
		socketBase();
		socketBase(socketBase &sockB);
		socketBase & operator = (socketBase &sockB);
		virtual ~socketBase();
		virtual void Close();
		socketBase *parent() const { return m_parent; }
		void setParent(socketBase * psock) { m_parent=psock; return; }
		long getErrcode() const { return m_errcode;}
		int  getFlag() const { return m_sockflag; }
		SOCKETTYPE type() const { return m_socktype; }
		SOCKETSTATUS status() const { return m_sockstatus; }
		int getLocalPort() const { return ntohs(m_localAddr.sin_port); }
		int getRemotePort() const { return ntohs(m_remoteAddr.sin_port); }
		const char *getLocalIP() const { return inet_ntoa(m_localAddr.sin_addr); }
		const char *getRemoteIP() const { return inet_ntoa(m_remoteAddr.sin_addr); }
		unsigned long getLocalip() const { return m_localAddr.sin_addr.S_un.S_addr; }
		unsigned long getRemoteip() const { return m_remoteAddr.sin_addr.S_un.S_addr; }
		time_t getStartTime() const { return m_tmOpened; } //返回socket打开开始时间
		SOCKSRESULT setLinger(bool bEnabled,time_t iTimeout=5); //s
		
		SOCKSRESULT setRemoteInfo(const char *host,int port);
		void SetRemoteInfo(unsigned long ipAddr,int port)
		{
			m_remoteAddr.sin_port=htons(port);
			m_remoteAddr.sin_addr.s_addr=ipAddr;
		}
		
		unsigned long getMaxSendRatio() const { return m_maxSendRatio; }
		unsigned long getMaxRecvRatio() const { return m_maxRecvRatio; }
		//设置发送或接收流量速度 Bytes/s
		void setSpeedRatio(unsigned long sendRatio,unsigned long recvRatio)
		{
			m_maxSendRatio=sendRatio;
			m_maxRecvRatio=recvRatio;
			return;
		}
		//检查本是否可读/可写。
		//如果可读或可写则返回1，超时返回0，否则返回错误，如果返回-1则发生系统错误
		int checkSocket(time_t wait_usec,SOCKETOPMODE opmode)
		{
			if(m_sockfd==INVALID_SOCKET) return SOCKSERR_INVALID;
			if(m_parent && m_parent->m_sockstatus<=SOCKS_CLOSED)
				return SOCKSERR_PARENT;
			int fd=m_sockfd;
			int iret=checkSocket(&fd,1,wait_usec,opmode);
			if(iret==SOCKET_ERROR) m_errcode=SOCK_M_GETERROR;
			return iret;
		}
		//接收数据(TCP/UDP/RAW),返回接收数据的长度
		SOCKSRESULT Receive(char *buf,size_t buflen,time_t lWaitout)
		{
			return _Receive(buf,buflen,lWaitout,SOCKS_OP_READ);
		}
		SOCKSRESULT Peek(char *buf,size_t buflen,time_t lWaitout)
		{
			return _Receive(buf,buflen,lWaitout,SOCKS_OP_PEEK);
		}
		//接收带外数据
		SOCKSRESULT RecvOOB(char *buf,size_t buflen,time_t lWaitout)
		{
			return _Receive(buf,buflen,lWaitout,SOCKS_OP_ROOB);
		}
		//向目的发送数据,返回发送数据的大小，如果<0则发生错误
		SOCKSRESULT Send(size_t buflen,const char *buf,time_t lWaitout=-1);
		SOCKSRESULT Send(LPCTSTR fmt,...);
		SOCKSRESULT SendOOB(size_t buflen,const char *buf);
		//得到本机IP，返回得到本机IP的个数
		static long getLocalHostIP(std::vector<std::string> &vec);
		static const char *getLocalHostIP();

		//解析指定的域名 ,only for IPV4
		static unsigned long Host2IP(const char *host);
		static const char *IP2A(unsigned long ipAddr)
		{
			struct in_addr in;
			in.S_un.S_addr =ipAddr;
			return inet_ntoa(in);
		}

	protected:
		bool m_ipv6; //是否支持IPV6
		time_t m_tmOpened;//本socket打开的时间，用于限制流量

		socketBase *m_parent; //关联父socket指针
		long m_errcode;//错误代码 系统错误代码或自定义错误代码
		int m_sockfd;//socket的访问句柄
		int m_sockflag; //保留socks的而外标志，位0只是TCP的连接方向
		SOCKETTYPE m_socktype;//socket句柄的类型
		SOCKETSTATUS m_sockstatus;//socket句柄的状态
		SOCKADDR_IN m_localAddr;//本socket绑定的本机端口和ip
		SOCKADDR_IN m_remoteAddr;//本socket连接的远端端口和ip(only for tcp)
							//对于非tcp类型的socket，此处保存接收到数据的远端端口和ip
							//或要发送数据的远端端口和ip
		unsigned long m_recvBytes;//总接收字节数
		unsigned long m_sendBytes;//总发送字节数
		unsigned long m_maxSendRatio;//最大发送流量 Bytes/秒，0-不限流量
		unsigned long m_maxRecvRatio;//最大接收流量 Bytes/秒，0-不限流量

		int getAF();
		int getSocketInfo();
		static int checkSocket(int *sockfds,size_t len,time_t wait_usec,SOCKETOPMODE opmode);
	protected:

		SOCKSRESULT setNonblocking(bool bNb);
		bool create(SOCKETTYPE socktype);
		SOCKSRESULT Bind(int port,BOOL bReuseAddr,const char *bindip);
		SOCKSRESULT Bind(int startport,int endport,BOOL bReuseAddr,const char *bindip);
		SOCKSRESULT _Receive(char *buf,size_t buflen,time_t lWaitout,SOCKETOPMODE opmode);
		SOCKSRESULT _Send(const char *buf,size_t buflen,time_t lWaitout);
		virtual size_t v_read(char *buf,size_t buflen);
		virtual size_t v_peek(char *buf,size_t buflen);
		virtual size_t v_write(const char *buf,size_t buflen);
		size_t v_writeto(const char *buf,size_t buflen,SOCKADDR_IN &addr);
	};

	//初始化windows的网络环境----------------------------
	class NetEnv
	{
		WSADATA m_wsadata;
		bool m_bState;
	public:
		NetEnv();
		~NetEnv();
		bool getState(){return m_bState;}
		static NetEnv &getInstance();
	};
	//初始化windows的网络环境----------------------------
}//?namespace net4cpp21

#endif