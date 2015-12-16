/*******************************************************************
   *	vidcManager_ini.cpp
   *    DESCRIPTION: vidc配置的保存和读取
   *
   *    AUTHOR:yyc
   *
   *    HISTORY:
   *	
   *******************************************************************/
#include "../../../rmtsvc.h"

//读取vidc的注册表中ini的配置
bool vidcManager :: readIni()
{
	HKEY  hKEY;
	LPCTSTR lpRegPath = "Software\\Microsoft\\Windows\\CurrentVersion";
	if(::RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpRegPath, 0, KEY_WRITE|KEY_READ, &hKEY)!=ERROR_SUCCESS)
		return false;
	DWORD dwType=REG_SZ,dwSize=0;
	char *pbuffer=NULL;
	if(::RegQueryValueEx(hKEY, "vidcsets", NULL,&dwType,NULL,&dwSize)==ERROR_SUCCESS)
	{
		if( (pbuffer=new char[dwSize]) )
			::RegQueryValueEx(hKEY, "vidcsets", NULL,&dwType,(LPBYTE)pbuffer,&dwSize);
	}
	::RegCloseKey(hKEY);
	if(pbuffer==NULL) return false;
	dwSize=cCoder::base64_decode(pbuffer,dwSize-1,pbuffer);
	pbuffer[dwSize]=0;
	parseIni(pbuffer,dwSize);

	delete[] pbuffer; return true;
}

bool vidcManager :: parseIni(char *pbuffer,long lsize)
{
	if(pbuffer==NULL) return false;
	if(lsize<=0) lsize=strlen(pbuffer);
	if(strncasecmp(pbuffer,"base64",6)==0)
		lsize=cCoder::base64_decode(pbuffer+6,lsize-6,pbuffer);
//	RW_LOG_DEBUG("read vIDC param.........\r\n%s\r\n",pbuffer);
	//开始解析Ini文件
	const char *pstart=pbuffer;
	const char *ptr=strchr(pstart,'\r');
	while(true){
		if(ptr) *(char *)ptr=0;
		if(pstart[0]!='!') //不解释注释行
			parseCommand(pstart);
		if(ptr==NULL) break;
		*(char *)ptr='\r'; pstart=ptr+1;
		if(*pstart=='\n') pstart++;
		ptr=strchr(pstart,'\r');
	}//?while
	return true;
}

bool vidcManager::parseCommand(const char *pstart)
{
	if(strncasecmp(pstart,"mtcpl ",6)==0) //本地端口映射
		docmd_mtcpl(pstart+6);
	else if(strncasecmp(pstart,"sslc ",5)==0) //设置客户端证书
		docmd_sslc(pstart+5);
	else if(strncasecmp(pstart,"iprules ",8)==0)
		docmd_iprules(pstart+8);
	else if(strncasecmp(pstart,"mdhrsp ",7)==0)
		docmd_mdhrsp(pstart+7);
	else if(strncasecmp(pstart,"mdhreq ",7)==0)
		docmd_mdhreq(pstart+7);
	else if(strncasecmp(pstart,"vidcs ",6)==0) //vIDCs服务配置
		docmd_vidcs(pstart+6);
	else if(strncasecmp(pstart,"vidcc ",6)==0) //远程端口映射配置的远端vidcs信息
		docmd_vidcc(pstart+6);
	else if(strncasecmp(pstart,"mtcpr ",6)==0) //远程端口映射
		docmd_mtcpr(pstart+6);
	else if(strncasecmp(pstart,"upnp ",5)==0) //UPnP端口映射
		docmd_upnp(pstart+5);
	else return false;
	
	return true;
}

bool vidcManager :: saveAsstring(std::string &strini)
{
	char buf[256]; int len=0;
	len=sprintf(buf,"!本地端口映射配置信息\r\n"); strini.append(buf,len);
	std::map<std::string,mportTCP *>::iterator it_tcpsets=m_tcpsets.begin();
	for(;it_tcpsets!=m_tcpsets.end();it_tcpsets++)
	{
		mportTCP *ptr_mtcp=(*it_tcpsets).second;
		len=ptr_mtcp->str_info_mtcp((*it_tcpsets).first.c_str(),buf);
		strini.append(buf,len);
		TMapParam *p=(TMapParam *)(ptr_mtcp->Tag());
		if(p){
			if(p->clicert==""){
				len=sprintf(buf,"sslc name=%s cert= \r\n",(*it_tcpsets).first.c_str());
				strini.append(buf,len);
			}else{
				len=sprintf(buf,"sslc name=%s cert=",(*it_tcpsets).first.c_str());
				strini.append(buf,len);
				strini+=p->clicert; strini+=string(",");
				strini+=p->clikey; strini+=string(",");
				strini+=p->clikeypswd; strini+=string("\r\n");
			}
			len=sprintf(buf,"iprules name=%s type=mtcpl autorun=%d access=%d ipaddr=",
				(*it_tcpsets).first.c_str(),((p->bAutorun)?1:0),p->ipaccess);
			strini.append(buf,len);
			strini+=p->ipRules; strini.append("\r\n",2);
		}//?if(p)
		ptr_mtcp->str_info_regcond((*it_tcpsets).first.c_str(),strini);
	}//?for(;it_tcpsets!=
	len=sprintf(buf,"!远程端口映射配置信息\r\n"); strini.append(buf,len);
	this->m_vidccSets.str_list_vidcc(strini);
	len=sprintf(buf,"!vIDCs服务配置信息\r\n"); strini.append(buf,len);
	len=sprintf(buf,"vidcs port=%d bindip=%s autorun=%d bauth=%d pswd=%s\r\n",
		m_vidcsvr.m_svrport,m_vidcsvr.m_bindip.c_str(),
		((m_vidcsvr.m_autorun)?1:0),
		(m_vidcsvr.bAuthentication()?1:0),
		m_vidcsvr.accessPswd().c_str() );
	strini.append(buf,len);
	len=sprintf(buf,"iprules type=vidcs access=%d ipaddr=%s\r\n",
		m_vidcsvr.m_ipaccess,m_vidcsvr.m_ipRules.c_str());
	strini.append(buf,len);

	len=sprintf(buf,"!UPnP端口映射配置信息\r\n"); strini.append(buf,len);
	std::vector<UPnPInfo *> &upnpsets=m_upnp.upnpinfo();
	std::vector<UPnPInfo *> ::iterator it_upnpsets=upnpsets.begin();
	for(;it_upnpsets!=upnpsets.end();it_upnpsets++){
		UPnPInfo *p=*it_upnpsets;
		//	upnp type=[TCP|UDP] appsvr=<应用服务> mport=<映射端口> appdesc=<描述>"
		len=sprintf(buf,"upnp type=%s appsvr=%s:%d mport=%d appdesc=\"",
				   ((p->budp)?"UDP":"TCP"), p->appsvr.c_str(),p->appport,p->mapport);
		strini.append(buf,len);
		strini+=p->appdesc; strini.append("\"\r\n");
	}
	
	return true;
}

//保存当前vIDC的配置到注册表
bool vidcManager :: saveIni()
{
	std::string strini;
	saveAsstring(strini);
	//写注册表--------------------------------------
	HKEY  hKEY;
	LPCTSTR lpRegPath = "Software\\Microsoft\\Windows\\CurrentVersion";
	if(::RegOpenKeyEx(HKEY_LOCAL_MACHINE, lpRegPath, 0, KEY_WRITE|KEY_READ, &hKEY)==ERROR_SUCCESS)
	{
		long l=cCoder::Base64EncodeSize(strini.length());
		char *pbuf=new char[l];
		if(pbuf){
			l=cCoder::base64_encode ((char *)strini.c_str(),strini.length(),pbuf);
			::RegSetValueEx(hKEY, "vidcsets", NULL,REG_SZ, (LPBYTE)pbuf,l+1);
			delete[] pbuf;
		}
		::RegCloseKey(hKEY);
	}
	return true;
}
//更改http响应头
//命令格式:
//	mdhrsp vname=<XXX> name=<XXX> cond=<http响应代码> header=<响应头名称>
//							  pattern=<匹配模式> replto=<替换字符串>
//cond=<http响应代码> - 确定更改HTTP响应代码为指定代码的头
//pattern=<匹配模式>  - 如果匹配模式为空则直接用replto指定的字符串替换
//replto=<替换字符串> - 如果replto为空则直接删除此头
void vidcManager :: docmd_mdhrsp(const char *strParam)
{
	std::map<std::string,std::string> maps;
	if(splitString(strParam,' ',maps)<=0) return;
	std::string vname,mapname;
	std::map<std::string,std::string>::iterator it;
	if( (it=maps.find("name"))!=maps.end() )
		mapname=(*it).second;
	::strlwr((char *)mapname.c_str()); //转换为小写
	if( (it=maps.find("vname"))!=maps.end() )
		vname=(*it).second;

	int rspcode; std::string strHeader,strPattern,strReplto;
	if( (it=maps.find("cond"))!=maps.end())
		rspcode=atoi((*it).second.c_str());
	else rspcode=0;
	if( (it=maps.find("header"))!=maps.end())
		strHeader=(*it).second;
	else strHeader="";
	if( (it=maps.find("pattern"))!=maps.end())
		strPattern=(*it).second;
	else strPattern="";
	if( (it=maps.find("replto"))!=maps.end())
		strReplto=(*it).second;
	else strReplto="";
	if(mapname=="" || strHeader=="") return;
	if(vname=="") //本地端口映射
	{//设置本地端口映射的HTTP响应头修改规则
		std::map<std::string,mportTCP *>::iterator it1=m_tcpsets.find(mapname);
		mportTCP *ptr_mtcp=(it1==m_tcpsets.end())?NULL:(*it1).second;
		if(ptr_mtcp==NULL) return;	
		ptr_mtcp->addRegCond(rspcode,strHeader.c_str(),strPattern.c_str(),strReplto.c_str());
	}else //远程端口映射
	{//设置远程端口映射的HTTP响应头修改规则
		vidcClient *pvidcc=this->m_vidccSets.GetVidcClient(vname.c_str(),false);
		mapInfo * pinfo=(pvidcc)?pvidcc->mapinfoGet(mapname.c_str(),false):NULL;
		if(pvidcc==NULL || pinfo==NULL) return;
		char buf[1024]; 
		sprintf(buf,"cond=%d header=\"%s\" pattern=\"%s\" replto=\"%s\"",rspcode,
					strHeader.c_str(),strPattern.c_str(),strReplto.c_str());
		pinfo->m_hrspRegCond.push_back(buf);	
	}//远程端口映射
}
//更改http请求头
//命令格式:
//	mdhreq vname=<XXX> name=<XXX> cond=<http请求url> header=<响应头名称>
//							  pattern=<匹配模式> replto=<替换字符串>
//cond=<http请求url>  - 更改符合条件的HTTP请求头，请求的Url和指定的cond包含匹配
//pattern=<匹配模式>  - 如果匹配模式为空则直接用replto指定的字符串替换
//replto=<替换字符串> - 如果replto为空则直接删除此头
void vidcManager :: docmd_mdhreq(const char *strParam)
{
	std::map<std::string,std::string> maps;
	if(splitString(strParam,' ',maps)<=0) return;
	std::string vname,mapname;
	std::map<std::string,std::string>::iterator it;
	if( (it=maps.find("name"))!=maps.end() )
		mapname=(*it).second;
	::strlwr((char *)mapname.c_str()); //转换为小写
	if( (it=maps.find("vname"))!=maps.end() )
		vname=(*it).second;

	std::string strUrl,strHeader,strPattern,strReplto;
	if( (it=maps.find("cond"))!=maps.end())
		strUrl=(*it).second;
	if(strUrl=="") strUrl="/";
	if( (it=maps.find("header"))!=maps.end())
		strHeader=(*it).second;
	else strHeader="";
	if( (it=maps.find("pattern"))!=maps.end())
		strPattern=(*it).second;
	else strPattern="";
	if( (it=maps.find("replto"))!=maps.end())
		strReplto=(*it).second;
	else strReplto="";
	//yyc modify 2010-02-23 增加了URL重写功能，此时header无意义
	if(mapname=="") return; // || strHeader=="") return; 
	if(vname=="") //本地端口映射
	{//设置本地端口映射的HTTP响应头修改规则
		std::map<std::string,mportTCP *>::iterator it1=m_tcpsets.find(mapname);
		mportTCP *ptr_mtcp=(it1==m_tcpsets.end())?NULL:(*it1).second;		
		if(ptr_mtcp==NULL) return;
		ptr_mtcp->addRegCond(strUrl.c_str(),strHeader.c_str(),strPattern.c_str(),strReplto.c_str());
	}else //远程端口映射
	{//设置远程端口映射的HTTP响应头修改规则
		vidcClient *pvidcc=this->m_vidccSets.GetVidcClient(vname.c_str(),false);
		mapInfo * pinfo=(pvidcc)?pvidcc->mapinfoGet(mapname.c_str(),false):NULL;
		if(pvidcc==NULL || pinfo==NULL) return;
		char buf[1024]; 
		sprintf(buf,"cond=\"%s\" header=\"%s\" pattern=\"%s\" replto=\"%s\"",
				strUrl.c_str(),strHeader.c_str(),strPattern.c_str(),strReplto.c_str());
		pinfo->m_hreqRegCond.push_back(buf);
	}//远程端口映射
}
//设置端口映射服务的客户端验证证书信息
//仅仅对于那些－ssl的映射服务有效
//格式: sslc vname=<XXX> name=<XXX> cert=<客户端证书>,<证书私钥>,<私钥密码>
//如果没有指定vname则是本地端口映射，否则为远程端口映射
bool vidcManager :: docmd_sslc(const char *strParam)
{
	if(strParam==NULL) return false;
	std::map<std::string,std::string> maps;
	if(splitString(strParam,' ',maps)<=0) return false;
	std::string vname,mapname;
	std::map<std::string,std::string>::iterator it;
	if( (it=maps.find("name"))==maps.end() || (*it).second=="")
		return false;
	else mapname=(*it).second;
	::strlwr((char *)mapname.c_str()); //转换为小写
	if( (it=maps.find("vname"))!=maps.end() )
		vname=(*it).second;

	const char *ptr,*ptr_cert=NULL,*ptr_key=NULL,*ptr_pswd=NULL;
	if( (it=maps.find("cert"))!=maps.end() && (*it).second!="")
	{
		ptr_cert=(*it).second.c_str();
		if( (ptr=strchr(ptr_cert,',')) ){
			*(char *)ptr=0;
			ptr_key=ptr+1;
			if( (ptr=strchr(ptr_key,',')) )
			{
				*(char *)ptr=0;
				ptr_pswd=ptr+1;
			}
		}
	}//?if( (it=maps.find("cert"))!=maps.end() && (*it).second!="")

	if(vname==""){//本地端口映射
		std::map<std::string,mportTCP *>::iterator it1=m_tcpsets.find(mapname);
		if(it1==m_tcpsets.end()) return false;
		mportTCP *ptr_mtcp=(*it1).second; if(ptr_mtcp==NULL) return false;
		TMapParam *p=(TMapParam *)(ptr_mtcp->Tag());
		if(p==NULL){ 
			if( (p=new TMapParam)==NULL) return false; 
			else ptr_mtcp->Tag()=(long)p;
			p->bAutorun=false; p->ipaccess=1;
		}
		if(ptr_cert) p->clicert.assign(ptr_cert); else p->clicert="";
		if(ptr_key) p->clikey.assign(ptr_key); else p->clikey="";
		if(ptr_key) p->clikeypswd.assign(ptr_pswd); else p->clikeypswd="";
	}else{
		vidcClient *pvidcc=this->m_vidccSets.GetVidcClient(vname.c_str(),false);
		if(pvidcc==NULL) return false;
		mapInfo * pinfo=pvidcc->mapinfoGet(mapname.c_str(),false);
		if(pinfo==NULL) return false;
		if(ptr_cert) pinfo->m_clicert.assign(ptr_cert); else pinfo->m_clicert="";
		if(ptr_key) pinfo->m_clikey.assign(ptr_key); else pinfo->m_clikey="";
		if(ptr_key) pinfo->m_clikeypswd.assign(ptr_pswd); else pinfo->m_clikeypswd="";
	}
	return true;
}
//本地端口映射
//命令格式: 
//	mtcpl name=<XXX> appsvr=<要映射的应用服务> mport=<映射端口>[+|-ssl] [sslverify=0|1] [bindip=<本服务绑定的本机IP>] [apptype=FTP|WWW|TCP|UNKNOW] [maxconn=<最大并发连接数>] [maxratio=<>] [appdesc=<描述>]
bool vidcManager :: docmd_mtcpl(const char *strParam)
{
	if(strParam==NULL) return false;
	std::map<std::string,std::string> maps;
	if(splitString(strParam,' ',maps)<=0) return false;
	
	std::map<std::string,std::string>::iterator it;
	if( (it=maps.find("appsvr"))==maps.end() || (*it).second=="")
		return false;

	//生成一个临时的映射服务名称
	char strMapname[32]; sprintf(strMapname,"mtcpl_%d",m_tcpsets.size());
	const char *mapname=strMapname;
	if( (it=maps.find("name"))!=maps.end() && (*it).second!="" )
		mapname=(*it).second.c_str();
	::strlwr((char *)mapname); //转换为小写

	mportTCP *ptr_mtcp=NULL;
	std::map<std::string,mportTCP *>::iterator it1=m_tcpsets.find(mapname);
	if(it1!=m_tcpsets.end()) //修改
		ptr_mtcp=(*it1).second;
	else if( (ptr_mtcp=new mportTCP) )
		m_tcpsets[mapname]=ptr_mtcp;
	else return false;
	
	const char *ptr,*ptr_appsvr,*ptr_appdesc,*ptr_bindip;
	int mportBegin=0,mportEnd=0;
	SSLTYPE ssltype=TCPSVR_TCPSVR;
	bool bSSLVerify=false;
	MPORTTYPE apptype=MPORTTYPE_UNKNOW;
	
	it=maps.find("apptype");
	if( (it=maps.find("apptype"))!=maps.end())
	{
		if((*it).second=="FTP")
			apptype=MPORTTYPE_FTP;
		else if((*it).second=="WWW")
			apptype=MPORTTYPE_WWW;
		else if((*it).second=="TCP")
			apptype=MPORTTYPE_TCP;
	}
	it=maps.find("bindip");
	ptr_bindip=(it!=maps.end())?(*it).second.c_str():NULL;
	it=maps.find("appsvr");
	ptr_appsvr=(*it).second.c_str();
	it=maps.find("appdesc");
	ptr_appdesc=(it!=maps.end())?(*it).second.c_str():NULL;
	
	it=maps.find("sslverify");
	if(it!=maps.end() && (*it).second=="1") bSSLVerify=true;
	it=maps.find("mport");
	if(it!=maps.end()){
		if((*it).second.length()>4){
			ptr=(*it).second.c_str()+(*it).second.length()-4;
			if(strcasecmp(ptr,"+ssl")==0)
			{
				ssltype=TCPSVR_SSLSVR;
				*(char *)ptr=0;
			}
			else if(strcasecmp(ptr,"-ssl")==0)
			{
				ssltype=SSLSVR_TCPSVR;
				*(char *)ptr=0;
			}
		}//?if((*it).second.length()>4)
		//解析映射端口
		ptr=strchr((*it).second.c_str(),'-');
		if(ptr){
			mportBegin=atoi((*it).second.c_str());
			mportEnd=atoi(ptr+1);
		}else{
			mportBegin=atoi((*it).second.c_str());
			mportEnd=mportBegin;
		}
		if(mportBegin<0) mportBegin=0;
		if(mportEnd<0) mportEnd=0;	
	}//?解析端口映射参数

	long ltmp; it=maps.find("maxratio"); //限制带宽
	if(it!=maps.end()) ltmp=atol((*it).second.c_str()); else ltmp=0;
	ptr_mtcp->setMaxRatio( ((ltmp<0)?0:ltmp) );
	it=maps.find("maxconn"); //限制最大连接数
	if(it!=maps.end()) ltmp=atol((*it).second.c_str()); else ltmp=0;
	ptr_mtcp->maxConnection( ((ltmp<0)?0:ltmp) );
	if( (it=maps.find("blogd"))!=maps.end() && (*it).second=="1")
		ptr_mtcp->setIfLogdata(true);
	else ptr_mtcp->setIfLogdata(false);

	ptr_mtcp->setMapping(mportBegin,mportEnd,ptr_bindip);
	ptr_mtcp->setSSLType(ssltype,bSSLVerify);
	ptr_mtcp->setAppsvr(ptr_appsvr,0,ptr_appdesc,apptype);
	return true;
}

//设置ip过滤规则和自动启动标志
//命令格式:
//	iprules type=[mtcpl|mupl|mtcpr] name=<映射服务名称>] [autorun=[1|0] [access=0|1] ipaddr="<IP>,<IP>,..."
//access=0|1     : 对符合下列IP条件的是拒绝还是放行
void vidcManager :: docmd_iprules(const char *strParam)
{
	std::map<std::string,std::string> maps;
	if(splitString(strParam,' ',maps)<=0) return;
	
	std::map<std::string,std::string>::iterator it;
	if( (it=maps.find("type"))==maps.end()) return;
	if((*it).second=="vidcs")
	{//设置代理服务的IP过滤规则
		if( (it=maps.find("access"))!=maps.end())
			m_vidcsvr.m_ipaccess=atoi((*it).second.c_str());
		
		if( (it=maps.find("ipaddr"))!=maps.end())
			m_vidcsvr.m_ipRules=(*it).second;
	}else if((*it).second=="mtcpl")
	{//设置本地端口映射的IP过滤规则
		if( (it=maps.find("name"))==maps.end()) return;
		std::string mapname=(*it).second;
		::strlwr((char *)mapname.c_str()); //转换为小写
		std::map<std::string,mportTCP *>::iterator it1=m_tcpsets.find(mapname);
		if(it1==m_tcpsets.end()) return;
		mportTCP *ptr_mtcp=(*it1).second;
		TMapParam *p=(TMapParam *)(ptr_mtcp->Tag());
		if(p==NULL) 
			if( (p=new TMapParam)==NULL) return; else ptr_mtcp->Tag()=(long)p;
		
		if( (it=maps.find("autorun"))!=maps.end() && (*it).second=="1" )
			p->bAutorun=true;
		else p->bAutorun=false;
		
		if( (it=maps.find("access"))!=maps.end())
			p->ipaccess=atoi((*it).second.c_str());
		else p->ipaccess=1;
		if( (it=maps.find("ipaddr"))!=maps.end())
			p->ipRules=(*it).second;
	}//?else if((*it).second=="mtcpl")
	else if((*it).second=="mtcpr")
	{//设置远程端口映射的IP过滤规则
		std::string vname,mapname;
		std::map<std::string,std::string>::iterator it;
		if( (it=maps.find("vname"))!=maps.end()) vname=(*it).second;
		::strlwr((char *)vname.c_str()); //转换为小写
		if( (it=maps.find("name"))!=maps.end()) mapname=(*it).second;
		::strlwr((char *)mapname.c_str()); //转换为小写
		if(vname=="" || mapname=="") return;
		vidcClient *pvidcc=this->m_vidccSets.GetVidcClient(vname.c_str(),false);
		if(pvidcc==NULL) return;
		mapInfo * pinfo=pvidcc->mapinfoGet(mapname.c_str(),false);
		if(pinfo==NULL) return;
		if( (it=maps.find("access"))!=maps.end()) pinfo->m_ipaccess=atoi((*it).second.c_str());
		if( (it=maps.find("ipaddr"))!=maps.end()) pinfo->m_ipRules=(*it).second;
	}//?else if((*it).second=="mtcpr")
	return;
}
//设置vIDCs服务参数
//命令格式:
//	vidcs port= [bindip=] [bauth=<1|0>] [pswd=<密码>] [autorun=[1|0]"
void vidcManager :: docmd_vidcs(const char *strParam)
{
	std::map<std::string,std::string> maps;
	if(splitString(strParam,' ',maps)<=0) return;
	
	std::map<std::string,std::string>::iterator it;

	if( (it=maps.find("port"))!=maps.end())
		m_vidcsvr.m_svrport=atoi((*it).second.c_str());
	if(m_vidcsvr.m_svrport<0) m_vidcsvr.m_svrport=0;
	if( (it=maps.find("bindip"))!=maps.end())
		m_vidcsvr.m_bindip=(*it).second;
	else m_vidcsvr.m_bindip="";
	
	if( (it=maps.find("autorun"))!=maps.end() && (*it).second=="1")
		m_vidcsvr.m_autorun=true;
	else m_vidcsvr.m_autorun=false;
	if( (it=maps.find("bauth"))!=maps.end() && (*it).second=="1")
		m_vidcsvr.bAuthentication(true);
	else m_vidcsvr.bAuthentication(false);
	
	if( (it=maps.find("pswd"))!=maps.end() )
		m_vidcsvr.accessPswd()=(*it).second;
	else m_vidcsvr.accessPswd()="";
}

//设置vidcc连接的远端vidcs信息
//命令格式:
//	vidcc vname=<名称> vhost=<host:port> vpswd=<访问密码> autorun<0|1>"
void vidcManager :: docmd_vidcc(const char *strParam)
{
	std::map<std::string,std::string> maps;
	if(splitString(strParam,' ',maps)<=0) return;
	
	std::string vname,vhost; int vport=0;
	std::map<std::string,std::string>::iterator it;
	if( (it=maps.find("vname"))!=maps.end()) vname=(*it).second;
	::strlwr((char *)vname.c_str()); //转换为小写
	if( (it=maps.find("vhost"))!=maps.end()){
		const char *ptr=strchr((*it).second.c_str(),':');
		if(ptr){
			vhost.assign((*it).second.c_str(),ptr-(*it).second.c_str());
			vport=atoi(ptr+1);
		}else vhost=(*it).second;
	}
	if(vname=="" || vhost=="" || vport<=0) return;
	vidcClient *pvidcc=this->m_vidccSets.GetVidcClient(vname.c_str(),true);
	if(pvidcc==NULL) return;
	VIDCSINFO &vinfo=pvidcc->vidcsinfo();
	vinfo.m_vidcsHost=vhost; vinfo.m_vidcsPort=vport;
	if( (it=maps.find("vpswd"))!=maps.end()) vinfo.m_vidcsPswd=(*it).second;
	if( (it=maps.find("autorun"))!=maps.end())
		vinfo.m_bAutoConn=(atoi((*it).second.c_str())==1)?true:false;
	return;
}

//设置远程端口映射信息
//命令格式:
//	mtcpr type=[TCP|UDP] vname=<vidcc名称> name=<映射名称> appsvr=<应用服务> mport=<映射端口>[+-ssl] [sslverify=0|1] bindip=<绑定ip> apptype=<FTP|WWW|TCP|UNK> automap=<0|1> appdesc=<描述>"
//	mtcpr type=PROXY vname=<vidcc名称> name=<映射名称> mport=<映射端口> bindip=<绑定ip> svrtype=<HTTPS|SOCKS4|SOCKS5> bauth=<0|1> account=<帐号:密码> automap=<0|1> appdesc=<描述>"
void vidcManager :: docmd_mtcpr(const char *strParam)
{
	std::map<std::string,std::string> maps;
	if(splitString(strParam,' ',maps)<=0) return;
	
	std::string vname,mapname;
	std::map<std::string,std::string>::iterator it;
	if( (it=maps.find("vname"))!=maps.end()) vname=(*it).second;
	::strlwr((char *)vname.c_str()); //转换为小写
	if( (it=maps.find("name"))!=maps.end()) mapname=(*it).second;
	::strlwr((char *)mapname.c_str()); //转换为小写
	if(vname=="" || mapname=="") return;
	vidcClient *pvidcc=this->m_vidccSets.GetVidcClient(vname.c_str(),false);
	if(pvidcc==NULL) return;
	mapInfo * pinfo=pvidcc->mapinfoGet(mapname.c_str(),true);
	if(pinfo==NULL) return;
	pinfo->m_mapType =VIDC_MAPTYPE_TCP;
	if( (it=maps.find("type"))!=maps.end())
	{
		if((*it).second=="PROXY")    pinfo->m_mapType =VIDC_MAPTYPE_PROXY;
		else if((*it).second=="UDP") pinfo->m_mapType =VIDC_MAPTYPE_UDP;
	}
	
	if( (it=maps.find("appsvr"))!=maps.end()) pinfo->m_appsvr=(*it).second;
	if( (it=maps.find("mport"))!=maps.end()){
		const char *ptr,*ptrBegin=(*it).second.c_str();
		if( (ptr=strstr(ptrBegin,"+ssl")) ){
			pinfo->m_ssltype=TCPSVR_SSLSVR;
			*(char *)ptr=0;
		}else if( (ptr=strstr(ptrBegin,"-ssl")) ){
			pinfo->m_ssltype=SSLSVR_TCPSVR;
			*(char *)ptr=0;
		}else pinfo->m_ssltype=TCPSVR_TCPSVR;
		if( (ptr=strchr(ptrBegin,'-')) ){
			pinfo->m_mportBegin=atoi(ptrBegin);
			pinfo->m_mportEnd=atoi(ptr+1);
		}else{
			pinfo->m_mportBegin=atoi(ptrBegin);
			pinfo->m_mportEnd=pinfo->m_mportBegin;
		}
	}

	long ltmp; it=maps.find("maxratio"); //限制带宽
	if(it!=maps.end()) ltmp=atol((*it).second.c_str()); else ltmp=0;
	pinfo->m_maxratio=ltmp;
	it=maps.find("maxconn"); //限制最大连接数
	if(it!=maps.end()) ltmp=atol((*it).second.c_str()); else ltmp=0;
	pinfo->m_maxconn=ltmp;

	it=maps.find("sslverify");
	if(it!=maps.end() && (*it).second=="1") pinfo->m_sslverify=true;
	else pinfo->m_sslverify=false;
	if( (it=maps.find("bindip"))!=maps.end())
		strcpy(pinfo->m_bindLocalIP,(*it).second.c_str());
	if( (it=maps.find("apptype"))!=maps.end())
	{
		if((*it).second=="FTP")
			pinfo->m_apptype=MPORTTYPE_FTP;
		else if((*it).second=="WWW")
			pinfo->m_apptype=MPORTTYPE_WWW;
		else if((*it).second=="TCP")
			pinfo->m_apptype=MPORTTYPE_TCP;
		else pinfo->m_apptype=MPORTTYPE_UNKNOW;
	}
	if( (it=maps.find("automap"))!=maps.end())
		pinfo->m_bAutoMap=(atoi((*it).second.c_str()))?true:false;
	if( (it=maps.find("appdesc"))!=maps.end()) pinfo->m_appdesc=(*it).second;
	
	if( (it=maps.find("svrtype"))!=maps.end())
	{
		int svrtype=0;
		if(strstr((*it).second.c_str(),"HTTPS"))
			svrtype|=PROXY_HTTPS;
		if(strstr((*it).second.c_str(),"SOCKS4"))
			svrtype|=PROXY_SOCKS4;
		if(strstr((*it).second.c_str(),"SOCKS5"))
			svrtype|=PROXY_SOCKS5;
		if(svrtype!=0) pinfo->m_proxyType=svrtype;
		else pinfo->m_proxyType=PROXY_HTTPS|PROXY_SOCKS4|PROXY_SOCKS5;
	}else pinfo->m_proxyType=PROXY_HTTPS|PROXY_SOCKS4|PROXY_SOCKS5;
	if( (it=maps.find("bauth"))!=maps.end() && (*it).second=="1")
		pinfo->m_proxyauth=true;
	else pinfo->m_proxyauth=false;
	if( (it=maps.find("account"))!=maps.end() )
	{
		const char *ptr=strchr((*it).second.c_str(),':');
		if(ptr){
			int l=ptr-(*it).second.c_str();
			if(l>0)
				pinfo->m_proxyuser.assign((*it).second.c_str(),l);
			else pinfo->m_proxyuser="";
			pinfo->m_proxypswd.assign(ptr+1);
		}else{
			pinfo->m_proxyuser=(*it).second;
			pinfo->m_proxypswd="";
		}
	}

	if(pinfo->m_mapType==VIDC_MAPTYPE_PROXY){ 
		pinfo->m_apptype=MPORTTYPE_TCP; 
		pinfo->m_ssltype=TCPSVR_TCPSVR; 
	}
	return;
}

//UPnP端口映射信息
//命令格式:
//	upnp type=[TCP|UDP] appsvr=<应用服务> mport=<映射端口> appdesc=<描述>"
void vidcManager :: docmd_upnp(const char *strParam)
{
	std::map<std::string,std::string> maps;
	if(splitString(strParam,' ',maps)<=0) return;
	std::map<std::string,std::string>::iterator it;

	std::string appsvr,appdesc; 
	int appport,mapport; bool btcp;
	if( (it=maps.find("appsvr"))==maps.end()) return;

	appsvr=(*it).second;
	const char *ptr=strchr(appsvr.c_str(),':');
	if(ptr){
		appsvr.erase(ptr-appsvr.c_str());
		appport=atoi(ptr+1);
	}else appport=0;

	if( (it=maps.find("mport"))!=maps.end()) mapport=atoi((*it).second.c_str());
	else mapport=0;
	if(mapport<=0) mapport=appport;
	
	if( (it=maps.find("type"))!=maps.end() && (*it).second=="UDP")
		 btcp=false;
	else btcp=true;

	if( (it=maps.find("appdesc"))!=maps.end()) appdesc=(*it).second;
	else appdesc="";
	
	if(appsvr=="" || appport<=0) return;
	m_upnp.AddPortMapping(btcp,appsvr.c_str(),appport,mapport,appdesc.c_str());
	return;
}



