/*******************************************************************
   *	ftpsvr.cpp
   *    DESCRIPTION:FTP协议服务端实现
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-12-16
   *
   *	net4cpp 2.1
   *	文件传输协议
   *******************************************************************/

#include "../include/sysconfig.h"
#include "../include/ftpsvr.h"
#include "../utils/utils.h"
#include "../include/cLogger.h"

using namespace std;
using namespace net4cpp21;

cFtpsvr :: cFtpsvr()
{
	m_helloTip="";
	m_dataport_start=0;
	m_dataport_end=0;
}

SOCKSRESULT cFtpsvr :: delAccount(const char *struser)
{
	if((long)struser==-1){ m_accounts.clear(); return SOCKSERR_OK;}
	if(struser==NULL || struser[0]==0) return SOCKSERR_FTP_USER;
	::strlwr((char *)struser);
	std::map<std::string,FTPACCOUNT>::iterator it=m_accounts.find(struser);
	if(it==m_accounts.end()) return SOCKSERR_FTP_USER;
	if((*it).second.m_loginusers>0) return SOCKSERR_FTP_DENY;
	m_accounts.erase(it);
	return SOCKSERR_OK;
}

//获取帐号信息，返回指定的帐号对象
FTPACCOUNT * cFtpsvr :: getAccount(const char *struser)
{
	if(struser==NULL || struser[0]==0) return NULL;
	::strlwr((char *)struser);
	std::map<std::string,FTPACCOUNT>::iterator it=m_accounts.find(struser);
	if(it!=m_accounts.end()) return &(*it).second;
	return NULL;
}

//添加新帐号信息
FTPACCOUNT * cFtpsvr :: newAccount(const char *struser)
{
	if(struser==NULL || struser[0]==0) return NULL;
	FTPACCOUNT ftpa;
	ftpa.m_username.assign(struser);
	ftpa.m_maxupratio=0;
	ftpa.m_maxdwratio=0;
	ftpa.m_maxupfilesize=0;
	ftpa.m_loginusers=0;
	ftpa.m_maxLoginusers=0;
	ftpa.m_maxdisksize=0;
	ftpa.m_curdisksize=0;
	ftpa.m_limitedTime=0;
	ftpa.m_bitQX=0;
	ftpa.lRemoteAdmin(ACCOUNT_NORMAL);
	ftpa.lPswdMode(OTP_NONE);
	ftpa.bDsphidefiles(true);
	
	::strlwr((char *)struser);
	std::map<std::string,FTPACCOUNT>::iterator it=m_accounts.find(struser);
	if(it!=m_accounts.end()){
		if((*it).second.m_loginusers>0) return NULL;
		m_accounts.erase(it);
	}
	m_accounts[struser]=ftpa;
	it=m_accounts.find(struser);
	if(it==m_accounts.end()) return NULL;
	FTPACCOUNT &reftpa=(*it).second;
	reftpa.m_dirAccess["/"]=std::pair<std::string,long>("",FTP_ACCESS_ALL);
	return &reftpa;
}


//有一个新的客户连接此服务
void cFtpsvr :: onConnect(socketTCP *psock,time_t tmOpened,
						  unsigned long curConnection,unsigned long maxConnection)
{
	char buf[FTP_MAX_COMMAND_SIZE]; int buflen=0;
	buflen=sprintf(buf,"220-ftp4U(sftp2.2) FTP Server for ready.\r\n"
					   "220-copyright @yyc 2006. http://yycnet.yeah.net\r\n"
					   "%s",m_helloTip.c_str());
	psock->Send(buflen,buf,-1);
	
	struct tm * ltime=NULL;
	if(tmOpened!=0){//当前服务器运行状态
		ltime=localtime(&tmOpened);
		buflen=sprintf(buf,"220-服务器开始运行的时间是%04d-%02d-%02d %02d:%02d:%02d\r\n",
				(1900+ltime->tm_year), ltime->tm_mon+1, ltime->tm_mday, 
				ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
		psock->Send(buflen,buf,-1);
	}

	cFtpSession clientSession(psock,this); 
	clientSession.m_tmLogin=time(NULL); //用户连接时间
	ltime=localtime(&clientSession.m_tmLogin);
	buflen=sprintf(buf,"220-目前服务器所在的时间是%04d-%02d-%02d %02d:%02d:%02d\r\n"
					   "220-当前登陆用户数量 %d \r\n"
					   "220-最大允许连接用户数 %d \r\n"
					   "220 FTP Server for ready...\r\n",
					   (1900+ltime->tm_year), ltime->tm_mon+1, ltime->tm_mday, 
					   ltime->tm_hour, ltime->tm_min, ltime->tm_sec,
					   curConnection,maxConnection);
	psock->Send(buflen,buf,-1);

	buflen=0; //清空命令接收缓冲
	while( psock->status()==SOCKS_CONNECTED )
	{
		int iret=psock->checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ);
		if(iret<0) break; //如果登录超时则关闭连接
		if(clientSession.m_paccount==NULL && //判断是否登录超时
			(time(NULL)-clientSession.m_tmLogin)>FTP_MAX_LOGINTIMEOUT) break;
		if(iret==0) continue;
		//读客户端发送的数据
		iret=psock->Receive(buf+buflen,FTP_MAX_COMMAND_SIZE-buflen-1,-1);
		if(iret<0) break; //==0表明接收数据流量超过限制
		if(iret==0){ cUtils::usleep(MAXRATIOTIMEOUT); continue; }
		buflen+=iret; buf[buflen]=0;
		//解析ftp命令
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
	psock->Close(); //关闭连接
	while(clientSession.m_opMode!=0) cUtils::usleep(1000*100);//休眠100ms,等待数据传输线程结束
	if(clientSession.m_paccount){
		clientSession.m_paccount->m_loginusers--;
		onLogEvent(FTP_LOGEVENT_LOGOUT,clientSession);
	}
	RW_LOG_FFLUSH()
	return;
}

//当前连接数大于当前设定的最大连接数
void cFtpsvr :: onManyClient(socketTCP *psock)
{
	char resp[]="220 access denied, Too many users.\r\n";
	psock->Send(sizeof(resp)-1,resp,-1);
	return;
}

//-------------------FTP 命令解析处理 begin-----------------------------------
void cFtpsvr :: parseCommand(cFtpSession &clientSession,socketTCP *psock
									  ,const char *ptrCommand)
{
	RW_LOG_DEBUG("[ftpsvr] c--->s:\r\n\t%s\r\n",ptrCommand);
	if(strcasecmp(ptrCommand,"AUTH SSL")==0)
		docmd_authssl(psock);
	else if(strncasecmp(ptrCommand,"USER ",5)==0)
	{
		if(docmd_user(psock,ptrCommand+5,clientSession))
			onLogEvent(FTP_LOGEVENT_LOGIN,clientSession);
	}//?else if(strncasecmp(ptrCommand,"USER ",5)==0)
	else if(strcasecmp(ptrCommand,"FEAT")==0)
		docmd_feat(psock);
	else if(strcasecmp(ptrCommand,"QUIT")==0)
		docmd_quit(psock);
	else if(strcasecmp(ptrCommand,"NOOP")==0)
		resp_OK(psock);
	else if(strcasecmp(ptrCommand,"REIN")==0)
		docmd_rein(psock,clientSession);
	else if(clientSession.m_paccount==NULL) 
		resp_noLogin(psock);
	//以下的命令只有在登录后才可用
	else if(strncasecmp(ptrCommand,"TYPE ",5)==0)
		docmd_type(psock,ptrCommand+5,clientSession);
	else if(strncasecmp(ptrCommand,"REST ",5)==0)
		docmd_rest(psock,ptrCommand+5,clientSession);
	else if(strcasecmp(ptrCommand,"PWD")==0 || 
		    strcasecmp(ptrCommand,"XPWD")==0 )
		docmd_pwd(psock,clientSession);
	else if(strcasecmp(ptrCommand,"ABOR")==0)
		docmd_abor(psock,clientSession);
	else if(strcasecmp(ptrCommand,"CDUP")==0)
		docmd_cdup(psock,clientSession);
	else if(strncasecmp(ptrCommand,"CWD ",4)==0)
		docmd_cwd(psock,ptrCommand+4,clientSession);
	else if(strncasecmp(ptrCommand,"MKD ",4)==0) /*only for windows ftp控制台命令 */
		docmd_mkd(psock,ptrCommand+4,clientSession);
	else if(strncasecmp(ptrCommand,"XMKD ",5)==0)
		docmd_mkd(psock,ptrCommand+5,clientSession);
	else if(strncasecmp(ptrCommand,"RMD ",4)==0 || /*only for windows ftp控制台命令 */
		    strncasecmp(ptrCommand,"XRMD ",5)==0 )
	{
		unsigned long lsize=docmd_rmd(psock,ptrCommand+4,clientSession);
		if(clientSession.m_paccount->m_curdisksize<lsize )
			clientSession.m_paccount->m_curdisksize=0;
		else
			clientSession.m_paccount->m_curdisksize-=lsize;
		FTPACCOUNT *pftpa=getAccount(clientSession.m_paccount->m_username_root.c_str());
		if(pftpa)
		{
			if(pftpa->m_curdisksize<lsize)
				pftpa->m_curdisksize=0;
			else
				pftpa->m_curdisksize-=lsize;
		}//?if(pftpa)
	}
	else if(strncasecmp(ptrCommand,"DELE ",5)==0) //从服务器删除指定的文件
	{
		unsigned long lsize=docmd_dele(psock,ptrCommand+5,clientSession);
		if(clientSession.m_paccount->m_curdisksize<lsize )
			clientSession.m_paccount->m_curdisksize=0;
		else
			clientSession.m_paccount->m_curdisksize-=lsize;
		FTPACCOUNT *pftpa=getAccount(clientSession.m_paccount->m_username_root.c_str());
		if(pftpa)
		{
			if(pftpa->m_curdisksize<lsize)
				pftpa->m_curdisksize=0;
			else
				pftpa->m_curdisksize-=lsize;
		}//?if(pftpa)
	}
	else if(strncasecmp(ptrCommand,"RNFR ",5)==0) //更改文件或目录的名称
		docmd_rnfr(psock,ptrCommand+5,clientSession);
	else if(strncasecmp(ptrCommand,"RNTO ",5)==0) 
		docmd_rnto(psock,ptrCommand+5,clientSession);
	else if(strncasecmp(ptrCommand,"SIZE ",5)==0) //获取指定文件的大小
		docmd_size(psock,ptrCommand+5,clientSession);
	else if(strcasecmp(ptrCommand,"PASV")==0)
		docmd_pasv(psock,clientSession);
	else if(strncasecmp(ptrCommand,"PORT ",5)==0) 
		docmd_port(psock,(char *)ptrCommand+5,clientSession);
	else if(strncasecmp(ptrCommand,"PROT ",5)==0) 
		docmd_prot(psock,ptrCommand+5,clientSession);
	else if(strcasecmp(ptrCommand,"LIST")==0 || strcasecmp(ptrCommand,"NLST")==0)
		docmd_list(psock,ptrCommand+4,clientSession); 
	else if(strncasecmp(ptrCommand,"LIST ",5)==0)
		docmd_list(psock,ptrCommand+5,clientSession); 
	else if(strncasecmp(ptrCommand,"RETR ",5)==0) 
		docmd_retr(psock,ptrCommand+5,clientSession);
	else if(strncasecmp(ptrCommand,"STOR ",5)==0) 
		docmd_stor(psock,ptrCommand+5,clientSession);
	else if(strncasecmp(ptrCommand,"SITE PSWD ",10)==0) 
		docmd_pswd(psock,ptrCommand+10,clientSession); //修改密码
	else if(strcasecmp(ptrCommand,"SITE LIST")==0)//列出所有帐号
		docmd_sitelist(psock,clientSession);
	else if(!onCommandEx(psock,ptrCommand,clientSession))
		resp_unknowed(psock);

	return;
}

//数据传输任务包括LIST，文件的下载上载
bool fileio_list(std::string &strPath,long lAccess,bool bDsphidefiles,socketTCP &sock);
void cFtpsvr::dataTask(cFtpSession *psession)
{
//	ASSERT(psession!=NULL);
	socketTCP *pcmdsock=psession->m_pcmdsock;
//	ASSERT(pcmdsock!=NULL);
	cFtpsvr *psvr=psession->pserver();
//	ASSERT(psvr!=NULL);
	{
		char resp[]="150 Opening data connection for ready.\r\n";
		pcmdsock->Send(sizeof(resp)-1,resp,-1);
	}
	socketTCP &datasock=psession->m_datasock;
	char resp_err[]="425 Can not open data connection.\r\n";
	char resp_file[]="550 Can not open file.\r\n";
	char resp_ok[]="226 Transfer complete.\r\n";
	char resp_limit[]="502 upload file is limited.\r\n";
	const char *resp=resp_err; int resplen=sizeof(resp_err)-1;

	SOCKSRESULT sr=SOCKSERR_OK;
	if(psession->m_dataconnMode==FTP_DATACONN_PORT) //PORT mode
		sr=datasock.Connect(NULL,0,-1); //要连接的host:port在处理PORT命令时已经设置
	else if(psession->m_datasock.status()==SOCKS_LISTEN) //PASV mode
		sr=datasock.Accept(-1,NULL);
	if(sr>0)
	{
#ifdef _SURPPORT_OPENSSL_
		if(psession->m_sslMode=='P')
			if(!datasock.SSL_Associate()) goto EXIT1;
#endif
		resp=resp_ok; resplen=sizeof(resp_ok)-1;
		//------------------------文件上载操作----------------------------
		if(psession->m_opMode=='S') 
		{
			//即使type指定为A也不能以文本方式打开文件写，否则用fwrite写入时会对任何的0x0D转换为0x0D 0x0A
			//If stream is opened in text mode, each carriage return is replaced with a carriage-return – linefeed pair.
			//The replacement has no effect on the return value.
			FILE *fp=NULL;
			if(psession->m_startPoint!=0)
			{
				if( (fp=::fopen(psession->m_filename.c_str(),"ab")) )
					::fseek(fp,psession->m_startPoint,SEEK_SET);
			}
			else
				fp=::fopen(psession->m_filename.c_str(),"wb");
			if(fp){//开始接收数据并写入文件
				long filesize=::ftell(fp); 
				filesize=filesize>>10;//转换为Kbytes
				long maxfilesize=psession->m_paccount->m_maxupfilesize;
				unsigned long maxdisksize=psession->m_paccount->m_maxdisksize;
				unsigned long curdisksize=psession->m_paccount->m_curdisksize;
				FTPACCOUNT *pftpa=psvr->getAccount(psession->m_paccount->m_username_root.c_str());
				char recvbuf[SSENDBUFFERSIZE]; int recvlen=0;
				while(true)
				{
					recvlen=datasock.checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ);
					if(recvlen<0) break; //判断是否可读
					if(recvlen==0) continue;
					recvlen=datasock.Receive(recvbuf,SSENDBUFFERSIZE,-1);
					if(recvlen<0) break; //==0表明接收数据流量超过限制
					if(recvlen==0){ cUtils::usleep(SCHECKTIMEOUT); continue; }
					recvlen=::fwrite((const void *)recvbuf,sizeof(char),recvlen,fp);
					filesize+=(recvlen>>10);//转换为kbytes
					if(maxfilesize>0 && filesize>maxfilesize)
					{
						resp=resp_limit; resplen=sizeof(resp_limit)-1;
						break;
					}//?if(psession->m_paccount->m_maxupfilesize>0)
					if( (maxdisksize!=0 && (curdisksize+filesize)>maxdisksize ) || 
						(pftpa && pftpa->m_maxdisksize!=0 && (pftpa->m_curdisksize+filesize)>pftpa->m_maxdisksize) 
						)
					{
						resp=resp_limit; resplen=sizeof(resp_limit)-1;
						break;
					}//?if(psession->m_paccount->m_maxdisksize!=0)
				}//?while
				::fclose(fp); 
				if(resp==resp_limit) 
					FILEIO::fileio_deleteFile(psession->m_filename.c_str());
				else
				{
					psession->m_paccount->m_curdisksize+=filesize;
					if(pftpa) pftpa->m_curdisksize+=filesize;
					psvr->onLogEvent(FTP_LOGEVENT_UPLOAD,*psession);
				}
			}//?if(fp)
			else{ resp=resp_file; resplen=sizeof(resp_file)-1; }
		}//?if(psession->m_opMode=='S')
		//------------------------文件下载操作----------------------------
		else if(psession->m_opMode=='R') 
		{
			FILE *fp=::fopen(psession->m_filename.c_str(),"rb");
			if(fp){//开始读文件并发送文件数据
				if(psession->m_startPoint!=0)
					::fseek(fp,psession->m_startPoint,SEEK_SET);
				char readbuf[SSENDBUFFERSIZE]; int itemp,readlen=0;
				while(true)
				{
					if(readlen==0) //读数据
						if( (readlen=::fread(readbuf,sizeof(char),SSENDBUFFERSIZE,fp))<=0 ) break;
					itemp=datasock.checkSocket(SCHECKTIMEOUT,SOCKS_OP_WRITE);
					if(itemp<0) break; //判断是否可写
					if(itemp==0) continue;
					itemp=datasock.Send(readlen,readbuf,-1);
					if(itemp<0) break; //==0表明发送数据流量超过限制
					if(itemp==0) { cUtils::usleep(SCHECKTIMEOUT); continue; }
					if( (readlen-=itemp)> 0) ::memmove(readbuf,readbuf+itemp,readlen);
				}//?while
				::fclose(fp);
				psvr->onLogEvent(FTP_LOGEVENT_DWLOAD,*psession);
			}//?if(fp)
			else{ resp=resp_file; resplen=sizeof(resp_file)-1; }
		}//?else if(psession->m_opMode=='R')
		//------------------LIST 目录和文件-----------------------------------------
		else if(psession->m_opMode=='L') //LIST
		{
			long lAccess=psession->m_startPoint;
			if(psession->m_filename=="") //LIST当前目录
			{
				lAccess=psession->getRealPath(psession->m_filename);
				if(psession->m_filename!="")
					psession->m_filename.append("*");//LIST目录
				//列出当前虚目录下的下一级虚目录
				psession->list();
			}//?if(psession->m_filename=="")
			//此时m_filename指向的是实际系统目录--------------
			fileio_list(psession->m_filename,lAccess,
				psession->m_paccount->bDsphidefiles(),datasock);
		}//?else if(psession->m_opMode=='L')
	}//if(sr>0)

EXIT1:
	pcmdsock->Send(resplen,resp,-1);
	psession->m_filename=""; psession->m_opMode=0;
	psession->m_startPoint=0;
	datasock.Close();
	return;
}

//列举文件目录内容
char *strMonth[12]=
{
	"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"
};
bool fileio_list(std::string &strPath,long lAccess,bool bDsphidefiles,socketTCP &sock)
{
	char isDirRead=(lAccess & FTP_ACCESS_DIR_LIST)?'r':'-';
	char isDirWrite=(lAccess & FTP_ACCESS_DIR_CREATE)?'w':'-';//是否可写
	char isE=(lAccess & FTP_ACCESS_FILE_EXEC)?'x':'-';//是否可执行
	char isFileRead=(lAccess & FTP_ACCESS_FILE_READ)?'r':'-';
	char isFileWrite=(lAccess & FTP_ACCESS_FILE_WRITE)?'w':'-';
	char buf[512]; int buflen=0;
#ifdef WIN32
	if(strPath=="") //列举出windows下的所有盘符
	{
		DWORD d=::GetLogicalDrives();
		for(int i=0;i<26;i++)
		{
			if(d&(1<<i)){
				buflen=sprintf(buf,"d%c%c%c%c%c%c%c%c%c   2 user     group           0 Feb 6 00:00 %c:\r\n",
					isDirRead,isDirWrite,isE,isFileRead,isFileWrite,isE,
					isFileRead,isFileWrite,isE,'A'+i);
				if(sock.Send(buflen,buf,-1)<0) break; }
		}//?for(int i=0;i<32;i++)
	}
	else{
		WIN32_FIND_DATA finddata;	
		HANDLE hd=::FindFirstFile(strPath.c_str(), &finddata);
		if(hd==INVALID_HANDLE_VALUE) return false;
		do{
			if(bDsphidefiles || !(finddata.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
			{//判断是否显示隐藏文件
				char isD=(finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)?'d':'-'; //是否为目录
				char isWrite=(finddata.dwFileAttributes&FILE_ATTRIBUTE_READONLY)?'-':'w';
				if(isWrite=='w') isWrite=(isD=='d')?isDirWrite:isFileWrite;
				//先将UTC时间转为本地时间
				::FileTimeToLocalFileTime(&finddata.ftLastWriteTime,&finddata.ftLastAccessTime);
				SYSTEMTIME systime; ::FileTimeToSystemTime(&finddata.ftLastAccessTime,&systime);
				buflen=sprintf(buf,"%c%c%c%c%c%c%c%c%c%c   1 user      group          %d %s %d %d:%d %s\r\n",
						isD,isDirRead,isWrite,isE,isFileRead,isWrite,isE,isFileRead,isWrite,isE,
						finddata.nFileSizeLow,strMonth[systime.wMonth-1],
						systime.wDay,systime.wHour,systime.wMinute ,finddata.cFileName);
				if(sock.Send(buflen,buf,-1)<0) break;
			}
		}while(::FindNextFile(hd,&finddata));
		::FindClose(hd);
	}
#endif
	return true;
}
//------------------------------------------------------------------------
//------------------------------------------------------------------------

cFtpsvr::cFtpSession::cFtpSession(socketTCP *psock,cFtpsvr *psvr):m_pcmdsock(psock),m_psvr(psvr)
{
	//设置数据通道socket的父为命令通道socket这样如果命令通道异常则数据通道也要关闭
	m_datasock.setParent(psock);
	m_iAccess=FTP_ACCESS_NONE;//默认没有任何的权限 FTP_ACCESS_ALL;
	m_paccount=NULL;
	m_startPoint=0;
	m_dataconnMode=FTP_DATACONN_PORT;//数据连接模式
	m_opMode=0; //L R S
	m_dataMode='S';
	m_dataType='A';
	m_sslMode=0; //SSL加密模式 1)Clear (requested by 'PROT C') 2)Private (requested by 'PROT P')
}

//将相对虚路径转换为绝对虚路径
inline const char * cFtpsvr::cFtpSession::cvtRelative2Absolute(std::string &vpath)
{
	if(vpath[0]=='/') return vpath.c_str();
	if(vpath=="" || vpath=="./"){ vpath=m_relativePath; return vpath.c_str();}
	if(vpath[0]!='.'){
		vpath.insert(0,m_relativePath.c_str());
		return vpath.c_str();
	}
	const char *p=m_relativePath.c_str()+m_relativePath.length()-1;
	const char *ptrBegin=vpath.c_str();
	while(true)
	{
		if(strncmp(ptrBegin,"./",2)==0)
			ptrBegin+=2;
		if(strncmp(ptrBegin,"../",3)==0)
		{
			*(char *)p=0;
			const char *ptr=strrchr(m_relativePath.c_str(),'/');
			*(char *)p='/';
			if(ptr!=NULL) p=ptr;
			ptrBegin+=3;
		}
		else break;
	}//?while
	vpath.erase(0,ptrBegin-vpath.c_str());
	char c=*(p+1); *(char *)(p+1)=0;
	vpath.insert(0,m_relativePath.c_str());
	*(char *)(p+1)=c;
	return vpath.c_str();
}
//将绝对虚路径转换为绝对实路径(!!!虚路径区分大小写)
//返回对当前实路径的可操作权限
//vpath -- [in|out] 输入绝对虚路径，输出绝对实路径
inline long cFtpsvr::cFtpSession::cvtVPath2RPath(std::string &vpath)
{
//	ASSERT(vpath!="" && vpath[0]!='/');
	FTPACCOUNT &ftpa=*m_paccount;

	std::map<std::string,std::pair<std::string,long> >::iterator it=ftpa.m_dirAccess.end();
	const char *ptr=vpath.c_str();
	const char *ptrBegin=ptr;
	do
	{	
		ptr++; char c=*ptr;
		*(char *)ptr=0;
		std::map<std::string,std::pair<std::string,long> >::iterator itTmp=
			ftpa.m_dirAccess.find(ptrBegin);
		*(char *)ptr=c;
		if(itTmp==ftpa.m_dirAccess.end()) break;
		it=itTmp;
	}while( (ptr=strchr(ptr,'/'))!=NULL );

	//实际上不可能发生这种情况,m_dirAccess总要设置根'/'
	if(it==ftpa.m_dirAccess.end()) { return FTP_ACCESS_NONE;}
 
	vpath.erase(0,(*it).first.length());
	long lAccess=(*it).second.second;
	if((lAccess & FTP_ACCESS_SUBDIR_INHERIT)!=0)
	{
		if(strchr(vpath.c_str(),'/')!=NULL) lAccess=0; //目录权限禁止继承，则下级目录的权限为0
	}
	vpath.insert(0,(*it).second.first.c_str());
	return lAccess;
}

//设置当前虚目录
//返回值如果<0则发生错误
//>=0成功,返回当前目录的访问权限
SOCKSRESULT cFtpsvr::cFtpSession::setvpath(const char *vpath_p) 
{
//	ASSERT(vpath_p!=NULL && vpath_p[0]!=0);
	if(strcmp(vpath_p,"./")==0) return m_iAccess;
	std::string vpath(vpath_p);
	if(vpath[vpath.length()-1]!='/') vpath.append("/");
	cvtRelative2Absolute(vpath);
	if(vpath==m_relativePath) return m_iAccess;
	std::string rpath=vpath;
	long lAccess=cvtVPath2RPath(rpath);
	
	//判断此实目录是否存在
	if(rpath!="")
	{
		char c=rpath[rpath.length()-1];
		rpath[rpath.length()-1]=0;
		if(FILEIO::fileio_exist(rpath.c_str())==-1) return SOCKSERR_FTP_NOEXIST;
		rpath[rpath.length()-1]=c;
	}//?if(vpath!="")
	m_relativePath=vpath;
	m_realPath=rpath;
	m_iAccess=lAccess;
	return lAccess;
}

SOCKSRESULT cFtpsvr::cFtpSession::getRealPath(std::string &vpath)
{
	if(vpath=="" || vpath=="./")
	{
		vpath=m_realPath;
		return m_iAccess;
	}
	cvtRelative2Absolute(vpath);
	return cvtVPath2RPath(vpath);
}
//是否为设置的虚目录
//如果是则返回虚目录的权限否则返回0
SOCKSRESULT cFtpsvr::cFtpSession::ifvpath(std::string &vpath)
{
	
	cvtRelative2Absolute(vpath);
	bool bEnd=(vpath[vpath.length()-1]!='/');
	if(bEnd) vpath.append("/");
	FTPACCOUNT &ftpa=*m_paccount;
	std::map<std::string,std::pair<std::string,long> >::iterator itTmp=
			ftpa.m_dirAccess.find(vpath);
	if(bEnd) vpath.erase(vpath.length()-1);
	if(itTmp==ftpa.m_dirAccess.end()) return 0;
	return (*itTmp).second.second;
}
//List子虚目录
void cFtpsvr::cFtpSession::list()
{
	FTPACCOUNT &ftpa=*m_paccount;
	std::map<std::string,std::pair<std::string,long> >::iterator itTmp=
					ftpa.m_dirAccess.find(m_relativePath);
	if(itTmp==ftpa.m_dirAccess.end()) return;
	const std::string &vpath=(*itTmp).first;
	itTmp++;//列出此虚目录下的下一级虚目录
	char buf[128]; int buflen=0; const char *ptr=NULL;
	for(;itTmp!=ftpa.m_dirAccess.end();itTmp++)
	{
		ptr=(*itTmp).first.c_str();
		if((*itTmp).first.length()>vpath.length() && strncmp(ptr,vpath.c_str(),vpath.length())==0)
		{
			long lAccess=(*itTmp).second.second;
			char isExe=(lAccess & FTP_ACCESS_FILE_EXEC)?'x':'-';//是否可执行
			buflen=sprintf(buf,"d%c%c%c%c%c%c%c%c%c   1 user      group          0 Feb 6 12:00 ",
							(lAccess & FTP_ACCESS_DIR_LIST)?'r':'-',
							(lAccess & FTP_ACCESS_DIR_CREATE)?'w':'-',isExe,
							(lAccess & FTP_ACCESS_FILE_READ)?'r':'-',
							(lAccess & FTP_ACCESS_FILE_WRITE)?'w':'-',isExe,
							(lAccess & FTP_ACCESS_FILE_READ)?'r':'-',
							(lAccess & FTP_ACCESS_FILE_WRITE)?'w':'-',isExe);
			
			ptr+=vpath.length();
			while(*ptr!=0 && *ptr!='/' && buflen<126) buf[buflen++]=*ptr++; 
			buf[buflen++]='\r'; buf[buflen++]='\n';
			if(m_datasock.Send(buflen,buf,-1)<0) break;
		}
	}//?for
	return;
}

//------------------------------------------------------------
ftpServer :: ftpServer()
{
	m_strSvrname="FTP Server";
}
ftpServer :: ~ftpServer()
{
	 Close();
	 m_threadpool.join();
}
