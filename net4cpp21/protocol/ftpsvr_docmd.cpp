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

#ifdef _SURPPORT_OPENSSL_
#include "../utils/OTP.h"
#endif

using namespace std;
using namespace net4cpp21;

const char RESPINFO_200OK[]="200 command okey.\r\n";
const char RESPINFO_502IMP[]="502 Command not implemented.\r\n";
const char RESPINFO_507NOSUP[]="507 no superior directory.\r\n";
const char RESPINFO_425RAND[]="425 random file system error.\r\n";
const char RESPINFO_550BAD[]="550 bad pathname syntax or ambiguous.\r\n";
const char RESPINFO_521ACC[]="521 access denied.\r\n";
const char RESPINFO_425TASK[]="425 Can not start data Task.\r\n";
const char RESPINFO_521EXIST[]="521 path/file already exists.\r\n";
const char RESPINFO_521VPATH[]="521 vpath access denied.\r\n";
const char RESPINFO_550EXIST[]="550 path/file not exists.\r\n";
const char RESPINFO_332USER[]="332 Need account for login.\r\n";

inline SOCKSRESULT response(socketTCP *psock,const char *buf,int buflen)
{
	SOCKSRESULT sr=psock->Send(buflen,buf,-1);
	RW_LOG_DEBUG("[ftpsvr] s--->c: %dbytes\r\n\t%s",sr,buf);
	return sr;
}

void cFtpsvr :: docmd_authssl(socketTCP *psock)
{
#ifdef _SURPPORT_OPENSSL_
	response(psock,RESPINFO_200OK,sizeof(RESPINFO_200OK)-1);
	//yyc remove 2006-11-23 cFtpsvr对象不是继承自socketSSL
//	psock->initSSL(true,(socketTCP *)this);
	psock->initSSL(true,(socketTCP *)psock->parent());
	if(!psock->SSL_Associate()) psock->Close();
#else
	response(psock,RESPINFO_502IMP,sizeof(RESPINFO_502IMP)-1);
#endif
	return;
}

void cFtpsvr :: docmd_prot(socketTCP *psock,const char *strParam,
							 cFtpSession &clientSession)
{
	while(*strParam==' ') strParam++;//删除前导空格
	clientSession.m_sslMode=(strParam[0]=='C')?'C':'P';
	response(psock,RESPINFO_200OK,sizeof(RESPINFO_200OK)-1);
	return;
}

bool cFtpsvr :: docmd_user(socketTCP *psock,const char *strUser,
							 cFtpSession &clientSession)
{
	while(*strUser==' ') strUser++;//删除前导空格
	::_strlwr((char *)strUser);//将帐号转换为小写
	std::map<std::string,FTPACCOUNT>::iterator it=m_accounts.find(strUser);
	if(it==m_accounts.end())
	{
		response(psock,RESPINFO_332USER,sizeof(RESPINFO_332USER)-1);
		return false;
	}
	FTPACCOUNT &ftpa=(*it).second;
	char resp[FTP_MAX_COMMAND_SIZE];
	int len=0, count=0;//计数
	long pswdmode=ftpa.lPswdMode();
	char seed[16]; seed[0]=0; //种子最大16字节只能包含字母和数字
	if(pswdmode==OTP_MD4 || pswdmode==OTP_MD5)
	{
		seed[8]=0;//随机产生8位只包含数字和字母的种子
		//count计数总为0，因为不知道此参数算法说明见OTP一次性口令说明
		for(int i = 0; i < 8; i++ )
		{
				 seed[i]=(rand()*35)/RAND_MAX+48;
				 if(seed[i]>=58 && seed[i]<84)
					 seed[i]=97+(seed[i]-58);
		}
		if(pswdmode==OTP_MD4)
			len=sprintf(resp,"331 Response to otp-md4 %d %s required for skey.\r\n",count,seed);
		else
			len=sprintf(resp,"331 Response to otp-md5 %d %s required for skey.\r\n",count,seed);
	}
	else
	{
		pswdmode=OTP_NONE;
		len=sprintf(resp,"331 Password required for %s.\r\n",strUser);
	}
	response(psock,resp,len);

	//等待对方发送密码过来
	len=psock->Receive(resp,FTP_MAX_COMMAND_SIZE-1,FTP_MAX_LOGINTIMEOUT);
	if(len<0 || strncasecmp(resp,"PASS ",5)!=0)
	{
		resp_noLogin(psock);
		return false;
	}
	resp[len]=0;
	const char *strpwd=resp+5;
	//去掉回车换行符号
	const char *ptr=strchr(strpwd,'\r'); if(ptr) *(char*)ptr=0;
	bool bAccess=false;
	if(ftpa.m_userpwd=="") //密码设成空则无需密码验证
		bAccess=true; 
	else if(pswdmode==OTP_NONE)
		bAccess=(strcmp(ftpa.m_userpwd.c_str(),strpwd)==0);
#ifdef _SURPPORT_OPENSSL_
	else //OTP_MD4或OTP_MD5加密
	{
		OTP otps; const char *ptrmd=NULL;
		if(pswdmode==OTP_MD4)
			ptrmd=otps.md4(seed,ftpa.m_userpwd.c_str(),count);
		else
			ptrmd=otps.md5(seed,ftpa.m_userpwd.c_str(),count);
		bAccess=(strcmp(ptrmd,strpwd)==0);
	}
#endif
	if(bAccess)
	{
		if(ftpa.m_limitedTime==0 || time(NULL)<ftpa.m_limitedTime )
		{
			if(ftpa.m_maxLoginusers<=0 || ftpa.m_loginusers<=ftpa.m_maxLoginusers)
			{
				if( ftpa.m_ipRules.check(psock->getRemoteip(),
						psock->getRemotePort(),RULETYPE_TCP) )
				{
					clientSession.m_paccount=&ftpa;
					clientSession.setvpath("/"); //设置当前虚目录
					ftpa.m_loginusers++;
					//设置数据通道的流量限制
					clientSession.m_datasock.setSpeedRatio(ftpa.m_maxdwratio*1024,ftpa.m_maxupratio*1024);

					len=sprintf(resp,"230 %s user logged in, %d users.\r\n",strUser,ftpa.m_loginusers);
				}
				else
					len=sprintf(resp,"530 %s cannot access.\r\n",psock->getRemoteIP());
			}
			else
				len=sprintf(resp,"530 access denied, the login limit is %d.\r\n",ftpa.m_maxLoginusers);
		}
		else
			len=sprintf(resp,"530 account has been expired.\r\n");
	}//?if(bAccess)
	else //输入密码错误
		len=sprintf(resp,"530 User %s cannot log in.\r\n",strUser);
	response(psock,resp,len);
	return (clientSession.m_paccount!=NULL);
}

void cFtpsvr :: docmd_type(socketTCP *psock,const char *strType,
							 cFtpSession &clientSession)
{
	while(*strType==' ') strType++;//删除前导空格
	char cType=strType[0];
	char resp[64]; int len=0;
	if(cType=='A'||cType=='a'||cType=='E'||cType=='e'||cType=='I'||cType=='i'||cType=='L'||cType=='l')
	{
		clientSession.m_dataType =(cType>=0x61)?(cType-0x20):cType;//转换为大写
		len=sprintf(resp,"200 Type set to %c.\r\n",clientSession.m_dataType);
	}
	else
		len=sprintf(resp,"501 %c: unknown mode.\r\n",cType);
	response(psock,resp,len);
	return;
}

void cFtpsvr :: docmd_rest(socketTCP *psock,const char *strRest,
							 cFtpSession &clientSession)
{
	while(*strRest==' ') strRest++;//删除前导空格
	clientSession.m_startPoint=atol(strRest);
	if(clientSession.m_startPoint<0) clientSession.m_startPoint=0;
	char resp[64];
	int len=sprintf(resp,"350 REST %d command okey.\r\n",clientSession.m_startPoint);
	response(psock,resp,len);
	return;
}
//PWD
//	257 (251) "pathname"（“路径名”）
//	425 (451) random file system error（随机文件系统错误）
//	506 (502) action not implemented（操作未执行）
void cFtpsvr :: docmd_pwd(socketTCP *psock,cFtpSession &clientSession)
{
	char resp[FTP_MAX_COMMAND_SIZE];
	int len=sprintf(resp,"257 \"%s\" is current directory.\r\n",
		clientSession.getvpath());
	response(psock,resp,len);
	return;
}
//请求服务器忽略上一个命令及相关的数据传输操作
void cFtpsvr :: docmd_abor(socketTCP *psock,cFtpSession &clientSession)
{
	clientSession.m_datasock.Close();
	char resp[]="226 Closing data connection.\r\n";
	response(psock,resp,sizeof(resp)-1);
	return;
}
//XCUP                    改变目录到上一级目录
//	      200 (200) working directory changed（工作目录已改变）
//	      506 (502) action not implemented（操作未执行）
//	      507 (551) no superior directory（无上一级目录）
//	      521 (450) access denied（拒绝访问）
//	      425 (451) random file system error（随机文件系统错误）
void cFtpsvr :: docmd_cdup(socketTCP *psock,cFtpSession &clientSession)
{
	if(strcmp(clientSession.getvpath(),"/")==0) //已经到头了
	{
		char resp[]="507 \"/\" is root directory.\r\n";
		response(psock,resp,sizeof(resp)-1);
		return;
	}
	SOCKSRESULT fr=clientSession.setvpath("../");
	if(fr>=SOCKSERR_OK)
	{
		char resp[]="200 working directory changed.\r\n";
		response(psock,resp,sizeof(resp)-1);
		return;
	}
	else if(fr==SOCKSERR_FTP_NOEXIST)
	{
		response(psock,RESPINFO_507NOSUP,sizeof(RESPINFO_507NOSUP)-1);
		return;
	}
	response(psock,RESPINFO_425RAND,sizeof(RESPINFO_425RAND)-1);
	return;
}
void cFtpsvr :: docmd_cwd(socketTCP *psock,const char *strDir,
							 cFtpSession &clientSession)
{
	while(*strDir==' ') strDir++;//删除前导空格
	SOCKSRESULT fr=SOCKSERR_OK;
	if(strDir[0]!=0 && strcmp(strDir,"./")!=0)
		fr=clientSession.setvpath(strDir);
	if(fr>=SOCKSERR_OK)
	{
		char resp[]="200 working directory changed.\r\n";
		response(psock,resp,sizeof(resp)-1);
		return;
	}
	else if(fr==SOCKSERR_FTP_NOEXIST)
	{
		response(psock,RESPINFO_550BAD,sizeof(RESPINFO_550BAD)-1);
		return;
	}
	response(psock,RESPINFO_425RAND,sizeof(RESPINFO_425RAND)-1);
	return;
}
//XMKD               创建目录
//	      257 (251) "pathname" created（"pathname" 已创建）
//	      521 (450) "pathname" already exists（"pathname" 已存在）
//	      506 (502) action not implemented（操作未执行）
//	      521 (450) access denied（拒绝访问）
//	      550 (501) bad pathname syntax or ambiguous（错误或不明确的路径名）
//	      425 (451) random file system error（随机文件系统错误）
void cFtpsvr :: docmd_mkd(socketTCP *psock,const char *strDir,
							 cFtpSession &clientSession)
{
	while(*strDir==' ') strDir++;//删除前导空格
	if(strDir[0]==0){
		response(psock,RESPINFO_550BAD,sizeof(RESPINFO_550BAD)-1);
		return;
	}
	std::string vpath(strDir);
	if(clientSession.ifvpath(vpath))
	{//如果要创建的目录是一个已存在的虚目录
		response(psock,RESPINFO_521VPATH,sizeof(RESPINFO_521VPATH)-1);
		return;
	}
	//转换为绝对实路径
	SOCKSRESULT fr=clientSession.getRealPath(vpath);
	if(fr<SOCKSERR_OK)
	{
		response(psock,RESPINFO_425RAND,sizeof(RESPINFO_425RAND)-1);
		return;
	}
	if((fr & FTP_ACCESS_DIR_CREATE)==0)
	{
		response(psock,RESPINFO_521ACC,sizeof(RESPINFO_521ACC)-1);
		return;
	}
	if(FILEIO::fileio_exist(vpath.c_str())!=-1)
	{
		response(psock,RESPINFO_521EXIST,sizeof(RESPINFO_521EXIST)-1);
		return;
	}
	
	if(!FILEIO::fileio_createDir(vpath.c_str()))
	{
		char resp[]="425 Failed to create Directory.\r\n";
		response(psock,resp,sizeof(resp)-1);
		return;
	}
	char resp[]="257 Directory created.\r\n";
	response(psock,resp,sizeof(resp)-1);
	return;
}
//XRMD                    删除目录
//	      224 (250) deleted ok（删除完成）
//	      506 (502) action not implemented（操作未执行）
//	      521 (450) access denied（拒绝访问）
//	      550 (501) bad pathname syntax or ambiguous（错误或不明确的路径名）
//	      425 (451) random file system error（随机文件系统错误）
unsigned long cFtpsvr :: docmd_rmd(socketTCP *psock,const char *strDir,
							 cFtpSession &clientSession)
{
	while(*strDir==' ') strDir++;//删除前导空格
	if(strDir[0]==0){
		response(psock,RESPINFO_550BAD,sizeof(RESPINFO_550BAD)-1);
		return 0;
	}
	std::string vpath(strDir);
	if(clientSession.ifvpath(vpath))
	{//如果要删除的目录是一个已存在的虚目录
		response(psock,RESPINFO_521VPATH,sizeof(RESPINFO_521VPATH)-1);
		return 0;
	}
	//转换为绝对实路径
	SOCKSRESULT fr=clientSession.getRealPath(vpath);
	if(fr<SOCKSERR_OK)
	{
		response(psock,RESPINFO_425RAND,sizeof(RESPINFO_425RAND)-1);
		return 0;
	}
	if((fr & FTP_ACCESS_DIR_DELETE)==0)
	{
		response(psock,RESPINFO_521ACC,sizeof(RESPINFO_521ACC)-1);
		return 0;
	}
	if(FILEIO::fileio_exist(vpath.c_str())!=-2)
	{
		char resp[]="550 Directory not exists.\r\n";
		response(psock,resp,sizeof(resp)-1);
		return 0;
	}
	unsigned long lsize=FILEIO::fileio_deleteDir(vpath.c_str());
	char resp[]="250 RMD command okey.\r\n";
	response(psock,resp,sizeof(resp)-1);
	clientSession.m_filename=vpath;
	onLogEvent(FTP_LOGEVENT_RMD,clientSession);
	return lsize;
}
//更改文件或目录名字
void cFtpsvr :: docmd_rnfr(socketTCP *psock,const char *strfile,
							 cFtpSession &clientSession)
{
	while(*strfile==' ') strfile++;//删除前导空格
	if(strfile[0]==0){
		response(psock,RESPINFO_550BAD,sizeof(RESPINFO_550BAD)-1);
		return;
	}
	std::string vpath(strfile);
	if(clientSession.ifvpath(vpath))
	{//如果要改名的目录是一个已存在的虚目录
		response(psock,RESPINFO_521VPATH,sizeof(RESPINFO_521VPATH)-1);
		return;
	}
	//转换为绝对实路径
	SOCKSRESULT fr=clientSession.getRealPath(vpath);
	if(fr<SOCKSERR_OK)
	{
		response(psock,RESPINFO_425RAND,sizeof(RESPINFO_425RAND)-1);
		return;
	}
	if((fr & FTP_ACCESS_FILE_WRITE)==0)
	{
		response(psock,RESPINFO_521ACC,sizeof(RESPINFO_521ACC)-1);
		return;
	}
	if(FILEIO::fileio_exist(vpath.c_str())==-1)
	{
		response(psock,RESPINFO_550EXIST,sizeof(RESPINFO_550EXIST)-1);
		return;
	}
	
	clientSession.m_filename=vpath;
	char resp[]="350 path/file exists, ready for destination name.\r\n";
	response(psock,resp,sizeof(resp)-1);
	return;
}

void cFtpsvr :: docmd_rnto(socketTCP *psock,const char *strfile,
							 cFtpSession &clientSession)
{
	if(clientSession.m_filename=="")
	{
		const char resp[]="503 Bad sequence of commands.\r\n";
		response(psock,resp,sizeof(resp)-1);
		return;
	}
	while(*strfile==' ') strfile++;//删除前导空格
	if(strfile[0]==0){
		response(psock,RESPINFO_550BAD,sizeof(RESPINFO_550BAD)-1);
		return;
	}

	std::string vpath(strfile);
	if(clientSession.ifvpath(vpath))
	{
		response(psock,RESPINFO_521VPATH,sizeof(RESPINFO_521VPATH)-1);
		return;
	}
	//转换为绝对实路径
	SOCKSRESULT fr=clientSession.getRealPath(vpath);
	if(fr<SOCKSERR_OK)
	{
		response(psock,RESPINFO_425RAND,sizeof(RESPINFO_425RAND)-1);
		return;
	}
	if((fr & FTP_ACCESS_FILE_WRITE)==0)
	{
		response(psock,RESPINFO_521ACC,sizeof(RESPINFO_521ACC)-1);
		return;
	}
	if(FILEIO::fileio_exist(vpath.c_str())!=-1)
	{
		response(psock,RESPINFO_521EXIST,sizeof(RESPINFO_521EXIST)-1);
		return;
	}
	
	if( !FILEIO::fileio_rename(clientSession.m_filename.c_str(),vpath.c_str()) )
	{
		char resp[]="553 Rename failed.\r\n";
		response(psock,resp,sizeof(resp)-1);
		return;
	}
	char resp[]="250 RNTO command successful.\r\n";
	response(psock,resp,sizeof(resp)-1);
	return;
}
//获取指定文件的大小
void cFtpsvr :: docmd_size(socketTCP *psock,const char *strfile,
							 cFtpSession &clientSession)
{
	while(*strfile==' ') strfile++;//删除前导空格
	if(strfile[0]==0){
		response(psock,RESPINFO_550BAD,sizeof(RESPINFO_550BAD)-1);
		return;
	}
	std::string vpath(strfile);
	//转换为绝对实路径
	SOCKSRESULT fr=clientSession.getRealPath(vpath);
	if(fr<SOCKSERR_OK)
	{
		response(psock,RESPINFO_425RAND,sizeof(RESPINFO_425RAND)-1);
		return;
	}
	if(fr==0)
	{
		response(psock,RESPINFO_521ACC,sizeof(RESPINFO_521ACC)-1);
		return;
	}
	long lsize=FILEIO::fileio_exist(vpath.c_str());
	if(lsize<0)
	{
		response(psock,RESPINFO_550EXIST,sizeof(RESPINFO_550EXIST)-1);
		return;
	}
	
	char resp[64];
	int len=sprintf(resp,"213 %d\r\n",lsize);
	response(psock,resp,len);return;
}
//返回删除文件的大小 KBytes
unsigned long cFtpsvr :: docmd_dele(socketTCP *psock,const char *strfile,
							 cFtpSession &clientSession)
{
	while(*strfile==' ') strfile++;//删除前导空格
	if(strfile[0]==0){
		response(psock,RESPINFO_550BAD,sizeof(RESPINFO_550BAD)-1);
		return 0;
	}
	std::string vpath(strfile);
	//转换为绝对实路径
	SOCKSRESULT fr=clientSession.getRealPath(vpath);
	if(fr<SOCKSERR_OK)
	{
		response(psock,RESPINFO_425RAND,sizeof(RESPINFO_425RAND)-1);
		return 0;
	}
	if((fr & FTP_ACCESS_FILE_DELETE)==0)
	{
		response(psock,RESPINFO_521ACC,sizeof(RESPINFO_521ACC)-1);
		return 0;
	}
	long lsize=FILEIO::fileio_exist(vpath.c_str());
	if(lsize<0)
	{
		response(psock,RESPINFO_550EXIST,sizeof(RESPINFO_550EXIST)-1);
		return 0;
	}
	
	if( !FILEIO::fileio_deleteFile(vpath.c_str()) )
	{
		char resp[]="553 Failed to delete file.\r\n";
		response(psock,resp,sizeof(resp)-1);
		return 0;
	}
	char resp[]="250 delete file successfully.\r\n";
	response(psock,resp,sizeof(resp)-1);
	clientSession.m_filename=vpath;
	onLogEvent(FTP_LOGEVENT_DELETE,clientSession);
	return (lsize<0)?0:(lsize>>10);
}

void cFtpsvr :: docmd_pasv(socketTCP *psock,cFtpSession &clientSession)
{
	clientSession.m_dataconnMode=FTP_DATACONN_PASV;
	int iPort=0;
	if(clientSession.m_datasock.status()!=SOCKS_LISTEN)
	{
		const char *bindip=psock->getLocalIP();
		RW_LOG_DEBUG("[ftpsvr] pasv -- datasock bind ip %s.\r\n",bindip);
		if(m_dataport_start==0 && m_dataport_end==0)
			iPort=clientSession.m_datasock.ListenX(0,FALSE,bindip);
		else
			iPort=clientSession.m_datasock.ListenX(m_dataport_start,m_dataport_end,FALSE,bindip);
		if(iPort<=0){
			char resp[]="425 Can not listen.\r\n";
			response(psock,resp,sizeof(resp)-1);
			return;
		}
#ifdef _SURPPORT_OPENSSL_
		if(clientSession.m_sslMode=='P') //是否指定加密数据传输
			clientSession.m_datasock.initSSL(true,(socketTCP *)psock->parent() ); //yyc modify 2006-11-23
#endif
	}
	else
		iPort=clientSession.m_datasock.getLocalPort();
	//Example of the string passed with sData argument
	//227 Entering Passive Mode (194,220,224,2,7,189)
	char resp[64];
	int len=sprintf(resp,"227 Entering Passive Mode (%s,%d,%d)\r\n",psock->getLocalIP(),
		(iPort&0x0000ff00)>>8,iPort&0x000000ff);
	resp[len]=0; len=0; 
	while(resp[len]){ if(resp[len]=='.') resp[len]=','; len++; }
	response(psock,resp,len);
	return;
}

void cFtpsvr :: docmd_port(socketTCP *psock,char *strParam,
							 cFtpSession &clientSession)
{
	while(*strParam==' ') strParam++;//删除前导空格
	//format:PORT <host-number>,<PORT-NUMBER><CRLF>
	//<host-number>:<number>,<number>,<number>,<number>
	//<port-number>:<nubmer>,<number>
	clientSession.m_dataconnMode=FTP_DATACONN_PORT;
	if(clientSession.m_datasock.status()!=SOCKS_CLOSED)
		clientSession.m_datasock.Close();
	//获取要连接的IP和端口
	int i=0,iport=0; char *ptrIP=strParam;
	while(*strParam)
	{
		if(*strParam==','){
			i++;
			if(i==4)
			{
				*strParam++=0;
				iport=atoi(strParam);
			}
			else if(i>4)
			{
				iport=iport*256+atoi(++strParam);
				break;
			}
			else *strParam++='.';
		}//?if(*strParam==',')
		else strParam++;
	}//?while
	clientSession.m_datasock.setRemoteInfo(ptrIP,iport);
#ifdef _SURPPORT_OPENSSL_
	if(clientSession.m_sslMode=='P') //是否指定加密数据传输
		clientSession.m_datasock.initSSL(true,(socketTCP *)psock->parent()); //yyc modify 2006-11-23
#endif
	char resp[64];
	int len=sprintf(resp,"200 PORT(%s:%d) command successful.\r\n",ptrIP,iport);
	response(psock,resp,len);
	return;
}

//获取指定文件目录list
//strfile --- 可能指向一个文件或目录
void cFtpsvr :: docmd_list(socketTCP *psock,const char *strfile,
							 cFtpSession &clientSession)
{
	if(clientSession.m_dataType!='A' && clientSession.m_dataType!='I')
	{
//		char resp[]="502 Only ASCII mode for LIST.\r\n";
		char resp[]="502 Only A/I mode for LIST.\r\n";
		response(psock,resp,sizeof(resp)-1);
		return;
	}
	while(*strfile==' ') strfile++;//删除前导空格
	std::string vpath(strfile);
	//转换为绝对实路径
	SOCKSRESULT fr=clientSession.getRealPath(vpath);
	if(fr<SOCKSERR_OK)
	{
		response(psock,RESPINFO_425RAND,sizeof(RESPINFO_425RAND)-1);
		return;
	}
	if((fr & FTP_ACCESS_DIR_LIST)==0)
	{
		response(psock,RESPINFO_521ACC,sizeof(RESPINFO_521ACC)-1);
		return;	
	}
	if(strfile[0]==0)
		clientSession.m_filename=""; //List当前目录,还要list虚目录
	else{
		long iret=FILEIO::fileio_exist(vpath.c_str());
		if(iret==-1)
		{
			response(psock,RESPINFO_550EXIST,sizeof(RESPINFO_550EXIST)-1);
			return;
		}
		if(iret==-2) vpath.append("\\*");//list的是目录,仅仅list实目录
		clientSession.m_filename=vpath;
	}
	clientSession.m_startPoint=fr;
	clientSession.m_opMode='L';
	if(!onNewTask((THREAD_CALLBACK *)&dataTask,(void *)&clientSession))
	{
		clientSession.m_opMode=0; 
		clientSession.m_filename=""; clientSession.m_startPoint=0;
		response(psock,RESPINFO_425TASK,sizeof(RESPINFO_425TASK)-1);
	}
	return;
}

//下载文件
void cFtpsvr :: docmd_retr(socketTCP *psock,const char *strfile,
							 cFtpSession &clientSession)
{
	while(*strfile==' ') strfile++;//删除前导空格
	if(strfile[0]==0){
		response(psock,RESPINFO_550BAD,sizeof(RESPINFO_550BAD)-1);
		return;
	}
	std::string vpath(strfile);
	//转换为绝对实路径
	SOCKSRESULT fr=clientSession.getRealPath(vpath);
	if(fr<SOCKSERR_OK)
	{
		response(psock,RESPINFO_425RAND,sizeof(RESPINFO_425RAND)-1);
		return;
	}
	if(FILEIO::fileio_exist(vpath.c_str())<0)
	{
		response(psock,RESPINFO_550EXIST,sizeof(RESPINFO_550EXIST)-1);
		return;
	}
	if((fr & FTP_ACCESS_FILE_READ)==0)
	{
		response(psock,RESPINFO_521ACC,sizeof(RESPINFO_521ACC)-1);
		return;
	}
	clientSession.m_filename=vpath;
	clientSession.m_opMode='R';
	if(!onNewTask((THREAD_CALLBACK *)&dataTask,(void *)&clientSession))
	{
		clientSession.m_opMode=0;
		clientSession.m_filename="";
		response(psock,RESPINFO_425TASK,sizeof(RESPINFO_425TASK)-1);
	}
	return;
}

//上载文件
void cFtpsvr :: docmd_stor(socketTCP *psock,const char *strfile,
							 cFtpSession &clientSession)
{
	while(*strfile==' ') strfile++;//删除前导空格
	if(strfile[0]==0){
		response(psock,RESPINFO_550BAD,sizeof(RESPINFO_550BAD)-1);
		return;
	}
	std::string vpath(strfile);
	//转换为绝对实路径
	SOCKSRESULT fr=clientSession.getRealPath(vpath);
	if(fr<SOCKSERR_OK)
	{
		response(psock,RESPINFO_425RAND,sizeof(RESPINFO_425RAND)-1);
		return;
	}
	if((fr & FTP_ACCESS_FILE_WRITE)==0)
	{
		response(psock,RESPINFO_521ACC,sizeof(RESPINFO_521ACC)-1);
		return;
	}
	clientSession.m_filename=vpath;
	clientSession.m_opMode='S';
	if(!onNewTask((THREAD_CALLBACK *)&dataTask,(void *)&clientSession))
	{
		clientSession.m_opMode=0;
		clientSession.m_filename="";
		response(psock,RESPINFO_425TASK,sizeof(RESPINFO_425TASK)-1);
	}
	return;
}

//修改密码，格式:<old password> <new password>
void cFtpsvr :: docmd_pswd(socketTCP *psock,const char *strpwd,
							 cFtpSession &clientSession)
{
	if(clientSession.m_paccount->lRemoteAdmin()==ACCOUNT_NORMAL)
	{
		const char resp[]="501 Cannot change password.\r\n";
		response(psock,resp,sizeof(resp)-1);
		return;
	}
	while(*strpwd==' ') strpwd++;//删除前导空格
	const char *ptr=strchr(strpwd,' ');
	if(ptr)
	{
		*(char *)ptr=0;
		bool b=(strcmp(strpwd,clientSession.m_paccount->m_userpwd.c_str())==0);
		*(char *)ptr=' ';
		if(b){
			clientSession.m_paccount->m_userpwd.assign(ptr+1);
			const char resp[]="230 Password changed okay.\r\n";
			response(psock,resp,sizeof(resp)-1);
			return;
		}
	}//?if(ptr)
	const char resp[]="502 Syntax error - use SITE PSWD <oldpassword> <newpassword>.\r\n";
	response(psock,resp,sizeof(resp)-1);
	return;
}

void cFtpsvr :: docmd_rein(socketTCP *psock,cFtpSession &clientSession)
{
	if(clientSession.m_paccount) 
		clientSession.m_paccount->m_loginusers--;
	clientSession.m_paccount=NULL;
	clientSession.m_tmLogin=time(NULL);
	const char resp[]="220 Service ready for new user.\r\n";
	response(psock,resp,sizeof(resp)-1);
	return;
}

void cFtpsvr :: docmd_feat(socketTCP *psock)
{
	const char resp[]="211-Extension supported\r\n"
					  "SIZE\r\n"
					  "SITE INFO;PSWD;USERS;USER;VPATH;IPRULES;EXEC;KILL\r\n"
					  "REST STREAM\r\n"
					  "211 End\r\n";
	response(psock,resp,sizeof(resp)-1);
	return;
}

void cFtpsvr :: docmd_sitelist(socketTCP *psock,cFtpSession &clientSession)
{
	if(clientSession.m_paccount->lRemoteAdmin()==ACCOUNT_NORMAL)
	{
		const char resp[]="211 access denied.\r\n";
		response(psock,resp,sizeof(resp)-1);
		return;
	} 
	char resp[1024]; int count=0,resplen=24;
	std::map<std::string,FTPACCOUNT>::iterator it=m_accounts.begin();
	for(;it!=m_accounts.end();it++)
	{
		if(resplen>=960){
			resplen+=sprintf(resp+resplen,"......\r\n");
			break; //防止越界
		}
		if( clientSession.m_paccount->lRemoteAdmin()==ACCOUNT_ADMIN || 
			( clientSession.m_paccount->lRemoteAdmin()==ACCOUNT_ROOT && 
			  (*it).second.m_username_root==clientSession.m_paccount->m_username )
		  )
		{
			count++;
			resplen+=sprintf(resp+resplen,"[%s] account=%s pswd=%s\r\n",
				((*it).second.lRemoteAdmin()==ACCOUNT_ADMIN)?"ADMIN":
				 (((*it).second.lRemoteAdmin()==ACCOUNT_ROOT)?"ROOT":"NORMAL"),
				(*it).second.m_username.c_str(),(*it).second.m_userpwd.c_str());
		}
	}
	resplen+=sprintf(resp+resplen,"211 End\r\n");
	char c=resp[24];
	sprintf(resp,"211-Total %03d accounts\r\n",count);
	resp[24]=c;
	response(psock,resp,resplen);
	return;
}

void cFtpsvr :: docmd_quit(socketTCP *psock)
{
	const char resp[]="221 Bye\r\n";
	response(psock,resp,sizeof(resp)-1);
	psock->Close(); return;
}
void cFtpsvr :: resp_noLogin(socketTCP *psock)
{
	char resp[]="530 Not logged in.\r\n";
	response(psock,resp,sizeof(resp)-1);
	return;
}
void cFtpsvr :: resp_OK(socketTCP *psock)
{
	response(psock,RESPINFO_200OK,sizeof(RESPINFO_200OK)-1);
	return;
}
void cFtpsvr :: resp_noImplement(socketTCP *psock)
{
	response(psock,RESPINFO_502IMP,sizeof(RESPINFO_502IMP)-1);
	return;
}
void cFtpsvr :: resp_unknowed(socketTCP *psock)
{
	char resp[]="500 command unrecognized.\r\n";
	response(psock,resp,sizeof(resp)-1);
	return;
}

