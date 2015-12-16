/*******************************************************************
*	msnc0.cpp
*    DESCRIPTION:msnc0协议处理类的实现。
*				snp10/11现在支持msnc1.但msnp10/11仍兼容msnc0
*    AUTHOR:yyc
*
*    HISTORY:
*
*    DATE:2005-07-25
*
*******************************************************************/

#include "../../include/sysconfig.h"
#include "../../include/cCoder.h"
#include "../../include/cLogger.h"
#include "msnlib.h"

using namespace std;
using namespace net4cpp21;

/*
File transfer
For reasons of compatibility, MSNC1 clients must continue to support MSNC0-style file transfer, 
but a new "peer-to-peer" file transfer method has been created in MSNC1. 
As well as general-purpose file transfer, it has specialised modes for transferring backgrounds, 
emoticons, and user display pictures. Again, ZoRoNaX's site describes this well. 
Also, the new method resembles SIP, which is defined in RFC 2543 and RFC 3261.

MSNC1 file transfer allows clients to transmit more information about their NAT or proxy 
settings. This information is gathered from the "ClientPort" and "ClientIP" fields in the 
initial profile (which is the same in MSNP8 and MSNP9). In a session that has been NATed or 
proxied, the IP address and/or port will be those of the last NAT or proxy server before the 
MSN Messenger server. Connections sometimes pass through several NAT and proxy servers on 
their way to the server - if the client is using a proxy server, but the IP address is 
different to that of the proxy server, then the connection has been NATed or proxied again 
somewhere downstream. In an HTTP-proxied connection, ClientPort will always be 0. 
The official client compares its IP address and source port to that received from the server, 
and names the type of connection like this:

				Same IP address		Different IP address  
Same port		Direct-Connect		IP-Restrict-NAT  
Different port  Port-Restrict-NAT   Symmetric-NAT or Unknown-NAT  

If the client is somehow unable to determine its connection type, it will label the connection "Unknown-Connect".

*/

cMsnc0 :: cMsnc0(msnMessager *pmsnmessager,cContactor *pcontact,const char *inviteCookie)
			:cMsncx(pmsnmessager,pcontact,MSNINVITE_TYPE_UNKNOW)
{
	if(inviteCookie) 
		m_inviteCookie.assign(inviteCookie);
	if(pcontact)
		m_datasock.setParent((socketBase *)&pcontact->m_chatSock);
}
cMsnc0 :: ~cMsnc0()
{ 
	m_datasock.Close();
}

bool cMsnc0 :: sendmsg_ACCEPT(bool bListen)
{
	unsigned long trID=m_pmsnmessager->msgID();
	char buf[512];
	int len=sprintf(buf+56,"MIME-Version: 1.0\r\nContent-Type: text/x-msmsgsinvite; charset=UTF-8\r\n\r\n"
					"Invitation-Command: ACCEPT\r\nInvitation-Cookie: %s\r\n",m_inviteCookie.c_str());
	if(m_datasock.status()!=SOCKS_CLOSED) m_datasock.Close();
	if(bListen){//本地开侦听端口等待对方连接
		int listenport=m_datasock.ListenX(0,FALSE,NULL);
		m_authCookie=m_inviteCookie;
		len+=sprintf(buf+56+len,"IP-Address: %s\r\nPort: %d\r\nAuthCookie: %s\r\n",
			m_pcontact->m_chatSock.getLocalIP(),listenport,m_authCookie.c_str());
		if(!m_bSender) //被邀请者或文件接收者，要求发送者连接
			len+=sprintf(buf+56+len,"Sender-Connect: TRUE\r\n");
	}//?if(bListen)
	len+=sprintf(buf+56+len,"Launch-Application: FALSE\r\nRequest-Data: IP-Address:\r\n");
	int iret=sprintf(buf,"MSG %d N %d\r\n",trID,len);
	memmove(buf+(56-iret),buf,iret);
	return (m_pcontact->m_chatSock.Send(len+iret,buf+(56-iret),-1)>0)?true:false;
}
/* Cancel-Code:代码含义
FAIL 
The receiving client does not know any of the specified Session-protocols 
FTTIMEOUT 
There was an error transferring the file itself 
OUTBANDCANCEL 
The switchboard window in which the INVITE message was sent is closing 
REJECT 
The principal has declined the invitation 
REJECT_NOT_INSTALLED 
The client does not support that application GUID 
TIMEOUT 
The client sending an INVITE has got bored of waiting for your ACCEPT (or the principal has cancelled the request)
*/
bool cMsnc0 :: sendmsg_REJECT(const char *errCode)
{
	unsigned long trID=m_pmsnmessager->msgID();
	char buf[256];
	int len=sprintf(buf+32,"MIME-Version: 1.0\r\nContent-Type: text/x-msmsgsinvite; charset=UTF-8\r\n\r\n"
					"Invitation-Command: CANCEL\r\nInvitation-Cookie: %s\r\n"
					"Cancel-Code: %s\r\n",m_inviteCookie.c_str(),errCode);
	int iret=sprintf(buf,"MSG %d N %d\r\n",trID,len);
	memmove(buf+(32-iret),buf,iret);
	return (m_pcontact->m_chatSock.Send(len+iret,buf+(32-iret),-1)>0)?true:false;
}

bool cMsnc0 :: sendFile(const char *filename) //发送指定文件
{
	if(filename==NULL || filename[0]==0 ) return false;
	FILE *fp=::fopen(filename,"rb");
	if(fp==NULL) return false;
	fseek(fp,0,SEEK_END); 
	m_filesize=ftell(fp); 
	fseek(fp,0,SEEK_SET); m_fp=fp;
	const char *ptr=strrchr(filename,'\\');
	if(ptr){
		m_filename.assign(ptr+1);
		m_filepath.assign(filename,ptr-filename+1);
	}
	else
		m_filename.assign(filename);
	m_bSender=true;//SC
	m_inviteType=MSNINVITE_TYPE_FILE;
	
	unsigned long trID=m_pmsnmessager->msgID();
	char connectivity=m_pmsnmessager->Connectivity()?'Y':'N';
	char buf[1024]; std::string utf8_filename;
	int len=cCoder::utf8_encode(m_filename.c_str(),m_filename.length(),buf);
	buf[len]=0; utf8_filename.assign(buf);
	len=sprintf(buf+32,"MIME-Version: 1.0\r\nContent-Type: text/x-msmsgsinvite; charset=UTF-8\r\n\r\n"
					"Application-Name: File Transfer\r\n"
					"Application-GUID: {5D3E02AB-6190-11d3-BBBB-00C04F795683}\r\n"
					"Invitation-Command: INVITE\r\n"
					"Invitation-Cookie: %s\r\nApplication-File: %s\r\nApplication-FileSize: %d\r\nConnectivity: %c\r\n",
					m_inviteCookie.c_str(),utf8_filename.c_str(),m_filesize,connectivity);
	
	int iret=sprintf(buf,"MSG %d N %d\r\n",trID,len);
	memmove(buf+(32-iret),buf,iret);
	return (m_pcontact->m_chatSock.Send(len+iret,buf+(32-iret),-1)>0)?true:false;
}

void cMsnc0 :: setHostinfo(const char *hostip,int hostport,const char *authCookie)
{
	if(m_datasock.status()!=SOCKS_CLOSED) m_datasock.Close();
	m_datasock.setRemoteInfo(hostip,hostport);
	if(authCookie) m_authCookie.assign(authCookie);
	return;
}

//msnc0:msnftp协议的文件发送或接收处理线程
#define MAXMSNDATALENGTH 4096
void cMsnc0 :: msnc0Thread(cMsnc0 *pmsnc0)
{
	if(pmsnc0==NULL) return;
	pmsnc0->m_bDataThread_Running=true;
	RW_LOG_PRINT(LOGLEVEL_INFO,0,"msnc0:data-thread of msnMessager has been started.\r\n");
	socketTCP *pchatsession=&pmsnc0->m_pcontact->m_chatSock;
	if(pmsnc0->m_datasock.status()==SOCKS_LISTEN) //是否是侦听等待对方进行数据连接
		pmsnc0->m_datasock.Accept(MSN_MAX_RESPTIMEOUT,NULL);
	else if(pmsnc0->m_datasock.status()==SOCKS_CLOSED)//连接指定的对端数据传输服务
		pmsnc0->m_datasock.Connect(NULL,0,MSN_MAX_RESPTIMEOUT);

	socketTCP *pdatasock=&pmsnc0->m_datasock;
	const char *toEmail=pmsnc0->m_pcontact->m_email.c_str();
	const char *fromEmail=pmsnc0->m_pmsnmessager->thisEmail();
	if(pdatasock->status()==SOCKS_CONNECTED)
	{   //邀请接收者先发送VER命令
		if(!pmsnc0->m_bSender) pdatasock->Send(0,"VER MYPROTO MSNFTP\r\n",-1);
		char cmdBuf[MAXMSNDATALENGTH]; //接收客户端命令
		int cmdbufLen=0;//缓冲区中命令的长度
		long receivedbytes=0; FILE *fp=NULL;//打开写文件句柄
		while(pchatsession->status()==SOCKS_CONNECTED)
		{
			int iret=pdatasock->checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ);
			if(iret<0) break; //此socket发生错误
			if(iret==0) continue; //没有数据
			//有数据到达,接收数据
			iret=pdatasock->Receive(cmdBuf+cmdbufLen,MAXMSNDATALENGTH-cmdbufLen-1,-1);
			if(iret<=0) break; //对方已经关闭或发生一个系统错误
			cmdbufLen+=iret; cmdBuf[cmdbufLen]=0;
			char *ptrCmd=NULL,*pcmdbuf=cmdBuf;//处理命令
			if(fp){//如果打开文件写，则收到的数据应该是文件数据而不是命令数据
				//第1字节为0 ,2-3字节为数据包长度
				//按照协议上说第1字节还可能为1，2-3字节为0 。代表文件发送结束，可能为老版本
				//协议规定每包数据长度最大为2045字节，加上3字节的头，则最大包长为2048
				while(cmdbufLen>0){
					long len=256*((unsigned char)pcmdbuf[2])+(unsigned char)pcmdbuf[1];
					if(cmdbufLen<len) break;
					if(len>0) ::fwrite(cmdBuf+3,len,1,fp); receivedbytes+=len;
					
					if(pcmdbuf[0]!=0 || receivedbytes>=pmsnc0->m_filesize)
					{
						pdatasock->Send(0,"BYE 16777989\r\n",-1);//接收完毕
						pdatasock->Close(); break;
					}
					pcmdbuf+=(len+3); cmdbufLen-=(len+3);
				}//?while(cmdbufLen>0)
				//yyc add 2006-05-19
				pmsnc0->m_pmsnmessager->onInvite((HCHATSESSION)pmsnc0->m_pcontact,
					pmsnc0->inviteType(),MSNINVITE_CMD_COMPLETED,pmsnc0);

				if(cmdbufLen>0 && pcmdbuf!=cmdBuf) memmove(cmdBuf,pcmdbuf,cmdbufLen);
			}//?if(fp)
			else{
				while( (ptrCmd=strchr(pcmdbuf,'\r')) )
				{
					*ptrCmd=0; 
					RW_LOG_PRINT(LOGLEVEL_DEBUG,"[msnc0] recevied Command: %s.\r\n",pcmdbuf);
					if(pcmdbuf[0]!=0){//仅仅处理非空字符串
						if(strncmp(pcmdbuf,"VER ",4)==0)
						{
							if(!pmsnc0->m_bSender) //接收者
								pdatasock->Send("USR %s %s\r\n",toEmail,pmsnc0->m_authCookie.c_str());
							else
								pdatasock->Send(0,"VER MSNFTP\r\n",-1);
						}//?if(strncmp(pcmdbuf,"VER ",4)==0)
						else if(strncmp(pcmdbuf,"USR ",4)==0)
						{//格式 USR email auth-cookie\r\n
							pdatasock->Send("FIL %d\r\n",pmsnc0->m_filesize);
						}//?else if(strncmp(pcmdbuf,"USR ",4)==0)
						else if(strncmp(pcmdbuf,"FIL ",4)==0)
						{
							pmsnc0->m_filesize=atoi(pcmdbuf+4);
							if( (fp=pmsnc0->m_fp)==NULL ) {pdatasock->Close(); break; }
							pdatasock->Send(5,"TFR\r\n",-1); //准备好开始接收文件
						}//?else if(strncmp(pcmdbuf,"FIL ",4)==0)
						else if(strcmp(pcmdbuf,"TFR")==0)//开始发送文件数据
						{
							long sendbytes=0; FILE *rfp=pmsnc0->m_fp;
							if(rfp==NULL) {pdatasock->Close(); break; }
							while(pchatsession->status()==SOCKS_CONNECTED)
							{
								int readLen=::fread(cmdBuf+3,sizeof(char),2045,rfp);
								if(readLen<=0) break;
								cmdBuf[0]=0; cmdBuf[1]=readLen%256;
								cmdBuf[2]=readLen/256; sendbytes+=readLen;
								if(pdatasock->Send(readLen+3,cmdBuf,-1)<0) break;
							}//?while(
							//yyc add 2006-05-19
							pmsnc0->m_pmsnmessager->onInvite((HCHATSESSION)pmsnc0->m_pcontact,
								pmsnc0->inviteType(),MSNINVITE_CMD_COMPLETED,pmsnc0);

							pdatasock->Close(); break;
						}//?else if(strcmp(pcmdbuf,"TFR")==0)
						else if(strncmp(pcmdbuf,"BYE ",4)==0 || strncmp(pcmdbuf,"CCL",3)==0)
						{//文件发送完毕
							pdatasock->Close(); break;
						}

					}//?if(pcmdbuf[0]!=0)
					pcmdbuf=ptrCmd+1; while(*pcmdbuf=='\r' || *pcmdbuf=='\n') pcmdbuf++; //跳过\r\n
				}//?while( (ptrCmd=strchr(pcmdbuf,'\r')) )
				//如果有未接收完的命令则移动
				if( (iret=cmdbufLen-(pcmdbuf-cmdBuf))>0 ){
					if( (cmdbufLen=iret)>=MAXMSNDATALENGTH-1) cmdbufLen=0;
					if(pcmdbuf>cmdBuf) memmove((void *)cmdBuf,pcmdbuf,cmdbufLen);
				} else cmdbufLen=0;
				cmdBuf[cmdbufLen]=0;
			}//?if(fp)...else...
		}//?while(pchat->status()==SOCKS_CONNECTED)
	}//?if(pdatasock->status()==...
	RW_LOG_PRINT(LOGLEVEL_INFO,0,"msnc0:data-thread of msnMessager has been ended.\r\n");
	pmsnc0->m_bDataThread_Running=false; 
	delete pmsnc0;  //yyc add 2006-05-19
	return;
}

