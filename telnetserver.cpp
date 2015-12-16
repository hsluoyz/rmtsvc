/*******************************************************************
   *	telnetserver.h 
   *    DESCRIPTION: Telnet服务
   *
   *    AUTHOR:yyc
   *
   *    HISTORY:
   *	
   *******************************************************************/

#include "rmtsvc.h" 
#include "shellCommandEx.h"
#include "other/Wutils.h"

class clsOutput_sock : public clsOutput
{
public:
	clsOutput_sock(socketTCP *psock):m_psock(psock){};
	virtual ~clsOutput_sock(){}
	int print(const char *buf,int len)
	{
		if(m_psock==NULL) return 0;
		return m_psock->Send(len,buf,-1);
	}
	socketBase *psocket(){ return m_psock; }
private:
	socketTCP *m_psock;
};

void cTelnetEx :: onCommand(const char *strCommand,socketTCP *psock)
{
	//判断是否将输出定向到文件
	const char *ptr_outfile=strchr(strCommand,'>');
	if(ptr_outfile){ *(char *)ptr_outfile++=0;
		while(*ptr_outfile==' ') ptr_outfile++; //去掉前导空格
	}

	int cmdlen=strlen(strCommand); //去掉尾部空格
	while(cmdlen>0 && *(strCommand+cmdlen-1)==' ') cmdlen--;
	if(cmdlen<=0) return; else *((char *)strCommand+cmdlen)=0;
//----------  扩展控制命令 start------------------------------
	BOOL bRet=FALSE; std::string strOutput;
	const char *strCmd=strCommand;
	const char *strParam=strchr(strCommand,' ');
	if(strParam) { *(char *)strParam=0; strParam++; }

	if(strcasecmp(strCmd,"update")==0 || strcasecmp(strCmd,"down")==0) //升级或下载
	{
		if(strParam==NULL) return;
		bool bUpdate=(strcasecmp(strCmd,"update")==0);
		while(*strParam==' ') strParam++;
		int iType=0; const char *strurl=strParam;
		if(strncasecmp(strurl,"http://",7)==0) iType=1;
		else if(strncasecmp(strurl,"https://",8)==0) iType=1;
		else if(strncasecmp(strurl,"ftp://",6)==0) iType=2;
		std::string strSaveas;
		if(iType>0){//下载指定的文件
			clsOutput_sock sout(psock);
			const char *ptr=strchr(strurl,' ');
			if( ptr ){ *(char *)ptr=0; ptr+=1;
				while(*ptr==' ') ptr++; strSaveas.assign(ptr); 
			}
			if(strSaveas==""){ if( (ptr=strrchr(strurl,'/')) ) strSaveas.assign(ptr+1); }
			if(strSaveas[0]!='\\' && strSaveas[1]!=':') strSaveas.insert(0,g_savepath);
			if(bUpdate) strSaveas.append(".upd"); //防止下载的文件和要升级的程序重名
			bRet=(iType==2)?downfile_ftp(strurl,strSaveas.c_str(),sout):downfile_http(strurl,strSaveas.c_str(),sout);
			strurl=(bRet)?strSaveas.c_str():NULL;
		}else if(!bUpdate) strOutput.append("Failed , wrong URLs.\r\n");
		//升级rmtsvc
		if(bUpdate)  bRet=updateRV(strurl,strOutput);
	}else if(strcasecmp(strCmd,"telnet")==0) //开启telnet
		bRet=FALSE;
	else //执行扩展命令
		bRet=doCommandEx(strCmd,strParam,strOutput);
//----------  扩展控制命令  end ------------------------------
	
	if(bRet)
	{
		FILE *fp=(ptr_outfile)?::fopen(ptr_outfile,"w"):NULL;
		if(fp){
			::fwrite(strOutput.c_str(),sizeof(char),strOutput.length(),fp);
			::fclose(fp);
			strOutput.assign("output >> "); strOutput.append(ptr_outfile);
		}
		strOutput.append("\r\n****Success to action****\r\n");
	}else strOutput.append("****Failed to action*****\r\n");
	if(strOutput!="") psock->Send(strOutput.length(),strOutput.c_str(),-1);
	return;
}

//--------------------------------------------------------------
//--------------------------------------------------------------
telServerEx :: telServerEx()
{
	m_strSvrname.assign("Telnet Server");
	m_bindip="";
	m_svrport=0;
}
telServerEx :: ~telServerEx()
{
	 Stop(); m_threadpool.join();
}

//启动服务
bool telServerEx :: Start() 
{
	if(m_svrport==0) return true; //不启动服务
	
	const char *ip=(m_bindip=="")?NULL:m_bindip.c_str();
	BOOL bReuseAddr=(ip)?SO_REUSEADDR:FALSE;//绑定了IP则允许端口重用
	SOCKSRESULT sr=Listen( ((m_svrport<0)?0:m_svrport) ,bReuseAddr,ip);
	return (sr>0)?true:false;
}


void telServerEx ::revConnectionThread(socketTCP *psock)
{
	telServerEx *ptelsvr=(telServerEx *)psock->parent();
	MyService *pmysvr=MyService::GetService();
	socketBase *pevent=(pmysvr)?pmysvr->GetSockEvent():NULL;
	psock->setParent(pevent);
	ptelsvr->onConnect(psock);
	delete psock; return;
}
SOCKSRESULT telServerEx :: revConnect(const char *host,int port,time_t lWaitout)
{
	socketTCP *psock=new socketTCP;
	if(psock==NULL) return SOCKSERR_INVALID;
	else psock->setParent(this);
	SOCKSRESULT sr=psock->Connect(host,port,lWaitout);
	if(sr>0)
	if(m_threadpool.addTask((THREAD_CALLBACK *)&revConnectionThread,(void *)psock,THREADLIVETIME)==0)
		sr=SOCKSERR_THREAD;
	
	if(sr<=0) delete psock;
	return sr;
}

//设置telnet服务的相关信息
//命令格式: 
//	telnet [port=<服务端口>] [bindip=<本服务绑定的本机IP>]  [account=<访问帐号:密码>] 
//port=<服务端口>    : 设置服务端口，如果不设置则默认为0.设置为0则不启动web服务 <0则随即分配端口
//bindip=<本服务绑定的本机IP> : 设置本服务绑定的本机IP，如果不设置则默认绑定本机所有IP
//account=<访问帐号:密码>
void telServerEx :: docmd_sets(const char *strParam)
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
	
	if( (it=maps.find("account"))!=maps.end())
	{
		const char *ptr=strchr((*it).second.c_str(),':');
		if(ptr){
			*(char *)ptr=0;
			setTelAccount((*it).second.c_str(),ptr+1);
			*(char *)ptr=':';
		}else setTelAccount(NULL,NULL); //无需帐号密码
	}
	
	return;
}
//设置服务的ip过滤规则或针对某个帐号的IP过滤规则
//命令格式:
//	iprules [access=0|1] ipaddr="<IP>,<IP>,..."
//access=0|1     : 对符合下列IP条件的是拒绝还是放行
//例如:
// iprules access=0 ipaddr="192.168.0.*,192.168.1.10"
void telServerEx :: docmd_iprules(const char *strParam)
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
