/*******************************************************************
   *	smtpclnt.h
   *    DESCRIPTION:smtp协议客户端声明
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-11-20
   *
   *	net4cpp 2.1
   *	简单邮件传输协议(smtp)
   *******************************************************************/

#ifndef __YY_SMTP_CLINET_H__
#define __YY_SMTP_CLINET_H__

#include "smtpdef.h"
#include "proxyclnt.h"

namespace net4cpp21
{
	
	class smtpClient : public socketProxy
	{
	public:
		smtpClient(const char *ehlo=NULL);
		virtual ~smtpClient();
		void setTimeout(time_t s){ if((m_lTimeout=s)<1) m_lTimeout=SMTP_MAX_RESPTIMEOUT; }
		//设置smtp服务的验证类型和帐号
		void setSMTPAuth(SMTPAUTH_TYPE authType,const char *strAccount,const char *strPwd);
		// 函数功能：连接smtp服务器 ，发送指定邮件。成功返回SOCKSERR_OK
		SOCKSRESULT sendMail(mailMessage &mms,const char *smtpsvr,int smtpport);
		SOCKSRESULT sendMail_MX(mailMessage &mms,const char *dnssvr,int dnsport);
		//****************************************
		// 函数功能：邮件直投。成功返回SOCKSERR_OK
		//emlfile : 邮件格式文件，两种格式文件. 邮件正文前!开头的为注释行
		//如果第一行为Email body is base64 encoded，则代表是smtpsvr接收的要转发的邮件
		//否则为用户编辑要发送的邮件,格式:
		//FROM: <发件人>\r\n
		//TO: <收件人>,<收件人>,...\r\n
		//Attachs: <附件>,<附件>,...\r\n
		//Subject: <主题>\r\n
		//Bodytype: <TEXT|HTML>\r\n
		//\r\n
		//...
		//****************************************
		SOCKSRESULT sendMail(const char *emlfile,const char *smtpsvr,int smtpport,const char *from);
		SOCKSRESULT sendMail_MX(const char *emlfile,const char *dnssvr,int dnsport);
		std::vector<std::string> &errors() { return m_errors; }
	private:
		//连接指定的smtp服务器
		SOCKSRESULT ConnectSvr(const char *smtpsvr,int smtpport);
		SOCKSRESULT Auth_LOGIN();
		SOCKSRESULT _sendMail(mailMessage &mms,const char *toemail);
		bool sendCommand(int response_expected,char *buf,int buflen,int maxbuflen);

	private:
		SMTPAUTH_TYPE m_authType;//smtp服务是否需要验证,目前仅仅支持LOGIN验证方式
		std::string m_strAccount;//LOGIN验证的帐号和密码
		std::string m_strPwd;
		time_t m_lTimeout;//最大等待超时返回s
		std::vector<std::string> m_errors; //记录发送时的错误
		std::string m_ehloName;
	};
}//?namespace net4cpp21

#endif

