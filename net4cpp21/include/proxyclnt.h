/*******************************************************************
   *	proxyclnt.h
   *    DESCRIPTION:代理协议客户端声明
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-11-20
   *
   *	net4cpp 2.1
   *	支持HTTP，socks4，socks5
   *******************************************************************/

#ifndef __YY_PROXY_CLINET_H__
#define __YY_PROXY_CLINET_H__

#include "proxydef.h"
#include "socketTcp.h"
#include "cThread.h"

namespace net4cpp21
{
	class socketProxy : public socketTCP
	{
	public:
		socketProxy();
		virtual ~socketProxy();
		//连接指定的TCP服务。成功返回本地socket端口，否则返回错误
		virtual SOCKSRESULT Connect(const char *host,int port,time_t lWaitout=-1);
		//发送socks BIND请求到指定的socks服务上开一个侦听端口
		//成功返回开立的端口>0 否则失败
		//[out] svrIP/svrPort返回socks服务开的端口和IP地址
		//[in] svrIP,告诉socks服务开的端口等待并允许那个源IP连接，svrPort无意义
		SOCKSRESULT Bind(std::string &svrIP,int &svrPort,time_t lWaitout=-1);
		bool WaitForBinded(time_t lWaitout,bool BindedEvent);
		//发送UDP代理协商请求 (仅仅socks5代理协议支持UDP代理)
		//成功返回开立的UDP端口>0 否则失败
		//[out] svrIP/svrPort返回socks服务开的端口和IP地址
		SOCKSRESULT UdpAssociate(std::string &svrIP,int &svrPort,time_t lWaitout=-1);
		PROXYTYPE proxyType() const { return m_proxytype; }
		bool setProxy(PROXYTYPE proxytype,const char *proxyhost,int proxyport,const char *user,const char *pwd);
		void setProxy(socketProxy &socks);

	private: //禁止copy和赋值
		socketProxy(socketProxy &sockProxy){ return; }
		socketProxy & operator = (socketProxy &sockProxy) { return *this; }

	private:
		//发送代理连接请求，成功返回SOCKSERR_OK(0)
		SOCKSRESULT sendReq_Connect(const char *host,int port,time_t lWaitout);
		//发送代理BIND请求，成功返回SOCKSERR_OK(0)
		SOCKSRESULT sendReq_Bind(std::string &svrIP,int &svrPort,time_t lWaitout);
		//发送UDP代理协商请求，成功返回SOCKSERR_OK(0) (仅仅socks5代理协议支持UDP代理)
		SOCKSRESULT sendReq_UdpAssociate(std::string &svrIP,int &svrPort,time_t lWaitout);
		//socks5协商认证,成功返回真否则返回假
		bool socks5_Negotiation(time_t lWaitout);
	private:
		PROXYTYPE m_proxytype;//代理类型
		std::string m_proxyhost;//代理服务器主机
		int m_proxyport;//代理服务器端口
		std::string m_proxyuser;//连接代理服务器的帐号
		std::string m_proxypwd;//连接代理服务器的密码
		int m_dnsType;//域名解析方式 0:服务端解析 1：本地端解析  2:先尝试本地端解析，然后在尝试服务端解析
			//对于socks4只能本地端解析,https代理方式总是服务器端解析

		cThread m_thread; //Bind命令等待第二次响应线程
	};
};//namespace net4cpp21

#endif


