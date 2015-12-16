/*******************************************************************
   *	mportsvr_TCP.cpp
   *    DESCRIPTION:本地端口映射服务 for TCP
   *
   *    AUTHOR:yyc
   *	http://hi.baidu.com/yycblog/home
   *
   *	net4cpp 2.1
   *	
   *******************************************************************/
#include "../../net4cpp21/include/sysconfig.h"
#include "../../net4cpp21/utils/utils.h"
#include "../../net4cpp21/include/cLogger.h"
#include "../../net4cpp21/include/httpreq.h"
#include "mportsvr.h"

using namespace std;
using namespace net4cpp21;

class HttpHeader
{
public:
	HttpHeader(const char *httpHeader=NULL);
	~HttpHeader(){}
	bool GetUrl(std::string &strurl); //获取请求url
	long GetRspCode(); //获取响应代码
	bool Send(socketTCP *psock,FILE *fp);
	void Modify(std::vector<RegCond> &regcond);	
	//判断是否要进行URL重写 //yyc add 2010-02-23
	bool URLRewrite(std::map<std::string,std::string> &URLRewriter);
private:
	std::map<std::string,std::string> m_header;
	std::string m_strFirstLine;
};
//========================================================
//=============================================================
mportTCP :: mportTCP()
{
	m_strSvrname="mapped-Server";
#ifdef _SURPPORT_OPENSSL_
		setCacert(NULL,NULL,NULL,true); //默认加载内置的证书
#endif
		
	m_apptype=MPORTTYPE_UNKNOW;
	m_mportBegin=m_mportEnd=0; //默认随机映射端口
	::memset((void *)m_bindLocalIP,0,sizeof(m_bindLocalIP));
	m_ssltype=TCPSVR_TCPSVR;
	m_bSSLVerify=false;
	m_lUserTag=0;
	m_maxratio=0;
	m_bLogdatafile=false;
}

mportTCP :: ~mportTCP()
{
	Close();
	//服务析构前要保证线程都结束，因为线程中访问了mportTCP类的对象
	m_threadpool.join(); 
	m_modRspHeader.clear();
	m_modReqHeader.clear();
	m_modURLRewriter.clear();
}

//设置SSL证书信息并启动映射服务
SOCKSRESULT mportTCP :: Start(const char *strMyCert,const char *strMyKey,const char *strKeypwd,
					   const char *strCaCert,const char *strCaCRL) 
{
	if(this->status()==SOCKS_LISTEN) return this->getLocalPort();
#ifdef _SURPPORT_OPENSSL_
	if(m_ssltype==TCPSVR_SSLSVR){ //将普通TCP服务转换为SSL加密服务
		if(m_bSSLVerify)
			setCacert(strMyCert,strMyKey,strKeypwd,false,strCaCert,strCaCRL); //使用用户指定的证书和CRL
		else setCacert(strMyCert,strMyKey,strKeypwd,false,NULL,NULL);
		this->initSSL(true,NULL);
	}
#endif
	return this->Listen(m_mportBegin,m_mportEnd,FALSE,m_bindLocalIP);
}
//不设置SSL证书信息,启动映射服务
SOCKSRESULT mportTCP :: StartX() 
{
	if(this->status()==SOCKS_LISTEN) return this->getLocalPort();
#ifdef _SURPPORT_OPENSSL_
	if(m_ssltype==TCPSVR_SSLSVR) //将普通TCP服务转换为SSL加密服务
		this->initSSL(true,NULL);
#endif
	return this->Listen(m_mportBegin,m_mportEnd,FALSE,m_bindLocalIP);
}
//停止映射服务
void mportTCP :: Stop()
{ 
	Close();
#ifdef _SURPPORT_OPENSSL_
	freeSSL();
#endif
	return;
}

void mportTCP :: setSSLType(SSLTYPE ssltype,bool bSSLVerify)
{
	if( (m_ssltype=ssltype)!=TCPSVR_SSLSVR)
		m_bSSLVerify=false;
	else m_bSSLVerify=bSSLVerify;
	return;
}

//设置要映射的应用服务
void mportTCP :: setAppsvr(const char *appsvr,int apport,const char *appdesc,MPORTTYPE apptype)
{
	m_appSvr.clear();
	if(appsvr && appsvr[0]!=0)
	{
		const char *ptrEnd,*ptrBegin=appsvr;
		while(true)
		{
			while(*ptrBegin==' ') ptrBegin++;
			ptrEnd=strchr(ptrBegin,',');
			if(ptrEnd) *(char *)ptrEnd=0;

			const char *ptr=strchr(ptrBegin,':');
			int hostport=apport;
			if(ptr){ *(char *)ptr=0; hostport=atoi(ptr+1);}
			if(hostport>0 && ptrBegin[0]!=0)
			{ //二级代理服务地址和端口有效
				std::pair<std::string,int> p(ptrBegin,hostport);
				m_appSvr.push_back(p);
			}
			if(ptr) *(char *)ptr=':';

			if(ptrEnd) *(char *)ptrEnd=',';
			else break;
			ptrBegin=ptrEnd+1;
		}//?while
	}//?if(appsvr && appsvr[0]!=0) 
	m_apptype=apptype;
	if(appdesc) m_strSvrname.assign(appdesc);
	return;
}

void mportTCP :: setMapping(int mportStart,int mportEnd,const char *bindip)
{
	if( (m_mportBegin=mportStart)< 0) m_mportBegin=0;
	if( (m_mportEnd=mportEnd)< 0) m_mportEnd=0;
	if(bindip && strlen(bindip)<=15)
		strcpy(m_bindLocalIP,bindip);
	else m_bindLocalIP[0]=0;
}


//连接指定的应用服务并返回连接socketTCP对象
socketTCP * mportTCP :: connectAppsvr(char *strHost,socketTCP *psock)
{
	std::pair<std::string,int> *p=GetAppsvr();
	if(p==NULL) return NULL;
	socketTCP *ppeer=new socketTCP;
	if(ppeer==NULL) return NULL;
	if(ppeer->Connect(p->first.c_str(),p->second,MAX_CONNECT_TIMEOUT)>0 )
	{
		if(p->second!=80) 
			sprintf(strHost,"%s:%d",p->first.c_str(),p->second);
		else sprintf(strHost,"%s",p->first.c_str());
		return ppeer;
	}else RW_LOG_DEBUG("Can not connect Mapped server(%s:%d)\r\n",p->first.c_str(),p->second);
	delete ppeer; return NULL;
}

//当有一个新的客户连接此服务触发此函数
void mportTCP :: onAccept(socketTCP *psock)
{
	char strHost[64]; //strHost 实际连接主机的地址，格式Host:port
	socketTCP *ppeer=connectAppsvr(strHost,psock); //连接被映射的服务
	if(ppeer==NULL) return; else ppeer->setParent(psock);
	RW_LOG_DEBUG("Success to connect Mapped server(%s)\r\n",strHost);
	if(m_ssltype==SSLSVR_TCPSVR)
	{//如果应用服务是SSL加密服务且映射后为普通服务,加载客户端证书
		ppeer->setCacert(this,true); //从server复制客户端证书
		ppeer->initSSL(false,NULL); //初始化SSL客户端
		if(!ppeer->SSL_Associate()){ 
			RW_LOG_DEBUG("Failed to SSL_Associate,Mapped server(%s)\r\n",strHost);
			ppeer->Close(); delete ppeer; return; 
		}
	}//?else if(m_ssltype==SSLSVR_TCPSVR)
	//设置限制带宽
	psock->setSpeedRatio(m_maxratio*1024,m_maxratio*1024);
	ppeer->setSpeedRatio(psock->getMaxSendRatio(),psock->getMaxRecvRatio());
	FILE *fp=NULL;
	if(m_bLogdatafile){ //是否记录代理数据记录
		char logfilename[256]; //记录日志文件名
		int logfilenameLen=sprintf(logfilename,"%s_%d-",psock->getRemoteIP(),psock->getRemotePort());
		logfilenameLen+=sprintf(logfilename+logfilenameLen,"%s.log",strHost);
		logfilename[logfilenameLen]=0; //strHost中包含:
		const char *ptr=strchr(logfilename,':'); if(ptr) *(char *)ptr='_';
		if( (fp=::fopen(logfilename,"ab")) )
		{
			logfilenameLen=sprintf(logfilename,"C %s:%d <----> S ",psock->getRemoteIP(),psock->getRemotePort());
			::fwrite(logfilename,1,logfilenameLen,fp);
			logfilenameLen=sprintf(logfilename,"%s\r\n",strHost);
			::fwrite(logfilename,1,logfilenameLen,fp);
		}
	}//?if(m_bLogdatafile)
	std::pair<socketTCP *,FILE *> p(ppeer,fp);
	if(m_threadpool.addTask((THREAD_CALLBACK *)&transDataThread,(void *)&p,THREADLIVETIME)!=0)
	{
		char *pbuf,buffer[MAX_TRANSFER_BUFFER+1]; //转发缓冲大小
		long buflen=0; //转发缓冲区中待发送数据大小
		while( psock->status()==SOCKS_CONNECTED )
		{
			int iret=psock->checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ);
			if(iret<0) break; 
			if(iret==0) continue; //没有数据到达
			iret=psock->Receive(buffer+buflen,MAX_TRANSFER_BUFFER-buflen,-1);
			if(iret<0) break; //==0表明接收数据流量超过限制
			if(iret==0){ cUtils::usleep(MAXRATIOTIMEOUT); continue; }
			else{ buflen+=iret; pbuf=buffer; }
			pbuf[buflen]='\0'; //保证为空结束字符串
			//判断应用服务的类型---------------------
			if(this->m_apptype==MPORTTYPE_UNKNOW)
			{
				if(strncmp(pbuf,"GET ",4)==0 || strncmp(pbuf,"POST ",5)==0)
					this->m_apptype=MPORTTYPE_WWW;
			}//判断应用服务的类型---------------------
			if(this->m_apptype==MPORTTYPE_WWW && 
//				(strncmp(pbuf,"GET ",4)==0 || strncmp(pbuf,"POST ",5)==0) &&
			   //此端口映射配置了请求头改写或URL重写
			   (this->m_modReqHeader.size()>0 || this->m_modURLRewriter.size()>0) )
			{
				//判断HTTP响应头是否接收完毕
				const char *ptr=strstr(pbuf,"\r\n\r\n");
				if(ptr==NULL) continue; //未接收完毕，继续接收
				else *(char *)(ptr+2)=0;
				HttpHeader reqHeader(pbuf); //否则接收完毕，解析HTTP响应头			
				std::string requrl; long matchLen=0; //比较长度
				std::map<std::string,std::vector<RegCond> >::iterator it,findit=
					this->m_modReqHeader.end();
				if(reqHeader.GetUrl(requrl)){
				for(it=this->m_modReqHeader.begin();it!=this->m_modReqHeader.end();it++)
				{
					if(strncasecmp(requrl.c_str(),(*it).first.c_str(),
						(*it).first.length())==0 && (*it).first.length()>matchLen)
					{
						findit=it; matchLen=(*it).first.length();
					}
				} } //?if(reqHeader.GetUrl(requrl))
				
				//判断是否要进行URL重写 //yyc add 2010-02-23
				bool bURLRewrite=(this->m_modURLRewriter.size()>0)?
					reqHeader.URLRewrite(this->m_modURLRewriter):false;
				if(bURLRewrite || findit!=this->m_modReqHeader.end())
//				if(findit!=this->m_modReqHeader.end()) //yyc modify 2010-02-23
				{
					buflen-=(ptr+4-pbuf); pbuf=(char *)ptr+4;
					if(fp) ::fwrite("\r\nC ---> S\r\n",1,12,fp);
					if(findit!=this->m_modReqHeader.end()){
						std::vector<RegCond> &regcond=(*findit).second;
						reqHeader.Modify(regcond);
					}   reqHeader.Send(ppeer,fp);
				}//?if(findit!=this->m_modReqHeader.end())
				else *(char *)(ptr+2)='\r'; //恢复，直接转发
			}//?if(this->m_apptype==MPORTTYPE_WWW...)
			
			if(buflen<=0) continue;
///#ifdef PROXYDATA_LOG //是否记录代理数据记录
			if(fp){
				::fwrite("\r\nC ---> S\r\n",1,12,fp);
				::fwrite(pbuf,1,buflen,fp); }
///#endif
			iret=ppeer->Send(buflen,pbuf,-1);
			if(iret<0) break; else buflen=0; //清空待发送数据
		}//?while
		ppeer->Close(); //等待另外的转发线程结束
		while(ppeer->parent()!=NULL) cUtils::usleep(SCHECKTIMEOUT);
	}else RW_LOG_DEBUG(0,"Failed to create transfer-Thread\r\n");
///#ifdef PROXYDATA_LOG //是否记录代理数据记录
	if(fp){
		::fwrite("\r\n***Proxy End*** \r\n",1,20,fp);
		::fclose(fp); }
///#endif
	delete ppeer; return;
}

//转发线程
void mportTCP :: transDataThread(std::pair<socketTCP *,FILE *> *p)
{
	if(p==NULL) return;
	FILE *fp=p->second;
	socketTCP *psock=p->first;	
	if(psock==NULL) return;
	socketTCP *ppeer=(socketTCP *)psock->parent();
	if(ppeer==NULL) return;
	mportTCP *psvr=(mportTCP *)ppeer->parent();
	
	mportTCP *pftpDatasvr=NULL; //ftp服务的数据端口的映射
	char *pbuf,buffer[MAX_TRANSFER_BUFFER+1]; //转发缓冲大小
	long buflen=0; //转发缓冲区中待发送数据大小
	while( psock->status()==SOCKS_CONNECTED )
	{
		int iret=psock->checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ);
		if(iret<0) break; 
		if(iret==0) continue; //没有数据到达
		iret=psock->Receive(buffer+buflen,MAX_TRANSFER_BUFFER-buflen,-1);
		if(iret<0) break; //==0表明接收数据流量超过限制
		if(iret==0){ cUtils::usleep(MAXRATIOTIMEOUT); continue; }
		else{ buflen+=iret; pbuf=buffer; }
		pbuf[buflen]='\0'; //保证为空结束字符串
		//判断应用服务的类型---------------------
		if(psvr->m_apptype==MPORTTYPE_UNKNOW)
		{
			if(strncmp(pbuf,"220 ",4)==0 || strncmp(pbuf,"220-",4)==0)
				psvr->m_apptype=MPORTTYPE_FTP;
			else if(strncmp(pbuf,"HTTP/1.",7)==0)
				psvr->m_apptype=MPORTTYPE_WWW;
			else psvr->m_apptype=MPORTTYPE_TCP;
		}//判断应用服务的类型---------------------
		if(psvr->m_apptype==MPORTTYPE_FTP)
		{//判断是否为FTP服务PASV命令的返回
			if(strncmp(pbuf,"227 ",4)==0){
				if(psvr->AnalysePASV(pftpDatasvr,pbuf,iret,ppeer)) 
					buflen=0; //清空待发送缓冲数据
				else RW_LOG_DEBUG("Failed to Analyse PASV:\r\n\t%s.\r\n",pbuf);
			}//?if(strncmp(buf,"227 ",4)==0)
		}//?else if(psvr->m_apptype==MPORTTYPE_FTP)
		else if(psvr->m_apptype==MPORTTYPE_WWW && psvr->m_modRspHeader.size()>0 && 
			strncmp(pbuf,"HTTP/1.",7)==0 )
		{
			int rspcode=atoi(pbuf+9);
			std::map<int,std::vector<RegCond> >::iterator findit=
				psvr->m_modRspHeader.find(rspcode);
			if(findit!=psvr->m_modRspHeader.end())
			{//判断HTTP响应头是否接收完毕
				const char *ptr=strstr(pbuf,"\r\n\r\n");
				if(ptr==NULL) continue; //未接收完毕，继续接收
				else *(char *)(ptr+2)=0;
				//否则接收完毕，解析HTTP响应头
				HttpHeader rspHeader(pbuf);
				buflen-=(ptr+4-pbuf); pbuf=(char *)ptr+4;
				std::vector<RegCond> &regcond=(*findit).second;
				if(fp) ::fwrite("\r\nS ---> C\r\n",1,12,fp);
				rspHeader.Modify(regcond); rspHeader.Send(ppeer,fp);
			}//?if(psvr->m_modRspHeader.find(rspcode))
		}//?else if(psvr->m_apptype==MPORTTYPE_WWW)

		if(buflen<=0) continue;
///#ifdef PROXYDATA_LOG //是否记录代理数据记录
		if(fp){
			::fwrite("\r\nS ---> C\r\n",1,12,fp);
			::fwrite(pbuf,1,buflen,fp); }
///#endif
		iret=ppeer->Send(buflen,pbuf,-1);
		if(iret<0) break; else buflen=0; //清空待发送数据
	}//while (psock
	ppeer->Close(); delete pftpDatasvr;
	psock->setParent(NULL); //用于onAccept线程判断转发线程是否结束
}

//解析PASV的返回
int parsePASV(char *buf,int len,char* retIP,int &retPort);
bool mportTCP :: AnalysePASV(mportTCP* &pftpDatasvr,char *buf,int len,socketTCP *ppeer)
{
	char pasvbuf[64]; int iport=0;
	int l=parsePASV(buf,len,pasvbuf,iport); //解析PASV返回命令
	if(l<=0) return false;
	if(pftpDatasvr==NULL) //临时映射一个FTP数据传输服务端口
		if( (pftpDatasvr=new mportTCP)==NULL) return false;

	pftpDatasvr->m_appSvr.clear();//重新设置要映射的应用服务
	std::pair<std::string,int> p(pasvbuf,iport);
	pftpDatasvr->m_appSvr.push_back(p);
	pftpDatasvr->m_apptype=MPORTTYPE_TCP;   //服务类型
	pftpDatasvr->m_ssltype=TCPSVR_TCPSVR;
	strcpy(pftpDatasvr->m_bindLocalIP,ppeer->getLocalIP());//绑定指定的IP
	//指定只允许指定的IP地址连接限制
	iprules &ipr=pftpDatasvr->rules();
	ipr.setDefaultEnabled(false); //默认禁止
	sprintf(pasvbuf,"%s 255.255.255.255 0 RULETYPE_TCP true",ppeer->getRemoteIP());
	ipr.addRules(pasvbuf); //添加体格IP过滤规则，仅仅允许当前连接的客户端连接此映射服务
	if(this->m_mportBegin==this->m_mportEnd)
			pftpDatasvr->m_mportBegin=pftpDatasvr->m_mportEnd=0; //随机映射端口
	else{
			pftpDatasvr->m_mportBegin=this->m_mportBegin;
			pftpDatasvr->m_mportEnd=this->m_mportEnd;
	}
	iport=pftpDatasvr->Start(NULL,NULL,NULL,NULL,NULL);
	if(iport<=0) return false; //启动映射失败	
	iport=sprintf(pasvbuf,"227 Entering Passive Mode (%s,%d,%d)\r\n",
				pftpDatasvr->getLocalIP(),(iport&0x0000ff00)>>8,iport&0x000000ff);
	for(int i=20;i<iport;i++) if(pasvbuf[i]=='.') pasvbuf[i]=',';
	ppeer->Send(iport,pasvbuf,-1);
	if((len-l)>0) ppeer->Send(len-l,buf+l,-1);
	return true;
}
//不包含xml头和<mapinfo>标签
//<svrport></svrport>
//<starttime></starttime>
//<connected></connected>
//<appip></appip>
//<appport></appport>
//<appdesc></appdesc>
//<mapport></mapport>
//<bindip></bindip>
//<localip></localip>
//<ssltype></ssltype>
//<sslverify></sslverify>
//<maxconn></maxconn>
//<maxratio></maxratio>
//<blogd></blogd>
void mportTCP :: xml_info_mtcp(cBuffer &buffer)
{
	//一次分配足够空间
	if(buffer.Space()<512) buffer.Resize(buffer.size()+512);
	if(buffer.str()==NULL) return;
	if(this->status()==SOCKS_LISTEN)
	{
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<svrport>%d</svrport><ifssl>%d</ifssl><ifsslv>%d</ifsslv>",
			this->getLocalPort(),((this->ifSSL())?1:0),((this->ifSSLVerify())?1:0) );
	}else buffer.len()+=sprintf(buffer.str()+buffer.len(),"<svrport>0</svrport>");
	
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<connected>%d</connected>",this->curConnection());
	time_t t=m_tmOpened; struct tm * ltime=localtime(&t);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<starttime>%04d年%02d月%02d日 %02d:%02d:%02d</starttime>",
		(1900+ltime->tm_year), ltime->tm_mon+1, ltime->tm_mday,ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
	
	std::string appsvr;
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<appsvr>");
	if(m_appSvr.size()>0){
		int oldLen=buffer.len();
		for(int i=0;i<m_appSvr.size()-1;i++)
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"%s:%d,",m_appSvr[i].first.c_str(),m_appSvr[i].second);
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"%s:%d",m_appSvr[i].first.c_str(),m_appSvr[i].second);
		appsvr.assign(buffer.str()+oldLen);
	}//?if(m_appSvr.size()>0)
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"</appsvr>");
	if(m_appSvr.size()==1)
	{
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<appip>%s</appip>",m_appSvr[0].first.c_str());
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<appport>%d</appport>",m_appSvr[0].second);
	}else if(m_appSvr.size()>1)
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<appip>%s</appip><appport></appport>",appsvr.c_str());
	else
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<appip></appip><appport></appport>");
	
	if(m_apptype==MPORTTYPE_FTP)
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<apptype>FTP</apptype>");
	else if(m_apptype==MPORTTYPE_WWW)
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<apptype>WWW</apptype>");
	else if(m_apptype==MPORTTYPE_TCP)
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<apptype>TCP</apptype>");
	else buffer.len()+=sprintf(buffer.str()+buffer.len(),"<apptype>UNK</apptype>");
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<appdesc>%s</appdesc>",m_strSvrname.c_str());

	if(m_mportBegin==m_mportEnd)
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<mapport>%d</mapport>",m_mportBegin);
	else buffer.len()+=sprintf(buffer.str()+buffer.len(),"<mapport>%d-%d</mapport>",m_mportBegin,m_mportEnd);
	
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<bindip>%s</bindip>",m_bindLocalIP);
	if(m_ssltype==TCPSVR_SSLSVR)
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<ssltype>+ssl</ssltype>");
	else if(m_ssltype==SSLSVR_TCPSVR)
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<ssltype>-ssl</ssltype>");
	else buffer.len()+=sprintf(buffer.str()+buffer.len(),"<ssltype></ssltype>");
	if(m_bSSLVerify) buffer.len()+=sprintf(buffer.str()+buffer.len(),"<sslverify>1</sslverify>");
    //限制最大连接数
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<maxconn>%d</maxconn>",this->maxConnection());
	//限制最大带宽 kb/s
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<maxratio>%d</maxratio>",m_maxratio);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<blogd>%c</blogd>",(m_bLogdatafile)?'*':' ');
	return;
}

int mportTCP :: str_info_mtcp(const char *mapname,char *buf)
{
	int len=sprintf(buf,"mtcpl name=%s ",mapname);;
	if(m_appSvr.size()>0){
		len+=sprintf(buf+len,"appsvr=%s:%d",m_appSvr[0].first.c_str(),m_appSvr[0].second);
		for(int i=1;i<m_appSvr.size();i++)
			len+=sprintf(buf+len,",%s:%d",m_appSvr[i].first.c_str(),m_appSvr[i].second);
	}else len+=sprintf(buf+len,"appsvr=");
	
	if(m_mportBegin==m_mportEnd)
		len+=sprintf(buf+len," mport=%d",m_mportBegin);
	else len+=sprintf(buf+len," mport=%d-%d",m_mportBegin,m_mportEnd);
	if(m_ssltype==TCPSVR_SSLSVR)
		len+=sprintf(buf+len,"+ssl");
	else if(m_ssltype==SSLSVR_TCPSVR)
		len+=sprintf(buf+len,"-ssl");
	
	if(m_bSSLVerify) len+=sprintf(buf+len," sslverify=1");

	len+=sprintf(buf+len," bindip=%s",m_bindLocalIP);
	if(m_apptype==MPORTTYPE_FTP)
		len+=sprintf(buf+len," apptype=FTP");
	else if(m_apptype==MPORTTYPE_WWW)
		len+=sprintf(buf+len," apptype=WWW");
	else if(m_apptype==MPORTTYPE_TCP)
		len+=sprintf(buf+len," apptype=TCP");
	else 
		len+=sprintf(buf+len," apptype=UNK");

	//限制最大连接数
	len+=sprintf(buf+len," maxconn=%d",this->maxConnection());
	len+=sprintf(buf+len," maxratio=%d",m_maxratio);//限制最大带宽 kb/s
	len+=sprintf(buf+len," blogd=%d", (m_bLogdatafile)?1:0);

	len+=sprintf(buf+len," appdesc=\"%s\"",m_strSvrname.c_str());
	len+=sprintf(buf+len,"\r\n");
	return len;
}

//解析PASV的返回
int parsePASV(char *buf,int len,char* retIP,int &retPort)
{
	char *ptr1,*ptr2,*ptr=strchr(buf,'\r');
	if(ptr){
		*ptr='\0';
		if( (ptr1=strchr(buf,'(')) )
		{
			ptr1++;
			if( (ptr2=strchr(ptr1,')')) )
			{
				*ptr2='\0';
				int iret,i1,i2,i3,i4,i5,i6;
				iret=::sscanf(ptr1,"%d,%d,%d,%d,%d,%d",&i1,&i2,&i3,&i4,&i5,&i6);
				if(iret==6){
					sprintf(retIP,"%d.%d.%d.%d",i1,i2,i3,i4);
					retPort=i5*256+i6;
					return (*(ptr+1)=='\n')?(ptr-buf+2):(ptr-buf+1);
				}else *ptr2=')';
			}//?if( (ptr2=strchr(ptr1,')')) )
		}//?if( (ptr1=strchr(buf,'(')) )
		*ptr='\r';
	}//?if(ptr)
	return 0;
}

bool mportTCP :: addRegCond(int rspcode,const char *header,const char *pattern,const char *replto)
{
	if(header==NULL|| header[0]==0) return false;
	std::map<int,std::vector<RegCond> >::iterator it_rsp=m_modRspHeader.find(rspcode);
	if(it_rsp==m_modRspHeader.end())
	{
		std::vector<RegCond> regcond;
		RegCond rd; rd.strHeader.assign(header);
		if(pattern) rd.strPattern.assign(pattern);
		if(replto) rd.strReplto.assign(replto);
		regcond.push_back(rd);
		m_modRspHeader[rspcode]=regcond;
	}else{
		std::vector<RegCond> &regcond=(*it_rsp).second;
		int i=0;
		for(;i<regcond.size();i++) //查找此header是否已经存在
		{
			if(strcasecmp(regcond[i].strHeader.c_str(),header)==0) break;
		}//?for(int i=0;
		if(i<regcond.size()){
			if(pattern) regcond[i].strPattern.assign(pattern);
			else regcond[i].strPattern="";
			if(replto) regcond[i].strReplto.assign(replto);
			else regcond[i].strReplto="";
		}else{
			RegCond rd; rd.strHeader.assign(header);
			if(pattern) rd.strPattern.assign(pattern);
			if(replto) rd.strReplto.assign(replto);
			regcond.push_back(rd);
		}//?if(i<regcond.size())...else...
	}//?if(it_rsp==m_modRspHeader.end())...else
	return true;
}
bool mportTCP :: addRegCond(const char *url,const char *header,const char *pattern,const char *replto)
{
	//yyc add 2010-02-23 begin============================
	if(url && strcmp(url,"URLRewrite")==0){ //URL重写
		if(pattern==NULL || pattern[0]==0) return false;
		if(replto==NULL || replto[0]==0) return false;
		m_modURLRewriter[pattern]=std::string(replto);
		return true;
	}//?if(strcmp(header,...
	//yyc add 2010-02-23  end ============================
	if(header==NULL|| header[0]==0) return false;
	if(url==NULL || url[0]!='/') return false;
	std::map<std::string,std::vector<RegCond> >::iterator it_req=m_modReqHeader.find(url);
	if(it_req==m_modReqHeader.end())
	{
		std::vector<RegCond> regcond;
		RegCond rd; rd.strHeader.assign(header);
		if(pattern) rd.strPattern.assign(pattern);
		if(replto) rd.strReplto.assign(replto);
		regcond.push_back(rd);
		m_modReqHeader[url]=regcond;
	}else{
		std::vector<RegCond> &regcond=(*it_req).second;
		int i=0;
		for(;i<regcond.size();i++) //查找此header是否已经存在
		{
			if(strcasecmp(regcond[i].strHeader.c_str(),header)==0) break;
		}//?for(int i=0;
		if(i<regcond.size()){
			if(pattern) regcond[i].strPattern.assign(pattern);
			else regcond[i].strPattern="";
			if(replto) regcond[i].strReplto.assign(replto);
			else regcond[i].strReplto="";
		}else{
			RegCond rd; rd.strHeader.assign(header);
			if(pattern) rd.strPattern.assign(pattern);
			if(replto) rd.strReplto.assign(replto);
			regcond.push_back(rd);
		}//?if(i<regcond.size())...else...
	}//?if(it_rsp==m_modRspHeader.end())...else
	return true;
}
bool mportTCP :: str_info_regcond(const char *mapname,std::string &strini)
{
	if(mapname==NULL || mapname[0]==0) return false;
	char buf[1024]; int buflen=0;
	std::map<int,std::vector<RegCond> >::iterator it_rsp;
	for(it_rsp=m_modRspHeader.begin();it_rsp!=m_modRspHeader.end();it_rsp++)
	{
		std::vector<RegCond> &regcond=(*it_rsp).second;
		for(int i=0;i<regcond.size();i++)
		{
			buflen=sprintf(buf,"mdhrsp name=%s cond=%d header=\"%s\" pattern=\"%s\" replto=\"%s\"\r\n",
				mapname,(*it_rsp).first,regcond[i].strHeader.c_str(),
				regcond[i].strPattern.c_str(),regcond[i].strReplto.c_str());
			strini.append(buf,buflen);
		}//?for(int i=0;
	}//?for(it_rsp
	std::map<std::string,std::vector<RegCond> >::iterator it_req;
	for(it_req=m_modReqHeader.begin();it_req!=m_modReqHeader.end();it_req++)
	{
		std::vector<RegCond> &regcond=(*it_req).second;
		for(int i=0;i<regcond.size();i++)
		{
			buflen=sprintf(buf,"mdhreq name=%s cond=\"%s\" header=\"%s\" pattern=\"%s\" replto=\"%s\"\r\n",
				mapname,(*it_req).first.c_str(),regcond[i].strHeader.c_str(),
				regcond[i].strPattern.c_str(),regcond[i].strReplto.c_str());
			strini.append(buf,buflen);
		}//?for(int i=0;
	}//?for(it_rsp
	//yyc add 2010-02-23 begin============================
	std::map<std::string,std::string>::iterator it_url;
	for(it_url=m_modURLRewriter.begin();it_url!=m_modURLRewriter.end();it_url++)
	{
		buflen=sprintf(buf,"mdhreq name=%s cond=\"URLRewrite\" pattern=\"%s\" replto=\"%s\"\r\n",
				mapname,(*it_url).first.c_str(),(*it_url).second.c_str());
		strini.append(buf,buflen);
	}//?for(it_url
	//yyc add 2010-02-23  end ============================
	return true;
}

//================================HttpHeader=====================================
HttpHeader :: HttpHeader(const char *httpHead)
{
	if(httpHead==NULL) return;
	const char *ptrBegin=httpHead;
	const char *ptrEnd=strchr(ptrBegin,'\r');
	if(ptrEnd){ //先保存首行信息
		m_strFirstLine.assign(ptrBegin,ptrEnd-ptrBegin);
		ptrBegin=ptrEnd+2;
		while( (ptrEnd=strchr(ptrBegin,'\r')) )
		{
			*(char *)ptrEnd=0;
			const char *ptr1, *ptr=strchr(ptrBegin,':');
			if(ptr){
				*(char *)ptr=0; ptr1=ptr+1;
				while(*ptr1==' ') ptr1++; //去掉空格
				if(ptrBegin[0]!=0 && ptr1[0]!=0)
					m_header[ptrBegin]=string(ptr1);
				*(char *)ptr=':';
			}//?if(ptr)
			*(char *)ptrEnd='\r';
			ptrBegin=ptrEnd+2;
		}//?while
	}else m_strFirstLine.assign(httpHead);
}

bool HttpHeader :: GetUrl(std::string &strurl)
{
	if( strncmp(m_strFirstLine.c_str(),"GET ",4)==0 || 
		strncmp(m_strFirstLine.c_str(),"POST ",5)==0)
	{
		const char *ptrbegin=m_strFirstLine.c_str()+4;
		while(*ptrbegin==' ') ptrbegin++; //去掉前导空格
		const char *ptrend=strchr(ptrbegin,' ');
		if(ptrend==NULL) return false;
		strurl.assign(ptrbegin,ptrend-ptrbegin);
		return true;
	}
	return false;
}
long HttpHeader :: GetRspCode()
{
	return (strncmp(m_strFirstLine.c_str(),"HTTP/1.",7)==0)?
		atoi(m_strFirstLine.c_str()+9):0;
}

#define HttpHeader_SendBuffer 2048
bool HttpHeader :: Send(socketTCP *psock,FILE *fp)
{
//	if(psock==NULL) return false;
	char buf[HttpHeader_SendBuffer];
	long buflen=sprintf(buf,"%s\r\n",m_strFirstLine.c_str());
	std::map<std::string,std::string>::iterator it=m_header.begin();
	for(;it!=m_header.end();it++)
	{
		long l=(*it).first.length()+(*it).second.length(); l+=16;
		if(l>(HttpHeader_SendBuffer-buflen))
		{
			psock->Send(buflen,buf,-1);
			if(fp) ::fwrite(buf,1,buflen,fp);
			buflen=0; 
		}
		buflen+=sprintf(buf+buflen,"%s: %s\r\n",(*it).first.c_str(),(*it).second.c_str());
	}
	buflen+=sprintf(buf+buflen,"\r\n");
	psock->Send(buflen,buf,-1);
	if(fp) ::fwrite(buf,1,buflen,fp);
	return true;
}

void HttpHeader :: Modify(std::vector<RegCond> &regcond)
{
	for(int i=0;i<regcond.size();i++)
	{
		std::map<std::string,std::string>::iterator findit;
		for(findit=m_header.begin();findit!=m_header.end();findit++)
		{
			if(strcasecmp(regcond[i].strHeader.c_str(),(*findit).first.c_str())==0)
				break;
		}//?for(findit
		if(findit==m_header.end()) continue;
		if(regcond[i].strReplto=="") m_header.erase(findit);
		else if(regcond[i].strPattern=="") (*findit).second=regcond[i].strReplto;
		else{
			RW_LOG_DEBUG("Replace HTTP Header: %s\r\n",(*findit).first.c_str());
			RW_LOG_DEBUG("  Old Value: %s\r\n",(*findit).second.c_str());
			regexp reg(regcond[i].strPattern.c_str());
			char *ptrNew=reg.Replace((*findit).second.c_str(),regcond[i].strReplto.c_str());
			if(ptrNew) (*findit).second.assign(ptrNew);
			regexp::ReleaseString(ptrNew);
			RW_LOG_DEBUG("  New Value: %s\r\n",(*findit).second.c_str());
		}
	}//?for(int i=0;
}
//判断是否要进行URL重写 //yyc add 2010-02-23
bool HttpHeader :: URLRewrite(std::map<std::string,std::string> &URLRewriter)
{
	const char *ptrbegin=m_strFirstLine.c_str();
	if( strncmp(m_strFirstLine.c_str(),"GET ",4)==0 ) ptrbegin+=4;
	else if( strncmp(m_strFirstLine.c_str(),"POST ",5)==0 ) ptrbegin+=5;
	else return false;
	while(*ptrbegin==' ') ptrbegin++; //去掉前导空格
	const char *ptrend=strchr(ptrbegin,'?'); char c='?';
	if(ptrend==NULL){ ptrend=strchr(ptrbegin,' '); c=' '; }
	if(ptrend==NULL) return false; *(char *)ptrend=0;

	std::map<std::string,std::string >::iterator it;
	for(it=URLRewriter.begin();it!=URLRewriter.end();it++)
	{//判断是否匹配URL重写规则
		regexp reg((*it).first.c_str()); //URL匹配正则表达式
		MatchResult rs=reg.Match(ptrbegin);
		if(rs.IsMatched()!=0) break; //匹配成功
	}//?for(itURLRewriter.begin()...
	if(it==URLRewriter.end()){ *(char *)ptrend=c;  return false; } 

	//配置了针对此url的重写，按照规则进行替换
	RW_LOG_DEBUG("URLRewriter: %s Match %s\r\n",ptrbegin,(*it).first.c_str());
	regexp reg((*it).first.c_str());
	char *ptrNew=reg.Replace(ptrbegin,(*it).second.c_str());
	RW_LOG_DEBUG("URLRewriter: %s ----> %s\r\n",ptrbegin,ptrNew);
	*(char *)ptrend=c; //恢复替换字符
	int p0=ptrbegin-m_strFirstLine.c_str();
	m_strFirstLine.replace(p0,(ptrend-ptrbegin),ptrNew);
	if(ptrNew) regexp::ReleaseString(ptrNew); 
	return true; //对此URL进行了重写操作
}
