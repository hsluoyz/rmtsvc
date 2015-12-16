/*******************************************************************
   *	vIDCc.h
   *    DESCRIPTION:vIDC客户类的定义
   *
   *    AUTHOR:yyc
   *	http://hi.baidu.com/yycblog/home
   *
   *******************************************************************/

#include "../../net4cpp21/include/sysconfig.h"
#include "../../net4cpp21/utils/utils.h"
#include "../../net4cpp21/include/cLogger.h"
#include "../../net4cpp21/include/proxysvr.h"
#include "vidcc.h"

using namespace std;
using namespace net4cpp21;

//-----------------------------------vidcClient-------------------------------
vidcClient :: vidcClient(const char *strname,const char *strdesc) : socketProxy()
{
	if(strname) m_strName.assign(strname);
	if(strdesc) m_strDesc.assign(strdesc);

	m_szLastResponse[0]=0;
	m_lTimeout=VIDC_MAX_RESPTIMEOUT;
}

vidcClient :: ~vidcClient()
{
	if(this->status()!=SOCKS_CLOSED) this->Close();
	m_threadpool.join();//不在Close时等待线程结束，防止在回调中调用Close
}

void vidcClient :: Destroy()
{
	DisConnSvr();
	std::map<std::string,mapInfo *>::iterator it=m_mapsets.begin();
	for(;it!=m_mapsets.end();it++) delete (*it).second;
	m_mapsets.clear();
}

void vidcClient :: DisConnSvr()
{
	this->Close();
	std::map<std::string,mapInfo *>::iterator it=m_mapsets.begin();
	for(;it!=m_mapsets.end();it++){
		mapInfo *pinfo=(*it).second;
		if(pinfo){ pinfo->m_mappedPort=0; pinfo->m_mappedSSLv=false; }
	}
	m_vidcsinfo.m_vidccID=0;
	m_vidcsinfo.m_vidcsVer=0;
	m_vidcsinfo.m_vidcsIPList="";
}

SOCKSRESULT vidcClient :: ConnectSvr()
{
	SOCKSRESULT sr=this->Connect(m_vidcsinfo.m_vidcsHost.c_str(),m_vidcsinfo.m_vidcsPort,VIDC_MAX_RESPTIMEOUT);
	if(sr<=0) return sr;
	if( m_threadpool.addTask((THREAD_CALLBACK *)&onCommandThread,(void *)this,0)==0 )
	{ this->Close(); return SOCKSERR_THREAD; }
	
	int rspcode=0; const char *ptr;
	int buflen=sprintf(m_szLastResponse,"HELO %s:%s %d %s\r\n",m_strName.c_str(),
					  m_vidcsinfo.m_vidcsPswd.c_str(),VIDCC_VERSION,m_strDesc.c_str());
	if(sendCommand(200,m_szLastResponse,buflen))
	{
		//vIDCs返回的格式为 200 <vidccID> <vidcs_ver> <描述>
		m_vidcsinfo.m_vidccID=atoi(m_szLastResponse+4);
		if( (ptr=strchr(m_szLastResponse+4,' ')) ) m_vidcsinfo.m_vidcsVer=atoi(ptr+1);
		buflen=sprintf(m_szLastResponse,"ADDR \r\n"); //获取vIDCs服务器的IP地址列表
		if(sendCommand(200,m_szLastResponse,buflen))
			m_vidcsinfo.m_vidcsIPList.assign(m_szLastResponse+4);
		//自动映射指定的映射服务
		std::map<std::string,mapInfo *>::iterator it=m_mapsets.begin();
		for(;it!=m_mapsets.end();it++)
		{
			mapInfo *pinfo=(*it).second;
			if(pinfo && pinfo->m_bAutoMap) this->Mapped((*it).first.c_str(),pinfo);
		}
		return sr;
	}else rspcode=atoi(m_szLastResponse);
	
	if(rspcode==405) sr=SOCKSERR_VIDC_VER;
	else if(rspcode==221) sr=SOCKSERR_VIDC_PSWD;
	else sr=SOCKSERR_VIDC_RESP;
	this->Close(); return sr;
}

//删除指定的映射服务
bool vidcClient :: mapinfoDel(const char *mapname)
{
	std::map<std::string,mapInfo *>::iterator it=m_mapsets.find(mapname);
	if(it==m_mapsets.end()) return false;
	mapInfo *pinfo=(*it).second;
	m_mapsets.erase(it);
	Unmap(mapname,pinfo); //先发送ubind命令
	delete pinfo; return true;
}

mapInfo * vidcClient :: mapinfoGet(const char *mapname,bool bCreate)
{
	std::map<std::string,mapInfo *>::iterator it=m_mapsets.find(mapname);
	if(it!=m_mapsets.end()) return (*it).second;
	if(!bCreate) return NULL;
	mapInfo * pinfo=new mapInfo;
	m_mapsets[mapname]=pinfo;
	return pinfo;
}

long getFilesize(const char *file)
{
	if(file==NULL || file[0]==0) return 0;
	//将一个相对路径名转换为一个绝对路径名
	char buf[MAX_PATH];
	DWORD dwret=::GetModuleFileName(NULL,buf,MAX_PATH);
	buf[dwret]=0;
	const char *ptr=strrchr(buf,'\\');
	if(ptr==NULL) return 0;
	strcpy((char *)ptr+1,file);
	FILE *fp=::fopen(buf,"rb");
	if(fp==NULL) return 0;
	::fseek(fp,0,SEEK_END);
	long l=::ftell(fp);
	::fclose(fp); return l;
}
long readFile(const char *file,char *readbuf,long readsize)
{
	if(readsize<=0) return 0;
	if(file==NULL || file[0]==0) return 0;
	//将一个相对路径名转换为一个绝对路径名
	char buf[MAX_PATH];
	DWORD dwret=::GetModuleFileName(NULL,buf,MAX_PATH);
	buf[dwret]=0;
	const char *ptr=strrchr(buf,'\\');
	if(ptr==NULL) return 0;
	strcpy((char *)ptr+1,file);
	FILE *fp=::fopen(buf,"rb");
	if(fp==NULL) return 0;
	long l=::fread(readbuf,sizeof(char),readsize,fp);
	::fclose(fp); return l;
}

//成功返回>0
int vidcClient :: Mapped(const char *mapname,mapInfo *pinfo) //映射指定的服务
{
//	if(pinfo==NULL || mapname==NULL) return SOCKSERR_PARAM;
	if(this->status()!=SOCKS_CONNECTED) return SOCKSERR_CLOSED;
//	BIND type=[TCP|UDP] name=<XXX> appsvr=<要映射的应用服务> mport=<映射端口>[+|-ssl] [bindip=<本服务绑定的本机IP>] [apptype=FTP|WWW|TCP|UNKNOW] [appdesc=<描述>]
//	BIND type=PROXY name=<XXX> mport=<映射端口> [bindip=<本服务绑定的本机IP>] [appdesc=<描述>]
	int rspcode,buflen;
	if(pinfo->m_mapType==VIDC_MAPTYPE_PROXY)
	{
		buflen=sprintf(m_szLastResponse,"BIND type=PROXY name=%s mport=%d-%d bindip=%s maxconn=%d maxratio=%d appdesc=\"内网代理\"\r\n",
					   mapname,pinfo->m_mportBegin,pinfo->m_mportEnd,pinfo->m_bindLocalIP,pinfo->m_maxconn,pinfo->m_maxratio);
	}else{
		buflen=sprintf(m_szLastResponse,"BIND type=%s name=%s appsvr=%s mport=%d-%d%s sslverify=%d bindip=%s maxconn=%d maxratio=%d apptype=%s appdesc=\"%s\"\r\n",
					  ((pinfo->m_mapType==VIDC_MAPTYPE_UDP)?"UDP":"TCP"),mapname,
					  pinfo->m_appsvr.c_str(),pinfo->m_mportBegin,pinfo->m_mportEnd,
					  ((pinfo->m_ssltype==TCPSVR_SSLSVR)?"+ssl":((pinfo->m_ssltype==SSLSVR_TCPSVR)?"-ssl":"") ),
					  ((pinfo->m_sslverify)?1:0),
					  pinfo->m_bindLocalIP,pinfo->m_maxconn,pinfo->m_maxratio,
					  ((pinfo->m_apptype==MPORTTYPE_FTP)?"FTP":((pinfo->m_apptype==MPORTTYPE_WWW)?"WWW":((pinfo->m_apptype==MPORTTYPE_TCP)?"TCP":"UNK" ) ) ),
					  pinfo->m_appdesc.c_str() );
	}
	//vIDCs返回的格式为 200 <映射端口> <描述>
	if(sendCommand(200,m_szLastResponse,buflen))
	{
		if( (rspcode=atoi(m_szLastResponse+4))>0 ) pinfo->m_mappedPort=rspcode;
		pinfo->m_mappedSSLv=(strstr(m_szLastResponse+4," sslv=1 "))?true:false;
		if(pinfo->m_ipRules!="")
		{
			buflen=sprintf(m_szLastResponse,"FILT name=%s access=%d ipaddr=%s\r\n",
						   mapname,pinfo->m_ipaccess,pinfo->m_ipRules.c_str());
			sendCommand(200,m_szLastResponse,buflen);
		}
		//发送客户端证书
		if(pinfo->m_ssltype==SSLSVR_TCPSVR && pinfo->m_clicert!="")
		{
			long certlen=getFilesize(pinfo->m_clicert.c_str());
			long keylen=getFilesize(pinfo->m_clikey.c_str());
			long pwdlen=pinfo->m_clikeypswd.length();
			char *sbuf=new char[128+certlen+keylen+pwdlen+5];
			if(sbuf){
				long cmdlen=sprintf(sbuf,"SSLC name=%s certlen=%d keylen=%d pwdlen=%d\r\n",
							mapname,certlen+1,keylen+1,pwdlen+1);
				char *databuf=sbuf+cmdlen;
				long lread=readFile(pinfo->m_clicert.c_str(),databuf,certlen);
				databuf[lread++]=0;
				lread+=readFile(pinfo->m_clikey.c_str(),databuf+lread,keylen);
				databuf[lread++]=0;
				strcpy(databuf+lread,pinfo->m_clikeypswd.c_str());
				lread+=(pwdlen+1); 
				strcpy(databuf+lread,"\r\n"); lread+=2;
				if(lread==(certlen+keylen+pwdlen+5)) sendCommand(200,sbuf,lread+cmdlen);
				delete[] sbuf;
			}//?if(sbuf)
		}//?if(pinfo->m_ssltype==SSLSVR_TCPSVR && pinfo->m_clicert!="")
		int i;//发送修改HTTP请求头和响应头规则
		for(i=0;i<pinfo->m_hrspRegCond.size();i++)
		{
			buflen=sprintf(m_szLastResponse,"HRSP name=%s %s\r\n",mapname,pinfo->m_hrspRegCond[i].c_str());
			sendCommand(200,m_szLastResponse,buflen);
		}//?for(i=0;
		for(i=0;i<pinfo->m_hreqRegCond.size();i++)
		{
			buflen=sprintf(m_szLastResponse,"HREQ name=%s %s\r\n",mapname,pinfo->m_hreqRegCond[i].c_str());
			sendCommand(200,m_szLastResponse,buflen);
		}//?for(i=0;
		return pinfo->m_mappedPort; //返回映射的端口
	}else rspcode=atoi(m_szLastResponse);
	if(rspcode==501) return SOCKSERR_VIDC_NAME;
	else if(rspcode==502) return SOCKSERR_VIDC_MEMO;
	else if(rspcode==503) return SOCKSERR_VIDC_SURPPORT;
	else if(rspcode==504) return SOCKSERR_VIDC_MAP;
	return SOCKSERR_VIDC_RESP;
}

int vidcClient :: Unmap(const char *mapname,mapInfo *pinfo) //取消映射指定的服务
{
//	if(pinfo==NULL) return SOCKSERR_PARAM;
	if(this->status()!=SOCKS_CONNECTED) return SOCKSERR_CLOSED;
	
	pinfo->m_mappedPort=0; pinfo->m_mappedSSLv=false;
	//格式: UBND <SP> <映射名称> <CRLF>
	int buflen=sprintf(m_szLastResponse,"UBND %s\r\n",mapname);
	if(sendCommand(200,m_szLastResponse,buflen))
		return SOCKSERR_OK;
	int rspcode=atoi(m_szLastResponse);
	if(rspcode==501) return SOCKSERR_VIDC_NAME;
	return SOCKSERR_VIDC_RESP;
}

void vidcClient :: str_list_mapped(const char *vname,std::string &strini)
{
	char buf[1024]; int buflen=0;
	std::map<std::string,mapInfo *>::iterator it=m_mapsets.begin();
	for(;it!=m_mapsets.end();it++)
	{
		mapInfo *pinfo=(*it).second;
		if(pinfo->m_mapType==VIDC_MAPTYPE_PROXY)
		{
			buflen=sprintf(buf,"mtcpr type=PROXY vname=%s name=%s mport=%d-%d bindip=%s svrtype=%s|%s|%s bauth=%d account=%s:%s automap=%d maxconn=%d maxratio=%d appdesc=\"%s\"\r\n",
						   vname,(*it).first.c_str(),pinfo->m_mportBegin,pinfo->m_mportEnd,pinfo->m_bindLocalIP,
						   ((pinfo->m_proxyType &PROXY_HTTPS)?"HTTPS":""),
						   ((pinfo->m_proxyType &PROXY_SOCKS4)?"SOCKS4":""),
						   ((pinfo->m_proxyType &PROXY_SOCKS5)?"SOCKS5":""),
						   ((pinfo->m_proxyauth)?1:0),
						   pinfo->m_proxyuser.c_str(),pinfo->m_proxypswd.c_str(),
						   ((pinfo->m_bAutoMap)?1:0),
						   pinfo->m_maxconn,pinfo->m_maxratio,
						   pinfo->m_appdesc.c_str() );
		}else{
			buflen=sprintf(buf,"mtcpr type=%s vname=%s name=%s appsvr=%s mport=%d-%d%s sslverify=%d bindip=%s apptype=%s automap=%d maxconn=%d maxratio=%d appdesc=\"%s\"\r\n",
						  ((pinfo->m_mapType==VIDC_MAPTYPE_UDP)?"UDP":"TCP"),vname,(*it).first.c_str(),
						  pinfo->m_appsvr.c_str(),pinfo->m_mportBegin,pinfo->m_mportEnd,
						  ((pinfo->m_ssltype==TCPSVR_SSLSVR)?"+ssl":((pinfo->m_ssltype==SSLSVR_TCPSVR)?"-ssl":"") ),
						  ((pinfo->m_sslverify)?1:0),
						  pinfo->m_bindLocalIP,
						  ((pinfo->m_apptype==MPORTTYPE_FTP)?"FTP":((pinfo->m_apptype==MPORTTYPE_WWW)?"WWW":((pinfo->m_apptype==MPORTTYPE_TCP)?"TCP":"UNK" ) ) ),
						  ((pinfo->m_bAutoMap)?1:0), pinfo->m_maxconn,pinfo->m_maxratio,
						  pinfo->m_appdesc.c_str() );
		}
		strini.append(buf,buflen);
		if(pinfo->m_clicert=="")
		{
			buflen=sprintf(buf,"sslc vname=%s name=%s cert=\r\n",vname,(*it).first.c_str());
			strini.append(buf,buflen);
		}else{
			buflen=sprintf(buf,"sslc vname=%s name=%s cert=",vname,(*it).first.c_str());
			strini.append(buf,buflen); strini+=pinfo->m_clicert; strini+=string(",");
			strini+=pinfo->m_clikey; strini+=string(","); strini+=pinfo->m_clikeypswd; strini+=string("\r\n");
		}
		buflen=sprintf(buf,"iprules type=mtcpr vname=%s name=%s access=%d ipaddr=%s\r\n",
			vname,(*it).first.c_str(),pinfo->m_ipaccess,pinfo->m_ipRules.c_str());
		strini.append(buf,buflen);
		int i;//发送修改HTTP请求头和响应头规则
		for(i=0;i<pinfo->m_hrspRegCond.size();i++)
		{
			buflen=sprintf(buf,"mdhrsp vname=%s name=%s %s\r\n",vname,
				(*it).first.c_str(),pinfo->m_hrspRegCond[i].c_str());
			strini.append(buf,buflen);
		}//?for(i=0;
		for(i=0;i<pinfo->m_hreqRegCond.size();i++)
		{
			buflen=sprintf(buf,"mdhrsp vname=%s name=%s %s\r\n",vname,
				(*it).first.c_str(),pinfo->m_hreqRegCond[i].c_str());
			strini.append(buf,buflen);
		}//?for(i=0;
	}//?for(;it
}

void vidcClient :: xml_list_mapped(cBuffer &buffer,VIDC_MAPTYPE maptype)
{
	if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
	if(buffer.str())
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<maplist>");
	std::map<std::string,mapInfo *>::iterator it=m_mapsets.begin();
	for(;it!=m_mapsets.end();it++)
	{
		mapInfo *pinfo=(*it).second;
		if(pinfo->m_mapType!=maptype) continue;
		if(buffer.Space()<256) buffer.Resize(buffer.size()+256);
		if(buffer.str())
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<mapped name=\"%s\"><![CDATA[%s]]></mapped>",
				(*it).first.c_str(),pinfo->m_appdesc.c_str());
		else break;
	}
	if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
	if(buffer.str())
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"</maplist>");
	return;
}

//-------------------vIDC client 命令解析处理 begin-----------------------------------
void vidcClient :: parseCommand(const char *ptrCommand)
{
	RW_LOG_DEBUG("[vidcc] s--->c:\r\n\t%s\r\n",ptrCommand);
	
	if(strncmp(ptrCommand,"PIPE ",5)==0) //vIDCs请求一个管道
		m_threadpool.addTask((THREAD_CALLBACK *)&onPipeThread,(void *)this,0);
	else if(atoi(ptrCommand)>0) //vIDCs的命令返回消息
		strcpy(m_szLastResponse,ptrCommand);
	return;
}

void vidcClient :: onCommandThread(vidcClient *pvidcc)
{
	if(pvidcc==NULL) return;
	char buf[VIDC_MAX_COMMAND_SIZE]; int buflen=0;
	
	time_t tLastReceived=time(NULL);
	time_t tLastSended=time(NULL);
	while(pvidcc->status()==SOCKS_CONNECTED )
	{
		int iret=pvidcc->checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ);
		if(iret<0) break;
		if(iret==0){
			if((time(NULL)-tLastReceived)>VIDC_MAX_CLIENT_TIMEOUT) break;
			if((time(NULL)-tLastSended)>VIDC_NOP_INTERVAL){
				pvidcc->Send(7,"vNOP \r\n",-1);
				tLastSended=time(NULL);
			}else continue;
		}
		//读客户端发送的数据
		iret=pvidcc->Receive(buf+buflen,VIDC_MAX_COMMAND_SIZE-buflen-1,-1);
		if(iret<0) break; //==0表明接收数据流量超过限制
		if(iret==0){ cUtils::usleep(MAXRATIOTIMEOUT); continue; }
		tLastReceived=time(NULL); //最后一次接收到数据时间
		buflen+=iret; buf[buflen]=0;
		//解析vidc命令
		const char *ptrCmd,*ptrBegin=buf;
		while( (ptrCmd=strchr(ptrBegin,'\r')) )
		{
			*(char *)ptrCmd=0;//开始解析命令
			if(ptrBegin[0]==0) goto NextCMD; //不处理空行数据
		
			pvidcc->parseCommand(ptrBegin);

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
	pvidcc->DisConnSvr(); return;
}

class cProxysvrEx : public cProxysvr
{
public:
	explicit cProxysvrEx(cThreadPool *p):m_pthreadpool(p),cProxysvr(){}
	virtual ~cProxysvrEx(){}
	void doProxyReq(socketTCP *psock){ cProxysvr::onConnect(psock); }
protected:
	//创建转发对任务线程
	virtual bool onTransferTask(THREAD_CALLBACK *pfunc,void *pargs)
	{
		return (m_pthreadpool && m_pthreadpool->addTask(pfunc,pargs,THREADLIVETIME)!=0);
	}
private:
	cThreadPool *m_pthreadpool;//服务线程池
};

void vidcClient :: onPipeThread(vidcClient *pvidcc)
{
	if(pvidcc==NULL) return;
	socketProxy *pipe=new socketProxy;
	if(pipe==NULL) return;
	pipe->setProxy(*pvidcc); //设置代理和vidcc的代理一致
	pipe->setParent(pvidcc);
	pipe->Connect(pvidcc->m_vidcsinfo.m_vidcsHost.c_str(),pvidcc->m_vidcsinfo.m_vidcsPort,-1);

	char buf[VIDC_MAX_COMMAND_SIZE];
	int buflen=sprintf(buf,"PIPE %d\r\n",pvidcc->m_vidcsinfo.m_vidccID);
	pipe->Send(buflen,buf,-1);
	while(pipe->status()==SOCKS_CONNECTED )
	{
		int iret=pipe->checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ);
		if(iret<0) break; else if(iret==0) continue;
		cProxysvrEx proxysvr(&pvidcc->m_threadpool);
		proxysvr.doProxyReq(pipe); //否则有数据
		break; //管道线程结束
	}//?while
	delete pipe; return;
}
//发送命令，并获取服务器响应
inline bool vidcClient :: sendCommand(int response_expected,const char *buf,int buflen)
{
	RW_LOG_DEBUG("[vidcc] c--->s:\r\n\t%s",buf);
	char c=buf[0];
	if( this->Send(buflen,buf,-1)<=0 ) return false;
	//发送成功，等待接收服务器响应,服务器的响应存储在m_szLastResponse缓冲中
	time_t tStart=time(NULL);
	while(m_szLastResponse[0]==c){
		if((time(NULL)-tStart)>m_lTimeout) break; //超时
		cUtils::usleep(SCHECKTIMEOUT);
	}//?while
	int responseCode=atoi(m_szLastResponse);
	return (response_expected==responseCode);
}

//-----------------------------------vidccSets--------------------------------------
vidccSets :: vidccSets()
{
	char pcname[MAX_COMPUTERNAME_LENGTH+1];
	DWORD dwSize=MAX_COMPUTERNAME_LENGTH;
	if( ::GetComputerName(pcname,&dwSize) ){
		pcname[dwSize]=0;
		m_strName.assign(pcname);
	}
	m_strDesc="vidcc client";
}

vidccSets :: ~vidccSets(){ Destroy(); }

void vidccSets :: Destroy()
{
	m_mutex.lock();
	std::map<std::string,vidcClient *>::iterator it=m_vidccs.begin();
	for(;it!=m_vidccs.end();it++)
	{
		vidcClient *pvidcc=(*it).second;
		if(pvidcc!=NULL) pvidcc->Destroy();
		delete pvidcc;
	}
	m_vidccs.clear();
	m_mutex.unlock();
}
//自动重连检测
void vidccSets :: autoConnect()
{
	m_mutex.lock();
	std::map<std::string,vidcClient *>::iterator it=m_vidccs.begin();
	for(;it!=m_vidccs.end();it++)
	{
		vidcClient *pvidcc=(*it).second;
		if(pvidcc && pvidcc->vidcsinfo().m_bAutoConn && pvidcc->status()!=SOCKS_CONNECTED)
		{
			SOCKSRESULT sr=pvidcc->ConnectSvr();
			if(sr==SOCKSERR_VIDC_VER || sr==SOCKSERR_VIDC_PSWD)
				pvidcc->vidcsinfo().m_bAutoConn=false; //下次不在尝试重连
		}
	}
	m_mutex.unlock();
	return;
}

vidcClient * vidccSets :: GetVidcClient(const char *vname,bool bCreate)
{
	vidcClient *pvidcc=NULL;
	m_mutex.lock();
	std::map<std::string,vidcClient *>::iterator it=m_vidccs.find(vname);
	if(it!=m_vidccs.end()) pvidcc=(*it).second;
	if(pvidcc || !bCreate){ m_mutex.unlock(); return pvidcc; }
	pvidcc=new vidcClient(m_strName.c_str(),m_strDesc.c_str());
	VIDCSINFO &vinfo=pvidcc->vidcsinfo();
	vinfo.m_bAutoConn=false;
	vinfo.m_vidcsPort=0;
	vinfo.m_vidcsVer=0;
	vinfo.m_vidccID=0;
	m_vidccs[vname]=pvidcc;
	m_mutex.unlock();
	return pvidcc;
}

bool vidccSets :: DelVidcClient(const char *vname)
{
	m_mutex.lock();
	std::map<std::string,vidcClient *>::iterator it=m_vidccs.find(vname);
	if(it==m_vidccs.end()){  m_mutex.unlock(); return false; }
	vidcClient *pvidcc=(*it).second;
	m_vidccs.erase(it);
	m_mutex.unlock();
	if(pvidcc!=NULL){
		pvidcc->vidcsinfo().m_bAutoConn=false;
		pvidcc->Destroy();
	}
	delete pvidcc; return true;
}

bool vidccSets :: xml_info_vidcc(cBuffer &buffer,const char *vname,VIDC_MAPTYPE maptype)
{
	m_mutex.lock();
	std::map<std::string,vidcClient *>::iterator it=m_vidccs.find(vname);
	if(it==m_vidccs.end()){ m_mutex.unlock(); return false; }
	vidcClient *pvidcc=(*it).second;
	if(pvidcc==NULL){ m_mutex.unlock(); return false; }
	
	if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
	if(buffer.str()==NULL){ m_mutex.unlock(); return false; }
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vidcsinfo>");
	
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vname>%s</vname>",vname);
	VIDCSINFO &vinfo=pvidcc->vidcsinfo();
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vhost>%s</vhost>",vinfo.m_vidcsHost.c_str());
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vport>%d</vport>",vinfo.m_vidcsPort);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vpswd>%s</vpswd>",vinfo.m_vidcsPswd.c_str());
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vconn>%d</vconn>",((vinfo.m_bAutoConn)?1:0) );
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<localip>%s</localip>",vinfo.m_vidcsIPList.c_str());
	if(pvidcc->status()==SOCKS_CONNECTED)
	{
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<connected>1</connected>");
		time_t t=pvidcc->getStartTime(); struct tm *ltime=localtime(&t);
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<starttime>%04d年%02d月%02d日 %02d:%02d:%02d</starttime>",
			(1900+ltime->tm_year), ltime->tm_mon+1, ltime->tm_mday,ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
	}
	else buffer.len()+=sprintf(buffer.str()+buffer.len(),"<connected>0</connected>");
	pvidcc->xml_list_mapped(buffer,maptype);
	m_mutex.unlock();
	if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
	if(buffer.str())
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"</vidcsinfo>");
	return true;
}

void vidccSets :: xml_list_vidcc(cBuffer &buffer)
{
	if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
	if(buffer.str()==NULL) return;
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vidcslist>");
	m_mutex.lock();
	std::map<std::string,vidcClient *>::iterator it=m_vidccs.begin();
	for(;it!=m_vidccs.end();it++)
	{
		if(buffer.Space()<64) buffer.Resize(buffer.size()+64);
		if(buffer.str()==NULL) break;
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vidcs>");
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vname>%s</vname>",(*it).first.c_str());
		vidcClient *pvidcc=(*it).second;
		VIDCSINFO &vinfo=pvidcc->vidcsinfo();
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vhost>%s</vhost>",vinfo.m_vidcsHost.c_str());
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vport>%d</vport>",vinfo.m_vidcsPort);
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vpswd>%s</vpswd>",vinfo.m_vidcsPswd.c_str());
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vconn>%d</vconn>",((vinfo.m_bAutoConn)?1:0) );
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"</vidcs>");
	}
	m_mutex.unlock();	
	if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
	if(buffer.str())
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"</vidcslist>");
	return;
}

void vidccSets :: str_list_vidcc(std::string &strini)
{
	char buf[256]; int buflen=0;
	m_mutex.lock();
	std::map<std::string,vidcClient *>::iterator it=m_vidccs.begin();
	for(;it!=m_vidccs.end();it++)
	{
		vidcClient *pvidcc=(*it).second;
		VIDCSINFO &vinfo=pvidcc->vidcsinfo();
		buflen=sprintf(buf,"vidcc vname=%s vhost=%s:%d vpswd=%s autorun=%d\r\n",
						(*it).first.c_str(),vinfo.m_vidcsHost.c_str(),vinfo.m_vidcsPort,
						vinfo.m_vidcsPswd.c_str(),((vinfo.m_bAutoConn)?1:0) );
		strini.append(buf,buflen);
		//端口映射信息
		pvidcc->str_list_mapped((*it).first.c_str(),strini);
	}
	m_mutex.unlock();
}



