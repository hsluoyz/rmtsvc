/*******************************************************************
   *	smtpsvr.h
   *    DESCRIPTION:smtp协议服务端声明
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-11-20
   *
   *	net4cpp 2.1
   *	简单邮件传输服务(smtp)
   *******************************************************************/

#ifndef __YY_SMTP_SERVER_H__
#define __YY_SMTP_SERVER_H__

#include "smtpdef.h"
#include "socketSvr.h"

namespace net4cpp21
{
	
	class smtpServer : public socketSvr
	{
		class cSmtpSession
		{
		public:
			bool m_bAccess; //此客户session是否通过了验证
			time_t m_tmLogin;//登录时间
			std::string m_ehlo;
			std::string m_fromemail;//邮件发送者
			std::vector<std::string> m_recp;//邮件接收者
			cSmtpSession():m_bAccess(false){}
			~cSmtpSession(){}
		};
	public:
		smtpServer();
		virtual ~smtpServer();
		//设置接收邮件的路径
		const char * setRecvPath(const char *recvpath)
		{
			if(recvpath)
				m_receivedpath.assign(recvpath);
			if(m_receivedpath!="" && m_receivedpath[m_receivedpath.length()-1]!='\\')
				m_receivedpath.append("\\");
			return m_receivedpath.c_str();
		}
//		void setHelloTip(const char *strTip){
//			if(strTip) m_helloTip.assign(strTip);
//			return;
//		}
	protected:
		virtual bool onAccess(const char *struser,const char *strpwd) {return true;}
		virtual void onReceive(const char *emlfile,cSmtpSession &clientSession)
		{
			return;
		}
	private:
		//当有一个新的客户连接此服务触发此函数
		virtual void onAccept(socketTCP *psock);
		//如果当前连接数大于当前设定的最大连接数则触发此事件
		virtual void onTooMany(socketTCP *psock);

		void parseCommand(cSmtpSession &clientSession,socketTCP *psock,const char *ptrCommand);
		void docmd_ehlo(cSmtpSession &clientSession,socketTCP *psock,const char *strParam);
		void docmd_auth(cSmtpSession &clientSession,socketTCP *psock,const char *strParam);
		void docmd_mailfrom(cSmtpSession &clientSession,socketTCP *psock,const char *strParam);
		void docmd_rcptto(cSmtpSession &clientSession,socketTCP *psock,const char *strParam);
		void docmd_data(cSmtpSession &clientSession,socketTCP *psock);
		void docmd_quit(socketTCP *psock);
		void resp_unknowed(socketTCP *psock);
		void resp_OK(socketTCP *psock);
	private:
		SMTPAUTH_TYPE m_authType;//SMTP服务是否要求验证
		std::string m_receivedpath;//接收邮件存放路径，以\结尾
		std::string m_helloTip;
	};
}//?namespace net4cpp21

#endif
