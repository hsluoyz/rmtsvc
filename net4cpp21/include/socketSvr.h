/*******************************************************************
   *	socketSvr.h
   *    DESCRIPTION:TCP 异步服务类类的定义
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-11-01
   *	
   *	net4cpp 2.1
   *******************************************************************/
#ifndef __YY_SOCKET_SVR_H__
#define __YY_SOCKET_SVR_H__

#include "socketTcp.h"
#include "IPRules.h"
#include "cThread.h"

namespace net4cpp21
{
	class socketSvr : public socketTCP
	{
	public:
		socketSvr();
		virtual ~socketSvr();
		//启动服务
		SOCKSRESULT Listen(int port,BOOL bReuseAddr=FALSE,const char *bindIP=NULL);
		SOCKSRESULT Listen(int startport,int endport,BOOL bReuseAddr=FALSE,const char *bindIP=NULL);
		const char *svrname() { return m_strSvrname.c_str();} //返回服务名称
		iprules &rules() { return m_srcRules;}
		void maxConnection(unsigned long ulMax) { m_maxConnection=ulMax; return; }
		unsigned long maxConnection() const { return m_maxConnection; }
		unsigned long curConnection() const { return m_curConnection; }
		BOOL GetReuseAddr() const { return m_bReuseAddr; } //获取端口复用状态
		
	private: //禁止copy和赋值
		socketSvr(socketSvr &sockSvr){ return; }
		socketSvr & operator = (socketSvr &sockSvr) { return *this; }	
	protected:
		cThreadPool m_threadpool;//服务线程池
		std::string m_strSvrname;//服务名称
		
		//当有一个新的客户连接此服务触发此函数
		virtual void onAccept(socketTCP *psock){ return; }
		//如果当前连接数大于当前设定的最大连接数则触发此事件
		virtual void onTooMany(socketTCP *psock) { return; }
		virtual void onIdle(void) { return; } //仅仅设置了m_lAcceptTimeOut异步超时时间才有此事件

		
	private:
		iprules m_srcRules;//默认访问过滤规则对象,来源IP的过滤规则
		unsigned long m_curConnection; //当前并发连接数
		unsigned long m_maxConnection; //允许的最大并发连接数, 0不限
		long	m_lAcceptTimeOut; //异步Accept的超时时间 ==-1一直等待

		BOOL m_bReuseAddr; //服务端口复用状态，值:SO_REUSEADDR/SO_EXCLUSIVEADDRUSE/FALSE
		static void transThread(socketTCP *psock); //yyc add 2007-03-29
		static void doRedirectTask(socketTCP *psock); //yyc add 2007-03-29

		static void listenThread(socketSvr *psvr);
		static void doacceptTask(socketTCP *psock);
	};
}//?namespace net4cpp21

#endif

