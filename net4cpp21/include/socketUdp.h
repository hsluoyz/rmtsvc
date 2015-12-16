/*******************************************************************
   *	socketUdp.h
   *    DESCRIPTION:UDP socket 类的定义
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2004-10-10
   *	Last modify: 2005-09-02
   *	net4cpp 2.1
   *******************************************************************/
#ifndef __YY_SOCKET_UDP_H__
#define __YY_SOCKET_UDP_H__

#include "socketBase.h"
#include "cThread.h"

namespace net4cpp21
{
	class socketUdp : public socketBase
	{
	public:
		socketUdp(){}
		virtual ~socketUdp(){}
		SOCKSRESULT Open(int port,bool bReuse=false,const char *bindIP=NULL);

	private:
	};
	//异步UDP socket
	class socketUdpAnsy : public socketUdp
	{
	public:
		socketUdpAnsy(){}
		virtual ~socketUdpAnsy();
		SOCKSRESULT Open(int port,bool bReuse=false,const char *bindIP=NULL);
	protected:
		//有数据到达
		virtual void onData(){ return; }
	private:
		cThread m_thread;
		static void recvThread(socketUdpAnsy *psock);
	};
}//?namespace net4cpp21

#endif


