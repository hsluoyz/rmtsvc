/*******************************************************************
   *	ftpserver.h 
   *    DESCRIPTION: FTP服务
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *	
   *******************************************************************/

#include "rmtsvc.h" 

ftpsvrEx :: ftpsvrEx()
{
#ifdef _SURPPORT_OPENSSL_
		setCacert(NULL,NULL,NULL,true); //默认加载内置的证书
#endif
	initSetting();
}

void ftpsvrEx ::initSetting()
{
	m_settings.ifSSLsvr=false;
	m_settings.autoStart=false;
	m_settings.svrport=FTP_SERVER_PORT;
	m_settings.dataportB=0;
	m_settings.dataportE=0;
	m_settings.maxUsers=0;
	m_settings.logEvent=0;
	m_settings.bindip="";
	m_settings.tips="";
}

//启动服务
bool ftpsvrEx :: Start() 
{
	if(this->status()==SOCKS_LISTEN) return true;
	//配置ftp服务的参数和帐号----start------------
#ifdef _SURPPORT_OPENSSL_
	if(m_settings.ifSSLsvr) 
		this->initSSL(true,NULL); //SSL加密FTP服务
#endif
	this->setHelloTip(m_settings.tips.c_str());
	this->setDataPort(m_settings.dataportB,m_settings.dataportE);
	this->maxConnection(m_settings.maxUsers); //最大连接用户数. 0:不限
	this->delAccount((const char *)-1); //清空所有帐号信息
	//配置ftp访问帐号信息
	std::map<std::string,TFTPUser>::iterator it=m_userlist.begin();
	for(;it!=m_userlist.end();it++)
	{
		TFTPUser &ftpuser=(*it).second;
		if(ftpuser.forbid==0) modiUser(ftpuser);
	}
	//配置ftp服务的参数和帐号---- end ------------
	const char *ip=(m_settings.bindip=="")?NULL:m_settings.bindip.c_str();
	BOOL bReuseAddr=(ip)?SO_REUSEADDR:FALSE;//绑定了IP则允许端口重用
	SOCKSRESULT sr=Listen( m_settings.svrport ,bReuseAddr,ip);

	return (sr>0)?true:false;
}

void ftpsvrEx :: Stop()
{ 
	Close();
#ifdef _SURPPORT_OPENSSL_
	freeSSL();
#endif
	return;
}
//删除指定的用户,返回0成功
//返回1无效的帐号,返回2删除失败
int ftpsvrEx::deleUser(const char *ptr_user)
{
	std::map<std::string,TFTPUser>::iterator it=m_userlist.end();
	if(ptr_user) it=m_userlist.find(ptr_user);
	if(it==m_userlist.end()) return 1;
	if(!this->delAccount(ptr_user)) return 2;
	m_userlist.erase(it);
	return 0;
}
//添加/修改用户
bool ftpsvrEx::modiUser(TFTPUser &ftpuser)
{
	FTPACCOUNT *ptr_ftpaccount=this->getAccount(ftpuser.username.c_str());
	if(ptr_ftpaccount==NULL) ptr_ftpaccount=this->newAccount(ftpuser.username.c_str());
	if(ptr_ftpaccount==NULL) return false;
	ptr_ftpaccount->m_userpwd=ftpuser.userpwd;
	ptr_ftpaccount->m_username_root="";
	ptr_ftpaccount->m_maxupratio=ftpuser.maxupratio;
	ptr_ftpaccount->m_maxdwratio=ftpuser.maxdwratio;
	ptr_ftpaccount->m_maxupfilesize=ftpuser.maxupfilesize;
	ptr_ftpaccount->m_maxdisksize=ftpuser.maxdisksize;
	ptr_ftpaccount->m_curdisksize=ftpuser.curdisksize;
	ptr_ftpaccount->m_maxLoginusers=ftpuser.maxLoginusers;
	ptr_ftpaccount->m_limitedTime=ftpuser.limitedTime;
	ptr_ftpaccount->m_bitQX=0;
	ptr_ftpaccount->lPswdMode(ftpuser.pswdmode);
	ptr_ftpaccount->bDsphidefiles((ftpuser.disphidden!=0)?true:false);
	ptr_ftpaccount->m_ipRules.addRules(NULL); //清空所有过滤规则
	ptr_ftpaccount->m_ipRules.addRules_new(RULETYPE_TCP,ftpuser.ipaccess,ftpuser.ipRules.c_str());

	if(ftpuser.forbid!=0)
	{
		ptr_ftpaccount->m_ipRules.addRules(NULL);
		ptr_ftpaccount->m_ipRules.setDefaultEnabled(false);
	}
	ptr_ftpaccount->m_dirAccess.clear();
	std::map<std::string,std::pair<std::string,long> >::iterator it=ftpuser.dirAccess.begin();
	std::string vname,vpath; long qx=0;
	for(;it!=ftpuser.dirAccess.end();it++)
	{
		vname=(*it).first;
		vpath=(*it).second.first;
		qx=(*it).second.second;
		if(vname[0]!='/') vname.insert(0,"/");
		if(vname[vname.length()-1]!='/') vname.append("/");
		if(vpath!="" && vpath[vpath.length()-1]!='\\')
			vpath.append("\\");
		std::pair<std::string,long> p(vpath,qx);
		ptr_ftpaccount->m_dirAccess[vname]=p;
	}
	return true;
}

void ftpsvrEx::onLogEvent(long eventID,cFtpSession &session)
{
	if((m_settings.logEvent & eventID)==0 || !RW_LOG_CHECK(LOGLEVEL_INFO) ) return;	
	if(eventID==FTP_LOGEVENT_LOGIN ) //有用户登录
	{
		RW_LOG_PRINTTIME(); //打印当前时间
		RW_LOG_PRINT(LOGLEVEL_INFO,"[login]  ip=%s - %s login.\r\n",
			session.m_pcmdsock->getRemoteIP(),
			session.m_paccount->m_username.c_str());
	}
	else if(eventID==FTP_LOGEVENT_LOGOUT) //有用户退出
	{
		RW_LOG_PRINTTIME(); //打印当前时间
		RW_LOG_PRINT(LOGLEVEL_INFO,"[logout] ip=%s - %s logout.\r\n",
			session.m_pcmdsock->getRemoteIP(),
			session.m_paccount->m_username.c_str());
	}
	else if(eventID==FTP_LOGEVENT_UPLOAD) //有用户上载文件
	{
		RW_LOG_PRINTTIME(); //打印当前时间
		RW_LOG_PRINT(LOGLEVEL_INFO,"[upload] ip=%s - %s upload %s.\r\n",
			session.m_pcmdsock->getRemoteIP(),
			session.m_paccount->m_username.c_str(),
			session.m_filename.c_str());
	}
	else if(eventID==FTP_LOGEVENT_DWLOAD) //有用户下载文件
	{
		RW_LOG_PRINTTIME(); //打印当前时间
		RW_LOG_PRINT(LOGLEVEL_INFO,"[dwload] ip=%s - %s download %s.\r\n",
			session.m_pcmdsock->getRemoteIP(),
			session.m_paccount->m_username.c_str(),
			session.m_filename.c_str());
	}
	else if(eventID==FTP_LOGEVENT_DELETE) //有用户删除文件
	{
		RW_LOG_PRINTTIME(); //打印当前时间
		RW_LOG_PRINT(LOGLEVEL_INFO,"[delete] ip=%s - %s delete %s.\r\n",
			session.m_pcmdsock->getRemoteIP(),
			session.m_paccount->m_username.c_str(),
			session.m_filename.c_str());
	}
	else if(eventID==FTP_LOGEVENT_RMD) //有用户删除目录
	{
		RW_LOG_PRINTTIME(); //打印当前时间
		RW_LOG_PRINT(LOGLEVEL_INFO,"[deldir] ip=%s - %s delete %s.\r\n",
			session.m_pcmdsock->getRemoteIP(),
			session.m_paccount->m_username.c_str(),
			session.m_filename.c_str());
	}
	else if(eventID==FTP_LOGEVENT_SITE) //有用户执行SITE命令
	{
		RW_LOG_PRINTTIME(); //打印当前时间
		RW_LOG_PRINT(LOGLEVEL_INFO,"[ SITE ] ip=%s - %s SITE %s,result=%d\r\n",
			session.m_pcmdsock->getRemoteIP(),
			session.m_paccount->m_username.c_str(),
			session.m_filename.c_str(),session.m_startPoint);
	}
	return;
}

//-------------------------------------------------------------
bool webServer::httprsp_ftpsets(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp)
{
	MyService *ptrService=MyService::GetService();
	ftpsvrEx *pftpsvr=&ptrService->m_ftpsvr;
	const char *ptr_cmd=httpreq.Request("cmd");
	cBuffer buffer(512);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<?xml version=\"1.0\" encoding=\"gb2312\" ?><xmlroot>");
	
	if(strcasecmp(ptr_cmd,"run")==0) //运行ftp服务
		pftpsvr->Start();
	else if(strcasecmp(ptr_cmd,"stop")==0)
		pftpsvr->Stop();
	else if(strcasecmp(ptr_cmd,"setting")==0) 
	{//设置FTP服务参数
		const char *ptr=httpreq.Request("svrport");
		if(ptr){ 
			pftpsvr->m_settings.svrport=atoi(ptr);
			if(pftpsvr->m_settings.svrport<=0) pftpsvr->m_settings.svrport=0;
		}
		ptr=httpreq.Request("bindip");
		if(ptr) pftpsvr->m_settings.bindip.assign(ptr);
		ptr=httpreq.Request("svrtype");
		if(ptr) pftpsvr->m_settings.ifSSLsvr=(atoi(ptr)==1)?true:false;
		ptr=httpreq.Request("autorun");
		if(ptr) pftpsvr->m_settings.autoStart=(atoi(ptr)==1)?true:false;
		ptr=httpreq.Request("maxconn");
		if(ptr){
			pftpsvr->m_settings.maxUsers=atoi(ptr);
			if(pftpsvr->m_settings.maxUsers<=0) pftpsvr->m_settings.maxUsers=0;
			pftpsvr->maxConnection(pftpsvr->m_settings.maxUsers);
		}
		ptr=httpreq.Request("dataport");
		if(ptr)
		{
			const char *p=strchr(ptr,'-');
			if(p){
				*(char *)p=0; p++;
				pftpsvr->m_settings.dataportB=atoi(ptr);
				if(pftpsvr->m_settings.dataportB<=0) pftpsvr->m_settings.dataportB=0;
				pftpsvr->m_settings.dataportE=atoi(p);
				if(pftpsvr->m_settings.dataportE<=0) pftpsvr->m_settings.dataportE=0;
			}else{
				pftpsvr->m_settings.dataportB=atoi(ptr);
				if(pftpsvr->m_settings.dataportB<=0) pftpsvr->m_settings.dataportB=0;
				pftpsvr->m_settings.dataportE=pftpsvr->m_settings.dataportB;
			}
			pftpsvr->setDataPort(pftpsvr->m_settings.dataportB,pftpsvr->m_settings.dataportE);
		}//?if(ptr) //dataport
		ptr=httpreq.Request("logging");
		if(ptr) pftpsvr->m_settings.logEvent=atoi(ptr);
		ptr=httpreq.Request("tips");
		if(ptr){
			pftpsvr->m_settings.tips.assign(ptr);
			pftpsvr->setHelloTip(pftpsvr->m_settings.tips.c_str());
		}
		pftpsvr->saveIni();
	}//?else if(strcasecmp(ptr_cmd,"setting")==0) 

	//获取FTP服务的参数设置和状态----start---------------------------------------------
	struct tm * ltime=NULL; time_t t;
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<ftp_status>");
	if(pftpsvr->status()==SOCKS_LISTEN) //服务已运行
	{
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<status>%d</status>",pftpsvr->getLocalPort());
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<ifssl>%d</ifssl>",pftpsvr->ifSSL());
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<connected>%d</connected>",pftpsvr->curConnection());
		t=pftpsvr->getStartTime(); ltime=localtime(&t);
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<starttime>%04d年%02d月%02d日 %02d:%02d:%02d</starttime>",
			(1900+ltime->tm_year), ltime->tm_mon+1, ltime->tm_mday,ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
	}
	else
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<status>0</status>");
	t=time(NULL); ltime=localtime(&t);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<curtime>%04d年%02d月%02d日 %02d:%02d:%02d</curtime>",
			(1900+ltime->tm_year), ltime->tm_mon+1, ltime->tm_mday,ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
	
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<svrport>%d</svrport>",pftpsvr->m_settings.svrport);
	//FTP服务类型  m_ifSSLsvr
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<svrtype>%d</svrtype>",pftpsvr->m_settings.ifSSLsvr);
	//FTP服务是否自动启动
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<autorun>%d</autorun>",pftpsvr->m_settings.autoStart);
	//日志记录
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<logging>%d</logging>",pftpsvr->m_settings.logEvent);
	//允许最大连接用户数
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<maxconn>%d</maxconn>",pftpsvr->m_settings.maxUsers);
	//PASV m_dataport_start
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<dataport>%d-%d</dataport>",
							pftpsvr->m_settings.dataportB,pftpsvr->m_settings.dataportE);
	//绑定本机IP
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<bindip>%s</bindip><localip>",pftpsvr->m_settings.bindip.c_str());
	std::vector<std::string> vec;//得到本机IP，返回得到本机IP的个数
	long n=socketBase::getLocalHostIP(vec);
	for(int i=0;i<n;i++) buffer.len()+=sprintf(buffer.str()+buffer.len(),"%s ",vec[i].c_str());
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"</localip>");
	//欢迎词信息
	if(buffer.Space()<pftpsvr->m_settings.tips.length()) buffer.Resize(buffer.size()+pftpsvr->m_settings.tips.length());
	if(buffer.str())
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<tips><![CDATA[%s]]></tips>",pftpsvr->m_settings.tips.c_str());
	//获取FTP服务的参数设置和状态---- end ---------------------------------------------
	
	if(buffer.Space()<32) buffer.Resize(buffer.size()+32);
	if(buffer.str())
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"</ftp_status></xmlroot>");
	
	httprsp.NoCache();//CacheControl("No-cache");
	//设置MIME类型，默认为HTML
	httprsp.set_mimetype(MIMETYPE_XML);
	//设置响应内容长度
	httprsp.lContentLength(buffer.len());
	httprsp.send_rspH(psock,200,"OK");
	
	if(buffer.str()) psock->Send(buffer.len(),buffer.str(),-1);
	return true;
}
//返回用户列表XML
void listuser(cBuffer &buffer,std::map<std::string,TFTPUser> &userlist)
{
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<userlist>");
	std::map<std::string,TFTPUser>::iterator it=userlist.begin();
	for(;it!=userlist.end();it++)
	{
		if(buffer.Space()<((*it).first.length()+32)) buffer.Resize(buffer.size()+((*it).first.length()+32));
		if(buffer.str())
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<username>%s</username>",(*it).first.c_str());
	}//?for 
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"</userlist>");
	return;
}
bool webServer::httprsp_ftpusers(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp)
{
	MyService *ptrService=MyService::GetService();
	ftpsvrEx *pftpsvr=&ptrService->m_ftpsvr;
	const char *ptr_cmd=httpreq.Request("cmd");
	cBuffer buffer(1024);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<?xml version=\"1.0\" encoding=\"gb2312\" ?><xmlroot>");
	
	if(strcasecmp(ptr_cmd,"list")==0) //列出所有FTP帐号
		listuser(buffer,pftpsvr->m_userlist);
	else if(strcasecmp(ptr_cmd,"dele")==0)
	{
		const char *ptr_user=httpreq.Request("user");
		int iret=pftpsvr->deleUser(ptr_user);
		if(iret==1)
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>无效的帐号!</retmsg>");
		else if(iret==2)
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>暂时无法删除帐号%s!</retmsg>",ptr_user);
		else{
			listuser(buffer,pftpsvr->m_userlist);
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>删除帐号成功!</retmsg>");
		}
	}//?else if(strcasecmp(ptr_cmd,"dele")==0)
	else if(strcasecmp(ptr_cmd,"save")==0) //添加或修改帐号
	{
		const char *ptr,*ptr_user=httpreq.Request("account");
		if(ptr_user && ptr_user[0]!=0)
		{
			std::map<std::string,TFTPUser>::iterator it=pftpsvr->m_userlist.end();
			if(ptr_user) it=pftpsvr->m_userlist.find(ptr_user);
			TFTPUser *ptr_ftpuser=NULL;
			if(it!=pftpsvr->m_userlist.end()) //修改指定帐号的信息
				ptr_ftpuser=&(*it).second;
			else{ //添加一个帐号
				TFTPUser ftpuser; 
				ftpuser.username.assign(ptr_user);
				ftpuser.ipaccess =0;
				ftpuser.curdisksize =0;
				ftpuser.limitedTime =0;
				ftpuser.maxdisksize =0;
				ftpuser.maxdwratio =0;
				ftpuser.maxLoginusers =0;
				ftpuser.maxupfilesize =0;
				ftpuser.maxupratio =0;
				ftpuser.pswdmode=0;
				ftpuser.disphidden=0;
				ftpuser.forbid=0;
				pftpsvr->m_userlist[ptr_user]=ftpuser;
				ptr_ftpuser=&pftpsvr->m_userlist[ptr_user];
			}
			ptr=httpreq.Request("pswd");
			if(ptr) ptr_ftpuser->userpwd.assign(ptr);
			ptr=httpreq.Request("pswdmode");
			if(ptr) ptr_ftpuser->pswdmode=atoi(ptr);
			ptr=httpreq.Request("desc");
			if(ptr) ptr_ftpuser->userdesc.assign(ptr);
			ptr=httpreq.Request("expired");
			if(ptr){
				if(ptr[0]==0) 
					ptr_ftpuser->limitedTime=0;
				else{
					struct tm ltm; ::memset((void *)&ltm,0,sizeof(ltm));
					::sscanf(ptr,"%d-%d-%d",&ltm.tm_year,&ltm.tm_mon,&ltm.tm_mday);
					ltm.tm_year-=1900; //年份从1900开始计数
					ltm.tm_mon-=1;//月份从0开始计数
					if(ltm.tm_year>100 && ltm.tm_year<200 && 
							ltm.tm_mon>=0 && ltm.tm_mon<=11 && 
							ltm.tm_mday>=1 && ltm.tm_mday<=31 )
					ptr_ftpuser->limitedTime= mktime(&ltm);
				}
			}//?ptr=httpreq.Request("expired");
			ptr=httpreq.Request("maxsignin");
			if(ptr) ptr_ftpuser->maxLoginusers=atoi(ptr);
			ptr=httpreq.Request("maxup");
			if(ptr) ptr_ftpuser->maxupratio=atoi(ptr);
			ptr=httpreq.Request("maxupfile");
			if(ptr) ptr_ftpuser->maxupfilesize=atoi(ptr);
			ptr=httpreq.Request("maxdw");
			if(ptr) ptr_ftpuser->maxdwratio=atoi(ptr);
			ptr=httpreq.Request("maxdisk");
			if(ptr) ptr_ftpuser->maxdisksize=atoi(ptr)*1000;
			ptr=httpreq.Request("usedisk");
			if(ptr) ptr_ftpuser->curdisksize=atoi(ptr)*1000;
			ptr=httpreq.Request("disphidden");
			if(ptr) ptr_ftpuser->disphidden=atoi(ptr);
			ptr=httpreq.Request("forbid");
			if(ptr) ptr_ftpuser->forbid=atoi(ptr);
			ptr=httpreq.Request("ipaccess");
			if(ptr) ptr_ftpuser->ipaccess=atoi(ptr);
			ptr=httpreq.Request("ipaddr");
			if(ptr) ptr_ftpuser->ipRules.assign(ptr);
			ptr_ftpuser->dirAccess.clear();
			ptr=httpreq.Request("rootqx");
			int rootqx=(ptr)?atoi(ptr):0;
			ptr=httpreq.Request("rootdir");
			if(ptr){
				std::pair<std::string,long> p(ptr,rootqx);
				ptr_ftpuser->dirAccess["/"]=p;
			}
			ptr=httpreq.Request("vpath");
			if(ptr && ptr[0]!=0){
				const char *ptrBegin=ptr;
				const char *p1,*p2,*p0=strchr(ptrBegin,';');
				while(true)
				{
					if(p0) *(char *)p0=0;
					p1=p2=NULL;
					p1=strchr(ptrBegin,',');
					if(p1) p2=strchr(p1+1,',');
					if(p1 && p2)
					{
						*(char *)p1=0; p1++;
						*(char *)p2=0; p2++;
						std::pair<std::string,long> p(p2,atoi(p1));
						ptr_ftpuser->dirAccess[ptrBegin]=p;
					}
					if(p0==NULL) break;
					ptrBegin=p0+1;
					p0=strchr(ptrBegin,';');
				}
			}//?ptr=httpreq.Request("vpath");
			pftpsvr->modiUser(*ptr_ftpuser);
			listuser(buffer,pftpsvr->m_userlist);
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>添加修改帐号成功!</retmsg>");
			pftpsvr->saveIni();
		}//?if(ptr_user && ptr_user[0]!=0)
		else
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>无效的帐号!</retmsg>");
	}//?else if(strcasecmp(ptr_cmd,"save")==0)
	else if(strcasecmp(ptr_cmd,"info")==0)
	{
		const char *ptr_user=httpreq.Request("user");
		std::map<std::string,TFTPUser>::iterator it=pftpsvr->m_userlist.end();
		if(ptr_user) it=pftpsvr->m_userlist.find(ptr_user);
		if(it!=pftpsvr->m_userlist.end())
		{
			TFTPUser &ftpuser=(*it).second;
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<userinfo>");
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<account>%s</account>",ftpuser.username.c_str());
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<password>%s</password>",ftpuser.userpwd.c_str());
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<pswdmode>%d</pswdmode>",ftpuser.pswdmode);
			if(ftpuser.limitedTime==0)
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<expired></expired>");
			else{
				struct tm * ltime=localtime(&ftpuser.limitedTime);
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<expired>%04d-%02d-%02d</expired>",
					(1900+ltime->tm_year), ltime->tm_mon+1, ltime->tm_mday);
			}
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<maxsignin>%d</maxsignin>",ftpuser.maxLoginusers);
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<maxup>%d</maxup>",ftpuser.maxupratio);
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<maxupfile>%d</maxupfile>",ftpuser.maxupfilesize);
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<maxdw>%d</maxdw>",ftpuser.maxdwratio);
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<maxdisk>%.1f</maxdisk>",ftpuser.maxdisksize/1000.0);
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<usedisk>%.1f</usedisk>",ftpuser.curdisksize/1000.0);
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<disphidden>%d</disphidden>",ftpuser.disphidden);
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<forbid>%d</forbid>",ftpuser.forbid);
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<ipfilter>");
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<access>%d</access>",ftpuser.ipaccess);
			if(buffer.Space()<(ftpuser.ipRules.length()+64)) buffer.Resize(buffer.size()+(ftpuser.ipRules.length()+64));
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<ipaddr>%s</ipaddr>",ftpuser.ipRules.c_str());
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"</ipfilter>");
			std::pair<std::string,long> &p=ftpuser.dirAccess["/"];
			if(buffer.Space()<(p.first.length()+64)) buffer.Resize(buffer.size()+(p.first.length()+64));
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<user_root access=\"%d\">%s</user_root>",p.second,p.first.c_str());
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vpaths>");
			std::map<std::string,std::pair<std::string,long> >::iterator it=ftpuser.dirAccess.begin();
			it++; 
			for(;it!=ftpuser.dirAccess.end();it++)
			{
				std::pair<std::string,long> &p1=(*it).second;
				if(buffer.Space()<(p1.first.length()+64)) buffer.Resize(buffer.size()+(p1.first.length()+64));
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vpath vname=\"%s\" access=\"%d\">%s</vpath>",
						(*it).first.c_str(),p1.second,p1.first.c_str());
			}
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"</vpaths>");
			if(buffer.Space()<(ftpuser.userdesc.length()+64)) buffer.Resize(buffer.size()+(ftpuser.userdesc.length()+64));
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<description>%s</description>",ftpuser.userdesc.c_str());

			buffer.len()+=sprintf(buffer.str()+buffer.len(),"</userinfo>");
		}//?if(it!=pftpsvr->m_userlist.end())
		else
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>无效的帐号!</retmsg>");
	}
	
	
	if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
	if(buffer.str())
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"</xmlroot>");
	
	httprsp.NoCache();//CacheControl("No-cache");
	//设置MIME类型，默认为HTML
	httprsp.set_mimetype(MIMETYPE_XML);
	//设置响应内容长度
	httprsp.lContentLength(buffer.len());
	httprsp.send_rspH(psock,200,"OK");
	
	if(buffer.str()) psock->Send(buffer.len(),buffer.str(),-1);
	return true;
}
//FTP服务配置的导入导出
bool webServer::httprsp_ftpini(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp)
{
	MyService *ptrService=MyService::GetService();
	ftpsvrEx *pftpsvr=&ptrService->m_ftpsvr;
	const char *ptr_cmd=httpreq.Request("cmd");
	cBuffer buffer(512);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<?xml version=\"1.0\" encoding=\"gb2312\" ?><xmlroot>");

	if(strcasecmp(ptr_cmd,"out")==0) //导出FTP服务配置
	{
		std::string strini;
		pftpsvr->saveAsstring(strini);
		if(buffer.Space()<(strini.length()+64)) buffer.Resize(buffer.size()+(strini.length()+64));
		if(buffer.str())
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<settings><![CDATA[%s]]></settings>",strini.c_str());
	}
	else if(strcasecmp(ptr_cmd,"in")==0) //导入FTP服务配置
	{
		const char *ptr=httpreq.Request("ini");
		if(ptr){
			pftpsvr->initSetting();
			pftpsvr->m_userlist.clear();
			pftpsvr->parseIni((char *)ptr,0);
			pftpsvr->saveIni(); //保存配置参数
		}//?if(ptr)
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>导入配置完成!</retmsg>");
	}//?else if(strcasecmp(ptr_cmd,"in")==0)

	if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
	if(buffer.str())
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"</xmlroot>");
	
	httprsp.NoCache();//CacheControl("No-cache");
	//设置MIME类型，默认为HTML
	httprsp.set_mimetype(MIMETYPE_XML);
	//设置响应内容长度
	httprsp.lContentLength(buffer.len());
	httprsp.send_rspH(psock,200,"OK");
	
	if(buffer.str()) psock->Send(buffer.len(),buffer.str(),-1);
	return true;
}