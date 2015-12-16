/*******************************************************************
   *	socketBase.cpp
   *    DESCRIPTION:socket基类封装类的定义
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2004-10-10
   *	Last modify: 2005-09-01
   *	net4cpp 2.1
   *******************************************************************/

#include "include/sysconfig.h"
#include "include/socketBase.h"
#include "include/cLogger.h"

#include <cstdarg>
#include <cerrno>

using namespace std;
using namespace net4cpp21;

socketBase::socketBase()
{
	m_sockfd=INVALID_SOCKET;
	m_socktype=SOCKS_NONE;
	m_sockstatus=SOCKS_CLOSED;
	m_sockflag=0;
	m_ipv6=false;
	memset((void *)&m_remoteAddr,0,sizeof(SOCKADDR_IN));
	memset((void *)&m_localAddr,0,sizeof(SOCKADDR_IN));
	m_recvBytes=0;
	m_sendBytes=0;
	m_maxSendRatio=m_maxRecvRatio=0;
	m_tmOpened=0;
	m_errcode=SOCKSERR_OK;
	m_parent=NULL;
}

socketBase :: socketBase(socketBase &sockB)
{
	m_sockfd=sockB.m_sockfd;
	m_socktype=sockB.m_socktype;
	m_sockstatus=sockB.m_sockstatus;
	m_sockflag=sockB.m_sockflag;
	m_ipv6=sockB.m_ipv6;
	m_remoteAddr=sockB.m_remoteAddr;
	m_localAddr=sockB.m_localAddr;
	m_recvBytes=sockB.m_recvBytes;
	m_sendBytes=sockB.m_sendBytes;
	m_maxSendRatio=sockB.m_maxSendRatio;
	m_maxRecvRatio=sockB.m_maxRecvRatio;
	m_tmOpened=sockB.m_tmOpened;
	m_errcode=sockB.m_errcode;
	m_parent=sockB.m_parent;
	
	sockB.m_sockfd=INVALID_SOCKET;
	sockB.m_socktype=SOCKS_NONE;
	sockB.m_sockstatus=SOCKS_CLOSED;
	memset((void *)&sockB.m_localAddr,0,sizeof(SOCKADDR_IN));
}

socketBase& socketBase:: operator = (socketBase &sockB)
{
	m_sockfd=sockB.m_sockfd;
	m_socktype=sockB.m_socktype;
	m_sockstatus=sockB.m_sockstatus;
	m_sockflag=sockB.m_sockflag;
	m_ipv6=sockB.m_ipv6;
	m_remoteAddr=sockB.m_remoteAddr;
	m_localAddr=sockB.m_localAddr;
	m_recvBytes=sockB.m_recvBytes;
	m_sendBytes=sockB.m_sendBytes;
	m_maxSendRatio=sockB.m_maxSendRatio;
	m_maxRecvRatio=sockB.m_maxRecvRatio;
	m_tmOpened=sockB.m_tmOpened;
	m_errcode=sockB.m_errcode;
	m_parent=sockB.m_parent;
	
	sockB.m_sockfd=INVALID_SOCKET;
	sockB.m_socktype=SOCKS_NONE;
	sockB.m_sockstatus=SOCKS_CLOSED;
	memset((void *)&sockB.m_localAddr,0,sizeof(SOCKADDR_IN));
	return *this;
}

socketBase::~socketBase()
{
	Close(); 
}

void socketBase :: Close()
{
	if(m_socktype==SOCKS_NONE) return;
	m_socktype=SOCKS_NONE;
	m_sockflag=0;
	if ( m_sockfd!=INVALID_SOCKET ){
//若设置了SO_LINGER（亦即linger结构中的l_onoff域设为非零，参见2.4，4.1.7和4.1.21各节），并设置了零超时间隔，
//则closesocket()不被阻塞立即执行，不论是否有排队数据未发送或未被确认。这种关闭方式称为“强制”或“失效”关闭，
//因为套接口的虚电路立即被复位，且丢失了未发送的数据。在远端的recv()调用将以WSAECONNRESET出错。
//若设置了SO_LINGER并确定了非零的超时间隔，则closesocket()调用阻塞进程，直到所剩数据发送完毕或超时。
//这种关闭称为“优雅的”关闭。请注意如果套接口置为非阻塞且SO_LINGER设为非零超时，则closesocket()调用将
//以WSAEWOULDBLOCK错误返回。
//windows系统默认SO_DONTLINGER，，即温和关闭
//		::shutdown(m_sockfd,SD_BOTH); 
		::closesocket(m_sockfd);
		m_sockfd=INVALID_SOCKET;
	}//?if ( m_sockfd!=INVALID_SOCKET ){
	m_sockstatus=SOCKS_CLOSED;
	m_localAddr.sin_port=0;
	m_localAddr.sin_addr.s_addr=INADDR_ANY;
	m_recvBytes=m_sendBytes=0;
	//yyc remove 2006-02-15,关闭的时候不清空远程地址信息
	//事先通过setRemoteInfo设置,防止create时清掉
//	m_remoteAddr.sin_port=0;
//	m_remoteAddr.sin_addr.s_addr=INADDR_ANY;
	return;
}

//设置要连接或UPD发送的远程主机信息
SOCKSRESULT socketBase :: setRemoteInfo(const char *host,int port)
{
	if(host==NULL || port<=0) return SOCKSERR_PARAM;
	//判断是否为一个有效的IP
	unsigned long ipAddr=socketBase::Host2IP(host);
	if(ipAddr==INADDR_NONE) return SOCKSERR_HOST;
	m_remoteAddr.sin_port=htons(port);
	m_remoteAddr.sin_addr.s_addr=ipAddr;
	return SOCKSERR_OK;
}

//返回发送数据的大小，如果<0则发生错误
SOCKSRESULT socketBase :: Send(LPCTSTR fmt,...)
{
	if(m_sockstatus<SOCKS_CONNECTED) return SOCKSERR_INVALID;
	TCHAR buf[2048];
	va_list args;
	va_start(args,fmt);
	int len=vsnprintf(buf,sizeof(buf)-1,fmt,args);
	va_end(args);
	if(len==-1) return SOCKSERR_BUFFER; //len=sizeof(buf)-1; 
	buf[len]=0;
	return (len>0)?_Send((const char *)buf,len*sizeof(TCHAR),-1):0;
}
SOCKSRESULT socketBase :: Send(size_t buflen,const char * buf,time_t lWaitout)
{
	if(m_sockstatus<SOCKS_CONNECTED) return SOCKSERR_INVALID;
	if(buf==NULL) return SOCKSERR_PARAM;
	if(buflen==0) if( (buflen=strlen(buf))==0) return SOCKSERR_PARAM;
	return (buflen>0)?_Send((const char *)buf,buflen,lWaitout):0;
}
//发送带外数据
SOCKSRESULT socketBase :: SendOOB(size_t buflen,const char *buf)
{
	if(m_sockstatus!=SOCKS_CONNECTED) return SOCKSERR_INVALID;
	if(buf==NULL) return SOCKSERR_PARAM;
	if(buflen==0) if( (buflen=strlen(buf))==0) return SOCKSERR_PARAM;
	buflen=::send(m_sockfd,buf,buflen,MSG_NOSIGNAL|MSG_OOB);
	if(buflen>=0)  return buflen; 
	
	m_errcode=SOCK_M_GETERROR;
	return SOCKSERR_ERROR;//发生系统错误，通过SOCK_M_GETERROR获得错误代码
}

//--------------------------static function---------------------------------------

//得到本机IP，返回得到本机IP的个数
long socketBase :: getLocalHostIP(vector<string> &vec)
{
	char buf[64];
	gethostname(buf,sizeof(buf));
	struct hostent * p=gethostbyname(buf);
	if(p==NULL) return 0;
	for(int i=0;p->h_addr_list[i];i++)
		vec.push_back( (char *)inet_ntoa(*((struct in_addr *)p->h_addr_list[i])) );
	return i;
}
const char *socketBase :: getLocalHostIP()
{
	char buf[64];
	gethostname(buf,sizeof(buf));
	struct hostent * p=gethostbyname(buf);
	if(p==NULL) return 0;
	return inet_ntoa(*((struct in_addr *)p->h_addr_list[0]));
}

//解析指定的域名,only for IPV4
unsigned long socketBase :: Host2IP(const char *host)
{
	unsigned long ipAddr=inet_addr(host);
	if(ipAddr!=INADDR_NONE) return ipAddr;
	//指定的不是一个有效的ip地址可能是一个主机域名
	struct hostent * p=gethostbyname(host);
	if(p==NULL) return INADDR_NONE;
	ipAddr=(*((struct in_addr *)p->h_addr)).s_addr;
	return ipAddr;
}

//windows下默认的FD_SETSIZE只有64大小!!!注意checkSocket中待检查的socket不能超过此值的设定
//!!!!!#define FD_SETSIZE      64 
//检查指定的sockets是否可读或可写
//sockfds --- 要检查的socket句柄数组，len-sockets句柄的个数
//wait_usec --- 检查超时等待时间，微秒
//opmode --- 检查socket是否可读/写/有OOB带外数据
//返回 --- 返回可读/可写的socket句柄个数
//	--- 如果返回小于0则说明发生了错误
//	此函数会改写sockfds数组中的值，如果某个socket句柄可读/可写则置1否则置0
//windows/linux,unix下FD_SET的实现差别：
//windows下默认的FD_SETSIZE为64，FD_SET宏遍历SOCKETs数组，如果发现
//此fd句柄在SOCKETs中已经存在，则退出，否则将fd插入到数组后面，同时计数++
//linux,unix下默认的FD_SETSIZE为65535，FD_SET宏将socket的句柄值插入到以fd值为下标
//的SOCKETs的数组位置。因此linux,unix下调用FD_SET宏时要进行fd<FD_SETSIZE检查。
//在linux,unix下socket的句柄fd的值如果大于＝FD_SETSIZE是无效的
int socketBase :: checkSocket(int *sockfds,size_t len,time_t wait_usec,SOCKETOPMODE opmode)
{
	int i,retv=0;
	struct timeval to;
	fd_set fds; FD_ZERO(&fds);
	for(i=0;i<len;i++)
	{
		FD_SET(sockfds[i], &fds);
	}
	if ( wait_usec>0)
    {
		to.tv_sec =wait_usec/1000000L;
		to.tv_usec =wait_usec%1000000L;
    }
	else
	{
		to.tv_sec = 0;
		to.tv_usec = 0;
	}
	if(opmode<=SOCKS_OP_READ) //判断是否有数据可读
		retv = select(FD_SETSIZE, &fds, NULL, NULL, &to);
	else if(opmode==SOCKS_OP_WRITE) //判断是否可写
		retv = select(FD_SETSIZE, NULL, &fds, NULL, &to);
	else if(opmode==SOCKS_OP_ROOB) //是否有OOB数据可读
		retv = select(FD_SETSIZE, NULL, NULL, &fds, &to);
	else return 1; //其他超作永远为真，例如写带外数据!!!!!
	if(retv==0) return 0; // timeout occured 
	if(retv==SOCKET_ERROR){ //有错误发生
		if(SOCK_M_GETERROR==EINPROGRESS) return 0;//正在处理过程中，不算错误
	}
	//对于其他的错误则直接返回,交由用户处理判断
	//有句柄可读或可写
	for(i=0;i<len;i++)
	{
		sockfds[i]=(FD_ISSET(sockfds[i], &fds))?1:0;
	}
	return retv;
}

//--------------------------private function---------------------------------------

inline int socketBase :: getAF()
{
	int af=AF_INET;
#ifdef IPPROTO_IPV6
	if(m_ipv6) af=AF_INET6;
#endif
	return af;
}

//获取本socket绑定的本地ip和端口，成功返回绑定的本地端口
//返回<=0则发生错误
int socketBase :: getSocketInfo()
{
//	if(m_sockfd==INVALID_SOCKET) return SOCKSERR_INVALID;
	//getsockname()函数用于获取一个套接口的名字。它用于一个已捆绑或已连接套接口s，本地地址将被返回。
	//本调用特别适用于如下情况：未调用bind()就调用了connect()，
	//这时唯有getsockname()调用可以获知系统内定的本地地址。
	//在返回时，namelen参数包含了名字的实际字节数。
	//若一个套接口与INADDR_ANY捆绑，也就是说该套接口可以用任意主机的地址，
	//此时除非调用connect()或accept()来连接，否则getsockname()将不会返回主机IP地址的任何信息。
	//除非套接口被连接，WINDOWS套接口应用程序不应假设IP地址会从INADDR_ANY变成其他地址。
	//这是因为对于多个主机环境下，除非套接口被连接，否则该套接口所用的IP地址是不可知的。
	int addr_len=sizeof(m_localAddr);
	m_localAddr.sin_family=getAF();
	m_localAddr.sin_port=0; m_localAddr.sin_addr.s_addr=0;
	getsockname(m_sockfd,(struct sockaddr *) &m_localAddr,(socklen_t *)&addr_len);
	return ntohs(m_localAddr.sin_port);
}

//绑定指定的端口和IP
//bReuse -- 指定是否可重用端口 bindip --- 指定要绑定的ip地址，如果为NULL/""则绑定所有的
//成功返回实际绑定的端口 >0
SOCKSRESULT socketBase :: Bind(int port,BOOL bReuseAddr,const char *bindip)
{
	if(m_sockfd==INVALID_SOCKET) return SOCKSERR_INVALID;
	if(bReuseAddr==SO_REUSEADDR){//可以绑定允许重用的端口
		int on=1;
		setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &on,sizeof(on));
	}else if(bReuseAddr==SO_EXCLUSIVEADDRUSE){ //绑定的端口禁止重用
		int on=1;
		setsockopt(m_sockfd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char *) &on,sizeof(on));
	}//否则不可以绑定允许重用的端口,本次绑定的端口允许重用

	SOCKADDR_IN addr; memset(&addr, 0, sizeof(addr));
	addr.sin_family = getAF();
	addr.sin_port =(port<=0)?0:htons(port);
	if(bindip && bindip[0]!=0) addr.sin_addr.s_addr=inet_addr(bindip);
	//否则 侦听绑定本机任何地址INADDR_ANY

	if (bind(m_sockfd, (struct sockaddr *) &addr, sizeof(addr)) == SOCKET_ERROR ) 
	{
		m_errcode=SOCK_M_GETERROR;
		return SOCKSERR_BIND;
	}
	return getSocketInfo();
}
//绑定指定的端口，端口在[startport,endport]之间随机选定
//startport>=0, endport<=65535.如果endport<=0则=65535
//返回实际绑定的端口
SOCKSRESULT socketBase :: Bind(int startport,int endport,BOOL bReuseAddr,const char *bindip)
{
	if(m_sockfd==INVALID_SOCKET) return SOCKSERR_INVALID;
	if(bReuseAddr==SO_REUSEADDR){//可以绑定允许重用的端口
		int on=1;
		setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &on,sizeof(on));
	}else if(bReuseAddr==SO_EXCLUSIVEADDRUSE){ //绑定的端口禁止重用
		int on=1;
		setsockopt(m_sockfd, SOL_SOCKET, SO_EXCLUSIVEADDRUSE, (char *) &on,sizeof(on));
	}//否则不可以绑定允许重用的端口,本次绑定的端口允许重用
	
	SOCKADDR_IN addr; memset(&addr, 0, sizeof(addr));
	addr.sin_family = getAF();
	if(bindip && bindip[0]!=0) addr.sin_addr.s_addr=inet_addr(bindip);
	//否则 侦听绑定本机任何地址INADDR_ANY
	if(startport<1) startport=1;
	if(endport<1 || endport>65535) endport=65535;
	if(endport<startport){ int iswap=startport; startport=endport; endport=iswap; }
	
	int icount=endport-startport;
	if(icount<10)
	{
		for(int i=0;i<=icount;i++)
		{
			addr.sin_port=htons(startport+i);
			if (bind(m_sockfd, (struct sockaddr *) &addr, sizeof(addr)) != SOCKET_ERROR )
				break;
		}
		if(i>icount) return SOCKSERR_BIND;
	}
	else
	{
		for(int i=1;i<=10;i++)
		{
			int port=startport+rand()*icount/RAND_MAX;
			addr.sin_port=htons(port);
			if (bind(m_sockfd, (struct sockaddr *) &addr, sizeof(addr)) != SOCKET_ERROR )
				break;
		}
		if(i>10) return SOCKSERR_BIND;
	}
	return getSocketInfo();
}
//使能/禁止SO_LINGER，如果使能SO_LINGER，则关闭socket句柄时必须等待缓冲区内数据发送完毕
//iTimeout 秒--- 如果使能SO_LINGER，则指定如果多长时间内缓冲区内数据如果没有发送完毕则强行关闭
SOCKSRESULT socketBase::setLinger(bool bEnabled,time_t iTimeout)
{
	if(m_sockfd==INVALID_SOCKET) return SOCKSERR_INVALID;
	if(m_sockstatus<SOCKS_CONNECTED) return SOCKSERR_OK;
	int sr=SOCKSERR_OK;
	//使能SO_LINGER
	//When  enabled,  a  close(2) or shutdown(2) will not return until all queued messages for the socket have
    //      been successfully sent or the linger timeout has been reached. Otherwise, the call  returns  immediately
    //     and  the  closing  is  done  in the background.  When the socket is closed as part of exit(2), it always
    //      lingers in the background.
	struct linger lg;
	if(bEnabled)
		lg.l_onoff=1;
	else
		lg.l_onoff=0;
	lg.l_linger=iTimeout;//设置超时最大时间s
	if(setsockopt(m_sockfd,SOL_SOCKET,SO_LINGER,(const char *)&lg,sizeof(lg))!=0)
		return SOCKSERR_SETOPT;
	return SOCKSERR_OK;
}
//设置socket阻塞/非阻塞
//成功返回SOCKSERR_OK
SOCKSRESULT socketBase::setNonblocking(bool bNb)
{
	if(m_sockfd==INVALID_SOCKET) return SOCKSERR_INVALID;
	int sr=SOCKSERR_OK;
#ifdef WIN32
	unsigned long l = bNb ? 1 : 0; //1 -- 非阻塞
	sr=ioctlsocket(m_sockfd,FIONBIO,&l); //设置socket为阻塞/非阻塞方式
	if(sr!=0) sr=SOCKSERR_ERROR;
#else
	if (bNb)
	{
		sr=fcntl(s, F_SETFL, O_NONBLOCK);
	}
	else
	{
		sr=fcntl(s, F_SETFL, 0);
	}
#endif
	//发生系统错误，通过SOCK_M_GETERROR获得错误代码
	if(sr==SOCKSERR_ERROR) m_errcode=SOCK_M_GETERROR;
	return sr;
}

//创建指定类型的socket句柄
//成功返回真否则返回假
bool socketBase :: create(SOCKETTYPE socktype) 
{
	if( type()!=SOCKS_NONE ) Close();
	m_localAddr.sin_family=getAF();
	m_remoteAddr.sin_family=getAF();
	int af=m_localAddr.sin_family;
	switch(socktype)
	{
		case SOCKS_TCP:
			if( (m_sockfd=::socket(af,SOCK_STREAM,0))!=INVALID_SOCKET )
				m_socktype=SOCKS_TCP;
			break;
		case SOCKS_UDP:
			if( (m_sockfd=::socket(af,SOCK_DGRAM,0))!=INVALID_SOCKET )
			{
				m_socktype=SOCKS_UDP;
				int on=1; //Allows transmission of broadcast messages on the socket.
						// 支持广播发送,目的地址是255.255.255.255,意味着向本网段所有机器发送
				setsockopt(m_sockfd, SOL_SOCKET, SO_BROADCAST, (char *) &on,sizeof(on));
			}
			break;
		case SOCKS_RAW:
			break;
	}//?switch(socktype)
	if(m_socktype==SOCKS_NONE) return false;
	m_tmOpened=time(NULL);
	return true;
}
//注意:对于面向消息socket(message-oriented) 一次发送的数据长度不能大于socket的发送缓冲区的大小，否则返回SOCKET_ERROR
//一般系统默认的socket的发送缓冲区的SO_MAX_MSG_SIZE 
/*inline size_t socketBase :: v_write(const char *buf,size_t buflen)
{
	size_t len=0;
	if(m_socktype==SOCKS_TCP)
		len=::send(m_sockfd,buf,buflen,MSG_NOSIGNAL);
	else
		len=::sendto(m_sockfd,buf,buflen,MSG_NOSIGNAL,(struct sockaddr *)&m_remoteAddr,sizeof(SOCKADDR_IN));
	return len;
}*/
inline size_t socketBase :: v_write(const char *buf,size_t buflen)
{
	return ::send(m_sockfd,buf,buflen,MSG_NOSIGNAL);
}
inline size_t socketBase :: v_writeto(const char *buf,size_t buflen,SOCKADDR_IN &addr)
{
	return ::sendto(m_sockfd,buf,buflen,MSG_NOSIGNAL,(struct sockaddr *)&addr,sizeof(SOCKADDR_IN));
}

inline size_t socketBase :: v_read(char *buf,size_t buflen)
{
	size_t len=0;
	if(m_socktype==SOCKS_TCP)
		len=::recv(m_sockfd,buf,buflen,MSG_NOSIGNAL);
	else
	{
		m_remoteAddr.sin_addr.S_un.S_addr=INADDR_ANY;
		m_remoteAddr.sin_port=0;
		int addrlen=sizeof(m_remoteAddr);
		len=::recvfrom(m_sockfd,buf,buflen,MSG_NOSIGNAL,
			(struct sockaddr *) &m_remoteAddr, (socklen_t *)&addrlen);
	}	
	return len;
}
inline size_t socketBase :: v_peek(char *buf,size_t buflen)
{
	size_t len=0;
	if(m_socktype==SOCKS_TCP)
		len=::recv(m_sockfd,buf,buflen,MSG_NOSIGNAL|MSG_PEEK);
	else
	{
		m_remoteAddr.sin_addr.S_un.S_addr=INADDR_ANY;
		m_remoteAddr.sin_port=0;
		int addrlen=sizeof(m_remoteAddr);
		len=::recvfrom(m_sockfd,buf,buflen,MSG_NOSIGNAL|MSG_PEEK,
			(struct sockaddr *) &m_remoteAddr, (socklen_t *)&addrlen);
	}
		
	return len;
}

//接收数据包
//返回接收数据的长度，如果<0则发生错误
//如果返回==0则表示接收速度超过了设定的流量限制，请暂缓接收
//bPeek --- 是否只PEEK数据，但不从接收缓冲区中移除
//==-1说明发生系统错误，通过SOCK_M_GETERROR获得错误代码 SOCKETOPMODE
SOCKSRESULT socketBase :: _Receive(char *buf,size_t buflen,time_t lWaitout,SOCKETOPMODE opmode)
{
	if(m_sockstatus<SOCKS_CONNECTED) return SOCKSERR_INVALID;
	if(buf==NULL || buflen==0) return SOCKSERR_PARAM;
	int iret=1; //是否有数据可读
	if(lWaitout>=0)
	{
		time_t t=time(NULL);
		while( (iret=checkSocket(SCHECKTIMEOUT,opmode))== 0 )
		{//检查句柄是否可读
			if( (time(NULL)-t)>lWaitout ) break; //检查是否超时
			if(m_parent && m_parent->m_sockstatus<=SOCKS_CLOSED) 
				return SOCKSERR_PARENT;
		}//?while
	}//?if(lWaitout>=0)
	if(iret!=1)
	{
		if(iret==-1) {m_errcode=SOCK_M_GETERROR; return SOCKSERR_ERROR;}
		return SOCKSERR_TIMEOUT; //超时
	}
	//当对方gracefully closed时,recvfrom立即返回0，不管缓冲区中是否有数据
	//recv会继续获取缓冲区中的数据，如果缓冲区中有数据的话，否则也返回0
	if(opmode==SOCKS_OP_PEEK)
		iret=v_peek(buf,buflen);
	else if(opmode==SOCKS_OP_READ)
	{
		if(m_maxRecvRatio!=0) //如果限制了接收流量，则判断是否超过流量
		{
			time_t tNow=time(NULL);
			if( (m_recvBytes/(tNow-m_tmOpened+1))>m_maxRecvRatio) return 0; //流量超限制
		}//?if(m_maxRecvRatio!=0)
		if( (iret=v_read(buf,buflen))>0 ) m_recvBytes+=iret;
	}
	else if(opmode==SOCKS_OP_ROOB && m_socktype==SOCKS_TCP) 
	{//读取带外数据 ,only for TCP
		iret=::recv(m_sockfd,buf,buflen,MSG_NOSIGNAL|MSG_OOB);
	}
	else return 0;
	if(iret>0) return iret;

	if(iret==0) return SOCKSERR_CLOSED; //如果返回0则说明本socket连接已经被对方关闭(only for TCP)
	m_errcode=SOCK_M_GETERROR;
	return SOCKSERR_ERROR;//发生系统错误，通过SOCK_M_GETERROR获得错误代码 
}

//向目的发送数据
//返回发送数据的大小，如果<0则发生错误
//如果返回==0则表示发送速度超过了设定的流量限制，请暂缓发送
//对于非tcp连接，发送前要调用setRemoteInfo设置发送的目的主机和端口。
//注意:对于非tcp连接，调用Receive时，会将接收数据的源发送ip和端口写入目的主机和端口。
//
//当调用该函数时，send先比较待发送数据的长度len和套接字s的发送缓冲区的长度，
//如果len大于s的发送缓冲区的长度，该函数返回SOCKET_ERROR；如果len小于或者等于s的发送缓冲区
//的长度，那么send先检查协议是否正在发送s的发送缓冲中的数据，如果是就等待协议把数据发送完，
//如果协议还没有开始发送s的发送缓冲中的数据或者s的发送缓冲中没有数据，那么send就比较s的发送
//缓冲区的剩余空间和len，如果len大于剩余空间大小send就一直等待协议把s的发送缓冲中的数据发送
//完，如果len小于剩余空间大小send就仅仅把buf中的数据copy到剩余空间里（注意并不是send把s的发
//送缓冲中的数据传到连接的另一端的，而是协议传的，send仅仅是把buf中的数据copy到s的发送缓冲
//区的剩余空间里）。如果send函数copy数据成功，就返回实际copy的字节数，如果send在copy数据时
//出现错误，那么send就返回SOCKET_ERROR；如果send在等待协议传送数据时网络断开的话，那么send
//函数也返回SOCKET_ERROR。要注意send函数把buf中的数据成功copy到s的发送缓冲的剩余空间里后它
//就返回了，但是此时这些数据并不一定马上被传到连接的另一端。
//注意：在Unix系统下，如果send在等待协议传送数据时网络断开的话，调用send的进程会接收到一个
//SIGPIPE信号，进程对该信号的默认处理是进程终止。
inline SOCKSRESULT socketBase :: _Send(const char *buf,size_t buflen,time_t lWaitout)
{
	SOCKADDR_IN addr; memcpy((void *)&addr,(const void *)&m_remoteAddr,sizeof(addr));
	//如果UDP没有指定发送的目的则返回错误
//	if(m_socktype==SOCKS_UDP && m_remoteAddr.sin_port==0) return SOCKSERR_HOST;
	
	time_t tNow=time(NULL);
	if(m_maxSendRatio!=0) //如果限制了发送流量，则判断是否超过流量
		if( (m_sendBytes/(tNow-m_tmOpened+1))>m_maxSendRatio) return 0; //流量超限制
	while(lWaitout>=0)
	{
		int iret=checkSocket(SCHECKTIMEOUT,SOCKS_OP_WRITE);
		if(iret<0) { m_errcode=SOCK_M_GETERROR; return SOCKSERR_ERROR; }
		if(iret>0) break; //socket可写
		if(m_parent && m_parent->m_sockstatus<=SOCKS_CLOSED) 
			return SOCKSERR_PARENT;
		if( (time(NULL)-tNow)>lWaitout ) return SOCKSERR_TIMEOUT; //超时
	}//?while
	
	int iret=(m_socktype==SOCKS_UDP)?v_writeto(buf,buflen,addr):v_write(buf,buflen);
	if(iret>=0) { m_sendBytes+=iret; return iret; }//返回实际发送的字节数
	
	m_errcode=SOCK_M_GETERROR;
	if(iret==SOCKET_ERROR && m_errcode==WSAEACCES) 
		return SOCKSERR_EACCES; //指定的地址是一个广播地址，但没有设置广播标志
	//否则iret应该等于-1,此时表明send/sendto的用发生了错误
	return SOCKSERR_ERROR;//发生系统错误，通过SOCK_M_GETERROR获得错误代码 
}

//----------------------------------------------------------------------------------
//----------------------------------------------------------------------------------
NetEnv netEnv;

NetEnv::NetEnv()
{
	m_bState=true;
#ifdef WIN32
	if(WSAStartup(MAKEWORD(2,2),&m_wsadata)!=0)
		m_bState=false;
#endif
}

NetEnv::~NetEnv()
{
#ifdef WIN32
	if(m_bState) WSACleanup();
#endif
}

NetEnv &getInstance(){ return netEnv; }
