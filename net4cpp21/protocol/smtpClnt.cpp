/*******************************************************************
   *	smtpclnt.cpp
   *    DESCRIPTION:smtp协议客户端实现
   *				支持直接邮件投递(不经过中继邮件服务器，直接投递到目的邮箱)
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-11-20
   *
   *	net4cpp 2.1
   *	简单邮件传输协议(smtp)
   *******************************************************************/

#include "../include/sysconfig.h"
#include "../include/smtpclnt.h"
#include "../include/dnsclnt.h"
#include "../include/cCoder.h"
#include "../utils/cTime.h"
#include "../include/cLogger.h"

using namespace std;
using namespace net4cpp21;

//MX域名解析临时缓存，first邮箱域名
typedef struct _MXINFO
{
	std::vector<std::string> mxhosts;
	time_t tStart; //更新时间
	time_t tExpire; //有效时间
	void setmx(const char *ptrmx)
	{
		if(ptrmx==NULL) return;
		const char *ptr=strchr(ptrmx,',');
		while(true){
			while(*ptrmx==' ') ptrmx++; //去掉前导空格
			if(ptr) *(char *)ptr=0;
			if(ptrmx[0]!=0) mxhosts.push_back(ptrmx);
			if(ptr==NULL) break;
			*(char *)ptr=','; ptrmx=ptr+1;
			ptr=strchr(ptrmx,',');
		}//?while(true)
	}
}MXINFO;
static std::map<std::string,MXINFO> map_mxhost;
//获取MX邮箱名称
MXINFO *dnsMX(const char *dnssvr,int dnsport,const char *domainMX)
{
	if(domainMX==NULL || domainMX[0]==0) return NULL;
	std::string mxname,dm; dm.assign(domainMX);
	::strlwr((char *)dm.c_str());//转换为小写
	std::map<std::string,MXINFO>::iterator it=map_mxhost.find(dm);
	if(it!=map_mxhost.end())
	{
		MXINFO &mxinfo=(*it).second;
		if(mxinfo.tExpire==-1 || (time(NULL)-mxinfo.tStart)<mxinfo.tExpire)
			return &mxinfo;
	}//?if(it!=map_mxhost.end())
	
	int icount=0;//尝试进行DNS域名解析计数
	dnsClient dnsc; DNS_RDATA_MX mxRdata;
	while(icount++<2)
	{
		if(dnsc.status()!=SOCKS_CLOSED) dnsc.Close();
		if(dnsc.Open(0)<=0) return NULL;
		RW_LOG_DEBUG("[smtpclnt] Begin to Query MX of %s\r\n",domainMX);
		SOCKSRESULT sr=dnsc.Query_MX(domainMX,dnssvr,dnsport);
		PDNS_HEADER pdnsh=dnsc.resp_dnsh();
		if(sr==DNS_RCODE_ERR_OK) //解析MX邮箱域名成功
		{
			if( pdnsh->answers<=0) continue; //继续尝试
			for(int i=0;i<pdnsh->answers;i++)
			{
				PDNS_RESPONSE pdnsr=dnsc.resp_dnsr(i);
				if(pdnsr->type==T_MX)
				{
					dnsc.parse_rdata_MX(&mxRdata);
					RW_LOG_DEBUG("\t %d [%d] %s\r\n",i+1,mxRdata.priority,mxRdata.mxname.c_str());
					mxname+=mxRdata.mxname+string(",");
				}
			}//for(...
			RW_LOG_DEBUG("[smtpclnt] Success to Query MX,results=%d\r\n%s\r\n",
				pdnsh->answers,mxname.c_str());
			if(mxname!="") break; else continue; //继续尝试
		}else if(sr==SOCKSERR_TIMEOUT)  continue; //超时尝试
		//否则错误，可能无法进行域名解析
		RW_LOG_DEBUG("[smtpclnt] Failed to Query MX,error=%d\r\n",sr);
		break;
	}//?while
	if(mxname=="") return NULL;
	MXINFO mxinfo,&rmxinfo=(it!=map_mxhost.end())?(*it).second:mxinfo;
	rmxinfo.mxhosts.clear(); rmxinfo.setmx(mxname.c_str());
	rmxinfo.tStart=time(NULL); rmxinfo.tExpire=3600*12; //有效时间，12小时
	if(it==map_mxhost.end()) map_mxhost[dm]=rmxinfo;
	return &map_mxhost[dm];
}

smtpClient :: smtpClient(const char *ehlo)
{
	m_lTimeout=SMTP_MAX_RESPTIMEOUT;
	if(ehlo) m_ehloName.assign(ehlo);
	//读取静态mx信息
	FILE *fp=::fopen("mx_hosts","rb");
	if(fp)
	{
		char *pline,sline[256];
		while( ::fgets(sline,256,fp) ){
			int len=strlen(sline);
			pline=sline+len-1;//去掉尾部的空格和回车换行符
			while(pline>=sline && (*pline=='\r' || *pline=='\n' || *pline==' '))
			{ *(char *)pline=0; pline--; }
			pline=sline; while(*pline==' ') pline++; //去掉前导空格
			if(pline[0]==0 || pline[0]=='!') continue; //空行或注释行
			
			const char *ptrmx=strchr(pline,' '); //劈分域名和MX信息
			if(ptrmx==NULL) if( (ptrmx=strchr(pline,'\t'))==NULL) continue;
			*(char *)ptrmx='\0'; if(pline[0]==0) continue; //无效的邮箱域名
			while(*ptrmx==' ' || *ptrmx=='\t') ptrmx++; //跳过分隔符号
			MXINFO mxinfo; mxinfo.tStart=mxinfo.tExpire=-1;
			mxinfo.setmx(ptrmx); 
			std::string dm; dm.assign(::strlwr(pline));//转换为小写
			if(mxinfo.mxhosts.size()>0) map_mxhost[dm]=mxinfo;
		}//?while( ::fgets(sline,256,fp)
		::fclose(fp);
	}//?if(fp)
}
smtpClient :: ~smtpClient(){}

//设置smtp服务的验证类型和帐号
void smtpClient :: setSMTPAuth(SMTPAUTH_TYPE authType,
							   const char *strAccount,const char *strPwd)
{
	m_authType=authType;//如果设置SMTPAUTH_NONE则帐号无意义
	if(strAccount) m_strAccount.assign(strAccount);
	if(strPwd) m_strPwd.assign(strPwd);
	return;
}

//****************************************
// 函数功能：邮件直投。成功返回SOCKSERR_OK
//****************************************
SOCKSRESULT smtpClient :: sendMail_MX(mailMessage &mms,const char *dnssvr,int dnsport)
{
	std::vector<std::pair<std::string,std::string> > *pvec=NULL;
	const char *emailDomain_ptr=NULL,*emailDomain_pre=NULL;
	std::string strDomainMX;//MX邮箱域名
	m_authType=SMTPAUTH_NONE;//邮件直投，连接MX服务器无需认证
	int i,okCount=0,errCount=0; m_errors.clear();
	SOCKSRESULT sr=SOCKSERR_SMTP_RECIPIENT;
	while(true)
	{
		if(pvec==NULL) //循环发送到各个目的邮箱
			pvec=&mms.vecTo(mailMessage::TO);
		else if(pvec==&mms.vecTo(mailMessage::TO) )
			pvec=&mms.vecTo(mailMessage::CC);
		else if(pvec==&mms.vecTo(mailMessage::CC))
			pvec=&mms.vecTo(mailMessage::BCC);
		else break;
		std::vector<std::pair<std::string,std::string> >::iterator it=pvec->begin();
		for(;it!=pvec->end();it++) 
		{//获取目的邮箱的域名
			errCount++; MXINFO *pmxinfo=NULL;
			emailDomain_ptr=strrchr((*it).first.c_str(),'@');
			if(emailDomain_ptr) //进行dns解析获取MX邮箱服务器域名
				pmxinfo=dnsMX(dnssvr,dnsport,emailDomain_ptr+1);
			if(pmxinfo==NULL)
			{
				string s=(*it).first; s.append(" failed to parse Domain name.\r\n");
				RW_LOG_PRINT(LOGLEVEL_DEBUG,"[smtpclnt] %s",s.c_str());
				m_errors.push_back(s); continue;
			}//?if(pmxinfo==NULL)
			for(i=0;i<pmxinfo->mxhosts.size();i++)
			{
				sr=ConnectSvr(pmxinfo->mxhosts[i].c_str(),SMTP_SERVER_PORT);
				if(sr!=SOCKSERR_OK){ Close(); continue; }
				RW_LOG_DEBUG("[smtpclnt] begin to send email to %s into %s\r\n",
					(*it).first.c_str(),pmxinfo->mxhosts[i].c_str());
				sr=_sendMail(mms,(*it).first.c_str());
				RW_LOG_DEBUG("[smtpclnt] end to send email to %s into %s\r\n",
					(*it).first.c_str(),pmxinfo->mxhosts[i].c_str());
				this->Close(); //断开连接
				if( sr==SOCKSERR_OK) break; //成功发送邮件
				//如果一个MX邮箱响应错误，则不再进行循环直接失败跳出
				if(sr==SOCKSERR_SMTP_RESP){ i=pmxinfo->mxhosts.size(); break; }
			}//?for(int i=0;i<pmxinfo->
			if(i<pmxinfo->mxhosts.size()){ 
				okCount++; errCount--;
				RW_LOG_DEBUG("[smtpclnt] Successs to send email to %s\r\n",(*it).first.c_str());
			}else RW_LOG_DEBUG("[smtpclnt] Failed to send email to %s\r\n",(*it).first.c_str());
		}//?for(;it!=pvec->end();it++) 
	}//?while(true)
//	if(errCount>0) sr=SOCKSERR_SMTP_FAILED; //邮件未完全发送成功
	return (okCount>0)?SOCKSERR_OK:SOCKSERR_SMTP_FAILED;
}

//****************************************
// 函数功能：邮件直投。成功返回SOCKSERR_OK
//****************************************
SOCKSRESULT smtpClient :: sendMail_MX(const char *emlfile,const char *dnssvr,int dnsport)
{
	mailMessage mms;
	if(mms.initFromemlfile(emlfile)!=SOCKSERR_OK)
		return SOCKSERR_SMTP_EMLFILE;
	return sendMail_MX(mms,dnssvr,dnsport);
}
//****************************************
// 函数功能：连接smtp服务器 ，发送指定邮件。成功返回SOCKSERR_OK
//****************************************
SOCKSRESULT smtpClient :: sendMail(const char *emlfile,const char *smtpsvr,int smtpport,const char *from)
{
	mailMessage mms; if(from) mms.setFrom(from,NULL);
	if(mms.initFromemlfile(emlfile)!=SOCKSERR_OK)
		return SOCKSERR_SMTP_EMLFILE;
	return sendMail(mms,smtpsvr,smtpport);
}
SOCKSRESULT smtpClient :: sendMail(mailMessage &mms,const char *smtpsvr,int smtpport)
{
	//连接指定的邮件服务器
	SOCKSRESULT sr=ConnectSvr(smtpsvr,smtpport);
	if(sr!=SOCKSERR_OK) return sr;
	sr=_sendMail(mms,NULL);
	this->Close(); return sr;
}

//****************************************
// 函数功能：发送指定邮件。成功返回SOCKSERR_OK
//toemail -- 如果toemail不为空则发送到toemail指定的邮箱，否则发送到mms指定的邮件接收者
//****************************************
SOCKSRESULT smtpClient :: _sendMail(mailMessage &mms,const char *toemail)
{
//	if(this->status()!=SOCKS_CONNECTED) return SOCKSERR_CLOSED;
	char buf[SMTP_MAX_PACKAGE_SIZE];
	//发送"MAIL FROM:"
	int buflen=sprintf(buf,"MAIL FROM: <%s>\r\n",mms.from());
	if(!sendCommand(250,buf,buflen,SMTP_MAX_PACKAGE_SIZE)){
		m_errors.push_back(string(buf)); //获取错误响应
		return SOCKSERR_SMTP_RESP;
	}

	int rcpt_count=0;//发送"RCPT TO:"
	if(toemail)
	{
		buflen=sprintf(buf,"RCPT TO: <%s>\r\n",toemail);
		if(sendCommand(250,buf,buflen,SMTP_MAX_PACKAGE_SIZE)) rcpt_count++;
	}else{
		std::vector<std::pair<std::string,std::string> > *pvec=NULL;
		while(true)
		{
			if(pvec==NULL) 
				pvec=&mms.vecTo(mailMessage::TO);
			else if(pvec==&mms.vecTo(mailMessage::TO) )
				pvec=&mms.vecTo(mailMessage::CC);
			else if(pvec==&mms.vecTo(mailMessage::CC))
				pvec=&mms.vecTo(mailMessage::BCC);
			else break;
			std::vector<std::pair<std::string,std::string> >::iterator it=pvec->begin();
			for(;it!=pvec->end();it++)
			{
				buflen=sprintf(buf,"RCPT TO: <%s>\r\n",(*it).first.c_str());
				if(sendCommand(250,buf,buflen,SMTP_MAX_PACKAGE_SIZE)) rcpt_count++;
			}
		}//?while(true)
	}//?if(toemail)...else
	if(rcpt_count<=0) return SOCKSERR_SMTP_RECIPIENT;
	
	//生成邮件数据，准备发送
	RW_LOG_DEBUG(0,"Be encoding mail data,please waiting...\r\n");
	//生成用于保存编码后邮件正文的临时文件
	const char *maildatafile_ptr=mms.createMailFile(NULL,true);
	RW_LOG_DEBUG("Ready for sending mail data, %s.\r\n",maildatafile_ptr);
	if(maildatafile_ptr==NULL) return SOCKSERR_SMTP_FAILED; 

	//发送"DATA\r\n"
	buflen=sprintf(buf,"DATA\r\n");
	if(!sendCommand(354,buf,buflen,SMTP_MAX_PACKAGE_SIZE))
	{
		m_errors.push_back(string(buf)); //获取错误响应
		return SOCKSERR_SMTP_RESP;
	}else{
		FILE *fp=::fopen(maildatafile_ptr,"rb");
		long pos=mms.MailFileStartPos();
		if(pos>0) ::fseek(fp,pos,SEEK_SET);
		char readbuf[2048];
		while(true)
		{
			buflen=::fread(readbuf,sizeof(char),2048,fp);
			if(buflen<=0) break;
			if( this->Send(buflen,readbuf,-1)< 0) break;
			if(m_parent && m_parent->status()<=SOCKS_CLOSED) break;
		}//?while
		RW_LOG_DEBUG("Success to send mail data, size=%d.\r\n",::ftell(fp));
		::fclose(fp);
	}
	//发送邮件数据传输完毕标志
	strcpy(buf,"\r\n.\r\n"); buflen=5;
	if(!sendCommand(250,buf,buflen,SMTP_MAX_PACKAGE_SIZE)){
		m_errors.push_back(string(buf)); //获取错误响应
		return SOCKSERR_SMTP_RESP;
	}
	buflen=sprintf(buf,"QUIT\r\n");
	sendCommand(221,buf,buflen,SMTP_MAX_PACKAGE_SIZE);
	return SOCKSERR_OK;
}



//****************************************
// 函数功能：连接smtp服务器 成功返回SOCKSERR_OK
//****************************************
SOCKSRESULT smtpClient :: ConnectSvr(const char *smtpsvr,int smtpport)
{
	SOCKSRESULT sr=this->Connect(smtpsvr,smtpport);
	if(sr<0){
		RW_LOG_DEBUG("[smtpclnt] Failed to connect SMTP (%s:%d),err=%d\r\n",smtpsvr,smtpport,sr);
		return SOCKSERR_SMTP_CONN;
	}
	
	//等待接收服务器的响应
	char buf[SMTP_MAX_PACKAGE_SIZE];
	if(!sendCommand(220,buf,0,SMTP_MAX_PACKAGE_SIZE)) return SOCKSERR_SMTP_CONN;
	//发送EHLO命令
	int buflen=sprintf(buf,"EHLO %s\r\n",(m_ehloName!="")?m_ehloName.c_str():smtpsvr);
	if(!sendCommand(250,buf,buflen,SMTP_MAX_PACKAGE_SIZE))
		return SOCKSERR_SMTP_CONN;

	if(m_authType==SMTPAUTH_NONE) return SOCKSERR_OK;
	//SMTP服务器要求进行LOGIN验证
	if(m_authType!=SMTPAUTH_LOGIN) return SOCKSERR_SMTP_SURPPORT;
	return (Auth_LOGIN()==SOCKSERR_OK)?SOCKSERR_OK:SOCKSERR_SMTP_AUTH;
}

//****************************************
// 函数功能：服务器验证 成功返回SOCKSERR_OK
//****************************************
SOCKSRESULT smtpClient::Auth_LOGIN()
{	
	char buf[SMTP_MAX_PACKAGE_SIZE]; 
	int buflen=sprintf(buf,"AUTH LOGIN\r\n");
	if(!sendCommand(334,buf,buflen,SMTP_MAX_PACKAGE_SIZE))
		return SOCKSERR_SMTP_RESP;
	//发送经过Base64编码的用户帐号：
	buflen=cCoder::base64_encode((char *)m_strAccount.c_str(),
			m_strAccount.length(),buf);
	buf[buflen++]='\r'; buf[buflen++]='\n'; buf[buflen]=0;
	if(!sendCommand(334,buf,buflen,SMTP_MAX_PACKAGE_SIZE))
		return SOCKSERR_SMTP_RESP;
	//发送经过Base64编码的用户密码：
	buflen=cCoder::base64_encode((char *)m_strPwd.c_str(),
			m_strPwd.length(),buf);
	buf[buflen++]='\r'; buf[buflen++]='\n'; buf[buflen]=0;
	if(!sendCommand(235,buf,buflen,SMTP_MAX_PACKAGE_SIZE))
		return SOCKSERR_SMTP_AUTH; //认证失败
	
	return SOCKSERR_OK;
}

//发送命令，并获取服务器响应
//[in] response_expected --- 期望服务的响应码
//[in] buf 发送缓冲，同时作为接收响应数据缓冲
//[in] buflen 要发送数据的大小， maxbuflen指明buf缓冲的大小
bool smtpClient :: sendCommand(int response_expected,char *buf,int buflen
									  ,int maxbuflen)
{
	if(buflen>0)
	{
		RW_LOG_DEBUG("[smtpclnt] c--->s:\r\n\t%s",buf);
		if( this->Send(buflen,buf,-1)<=0 ){ buf[0]=0;  return false; }
	}
	//发送成功，等待接收服务器响应
	SOCKSRESULT sr=this->Receive(buf,maxbuflen-1,m_lTimeout);
	if(sr<=0) {
		RW_LOG_DEBUG(0,"[smtpclnt] failed to receive responsed message.\r\n");
		buf[0]=0; return false; 
	} else buf[sr]=0;
	RW_LOG_DEBUG("[smtpclnt] s--->c:\r\n\t%s",buf);
	int responseCode=atoi(buf);

	//也许会是多行响应,如果不是最后一行则行响应头为 "DDD- ...\r\n"
	//多行响应的最后一行为 "DDD ...\r\n"
	while(true)
	{
		bool bReceivedAll=(buf[sr-1]=='\n'); //有可能接收完
		if(bReceivedAll)
		{
			buf[sr-1]=0; //获取最后一行
			const char *ptr=strrchr(buf,'\n');
			buf[sr-1]='\n';
			if(ptr==NULL) ptr=buf; else ptr++;
			if(atoi(ptr)!=responseCode || ptr[3]!='-') break;
		}
		sr=this->Receive(buf,maxbuflen-1,m_lTimeout);
		if(sr<=0) break; else buf[sr]=0;
		RW_LOG_DEBUG("%s",buf);
	}//?while
	return (response_expected==responseCode);
}

//-----------------------------------------------------------------------------
//---------------------------------mailMessage---------------------------------

mailMessage :: ~mailMessage()
{
	if(m_strMailFile!="" && m_bDeleteFile)
		::DeleteFile(m_strMailFile.c_str());
}

//解析email地址，获取名称和email。例如myname<my@163.com> 或my@163.com
void parseEmail(std::string &strEmail,std::string &strName)
{
	const char *ptrS,*ptrE,*ptremail=strEmail.c_str();
	ptrS=strchr(ptremail,'<');
	if(ptrS==NULL) return;
	ptrE=strchr(ptrS+1,'>');
	if(ptrE==NULL) return;
	if(ptrS==ptremail) strName.assign(ptrE+1);
	else strName.assign(ptremail,ptrS-ptremail);
	strEmail.assign(ptrS+1,ptrE-ptrS-1);
}
//****************************************
//从邮件文件初始化mailMessage对象
//成功返回SOCKSERR_OK
//emlfile : 邮件格式文件，两种格式文件
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
#define READ_BUFFER_SIZE 2048
SOCKSRESULT mailMessage :: initFromemlfile(const char *emlfile)
{
	FILE *fp=(emlfile==NULL)?NULL:(::fopen(emlfile,"rb"));
	if(fp==NULL) return SOCKSERR_SMTP_EMLFILE;
	char *pline,sline[READ_BUFFER_SIZE];
	mailMessage::EMAILBODY_TYPE bt=mailMessage::TEXT_BODY;
	int len,emltype=-1;//email body经base64编码 0未编码 1Base64编码
	while( ::fgets(sline,READ_BUFFER_SIZE,fp) )
	{
		len=strlen(sline); pline=sline+len-1;
		while(pline>=sline && (*pline=='\r' || *pline=='\n' || *pline==' '))
		{ *(char *)pline=0; pline--; }//去掉尾部的空格和回车换行符
		pline=sline; while(*pline==' ') pline++; //去掉前导空格
		if(emltype==-1){//根据获取的第一行，判断是否是base64编码的邮件
			emltype=(strcasecmp(pline,"Email body is base64 encoded")==0)?1:0;
			if(emltype==1) continue; //base64编码,读取下一行数据
		}//?if(emltype==-1)
		if(pline[0]==0) break; //空行，说明下面的为邮件正文，不再解释
		if(pline[0]=='!') continue; //注释行,跳过
		if(strncasecmp(pline,"FROM: ",6)==0)
		{//发件人
			pline+=6; while(*pline==' ') pline++;//删除前导空格
			m_strFrom.assign(pline); m_strName="";
			parseEmail(m_strFrom,m_strName); //拆分出name和邮件地址
		}else if(strncasecmp(pline,"TO: ",4)==0)
		{//收件人，多个接收者之间用逗号分割
			pline+=4; while(*pline==' ') pline++;//删除前导空格
			while(true){
				char *ptr=strchr(pline,',');
				if(ptr) *ptr='\0';
				std::string strName,strEmail(pline);
				parseEmail(strEmail,strName); //拆分出name和邮件地址
				this->AddRecipient(strEmail.c_str(),m_strName.c_str(),mailMessage::TO);
				if(ptr==NULL) break; else *ptr=',';
				pline=ptr+1;while(*pline==' ') pline++;
			}//?while(true)
		}else if(emltype==1) continue; //Base64编码的邮件，其余部分不再解析
		////未经base64编码eml邮件，解析其余部分
		else if(strncasecmp(pline,"Attachs: ",9)==0) 	
		{//邮件附件，多个附件之间用逗号分割
			pline+=9; while(*pline==' ') pline++;//删除前导空格
			while(true){
				char *ptr=strchr(pline,',');
				if(ptr) *ptr='\0'; //格式<contentID>aaa.jpg 或 aaa.jpg
				std::string strContentID,strFilename(pline); 
				parseEmail(strFilename,strContentID); //拆分，<>中的是contentID
				if(strContentID!="")//parseEmail解析是反的，借用此函数而已
					 this->AddAtach(strContentID.c_str(),emlfile,strFilename.c_str());
				else this->AddAtach(strFilename.c_str(),emlfile,NULL);
				if(ptr==NULL) break; else *ptr=',';
				pline=ptr+1;while(*pline==' ') pline++;
			}//?while(true)
		}else if(strncasecmp(pline,"Bodytype: ",10)==0)	
		{//邮件正文类型
			pline+=10; while(*pline==' ') pline++;//删除前导空格
			bt=(strcasecmp(pline,"HTML")==0)?mailMessage::HTML_BODY:mailMessage::TEXT_BODY;
		}else if(strncasecmp(pline,"Charsets: ",10)==0)	
		{//邮件正文类型采用的字符集编码
			pline+=10; while(*pline==' ') pline++;//删除前导空格
			if(pline[0]!=0) this->m_strBodyCharset.assign(pline);
		}else if(strncasecmp(pline,"Subject: ",9)==0)	
		{//邮件主题
			pline+=9; while(*pline==' ') pline++;//删除前导空格
			m_strSubject.assign(pline);
		}	
	}//?while
	this->m_contentType=bt; //设置邮件正文类型
	if( emltype==0 ){ //非Base64编码的邮件
		m_strMailFile="";
		while(true){
			size_t l=::fread((void *)sline,sizeof(char),READ_BUFFER_SIZE-1,fp);
			sline[l]=0; this->body().append(sline);
			if(l<(READ_BUFFER_SIZE-1)) break; //已读完毕
		} 
	}else this->setBody(emlfile,::ftell(fp));
	::fclose(fp); return SOCKSERR_OK;
}

//添加附件,filepath指定附件的相对路径
//contentID - 指定contentID，如果指定了则为html正文中的图像文件 //yyc add 2006-07-20
#include <io.h>
bool mailMessage::AddAtach(const char *filename,const char *filepath,const char *contentID)
{
	if(filename==NULL || filename[0]==0) return false;
	std::string strfile;
	if(filepath!=NULL && filename[1]!=':') //如果filename指向的不是绝对路径
	{
		const char *ptr=strrchr(filepath,'\\');
		if(ptr) strfile.assign(filepath,ptr-filepath+1);
		strfile.append(filename);
	}
	else strfile.assign(filename);
	if(_access(strfile.c_str(),0)==-1) return false;
	if(contentID && contentID[0]!=0)
	{
		char tmpbuf[128]; sprintf(tmpbuf,"<%s>",contentID);
		strfile.insert(0,tmpbuf);
	}
	m_attachs.push_back(strfile);
	return true;
}

//添加收件人
bool mailMessage::AddRecipient(const char *email,const char *nick,RECIPIENT_TYPE rt)
{
	if(email==NULL || email[0]==0) return false;
	if(nick==NULL || nick[0]==0) nick=email;
	std::pair<std::string,std::string> p(email,nick);
	std::vector<std::pair<std::string,std::string> > *pvec=&m_vecTo;
	if(rt==CC) pvec=&m_vecCc; else if(rt==BCC) pvec=&m_vecBc;
	pvec->push_back(p);
	return true;
}

const char MAILBOUNDARY_STRING[]="#yycnet.yeah.net#";
//对指定的文件进行base64编码，并写入指定的流
//contentID - 指定contentID，如果指定了则为html正文中的图像文件 //yyc add 2006-07-20
bool base64File(const char *filename,ofstream &out,const char *contentID)
{
	FILE *fp=::fopen(filename,"rb");
	if(fp==NULL) return false;
	long len=strlen(filename);
	const char *fname=filename;//获取文件名称
	for(int i=len-1;i>=0;i--)
	{
		if(*(filename+i)=='\\' || *(filename+i)=='/'){ fname=filename+i+1; break; }
	}//?for
	
	char srcbuf[1024],dstbuf[1500];
	static char *ct[]={"application/octet-stream","image/jpeg","image/gif","image/png","image/bmp","image/tif"};
	int idx_ct=0; const char *ptr=strrchr(fname,'.');
	if(ptr && (strcasecmp(ptr,".jpg")==0 || strcasecmp(ptr,".jpeg")==0) )
		idx_ct=1;
	else if(ptr && strcasecmp(ptr,".gif")==0)
		idx_ct=2;
	else if(ptr && strcasecmp(ptr,".png")==0)
		idx_ct=3;
	else if(ptr && strcasecmp(ptr,".bmp")==0)
		idx_ct=4;
	else if(ptr && strcasecmp(ptr,".tif")==0)
		idx_ct=5;
	
	if(contentID==NULL || contentID[0]==0)
	{
		len=sprintf(srcbuf,"--%s_000\r\n"
						   "Content-Type:%s;\r\n"
						   "\tName=\"%s\"\r\n"
					       "Content-Disposition:attachment;\r\n"
						   "\tFileName=\"%s\"\r\n"
					       "Content-Transfer-Encoding: base64\r\n\r\n",
							MAILBOUNDARY_STRING,ct[idx_ct],fname,fname);
	}else{
		const char *p1,*ptr_ct=ct[idx_ct];
		if(contentID[0]=='(' && (p1=strchr(contentID+1,')')) ){
			*(char *)p1='\0'; 
			ptr_ct=contentID+1; contentID=p1+1;
		}
		len=sprintf(srcbuf,"--%s_000\r\n"
						   "Content-Type: %s;\r\n"
						   "\tname=\"%s\"\r\n"
					       "Content-Transfer-Encoding: base64\r\n"
						   "Content-ID: <%s>\r\n\r\n",
							MAILBOUNDARY_STRING,ptr_ct,fname,contentID);
	}
	
	out.write(srcbuf,len);
	//开始base64编码指定的文件
	while(true)
	{ //每次读3的辈数字节，看base64编码
		len=::fread(srcbuf,sizeof(char),1020,fp);
		if(len<=0) break;
		len=cCoder::base64_encode(srcbuf,len,dstbuf);
		out.write(dstbuf,len); out.write("\r\n",2);
	}//?while
	out.write("\r\n",2);//加一个空行表示附件base64编码结束
	::fclose(fp); return true;
}
//生成Base64编码的邮件体文件
//bDelete -- 指示当mailMessage对象释放时是否删除生成的文件
//成功返回生成的文件名
const char * mailMessage::createMailFile(const char *file,bool bDelete)
{
	if(file==NULL || file[0]==0)
	{
		if(m_strMailFile!="" && _access(m_strMailFile.c_str(),0)!=-1)
			return m_strMailFile.c_str();
		//生成用于保存编码后邮件正文的临时文件
		char buf[64]; time_t tNow=time(NULL);
		srand( (unsigned)clock() );
		struct tm * ltime=localtime(&tNow);
		sprintf(buf,"tmp%04d%02d%02d%02d%02d%02d_%d.eml",
			(1900+ltime->tm_year), ltime->tm_mon+1, ltime->tm_mday, 
			ltime->tm_hour, ltime->tm_min, ltime->tm_sec,rand());
		m_strMailFile.assign(buf);
	}
	else m_strMailFile.assign(file);

	ofstream out(m_strMailFile.c_str(),ios::binary);
	if(out.is_open()==0){m_strMailFile=""; return NULL;}
	m_bDeleteFile=bDelete;

	//From:
	out << "From: " << ((m_strName!="")?m_strName:m_strFrom);
	out << " <" << m_strFrom << ">\r\n";
	//To:
	if(m_vecTo[0].second=="")
		out << "To: <" << m_vecTo[0].first << ">\r\n";
	else
		out << "To: \"" << m_vecTo[0].second << "\" <" << m_vecTo[0].first << ">\r\n";
	for(int i=1;i<m_vecTo.size();i++)
	{
		if(m_vecTo[i].second=="")
			out << "\t <" << m_vecTo[i].first << ">\r\n";
		else
			out << "\t \"" << m_vecTo[i].second << "\" <" << m_vecTo[i].first << ">\r\n";
	}
	//CC:
	if(m_vecCc.size()>0)
	{
		if(m_vecCc[0].second=="")
			out << "CC: <" << m_vecCc[0].first << ">\r\n";
		else
			out << "CC: \"" << m_vecCc[0].second << "\" <" << m_vecCc[0].first << ">\r\n";
		for(int i=1;i<m_vecCc.size();i++)
		{
			if(m_vecCc[i].second=="")
				out << "\t <" << m_vecCc[i].first << ">\r\n";
			else
				out << "\t \"" << m_vecCc[i].second << "\" <" << m_vecCc[i].first << ">\r\n";
		}
	}//?m_vecCc.size()
	//Subject:
	out << "Subject: " << m_strSubject << "\r\n";
	//Date:
	char datebuf[64];
	int len=sprintf(datebuf,"Date: "); datebuf[len]=0;
	cTime t=cTime::GetCurrentTime();
	int tmlen=t.Format(datebuf+len,48,"%a, %d %b %Y %H:%M:%S %Z");
	if(tmlen<=0) tmlen=0; len+=tmlen; 
	datebuf[len++]='\r'; datebuf[len++]='\n'; datebuf[len]=0;
	out << datebuf;

	out << "X-Mailer: YMailer v2.1\r\nMIME-Version: 1.0\r\n";
	out << "Content-Type: multipart/related;\r\n\ttype=\"multipart/alternative\";\r\n";
	out << "\tboundary=\"" << MAILBOUNDARY_STRING << "_000\"\r\n" << "\r\n";
	out << "This is a multi-part message in MIME format\r\n" << "\r\n";
	
	out << "--" << MAILBOUNDARY_STRING << "_000\r\n";
	out << "Content-Type: multipart/alternative;\r\n";
	out << "\tboundary=\"" << MAILBOUNDARY_STRING << "_001\"\r\n";
	out << "\r\n\r\n";
	
	char *ptr_encodeBody=NULL;
	if(strcasecmp(m_strBodyCharset.c_str(),"utf-8")==0)
	{
		//对邮件正文进行utf8和Base64编码
		long encode_bodylen=cCoder::Utf8EncodeSize(m_strBody.length());
		ptr_encodeBody=new char[encode_bodylen+1];
		if(ptr_encodeBody){
			encode_bodylen=cCoder::utf8_encode(m_strBody.c_str(),m_strBody.length(),ptr_encodeBody);
			char *ptr_utf8=ptr_encodeBody;
			//然后对utf8进行base64编码
			if( (ptr_encodeBody=new char[cCoder::Base64EncodeSize(encode_bodylen)+1]) )
				cCoder::base64_encode(ptr_utf8,encode_bodylen,ptr_encodeBody);
			delete[] ptr_utf8;
		}//?if(ptr_encodeBody)
	}
	//multi-part的第一部分邮件可选正文
	//文本正文-----------------
	out << "--" << MAILBOUNDARY_STRING << "_001\r\n";
	out << "Content-type: text/plain;\r\n";
	if(m_contentType!=HTML_BODY)
	{
		if(ptr_encodeBody)
		{	
			out << "\tCharset=\"utf-8\"\r\nContent-Transfer-Encoding: base64\r\n\r\n";
			out<< ptr_encodeBody;
		}else{
			out << "\tCharset=\"" << m_strBodyCharset << "\"\r\nContent-Transfer-Encoding: 8bit\r\n\r\n";
			out << m_strBody;
		}
	}else{
		out << "\tCharset=\"gb2312\"\r\nContent-Transfer-Encoding: 8bit\r\n\r\n";
		out << "The email is HTML format!\r\n这是一个HTML格式邮件!";
	}
	out <<"\r\n\r\n";
	//HTML正文-----------------
	if(m_contentType==HTML_BODY){
		out << "--" << MAILBOUNDARY_STRING << "_001\r\n";
		out << "Content-type: text/html;\r\n";
		if(ptr_encodeBody)
		{	
			out << "\tCharset=\"utf-8\"\r\nContent-Transfer-Encoding: base64\r\n\r\n";
			out<< ptr_encodeBody;
		}else{
			out << "\tCharset=\"" << m_strBodyCharset << "\"\r\nContent-Transfer-Encoding: 8bit\r\n\r\n";
			out << m_strBody;
		}
		out <<"\r\n\r\n";
	}
	if(ptr_encodeBody) delete[] ptr_encodeBody;
	out <<"--" << MAILBOUNDARY_STRING <<"_001--\r\n\r\n";
	//multi-part的第一部分邮件可选正文处理结束
	
	//开始进行邮件附件的处理
	int fileAttachs=0;//有效附件数
	for(i=0; i<m_attachs.size();i++)
	{//如果附件文件名的开头为<>,则<>中的类容代表contentID
		const char *ptr=m_attachs[i].c_str();
		const char *contentID=NULL;
		if(*ptr=='<'){
			contentID=ptr+1;
			if( (ptr=strchr(contentID,'>')) )
			{ *(char *)ptr='\0'; ptr++; }
			else{ptr=contentID; contentID=NULL; } 
		}
		if(base64File(ptr,out,contentID)) fileAttachs++;
	}//?for
	out <<"--" << MAILBOUNDARY_STRING <<((fileAttachs>0)?"_000--":"");
	return m_strMailFile.c_str();;
}

/*
//获取MX邮箱名称
//返回0 失败 返回1 从MX缓存获取MX邮箱域名成功 2 DNS解析获取MX邮箱域名
int dnsMX(const char *dnssvr,int dnsport,const char *domainMX,std::string &mxname)
{
	mxname="";//先通过mx_hosts获取MX邮箱名
	FILE *fp=::fopen("mx_hosts","rb");
	if(fp)
	{
		char *pline,sline[256]; int n=strlen(domainMX);
		while( ::fgets(sline,256,fp) ){
			int len=strlen(sline);
			pline=sline+len-1;//去掉尾部的空格和回车换行符
			while(pline>=sline && (*pline=='\r' || *pline=='\n' || *pline==' '))
			{ *(char *)pline=0; pline--; }
			pline=sline; while(*pline==' ') pline++; //去掉前导空格
			if(pline[0]==0 || pline[0]=='!') continue; //空行或注释行	
			if(strncasecmp(pline,domainMX,n)) continue; //不是所查找的域
			else pline+=n;
			if(*pline!=' ' && *pline!='\t') continue; //错误的行
			while(*pline==' ' || *pline=='\t') pline++; //跳过分隔符号
			mxname.assign(pline); break;
		}//?while( ::fgets(sline,256,fp)
		::fclose(fp);
	}//?if(fp)
	if(mxname!="") return 1;
	dnsClient dnsc; DNS_RDATA_MX mxRdata; 
	if(dnsc.Open(0)<=0) return 0;
	SOCKSRESULT sr=dnsc.Query_MX(domainMX,dnssvr,dnsport);
	if(sr==DNS_RCODE_ERR_OK) //解析MX邮箱域名成功
	{
		PDNS_HEADER pdnsh=dnsc.resp_dnsh();
		RW_LOG_DEBUG("[smtpclnt] Success to Query MX,results=%d.\r\n",pdnsh->answers);
		for(int i=0;i<pdnsh->answers;i++)
		{
			PDNS_RESPONSE pdnsr=dnsc.resp_dnsr(i);
			if(pdnsr->type==T_MX)
			{
				dnsc.parse_rdata_MX(&mxRdata);
				RW_LOG_DEBUG("\t %d [%d] %s\r\n",i+1,mxRdata.priority,mxRdata.mxname.c_str());
				mxname+=mxRdata.mxname+string(",");
			}
		}//for(...
		return 2;
	}else RW_LOG_DEBUG("[smtpclnt] Failed to Query MX,error=%d\r\n",sr);
	return 0;
}
SOCKSRESULT smtpClient :: sendMail_MX(mailMessage &mms,const char *dnssvr,int dnsport)
{
	std::vector<std::pair<std::string,std::string> > *pvec=NULL;
	const char *emailDomain_ptr=NULL,*emailDomain_pre=NULL;
	std::string strDomainMX;//MX邮箱域名
	m_authType=SMTPAUTH_NONE;//邮件直投，连接MX服务器无需认证
	int errCount=0; m_errors.clear();
	SOCKSRESULT sr=SOCKSERR_SMTP_RECIPIENT;
	while(true)
	{
		if(pvec==NULL) //循环发送到各个目的邮箱
			pvec=&mms.vecTo(mailMessage::TO);
		else if(pvec==&mms.vecTo(mailMessage::TO) )
			pvec=&mms.vecTo(mailMessage::CC);
		else if(pvec==&mms.vecTo(mailMessage::CC))
			pvec=&mms.vecTo(mailMessage::BCC);
		else break;
		std::vector<std::pair<std::string,std::string> >::iterator it=pvec->begin();
		for(;it!=pvec->end();it++) 
		{//获取目的邮箱的域名
			errCount++;
			emailDomain_ptr=strrchr((*it).first.c_str(),'@');
			if(emailDomain_ptr)
			{	
				emailDomain_ptr++; //进行dns解析获取MX邮箱服务器域名
				if(emailDomain_pre==NULL || strDomainMX=="" || 
					strcasecmp(emailDomain_ptr,emailDomain_pre)!=0)
					dnsMX(dnssvr,dnsport,emailDomain_ptr,strDomainMX);
				
				emailDomain_pre=emailDomain_ptr;
				RW_LOG_DEBUG("[smtpclnt] begin to send email to %s into %s.\r\n",
					(*it).first.c_str(),strDomainMX.c_str());

				if(strDomainMX!="")//MX域名解析完成
				{//准备发送邮件
					//strDomainMX是","号分割的多个域名字符串,循环获取MX域名
					const char *ptrDomainMX=strDomainMX.c_str();
					while(ptrDomainMX[0]!=0)
					{
						const char *delmPos=strchr(ptrDomainMX,',');
						if(delmPos) *(char *)delmPos='\0';
						
						sr=ConnectSvr(ptrDomainMX,SMTP_SERVER_PORT);
						if( sr==SOCKSERR_OK)
						{
							if( (sr=_sendMail(mms,(*it).first.c_str()))!=SOCKSERR_OK)
							{
								string s=(*it).first; s.append(" failed to send.\r\n");
								m_errors.push_back(s);
								RW_LOG_DEBUG("[smtpclnt] Failed to send, error=%d\r\n",sr);
								if(delmPos) { *(char *)delmPos=','; delmPos=NULL; } //退出循环
							}//?if( (sr=_sendMail(mms,(*it).first.c_str()))!=SOCKSERR_OK)
							else
							{ 
								errCount--;
								if(delmPos) { *(char *)delmPos=','; delmPos=NULL; } //退出循环
							}
						}//?if( sr==SOCKSERR_OK)
						else
						{
							string s=(*it).first; s.append(" failed to connect ");
							s+=string(ptrDomainMX); s.append(".\r\n"); m_errors.push_back(s);
							RW_LOG_DEBUG("[smtpclnt] Failed to connect %s,return %d.\r\n",ptrDomainMX,sr);
						}//?if( sr==SOCKSERR_OK)...else
						this->Close(); //断开连接

						if(delmPos){
							*(char *)delmPos=',';
							ptrDomainMX=delmPos+1;
						}else break;
					}//?while(...)
				}//?if(strDomainMX!="")
				else
				{
					sr=SOCKSERR_SMTP_DNSMX;
					string s=(*it).first; s.append(" failed to parse Domain name.\r\n");
					m_errors.push_back(s);
				}
				RW_LOG_DEBUG("[smtpclnt] end to send email to %s into %s.\r\n",
					(*it).first.c_str(),strDomainMX.c_str());
			}//?if(emailDomain_ptr)
			else
			{
				sr=SOCKSERR_SMTP_EMAIL;
				string s=(*it).first; s.append(" is not a valid email.\r\n");
				m_errors.push_back(s);
				RW_LOG_DEBUG(0,"[smtpclnt] "); RW_LOG_DEBUG(s.length(),s.c_str());
			}
		}//?for(;it!=pvec->end();it++)
	}//?while
	if(errCount>0) sr=SOCKSERR_SMTP_FAILED; //邮件未完全发送成功
	return sr;
}
*/