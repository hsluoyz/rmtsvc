/*******************************************************************
   *	smtpdef.h
   *    DESCRIPTION:定义smtp协议所用到的常量、结构以及enum的定义
   *				
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-11-20
   *	
   *	net4cpp 2.1
   *******************************************************************/

#ifndef __YY_SMTPDEF_H__
#define __YY_SMTPDEF_H__

#define SMTP_SERVER_PORT	25 //默认smtp服务的端口
#define SMTP_MAX_PACKAGE_SIZE 128 //发送或接收smtp命令缓冲大小
#define SMTP_MAX_RESPTIMEOUT 10 //s 响应最大延时


#define SOCKSERR_SMTP_RESP -301 //命令响应错误
#define SOCKSERR_SMTP_CONN SOCKSERR_SMTP_RESP-1 //连接服务器失败
#define SOCKSERR_SMTP_AUTH SOCKSERR_SMTP_RESP-2 //smtp认证失败
#define SOCKSERR_SMTP_SURPPORT SOCKSERR_SMTP_RESP-3 //不支持的smtp认证方式
#define SOCKSERR_SMTP_RECIPIENT SOCKSERR_SMTP_RESP-4 //没有指定邮件接收者
#define SOCKSERR_SMTP_FAILED SOCKSERR_SMTP_RESP-5 //一般性错误
#define SOCKSERR_SMTP_EMLFILE SOCKSERR_SMTP_RESP-6 //错误的邮件格式
#define SOCKSERR_SMTP_EMAIL SOCKSERR_SMTP_RESP-7 //无效的邮件地址
#define SOCKSERR_SMTP_DNSMX SOCKSERR_SMTP_RESP-8 //MX域名解析失败
#define SOCKSERR_SMTP_4XX   SOCKSERR_SMTP_RESP-9

typedef enum         //定义支持的SMTP验证类型
{
	SMTPAUTH_NONE=0,
	SMTPAUTH_LOGIN=1, //等效于 PLAIN，只为了与 SMTP 验证的预标准实现相兼容。缺省情况下，此机制仅可由 SMTP 使用
	SMTPAUTH_CRAM_MD5=2, //一种询问/响应验证机制，类似于 APOP，但也适合与其他协议配合使用。在 RFC 2195 中已定义
	SMTPAUTH_DIGEST_MD5=4, //RFC 2831 中定义的询问/响应验证机制
	SMTPAUTH_8BITMIME=8,
	SMTPAUTH_PLAIN=16 //PLAIN 此机制通过网络传递用户的纯文本口令，在网络上很容易窃听口令
}SMTPAUTH_TYPE;


#include <fstream>
#include <vector>
#include <string>
class mailMessage
{
	int m_contentType; //"text/plain" "text/html"
	std::string m_strSubject;
	std::string m_strBody;
	std::string m_strBodyCharset;//邮件正文的编码字符集，默认为utf-8
	std::vector<std::string> m_attachs;//要发送的附件

	std::string m_strName;//发送者的名称
	std::string m_strFrom;//发送者的邮箱
	std::vector<std::pair<std::string,std::string> > m_vecTo; //发送,first --收件人email，second 收件人昵称
	std::vector<std::pair<std::string,std::string> > m_vecCc; //抄送,first --收件人email，second 收件人昵称
	std::vector<std::pair<std::string,std::string> > m_vecBc; //暗送,first --收件人email，second 收件人昵称
	
	std::string m_strMailFile;//生成的邮件体文件路径名称
	long m_lMailFileStartPos;//邮件体文件中邮件体正文的起始位置
	bool m_bDeleteFile;//对象释放时是否删除m_strMailFile文件
public:
	enum RECIPIENT_TYPE { TO, CC, BCC };
	enum EMAILBODY_TYPE { TEXT_BODY,HTML_BODY };
	
	mailMessage():m_contentType(TEXT_BODY),m_strBodyCharset("utf-8"),m_bDeleteFile(false),m_lMailFileStartPos(0){}
	~mailMessage();
	const char *from() { return m_strFrom.c_str(); }
	std::string &body() { return m_strBody;}
	std::vector<std::pair<std::string,std::string> > & vecTo(RECIPIENT_TYPE rt=TO)
	{
		if(rt==CC)
			return m_vecCc;
		if(rt==BCC)
			return m_vecBc;
		return m_vecTo;
	}
	void setFrom(const char *from,const char *name)
	{
		if(from) m_strFrom.assign(from);
		if(name) m_strName.assign(name);
	}
	
	//设置邮件主题，正文
	void setBody(const char *strSubject,const char *strBody,EMAILBODY_TYPE bt=TEXT_BODY)
	{
		if(strSubject) m_strSubject.assign(strSubject);
		if(strBody) m_strBody.assign(strBody);
		m_contentType=bt; m_strMailFile="";
	}
	//添加附件
	bool AddAtach(const char *filename,const char *filepath,const char *contentID);
	//添加收件人
	bool AddRecipient(const char *email,const char *nick,RECIPIENT_TYPE rt=TO);
	//生成Base64编码的邮件体文件
	//bDelete -- 指示当mailMessage对象释放时是否删除生成的文件
	const char * createMailFile(const char *file,bool bDelete);
	long MailFileStartPos() const { return m_lMailFileStartPos; }
	void setBody(const char *mailfile,long startPos)
	{
		m_strMailFile.assign(mailfile);
		m_bDeleteFile=false;
		m_lMailFileStartPos=startPos;
		return;
	}
	int initFromemlfile(const char *emlfile);
};

#endif

/* 完整协议跟踪
[56 202.108.9.193:25-->127.0.0.1:1159] 220 Coremail SMTP(Anti Spam) System (163c
om[20050206])
.
[10 127.0.0.1:1159-->202.108.9.193:25] EHLO yyc
.
[96 202.108.9.193:25-->127.0.0.1:1159] 250-smtp14
250-PIPELINING
250-AUTH LOGIN PLAIN NTLM
250-AUTH=LOGIN PLAIN NTLM
250 8BITMIME
.
[12 127.0.0.1:1159-->202.108.9.193:25] AUTH LOGIN
.
[18 202.108.9.193:25-->127.0.0.1:1159] 334 VXNlcm5hbWU6
.
[10 127.0.0.1:1159-->202.108.9.193:25] eXljbmV0
.
[18 202.108.9.193:25-->127.0.0.1:1159] 334 UGFzc3dvcmQ6
.
[10 127.0.0.1:1159-->202.108.9.193:25] cYY6d8N6
.
[31 202.108.9.193:25-->127.0.0.1:1159] 235 Authentication successful
.
[29 127.0.0.1:1159-->202.108.9.193:25] MAIL FROM: <yycnet@163.com>
.
[8 202.108.9.193:25-->127.0.0.1:1159] 250 Ok
.
[33 127.0.0.1:1159-->202.108.9.193:25] RCPT TO: <yycmail@263.sina.com>
.
[8 202.108.9.193:25-->127.0.0.1:1159] 250 Ok
.
[6 127.0.0.1:1159-->202.108.9.193:25] Data
.
[37 202.108.9.193:25-->127.0.0.1:1159] 354 End data with <CR><LF>.<CR><LF>
.
[662 127.0.0.1:1159-->202.108.9.193:25] From: "yyc" <yycnet@163.com>
To: yycmail@263.sina.com <yycmail@263.sina.com>
Subject: hi,test
Organization: xxxx
X-mailer: Foxmail 4.2 [cn]
Mime-Version: 1.0
Content-Type: text/plain;
      charset="GB2312"
Content-Transfer-Encoding: quoted-printable
Date: Mon, 8 Aug 2005 10:47:32 +0800

yycmail=A3=AC=C4=FA=BA=C3=A3=A1

=09         aaaaaaaaaaaaaaaaaa

=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=D6=C2
=C0=F1=A3=A1
 =09=09=09=09

=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1yyc
=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1yycnet@163.com
=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A1=A12005-08-08


.
.
[38 202.108.9.193:25-->127.0.0.1:1159] 250 Ok: queued as 2cB_NXrE9kIrXhAE.1
.
[6 127.0.0.1:1159-->202.108.9.193:25] QUIT
.
[9 202.108.9.193:25-->127.0.0.1:1159] 221 Bye
.
*/
/*
211 系统状态或系统帮助响应 
　　　214 帮助信息 
　　　220 服务就绪 
　　　221 服务关闭传输信道 
　　　250 要求的邮件操作完成 
　　　251 用户非本地，将转发向 
　　　354 开始邮件输入，以.结束 
　　　421 服务未就绪，关闭传输信道（当必须关闭时，此应答可以作为对任何命令的响应） 
　　　450 要求的邮件操作未完成，邮箱不可用（例如，邮箱忙） 
　　　451 放弃要求的操作；处理过程中出错 
　　　452 系统存储不足，要求的操作未执行 
　　　500 格式错误，命令不可识别（此错误也包括命令行过长） 
　　　501 参数格式错误 
　　　502 命令不可实现 
　　　503 错误的命令序列 
　　　504 命令参数不可实现 
　　　550 要求的邮件操作未完成，邮箱不可用（例如，邮箱未找到，或不可访问） 
　　　551 用户非本地，请尝试 
　　　552 过量的存储分配，要求的操作未执行 
　　　553 邮箱名不可用，要求的操作未执行（例如邮箱格式错误） 
　　　554 操作失败 
*/

