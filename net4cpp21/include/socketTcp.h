/*******************************************************************
   *	socketTcp.h
   *    DESCRIPTION:TCP socket 类的定义
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2004-10-10
   *	Last modify: 2005-09-02
   *	net4cpp 2.1
   *******************************************************************/
#ifndef __YY_SOCKET_TCP_H__
#define __YY_SOCKET_TCP_H__

#ifndef _NOSSL_D
#define _SURPPORT_OPENSSL_ //如果要支持SSL，请定义此宏
						//同时在编译选项中加入openssl头文件和库文件路径
						//<net4cpp2.1目录>/OPENSSL
						//<net4cpp2.1目录>/OPENSSL/lib
						//tools菜单-->Options子菜单--->directories页
#endif
#ifdef _SURPPORT_OPENSSL_
	#include <openssl/crypto.h>
	#include <openssl/x509.h>
	#include <openssl/pem.h>
	#include <openssl/ssl.h>
	#include <openssl/err.h>
    #pragma comment( lib, "libeay32lib" )
	#pragma comment( lib, "SSLeay32lib" )
#endif
#include "socketBase.h"

namespace net4cpp21
{
	class socketTcp : public socketBase
	{
	public:
		socketTcp(){}
		socketTcp(socketTcp &sockTcp):socketBase(sockTcp){ return; }
		socketTcp & operator = (socketTcp &sockTcp) { socketBase::operator = (sockTcp); return *this; }
		virtual ~socketTcp(){}

		//连接指定的TCP服务。成功返回本地socket端口，否则返回错误
		virtual SOCKSRESULT Connect(const char *host,int port,time_t lWaitout=-1)
		{
			SOCKSRESULT iret=(host)?setRemoteInfo(host,port):SOCKSERR_OK;
			if(iret==SOCKSERR_OK) iret=Connect(lWaitout,0,NULL);
			return iret;
		}
		virtual SOCKSRESULT Accept(time_t lWaitout,socketTcp *psock);
		//TCP侦听，返回侦听服务端口
		SOCKSRESULT ListenX(int port,BOOL bReuseAddr,const char *bindIP);
		SOCKSRESULT ListenX(int startport,int endport,BOOL bReuseAddr,const char *bindIP);
		SOCKSRESULT Connect(const char *host,int port,int bindport,const char *bindip)
		{
			SOCKSRESULT iret=(host)?setRemoteInfo(host,port):SOCKSERR_OK;
			if(iret==SOCKSERR_OK) iret=Connect(-1,bindport,bindip);
			return iret;
		}
	protected:
		//成功返回本地socket端口，否则返回错误
//		SOCKSRESULT Connect(time_t lWaitout); //yyc remove 2007-08-07
		SOCKSRESULT Connect(time_t lWaitout,int bindport,const char *bindip); //yyc add 2007-08-07
	};

#ifdef _SURPPORT_OPENSSL_
	class socketSSL : public socketTcp
	{
	public:
		socketSSL();
		socketSSL(socketSSL &sockSSL);
		socketSSL & operator = (socketSSL &sockSSL);
		virtual ~socketSSL();

		virtual void Close();
		bool ifSSL() const { return m_ctx!=NULL; }
		bool ifSSLVerify() const { return m_bSSLverify; } //SSL服务是否需要客户端验证
		//设置SSL的证书私钥密码
		//bNotfile -- 指示strCaCert&strCaKey指向的是证书文件名还是证书内容
		//如果bNotfile=true且strCaCert或strCaKey为空则用默认的证书和私钥
		void setCacert(const char *strCaCert,const char *strCaKey,const char *strCaKeypwd,bool bNotfile,
					   const char *strCaRootFile=NULL,const char *strCRLfile=NULL);
		void setCacert(socketSSL *psock,bool bOnlyCopyCert);
		//进行SSL协商,当连接或者接受一个连接后,成功返回真
		bool SSL_Associate();
		//初始化SSL,bInitServer:指明是初始化服务端还是客户端
		//如果psock!=NULL则用psock的证书来初始化SSL服务端
		bool initSSL(bool bInitServer,socketSSL *psock=NULL);
		
		SOCKSRESULT Accept(time_t lWaitout,socketSSL *psock)
		{
			SOCKSRESULT sr=socketTcp::Accept(lWaitout,psock);
			if(sr>0 && psock){
				psock->m_ssltype=SSL_INIT_NONE;
				psock->m_ctx=this->m_ctx;
			}
			return sr;
		}

	protected:
		virtual size_t v_read(char *buf,size_t buflen);
		//!!! SSL_peek查看SSL数据后会改变socket的可读标志，此时如果通过
		//select 判断socket句柄，将永远返回不可读
		virtual size_t v_peek(char *buf,size_t buflen);
		virtual size_t v_write(const char *buf,size_t buflen);
		void freeSSL();
	private:
		SSL_INIT_TYPE m_ssltype;//SSL初始化类型
		SSL_CTX *m_ctx;
		SSL *    m_ssl;
		//SSL服务的证书，私钥以及私钥密码
		std::string m_cacert;//SSL 证书
		std::string m_cakey;//SSL 私钥
		std::string m_cakeypass;//SSL私钥密码
		bool m_bNotfile;//指明m_cacert&m_cakey指向的是证书文件名还是证书字符串
		bool m_bSSLverify; //SSL是否要验证客户端证书
		std::string m_carootfile; //CA验证根证书，用来校验客户端证书的真伪
		std::string m_crlfile; //CRL列表文件
	};
	
	typedef socketSSL socketTCP;
#else
	typedef socketTcp socketTCP;
#endif
}//?namespace net4cpp21

#endif

