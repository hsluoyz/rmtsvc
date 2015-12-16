/*******************************************************************
   *	smtpsvr.cpp
   *    DESCRIPTION:smtp协议服务端实现
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

#include "../include/sysconfig.h"
#include "../include/smtpsvr.h"
#include "../include/cCoder.h"
#include "../utils/cTime.h"
#include "../utils/utils.h"
#include "../include/cLogger.h"

using namespace std;
using namespace net4cpp21;


smtpServer :: smtpServer()
{
	m_strSvrname="smtp Server";
	m_authType=SMTPAUTH_NONE;
	m_helloTip="";
}

smtpServer :: ~smtpServer()
{
//本服务没有线程，因此无需进行下面的析构前处理
//	Close();
//	m_threadpool.join();
}


//当有一个新的客户连接此服务触发此函数
void smtpServer :: onAccept(socketTCP *psock)
{
	RW_LOG_DEBUG("%s is connected\r\n",psock->getRemoteIP());
	char buf[SMTP_MAX_PACKAGE_SIZE]; int buflen=0;
	buflen=sprintf(buf,"220 SMTP Server[mailpost1.0] for ready\r\n");
	psock->Send(buflen,buf,-1);
//	某些SMTP客户端不支持多行SMTP服务连接应答
/*	buflen=sprintf(buf,"220-SMTP Server[mailpost1.0] for ready\r\n"
					   "220-copyright @yyc 2006. http://yycnet.yeah.net\r\n"
					   "%s",m_helloTip.c_str());
	psock->Send(buflen,buf,-1);
	//当前服务器运行状态
	struct tm * ltime=localtime(&m_tmOpened);
	buflen=sprintf(buf,"220-服务器开始运行的时间是%04d-%02d-%02d %02d:%02d:%02d\r\n",
				(1900+ltime->tm_year), ltime->tm_mon+1, ltime->tm_mday, 
				ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
	psock->Send(buflen,buf,-1);
*/	
	cSmtpSession clientSession; 
	clientSession.m_tmLogin=time(NULL); //用户连接时间
/*	ltime=localtime(&clientSession.m_tmLogin);
	buflen=sprintf(buf,"220-目前服务器所在的时间是%04d-%02d-%02d %02d:%02d:%02d\r\n"
					   "220-当前登陆用户数量 %d \r\n"
					   "220-最大允许连接用户数 %d \r\n"
					   "220 SMTP Server for ready...\r\n",
					   (1900+ltime->tm_year), ltime->tm_mon+1, ltime->tm_mday, 
					   ltime->tm_hour, ltime->tm_min, ltime->tm_sec,
					   curConnection(),maxConnection());
	psock->Send(buflen,buf,-1);
*/
	if(this->m_authType==SMTPAUTH_NONE) clientSession.m_bAccess=true;
	buflen=0; //清空命令接收缓冲
	while(psock->status()==SOCKS_CONNECTED )
	{
		int iret=psock->checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ);
		if(iret<0) break; //如果登录超时则关闭连接
		if(!clientSession.m_bAccess && //判断是否登录超时
			(time(NULL)-clientSession.m_tmLogin)>SMTP_MAX_RESPTIMEOUT) break;
		if(iret==0) continue;
		//读客户端发送的数据
		iret=psock->Receive(buf+buflen,SMTP_MAX_PACKAGE_SIZE-buflen-1,-1);
		if(iret<0) break; //==0表明接收数据流量超过限制
		if(iret==0){ cUtils::usleep(MAXRATIOTIMEOUT); continue; }
		buflen+=iret; buf[buflen]=0;
		//解析smtp命令
		const char *ptrCmd,*ptrBegin=buf;
		while( (ptrCmd=strchr(ptrBegin,'\r')) )
		{
			*(char *)ptrCmd=0;//开始解析命令
			if(ptrBegin[0]==0) goto NextCMD; //不处理空行数据
		
			parseCommand(clientSession,psock,ptrBegin);

NextCMD:	//移动ptrBegin到下一个命令数据起始
			ptrBegin=ptrCmd+1; 
			while(*ptrBegin=='\r' || *ptrBegin=='\n') ptrBegin++; //跳过\r\n
		}//?while
		//如果有未接收完的命令则移动
		if((iret=(ptrBegin-buf))>0 && (buflen-iret)>0)
		{//如果ptrBegin-buf==0说明这是一个错误命令数据包
			buflen-=iret;
			memmove((void *)buf,ptrBegin,buflen);
		} else buflen=0;
	}//?while

	RW_LOG_DEBUG("%s is disconnected\r\n",psock->getRemoteIP());
	RW_LOG_FFLUSH()
	return;
}

//如果当前连接数大于当前设定的最大连接数则触发此事件
void smtpServer :: onTooMany(socketTCP *psock)
{
	char resp[]="220 access denied, Too many users.\r\n";
	psock->Send(sizeof(resp)-1,resp,-1);
	return;
}

//-------------------SMTP 命令解析处理 begin-----------------------------------
void smtpServer :: parseCommand(cSmtpSession &clientSession,socketTCP *psock,const char *ptrCommand)
{
	RW_LOG_DEBUG("[smtpsvr] c--->s:\r\n\t%s\r\n",ptrCommand);
	if(strncasecmp(ptrCommand,"EHLO ",5)==0)
		docmd_ehlo(clientSession,psock,ptrCommand+5);
	else if(strncasecmp(ptrCommand,"HELO ",5)==0)
		docmd_ehlo(clientSession,psock,ptrCommand+5);
	else if(strncasecmp(ptrCommand,"AUTH ",5)==0)
		docmd_auth(clientSession,psock,ptrCommand+5);
	else if(strncasecmp(ptrCommand,"MAIL FROM:",9)==0)
		docmd_mailfrom(clientSession,psock,ptrCommand+9);
	else if(strncasecmp(ptrCommand,"RCPT TO:",7)==0)
		docmd_rcptto(clientSession,psock,ptrCommand+7);
	else if(strcasecmp(ptrCommand,"Data")==0)
		docmd_data(clientSession,psock);
	else if(strcasecmp(ptrCommand,"quit")==0)
		docmd_quit(psock);
	else if(strcasecmp(ptrCommand,"rset")==0)
		resp_OK(psock);
	else 
		resp_unknowed(psock);
	return;
}

inline SOCKSRESULT response(socketTCP *psock,const char *buf,int buflen)
{
	RW_LOG_DEBUG("[smtpsvr] s--->c:\r\n\t%s",buf);
	return psock->Send(buflen,buf,-1);
}

inline void smtpServer :: resp_OK(socketTCP *psock)
{
	char resp[]="250 OK\r\n";
	response(psock,resp,sizeof(resp)-1);
	return;
}
inline void smtpServer :: resp_unknowed(socketTCP *psock)
{
	char resp[]="500 command unrecognized.\r\n";
	response(psock,resp,sizeof(resp)-1);
	return;
}
//注意smtp服务的多行数据的响应，如果不是最后一行则响应行的响应码和描述之间要用-连接
void smtpServer :: docmd_ehlo(cSmtpSession &clientSession,socketTCP *psock,const char *strParam)
{
	//去掉命令参数的前导空格
	while(*strParam==' ') strParam++;
	clientSession.m_ehlo.assign(strParam);
	const char resp[]="250-PIPELINING\r\n"
					  "250-AUTH LOGIN\r\n"
					  "250-AUTH=LOGIN\r\n"
					  "250-8BITMIME\r\n"
					  "250 OK\r\n";
	response(psock,resp,sizeof(resp)-1);
	return;
}

void smtpServer :: docmd_auth(cSmtpSession &clientSession,socketTCP *psock,const char *strParam)
{
	//去掉命令参数的前导空格
	while(*strParam==' ') strParam++;
	int authType=SMTPAUTH_NONE;
	if(strcasecmp(strParam,"LOGIN")==0)
	{
		authType=SMTPAUTH_LOGIN;
		const char resp[]="334 VXNlcm5hbWU6\r\n"; //VXNlcm5hbWU6为Username:的base64编码
		response(psock,resp,sizeof(resp)-1);
	}
	else if(strcasecmp(strParam,"PLAIN")==0) //明文传输
	{
		authType=SMTPAUTH_PLAIN;
		const char resp[]="334 Username:\r\n";
		response(psock,resp,sizeof(resp)-1);
	}
	else if(strcasecmp(strParam,"8BITMIME")==0)
	{
		authType=SMTPAUTH_8BITMIME;
		const char resp[]="334 Username%3A\r\n"; //Username%3A为Username:的Mime编码
		response(psock,resp,sizeof(resp)-1);
	}
	else
	{
		const char resp[]="504-命令参数不可实现\r\n"
						  "504 only Surpport LOGIN,8BITMIME\r\n";
		response(psock,resp,sizeof(resp)-1);
		psock->Close(); return;
	}
	//等待用户发送验证帐号和密码
	std::string strAccount,strPwd;
	char buf[SMTP_MAX_PACKAGE_SIZE];
	int iret=psock->Receive(buf,SMTP_MAX_PACKAGE_SIZE-1,SMTP_MAX_RESPTIMEOUT);
	if(iret<=0){ psock->Close(); return; }
	buf[iret]=0; strAccount.assign(buf);
	if(authType==SMTPAUTH_LOGIN)
	{//进行Base64解码
		iret=cCoder::base64_decode((char *)strAccount.c_str(),strAccount.length(),buf);
		if(iret>=0) buf[iret]=0; 
		strAccount.assign(buf);
		const char resp[]="334 UGFzc3dvcmQ6\r\n";
		response(psock,resp,sizeof(resp)-1);
	}
	else if(authType==SMTPAUTH_8BITMIME)
	{
		iret=cCoder::mime_decode(strAccount.c_str(),strAccount.length(),buf);
		if(iret>=0) buf[iret]=0; 
		strAccount.assign(buf);
		const char resp[]="334 Password%3A\r\n";
		response(psock,resp,sizeof(resp)-1);
	}
	else
	{
		const char resp[]="334 Password:\r\n";
		response(psock,resp,sizeof(resp)-1);
	}
	//获取密码
	iret=psock->Receive(buf,SMTP_MAX_PACKAGE_SIZE-1,SMTP_MAX_RESPTIMEOUT);
	if(iret<=0){ psock->Close(); return; }
	buf[iret]=0; strPwd.assign(buf);
	if(authType==SMTPAUTH_LOGIN)
	{//进行Base64解码
		iret=cCoder::base64_decode((char *)strPwd.c_str(),strPwd.length(),buf);
		if(iret>=0) buf[iret]=0; 
		strPwd.assign(buf);	
	}
	else if(authType==SMTPAUTH_8BITMIME)
	{
		iret=cCoder::mime_decode(strPwd.c_str(),strPwd.length(),buf);
		if(iret>=0) buf[iret]=0; 
		strPwd.assign(buf);	
	}
	
	//验证帐号和密码
	if(!onAccess(strAccount.c_str(),strPwd.c_str()))
	{
		const char resp[]="551 Authentication unsuccessful\r\n";
		response(psock,resp,sizeof(resp)-1);
		psock->Close(); //验证失败
	}
	else
	{
		clientSession.m_bAccess=true; //验证成功
		const char resp[]="235 Authentication successful\r\n";
		response(psock,resp,sizeof(resp)-1);
	}
	return;
}

void smtpServer :: docmd_mailfrom(cSmtpSession &clientSession,socketTCP *psock,const char *strParam)
{
	//去掉命令参数的前导空格
	while(*strParam==' ') strParam++;
	const char *ptrS=strchr(strParam,'<');
	if(ptrS)
	{	ptrS++;
		const char *ptrE=strchr(ptrS,'>');
		if(ptrE) *(char *)ptrE=0;
		clientSession.m_fromemail.assign(ptrS);
	}
	else
		clientSession.m_fromemail.assign(strParam);
	resp_OK(psock);
	return;
}

void smtpServer :: docmd_rcptto(cSmtpSession &clientSession,socketTCP *psock,const char *strParam)
{
	//去掉命令参数的前导空格
	while(*strParam==' ') strParam++;
	const char *ptrS=strchr(strParam,'<');
	if(ptrS)
	{	ptrS++;
		const char *ptrE=strchr(ptrS,'>');
		if(ptrE) *(char *)ptrE=0;
		clientSession.m_recp.push_back(ptrS);
	}
	else
		clientSession.m_recp.push_back(strParam);
	resp_OK(psock);
	return;
}

void smtpServer :: docmd_data(cSmtpSession &clientSession,socketTCP *psock)
{

	const char resp[]="354 End data with <CR><LF>.<CR><LF>\r\n";
	response(psock,resp,sizeof(resp)-1);
	//下面开始接收信体数据,直到收到<CR><LF>.<CR><LF>
	char buf[4096]; int buflen=0;
	std::string emlfile;//生成临时文件名
	time_t tNow=time(NULL);
	srand( (unsigned)clock() );
	struct tm * ltime=localtime(&tNow);
	buflen=sprintf(buf,"%s%04d%02d%02d%02d%02d%02d_%d.!em",m_receivedpath.c_str(),
		(1900+ltime->tm_year), ltime->tm_mon+1, ltime->tm_mday, 
		ltime->tm_hour, ltime->tm_min, ltime->tm_sec,rand());
	emlfile.assign(buf);
	FILE *fp=::fopen(emlfile.c_str(),"wb");
	if(fp==NULL){psock->Close(); return; }
	::fprintf(fp,"Email body is base64 encoded\r\nFROM: %s\r\nTO: ",clientSession.m_fromemail.c_str());
	for(int i=0;i<clientSession.m_recp.size();i++)
		::fprintf(fp,"%s,",clientSession.m_recp[i].c_str());
	::fprintf(fp,"\r\n\r\n");
	cTime t=cTime::GetCurrentTime();
	int tmlen=t.Format(buf,256,"%a, %d %b %Y %H:%M:%S %Z"); buf[tmlen]=0;
	::fprintf(fp,"Received: from %s [%s]\r\n\tby mailpost1.0 with SMTP\r\n"
				 "\t%s\r\n",clientSession.m_ehlo.c_str(),psock->getRemoteIP(),
				 buf);

	bool bRecvALL=false; //信体是否正常收完
	while( psock->status()==SOCKS_CONNECTED )
	{
		//读客户端发送的数据
		//如果超过SMTP_MAX_RESPTIMEOUT仍没收到数据可认为客户端异常
		buflen=psock->Receive(buf,4095,SMTP_MAX_RESPTIMEOUT);
		if(buflen<0){
			RW_LOG_PRINT(LOGLEVEL_WARN,"[smtpsvr] Failed to receive mail DATA,error=%d\r\n",buflen);
			break; 
		}
		if(buflen==0){ cUtils::usleep(SCHECKTIMEOUT); continue; }//==0表明接收数据流量超过限制
		buf[buflen]=0;
		if(buflen>=5 && strcmp(buf+buflen-5,"\r\n.\r\n")==0)
		{ 
			if( (buflen-=5)>0 ) ::fwrite(buf,sizeof(char),buflen,fp);
			bRecvALL=true; break;
		}
		else
			::fwrite(buf,sizeof(char),buflen,fp);
	}//?while
	::fclose(fp);
	if(bRecvALL)
	{
		buflen=sprintf(buf,"%s",emlfile.c_str());
		//将扩展名改为eml
		emlfile[buflen-3]='e';emlfile[buflen-2]='m';
		emlfile[buflen-1]='l';emlfile[buflen]=0;
		FILEIO::fileio_rename(buf,emlfile.c_str());
		const char resp[]="250 Ok: Success to receive Data.\r\n";
		response(psock,resp,sizeof(resp)-1);
		onReceive(emlfile.c_str(),clientSession);
	}
	else
	{
		const char resp[]="451 Error: Failed to receive Data.\r\n";
		response(psock,resp,sizeof(resp)-1);
		FILEIO::fileio_deleteFile(emlfile.c_str());
	}
	return;
}

inline void smtpServer :: docmd_quit(socketTCP *psock)
{
	const char resp[]="221 Bye\r\n";
	response(psock,resp,sizeof(resp)-1);
	psock->Close(); return;
}

