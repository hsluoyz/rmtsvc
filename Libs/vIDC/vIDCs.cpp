/*******************************************************************
   *	vIDCs.cpp
   *    DESCRIPTION:vIDC服务类的实现
   *
   *    AUTHOR:yyc
   *	http://hi.baidu.com/yycblog/home
   *
   *******************************************************************/

#include "../../net4cpp21/include/sysconfig.h"
#include "../../net4cpp21/utils/utils.h"
#include "../../net4cpp21/include/cLogger.h"
#include "vidcs.h"

using namespace std;
using namespace net4cpp21;

vidcsvr :: vidcsvr()
{
	m_bAuthentication=false;
}

vidcsvr :: ~vidcsvr() { Destroy(); }

void vidcsvr :: Destroy()
{
	std::map<long,vidccSession *>::iterator it=m_sessions.begin();
	for(;it!=m_sessions.end();it++)
	{
		vidccSession *pvidcc=(*it).second;
		if(pvidcc!=NULL) pvidcc->Destroy();
		delete pvidcc;
	}
	m_sessions.clear();
}

bool vidcsvr :: DisConnect(long vidccID) //强制断开某个vidccc的连接
{
	std::map<long,vidccSession *>::iterator it=m_sessions.find(vidccID);
	if(it==m_sessions.end()) return false;
	vidccSession *pvidcc=(*it).second;
	//关闭socket连接则 onConnect 中会自动删除此vidccSession
	if(pvidcc) pvidcc->Close(); else m_sessions.erase(it); 
	return true;
}
//设置对某个vidcc映射的服务进行日志记录
void vidcsvr :: setLogdatafile(long vidccID,bool b)
{
	std::map<long,vidccSession *>::iterator it=m_sessions.find(vidccID);
	if(it==m_sessions.end()) return;
	vidccSession *pvidcc=(*it).second;
	pvidcc->setIfLogdata(b); 
	return;
}

void vidcsvr :: xml_list_vidcc(cBuffer &buffer)
{
	if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
	if(buffer.str()==NULL) return;
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vidccs>");
	
	std::map<long,vidccSession *>::iterator it=m_sessions.begin();
	for(;it!=m_sessions.end();it++)
	{
		if(buffer.Space()<128) buffer.Resize(buffer.size()+128);
		if(buffer.str()==NULL) return;
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vidcc>");
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vID>%d</vID>",(*it).first);
		vidccSession *pvidcc=(*it).second;
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vname>%s</vname>",pvidcc->vidccName());
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vip>%s</vip>",pvidcc->vidccIP());
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"</vidcc>");
	}
		
	if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
	if(buffer.str())
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"</vidccs>");
	return;
}

void vidcsvr :: xml_info_vidcc(cBuffer &buffer,long vidccID)
{
	std::map<long,vidccSession *>::iterator it=m_sessions.find(vidccID);
	if(it==m_sessions.end())
	{
		if(buffer.Space()<48) buffer.Resize(buffer.size()+48);
		if(buffer.str())
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>inValid vidccID</retmsg>");
		return;
	}
	vidccSession *pvidcc=(*it).second;
	if(pvidcc==NULL) return;
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vidcc_status>");
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vID>%d</vID>",vidccID);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vname>%s</vname>",pvidcc->vidccName());
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vip>%s</vip>",pvidcc->vidccIP());
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vdesc>%s</vdesc>",pvidcc->vidccDesc());
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<ver>%.1f</ver>",pvidcc->vidccVer()/10.0);
	time_t t=pvidcc->ConnectTime(); struct tm *ltime=localtime(&t);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<starttime>%04d年%02d月%02d日 %02d:%02d:%02d</starttime>",
			(1900+ltime->tm_year), ltime->tm_mon+1, ltime->tm_mday,ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
	pvidcc->xml_list_mapped(buffer);
	if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
	if(buffer.str())
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"</vidcc_status>");
	return;
}

//有一个新的连接进来
void vidcsvr::onConnect(socketTCP *psock)
{
	socketBase *psocketsvr=psock->parent(); 
	if(psocketsvr==NULL) return; //保存vidcs服务的socket对象
	char buf[VIDC_MAX_COMMAND_SIZE];
	int iret=psock->Receive(buf,VIDC_MAX_COMMAND_SIZE-1,VIDC_MAX_RESPTIMEOUT);
	//如果有错误或在指定的时间内没有收到任何数据则返回并断开连接
	if(iret<=0) return; else buf[iret]=0;
	
	RW_LOG_DEBUG("[vidcSvr] c--->s:\r\n\t%s\r\n",buf);
	if(strncmp(buf,"HELO ",5)==0)
	{//某个客户端连接登陆
		//创建新的vidccSession并添加到m_sessions队列中
		vidccSession *pvidcc=docmd_helo(psock,buf+5);
		if(pvidcc==NULL) return; //登陆失败关闭连接
		char buf[VIDC_MAX_COMMAND_SIZE]; int buflen=0;//进入命令处理循环
		time_t tLastReceived=time(NULL);
		while(psocketsvr->status()==SOCKS_LISTEN && 
			  psock->status()==SOCKS_CONNECTED )
		{
			iret=psock->checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ);
			if(iret<0) break;
			if(iret==0)
				if((time(NULL)-tLastReceived)>VIDC_MAX_CLIENT_TIMEOUT) break; else continue;
			//读客户端发送的数据
			iret=psock->Receive(buf+buflen,VIDC_MAX_COMMAND_SIZE-buflen-1,-1);
			if(iret<0) break; //==0表明接收数据流量超过限制
			if(iret==0){ cUtils::usleep(MAXRATIOTIMEOUT); continue; }
			tLastReceived=time(NULL); //最后一次接收到数据时间
			buflen+=iret; buf[buflen]=0; //解析vIDC命令
			const char *ptrCmd,*ptrBegin=buf;
			while( (ptrCmd=strchr(ptrBegin,'\r')) )
			{
				*(char *)ptrCmd=0;//开始解析命令
				if(ptrBegin[0]==0) goto NextCMD; //不处理空行数据
				if(strncmp(ptrBegin,"SSLC ",5)==0) //设置某个映射服务的客户端验证证书信息
				{//格式: SSLC name=<XXX> certlen=<证书字节> keylen=<私钥字节> pwdlen=<密码长度>\r\n后续字节\r\n
					const char *ptrData=ptrCmd+1;
					if(*ptrData=='\r' || *ptrData=='\n') ptrData++; //跳过\r\n
					long dataLen=buflen-(ptrData-buf); //得到接收缓冲区中还未分析处理的数据
					//docmd_sslc返回从接收缓冲区中取出处理的数据个数
					dataLen=pvidcc->docmd_sslc(ptrBegin+5,ptrData,dataLen);
					ptrBegin=ptrData+dataLen; continue; //继续下面的处理
				}else pvidcc->parseCommand(ptrBegin);

NextCMD:		//移动ptrBegin到下一个命令数据起始
				ptrBegin=ptrCmd+1; 
				while(*ptrBegin=='\r' || *ptrBegin=='\n') ptrBegin++; //跳过\r\n
			}//?while( (ptrCmd=
			//如果有未接收完的命令则移动
			if((iret=(ptrBegin-buf))>0 && (buflen-iret)>0)
			{//如果ptrBegin-buf==0说明这是一个错误命令数据包
				buflen-=iret;
				memmove((void *)buf,ptrBegin,buflen);
			} else buflen=0;
		}//?while...
		//从m_sessions队列中删除vidccSession
		m_mutex.lock();
		std::map<long,vidccSession *>::iterator it=m_sessions.find((long)pvidcc);
		if(it!=m_sessions.end()) m_sessions.erase(it);
		m_mutex.unlock(); 
		pvidcc->Destroy(); delete pvidcc; //Destroy会等待所有的管道结束
	}//?if(strncmp(buf,"HELO ",5)==0)
	else if(strncmp(buf,"PIPE ",5)==0)
	{//此连接是某个vIDCc的管道连接
		//创建一个管道，这样在onConnect退出后释放psock不会影响到转发线程
		socketTCP *pipe=new socketTCP(*psock);
		//设置空的父对象，以便下面等待管道绑定
		if(pipe==NULL) return; else pipe->setParent(NULL);
		long vidccID=atol(buf+5);
		vidccSession *pvidcc=AddPipeFromVidcSession(pipe,vidccID);
		if(pvidcc==NULL){ delete pipe; return; }
		
		time_t t=time(NULL);
		while(psocketsvr->status()==SOCKS_LISTEN && 
		//	  pvidcc->isConnected() && //也许pvidcc已经被释放删除了
			  pipe->status()==SOCKS_CONNECTED )
		{//如果管道绑定成功，则pipe的parent指向绑定的对端
			if(pipe->parent()!=NULL) return; //已经绑定了转发socket
			//指定的时间内还没有被绑定则结束
			else if((time(NULL)-t)>VIDC_PIPE_ALIVETIME) break; 
			cUtils::usleep(200000); //延迟200ms
		}//?while
		if(DelPipeFromVidcSession(pipe,vidccID)) delete pipe;
		//否则此管道已经被取走占用,不删除
//		else{ pipe->Close(); pipe->setParent(NULL); }
	}//?else if(strncmp(ptrBegin,"PIPE ",5)==0)
	//yyc add 2007-08-21 surpport MakeHole命令，用于TCP穿洞
	else if(strcmp(buf,"HOLE\r\n")==0)
	{
		//返回连接的IP地址和端口
		iret=sprintf(buf,"200 %s:%d\r\n",psock->getRemoteIP(),psock->getRemotePort());
		iret=psock->Send(iret,buf,-1);
		//循环等待对方释放连接
		while(psocketsvr->status()==SOCKS_LISTEN && 
			  psock->status()==SOCKS_CONNECTED )
		{
			iret=psock->checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ);
			if(iret<0) break;
			if(iret==0) continue;
			//如果对方温和关闭了连接则checkSocket检测==1,但读数据::recv会返回0
			iret=psock->Receive(buf,VIDC_MAX_COMMAND_SIZE-1,-1);
			if(iret<0) break;
		}//?while
	}//?else if(strcmp(buf,"HOLE\r\n")==0)
	//一个新的连接过来，起始只有可能发送HELO和PIPE命令,对于其他请求则关闭连接
}
//客户端登录验证
//	格式:HELO <SP> <NAME:pswd> <SP> VER <SP> DESC
vidccSession * vidcsvr :: docmd_helo(socketTCP *psock,const char *param)
{
	int len,clntVer=0xff; char buf[64];
	string strUser,strPswd,strDesc;
	const char *ptr1,*ptr=strchr(param,' ');
	if(ptr){ //获取Name和密码
		*(char *)ptr=0;
		if( (ptr1=strchr(param,':')) )
		{
			strPswd.assign(ptr1+1);
			strUser.assign(param,ptr1-param);
		}else strUser.assign(param);
		*(char *)ptr=' '; param=ptr+1;
		//获取版本号
		if( (ptr=strchr(param,' ')) )
		{
			strDesc.assign(ptr+1);
			clntVer=atoi(param);
		}else clntVer=atoi(param);
	}//?if(ptr)
	if(clntVer<VIDCC_MIN_VERSION){
		len=sprintf(buf,msg_err_405,VIDCS_VERSION);
		psock->Send(len,buf,-1); return NULL;
	}
	if(m_bAuthentication && m_strPswd!=strPswd){
		len=sprintf(buf,msg_err_221,VIDCS_VERSION);
		psock->Send(len,buf,-1); return NULL;
	}
	vidccSession *pvidcc=new vidccSession(psock,clntVer,strUser.c_str(),strDesc.c_str());
	if(pvidcc==NULL) return NULL;
	m_mutex.lock();
	m_sessions[(long)pvidcc]=pvidcc;
	m_mutex.unlock();
	len=sprintf(buf,"200 %d %d command success.\r\n",(long)pvidcc,VIDCS_VERSION);
	psock->Send(len,buf,-1); return pvidcc;
}
//vIDC客户端新建立了一个管道
//	格式:PIPE <SP> CLNTID <CRLF>
vidccSession * vidcsvr :: AddPipeFromVidcSession(socketTCP *pipe,long vidccID)
{
	vidccSession *pvidcc=NULL;
	m_mutex.lock();
	std::map<long,vidccSession *>::iterator it=m_sessions.find(vidccID);
	if(it!=m_sessions.end())
	{
		pvidcc=(*it).second;
		if(!pvidcc->AddPipe(pipe)) pvidcc=NULL;
//		RW_LOG_DEBUG("[vidcsvr] new pipe from vidccID %d\r\n",vidccID);
	}
	m_mutex.unlock(); return pvidcc;
}
bool vidcsvr :: DelPipeFromVidcSession(socketTCP *pipe,long vidccID)
{
	bool bret=false; m_mutex.lock();
	std::map<long,vidccSession *>::iterator it=m_sessions.find(vidccID);
	if(it!=m_sessions.end())
	{
		vidccSession *pvidcc=(*it).second;
		if(pvidcc) bret=pvidcc->DelPipe(pipe);
	}
	m_mutex.unlock(); return bret;
}
//------------------------------------------------------------
vidcServer :: vidcServer()
{
	m_strSvrname="vIDC Server";
}
void vidcServer :: Stop()
{
	Close();
	vidcsvr::Destroy();
	m_threadpool.join();
}


