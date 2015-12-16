/*******************************************************************
   *	webAction_vidc.cpp web请求处理 - vidc管理
   *    DESCRIPTION:
   *
   *    AUTHOR:yyc
   *
   *    HISTORY:
   *
   *    DATE:
   *	
   *******************************************************************/
#include "../../../rmtsvc.h"

static const char errMsg_failed[]="操作执行失败";
static const char errMsg_ok[]="操作执行成功";

//列出所有本地映射端口
//<?xml version="1.0" encoding="gb2312" ?>
//...
//</xmlroot>
//获取本地映射服务信息
bool webServer::httprsp_mportl(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp)
{
	MyService *ptrService=MyService::GetService();
	vidcManager *pvidc=&ptrService->m_vidcManager;
	const char *ptr_cmd=httpreq.Request("type");
	bool bTcpType=(ptr_cmd && strcmp(ptr_cmd,"udp")==0)?false:true;
	ptr_cmd=httpreq.Request("cmd");
	
	cBuffer buffer(1024);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<?xml version=\"1.0\" encoding=\"gb2312\" ?><xmlroot>");
	
	if(strcasecmp(ptr_cmd,"list")==0){ //返回本地映射列表XML
		pvidc->xml_list_localip(buffer); //返回本机ip地址
		(bTcpType)?pvidc->xml_list_mtcp(buffer):
				   pvidc->xml_list_mudp(buffer);
	}
	else if(strcasecmp(ptr_cmd,"info")==0)
	{
		const char *ptr_mapname=httpreq.Request("mapname");
		(bTcpType)?pvidc->xml_info_mtcp(buffer,ptr_mapname):
				   pvidc->xml_info_mudp(buffer,ptr_mapname);
	}
	else if(strcasecmp(ptr_cmd,"start")==0)
	{
		const char *ptr_mapname=httpreq.Request("mapname");
		if(bTcpType)
			pvidc->xml_start_mtcp(buffer,ptr_mapname);
		else 
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>此版本暂时不支持UDP映射</retmsg>");
		//	pvidc->xml_start_mudp(buffer,ptr_mapname);
	}
	else if(strcasecmp(ptr_cmd,"stop")==0)
	{
		const char *ptr_mapname=httpreq.Request("mapname");
		(bTcpType)?pvidc->xml_stop_mtcp(buffer,ptr_mapname):
				   pvidc->xml_stop_mudp(buffer,ptr_mapname);
	}
	else if(strcasecmp(ptr_cmd,"dele")==0)
	{
		const char *ptr_mapname=httpreq.Request("mapname");
		(bTcpType)?pvidc->xml_dele_mtcp(buffer,ptr_mapname):
				   pvidc->xml_dele_mudp(buffer,ptr_mapname);
	}
	else if(strcasecmp(ptr_cmd,"save")==0)
	{
		const char *ptr_param=httpreq.Request("param");
		pvidc->parseIni((char *)ptr_param,0);
		(bTcpType)?pvidc->xml_list_mtcp(buffer):
				   pvidc->xml_list_mudp(buffer);
		if(buffer.Space()<48) buffer.Resize(buffer.size()+48);
		if(buffer.str())
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>%s</retmsg>",errMsg_ok);
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

//远程映射服务信息
bool webServer::httprsp_mportr(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp)
{
	MyService *ptrService=MyService::GetService();
	vidcManager *pvidc=&ptrService->m_vidcManager;
	vidccSets &vidccsets=pvidc->m_vidccSets;
	
	const char *ptr_cmd,*ptr_vname;
	VIDC_MAPTYPE maptype=VIDC_MAPTYPE_TCP;
	ptr_cmd=httpreq.Request("type");
	if(ptr_cmd && strcmp(ptr_cmd,"udp")==0) maptype=VIDC_MAPTYPE_UDP;
	else if(ptr_cmd && strcmp(ptr_cmd,"proxy")==0) maptype=VIDC_MAPTYPE_PROXY;
	ptr_cmd=httpreq.Request("cmd");
	ptr_vname=httpreq.Request("vname");
	vidcClient *pvidcc=NULL;

	cBuffer buffer(1024);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<?xml version=\"1.0\" encoding=\"gb2312\" ?><xmlroot>");
	
	if(strcasecmp(ptr_cmd,"vidcclist")==0){ //返回本地配置的vidcc列表
		vidccsets.xml_list_vidcc(buffer);
	}else if(strcasecmp(ptr_cmd,"vidccmodi")==0)
	{//添加修改一个vIDCc客户端和vIDCs的对应
		if(ptr_vname && ptr_vname[0]!=0){
			if( (pvidcc=vidccsets.GetVidcClient(ptr_vname,true)) )
			{
				VIDCSINFO &vinfo=pvidcc->vidcsinfo();
				const char *ptr_req=httpreq.Request("vhost");
				if(ptr_req) vinfo.m_vidcsHost.assign(ptr_req);
				ptr_req=httpreq.Request("vport");
				vinfo.m_vidcsPort=(ptr_req)?atoi(ptr_req):0;
				ptr_req=httpreq.Request("vpswd");
				if(ptr_req) vinfo.m_vidcsPswd.assign(ptr_req);
				ptr_req=httpreq.Request("vconn");
				vinfo.m_bAutoConn=(ptr_req && atoi(ptr_req)==1)?true:false;
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>%s</retmsg>",errMsg_ok);
			}//?if(pvidcc)
		}//?if(ptr_req && ptr_req[0]!=0)
		vidccsets.xml_list_vidcc(buffer);
	}else if(strcasecmp(ptr_cmd,"vidccdele")==0) 
	{//删除一个vIDCc客户端和vIDCs的对应
		if(ptr_vname && ptr_vname[0]!=0){
			if( vidccsets.DelVidcClient(ptr_vname) )
				 buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>%s</retmsg>",errMsg_ok);
			else buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>%s</retmsg>",errMsg_failed);
		}//?if(ptr_req && ptr_req[0]!=0)
		vidccsets.xml_list_vidcc(buffer);
	}else if(strcasecmp(ptr_cmd,"vidccinfo")==0)
	{
		if(ptr_vname==NULL || !vidccsets.xml_info_vidcc(buffer,ptr_vname,maptype))
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>%s</retmsg>",errMsg_failed);
	}else if(strcasecmp(ptr_cmd,"connect")==0)
	{//连接指定的vidcs服务
		pvidcc=(ptr_vname)?vidccsets.GetVidcClient(ptr_vname,false):NULL;
		if(pvidcc){
			SOCKSRESULT sr=pvidcc->ConnectSvr();
			if(sr<=0){
				if(sr==SOCKSERR_VIDC_VER)
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>连接vIDCs失败,版本不匹配</retmsg>");
				else if(sr==SOCKSERR_VIDC_PSWD)
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>连接vIDCs失败,访问密码错误</retmsg>");
				else if(sr==SOCKSERR_VIDC_RESP)
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>连接vIDCs失败,响应超时</retmsg>");
				else if(sr==SOCKSERR_THREAD)
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>创建线程失败</retmsg>");
				else if(sr==SOCKSERR_CONN)
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>连接vIDCs失败,vIDCs地址不正确</retmsg>");
				else if(sr==SOCKSERR_TIMEOUT)
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>连接vIDCs失败,连接超时</retmsg>");
				else
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>连接vIDCs失败,err=%d</retmsg>",sr);
			}
			vidccsets.xml_info_vidcc(buffer,ptr_vname,maptype);
		}else buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>%</retmsg>",errMsg_failed);
	}else if(strcasecmp(ptr_cmd,"disconn")==0)
	{//和指定的vidcs服务断开
		pvidcc=(ptr_vname)?vidccsets.GetVidcClient(ptr_vname,false):NULL;
		if(pvidcc){
			pvidcc->DisConnSvr();
			vidccsets.xml_info_vidcc(buffer,ptr_vname,maptype);
		}else buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>%s</retmsg>",errMsg_failed);
	}else if(strcasecmp(ptr_cmd,"mapdel")==0)
	{
		pvidcc=(ptr_vname)?vidccsets.GetVidcClient(ptr_vname,false):NULL;
		if(pvidcc){
			const char *ptr_req=httpreq.Request("mapname");
			if(ptr_req) pvidcc->mapinfoDel(ptr_req);
			vidccsets.xml_info_vidcc(buffer,ptr_vname,maptype);
		}else buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>%s</retmsg>",errMsg_failed);
	}else if(strcasecmp(ptr_cmd,"mapmodi")==0)
	{
		pvidcc=(ptr_vname)?vidccsets.GetVidcClient(ptr_vname,false):NULL;
		if(pvidcc){
			const char *ptr,*ptr_req=httpreq.Request("mapname");
			if(ptr_req && ptr_req[0]!=0){
				mapInfo *pinfo=pvidcc->mapinfoGet(ptr_req,true);
				if(pinfo){
					pinfo->m_mapType=maptype;
					ptr_req=httpreq.Request("appsvr");
					if(ptr_req) pinfo->m_appsvr.assign(ptr_req);
					ptr_req=httpreq.Request("mport");
					if(ptr_req){
						int l=strlen(ptr_req);
						if(l>=4 && strcmp(ptr_req+l-4,"+ssl")==0)
							pinfo->m_ssltype=TCPSVR_SSLSVR;
						else if(l>=4 && strcmp(ptr_req+l-4,"-ssl")==0)
							pinfo->m_ssltype=SSLSVR_TCPSVR;
						else pinfo->m_ssltype=TCPSVR_TCPSVR;
						pinfo->m_mportBegin=atoi(ptr_req);
						if( (ptr=strchr(ptr_req,'-')) && *(ptr+1)!='s') //即不是-ssl
							pinfo->m_mportEnd=atoi(ptr+1);
						else pinfo->m_mportEnd=pinfo->m_mportBegin;
					}
					ptr_req=httpreq.Request("sslverify");
					if(pinfo->m_ssltype==TCPSVR_SSLSVR && ptr_req && strcmp(ptr_req,"1")==0)
						pinfo->m_sslverify=true;
					else pinfo->m_sslverify=false;
					
					ptr_req=httpreq.Request("maxconn");
					if(ptr_req && atol(ptr_req)>0) 
						pinfo->m_maxconn=atol(ptr_req);
					else pinfo->m_maxconn=0;
					ptr_req=httpreq.Request("maxratio");
					if(ptr_req && atol(ptr_req)>0) 
						pinfo->m_maxratio=atol(ptr_req);
					else pinfo->m_maxratio=0;

					//客户端验证证书
					pinfo->m_clicert="";
					pinfo->m_clikey="";
					pinfo->m_clikeypswd="";
					ptr_req=httpreq.Request("clicert");
					if(ptr_req){
						const char *ptr_cert=ptr_req;
						const char *ptr,*ptr_key=NULL,*ptr_pswd=NULL;
						if( (ptr=strchr(ptr_cert,',')) )
						{
							*(char *)ptr=0; ptr_key=ptr+1;
							if( (ptr=strchr(ptr_key,',')) )
							{
								*(char *)ptr=0; ptr_pswd=ptr+1;
							}
						}//?if( (ptr=strchr(ptr_cert,',')) )
						if(ptr_cert) pinfo->m_clicert.assign(ptr_cert);
						if(ptr_key) pinfo->m_clikey.assign(ptr_key);
						if(ptr_pswd) pinfo->m_clikeypswd.assign(ptr_pswd);
					}//?if(ptr_req)
					

					ptr_req=httpreq.Request("apptype");
					if(ptr_req && strcmp(ptr_req,"FTP")==0)
						pinfo->m_apptype=MPORTTYPE_FTP;
					else if(ptr_req && strcmp(ptr_req,"WWW")==0)
						pinfo->m_apptype=MPORTTYPE_WWW;
					else if(ptr_req && strcmp(ptr_req,"TCP")==0)
						pinfo->m_apptype=MPORTTYPE_TCP;
					else pinfo->m_apptype=MPORTTYPE_UNKNOW;
					ptr_req=httpreq.Request("appdesc");
					if(ptr_req) pinfo->m_appdesc.assign(ptr_req);
					ptr_req=httpreq.Request("autorun");
					if(ptr_req && atoi(ptr_req)==1)
						pinfo->m_bAutoMap=true;
					else pinfo->m_bAutoMap=false;
					
					ptr_req=httpreq.Request("svrtype");
					if(ptr_req) pinfo->m_proxyType=atoi(ptr_req);
					ptr_req=httpreq.Request("authuser");
					if(ptr_req) pinfo->m_proxyuser.assign(ptr_req);
					ptr_req=httpreq.Request("authpswd");
					if(ptr_req) pinfo->m_proxypswd.assign(ptr_req);
					ptr_req=httpreq.Request("bauth");
					if(ptr_req && atoi(ptr_req)==1)
						pinfo->m_proxyauth=true;
					else pinfo->m_proxyauth=false;
					if(pinfo->m_mapType==VIDC_MAPTYPE_PROXY){
						pinfo->m_appsvr="";
						pinfo->m_apptype=MPORTTYPE_TCP;
						pinfo->m_ssltype=TCPSVR_TCPSVR;
					}

					ptr_req=httpreq.Request("ipaddr");
					if(ptr_req) pinfo->m_ipRules.assign(ptr_req); else pinfo->m_ipRules="";
					ptr_req=httpreq.Request("access");
					if(ptr_req) pinfo->m_ipaccess=atoi(ptr_req);
				}//?pinfo
			}
			vidccsets.xml_info_vidcc(buffer,ptr_vname,maptype);
		}else buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>%s</retmsg>",errMsg_failed);
	}else if( strcasecmp(ptr_cmd,"maped")==0 || //映射指定的服务
			  strcasecmp(ptr_cmd,"unmap")==0 || //取消指定的映射
			  strcasecmp(ptr_cmd,"mapinfo")==0 )
	{
		pvidcc=(ptr_vname)?vidccsets.GetVidcClient(ptr_vname,false):NULL;
		if(pvidcc){
			const char *ptr,*ptr_req=httpreq.Request("mapname");
			mapInfo *pinfo=NULL;
			if(ptr_req) pinfo=pvidcc->mapinfoGet(ptr_req,false);
			if(pinfo){
				int sr=0;
				if(strcasecmp(ptr_cmd,"unmap")==0)
					sr=pvidcc->Unmap(ptr_req,pinfo);
				else if(strcasecmp(ptr_cmd,"maped")==0)
					sr=pvidcc->Mapped(ptr_req,pinfo);
				if(sr<0){ //有错误发生
					if(sr==SOCKSERR_VIDC_NAME)
						buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>指定的映射服务无效</retmsg>");
					else if(sr==SOCKSERR_VIDC_MEMO)
						buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>内存分配失败</retmsg>");
					else if(sr==SOCKSERR_VIDC_SURPPORT)
						buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>此版本暂时不支持此功能</retmsg>");
					else if(sr==SOCKSERR_VIDC_MAP)
						buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>映射失败，确信映射端口没被占用</retmsg>");
					else
						buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>服务端响应超时</retmsg>");
				}
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<mapinfo>");
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<mapname>%s</mapname>",ptr_req);
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<svrport>%d</svrport>",pinfo->m_mappedPort);
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<ifssl>%d</ifssl><ifsslv>%d</ifsslv>",
					((pinfo->m_ssltype==TCPSVR_SSLSVR)?1:0),((pinfo->m_mappedSSLv)?1:0) );
				
				if( (ptr=strchr(pinfo->m_appsvr.c_str(),',')) ){
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<appip>%s</appip><appport></appport>",pinfo->m_appsvr.c_str());
				}else if( (ptr=strchr(pinfo->m_appsvr.c_str(),':')) )
				{
					*(char *)ptr=0;
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<appip>%s</appip>",pinfo->m_appsvr.c_str());
					*(char *)ptr=':';
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<appport>%s</appport>",ptr+1);
				}else buffer.len()+=sprintf(buffer.str()+buffer.len(),"<appip>%s</appip><appport></appport>",pinfo->m_appsvr.c_str());

				if(pinfo->m_apptype==MPORTTYPE_FTP)
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<apptype>FTP</apptype>");
				else if(pinfo->m_apptype==MPORTTYPE_WWW)
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<apptype>WWW</apptype>");
				else if(pinfo->m_apptype==MPORTTYPE_TCP)
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<apptype>TCP</apptype>");
				else buffer.len()+=sprintf(buffer.str()+buffer.len(),"<apptype>UNK</apptype>");
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<appdesc>%s</appdesc>",pinfo->m_appdesc.c_str());
				
				//限制最大连接数
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<maxconn>%d</maxconn>",pinfo->m_maxconn);
				//限制最大带宽 kb/s
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<maxratio>%d</maxratio>",pinfo->m_maxratio);

				if(pinfo->m_mportBegin==pinfo->m_mportEnd)
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<mapport>%d</mapport>",pinfo->m_mportBegin);
				else buffer.len()+=sprintf(buffer.str()+buffer.len(),"<mapport>%d-%d</mapport>",pinfo->m_mportBegin,pinfo->m_mportEnd);
				
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<bindip>%s</bindip>",pinfo->m_bindLocalIP);
				if(pinfo->m_ssltype==TCPSVR_SSLSVR)
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<ssltype>+ssl</ssltype>");
				else if(pinfo->m_ssltype==SSLSVR_TCPSVR)
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<ssltype>-ssl</ssltype>");
				else buffer.len()+=sprintf(buffer.str()+buffer.len(),"<ssltype></ssltype>");
				
				if(pinfo->m_sslverify)
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<sslverify>1</sslverify>");
				
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<svrtype>%d</svrtype>",pinfo->m_proxyType);
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<bauth>%d</bauth>",((pinfo->m_proxyauth)?1:0) );
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<authuser>%s</authuser>",pinfo->m_proxyuser.c_str());
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<authpswd>%s</authpswd>",pinfo->m_proxypswd.c_str());

				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<autorun>%d</autorun>",((pinfo->m_bAutoMap)?1:0) );
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<ipfilter>");
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<access>%d</access>",pinfo->m_ipaccess);
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<ipaddr>%s</ipaddr>",pinfo->m_ipRules.c_str());
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"</ipfilter>");

				if(pinfo->m_clicert!="")
				{
					int l=pinfo->m_clicert.length()+pinfo->m_clikey.length()+pinfo->m_clikeypswd.length()+32;
					if(buffer.Space()>l)
					buffer.len()+=sprintf(buffer.str()+buffer.len(),"<clicert>%s,%s,%s</clicert>",
						pinfo->m_clicert.c_str(),pinfo->m_clikey.c_str(),pinfo->m_clikeypswd.c_str());
				}
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"</mapinfo>");
			}else buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>%s</retmsg>",errMsg_failed);
		}else buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>%s</retmsg>",errMsg_failed);
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

//vIDCs服务管理
bool webServer::httprsp_vidcsvr(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp)
{
	MyService *ptrService=MyService::GetService();
	vidcManager *pvidc=&ptrService->m_vidcManager;
	vidcServerEx &vidcsvr=pvidc->m_vidcsvr;
	const char *ptr_cmd=httpreq.Request("cmd");
	cBuffer buffer(1024);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<?xml version=\"1.0\" encoding=\"gb2312\" ?><xmlroot>");
	
	if(strcasecmp(ptr_cmd,"run")==0) //启动vIDCs服务
	{
		SOCKSRESULT sr=vidcsvr.Start();
		if(sr<=0)
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>启动vIDCs服务失败,err=%d</retmsg>",sr);
	}else if(strcasecmp(ptr_cmd,"stop")==0)
	{
		vidcsvr.Close();
	}else if(strcasecmp(ptr_cmd,"setting")==0)
	{//保存服务参数
		const char *ptr=httpreq.Request("svrport");
		if(ptr){ 
			if( (vidcsvr.m_svrport=atoi(ptr))<=0 ) vidcsvr.m_svrport=0;
		}
		ptr=httpreq.Request("bindip");
		if(ptr) vidcsvr.m_bindip.assign(ptr);
		ptr=httpreq.Request("autorun");
		if(ptr) vidcsvr.m_autorun=(atoi(ptr)==1)?true:false;
		ptr=httpreq.Request("bauth");
		if(ptr && atoi(ptr)==1) 
			vidcsvr.bAuthentication(true);
		else vidcsvr.bAuthentication(false);
		ptr=httpreq.Request("pswd");
		if(ptr) vidcsvr.accessPswd().assign(ptr);
		ptr=httpreq.Request("ipaccess");
		if(ptr) vidcsvr.m_ipaccess=atoi(ptr);
		ptr=httpreq.Request("ipaddr");
		if(ptr) vidcsvr.m_ipRules.assign(ptr);
	}
	
	//获取vIDCs服务的参数设置和状态----start---------------------------------------------
	struct tm * ltime=NULL; time_t t;
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vidcs_status>");
	if(vidcsvr.status()==SOCKS_LISTEN) //服务已运行
	{
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<status>%d</status>",vidcsvr.getLocalPort());
		t=vidcsvr.getStartTime(); ltime=localtime(&t);
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<starttime>%04d年%02d月%02d日 %02d:%02d:%02d</starttime>",
			(1900+ltime->tm_year), ltime->tm_mon+1, ltime->tm_mday,ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
	}
	else
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<status>0</status>");
	t=time(NULL); ltime=localtime(&t);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<curtime>%04d年%02d月%02d日 %02d:%02d:%02d</curtime>",
			(1900+ltime->tm_year), ltime->tm_mon+1, ltime->tm_mday,ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
	
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<svrport>%d</svrport>",vidcsvr.m_svrport);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<autorun>%d</autorun>",(vidcsvr.m_autorun)?1:0);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<bauth>%d</bauth>",(vidcsvr.bAuthentication())?1:0);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<pswd>%s</pswd>",vidcsvr.accessPswd().c_str());
	//绑定本机IP
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<bindip>%s</bindip><localip>",vidcsvr.m_bindip.c_str());
	std::vector<std::string> vec;//得到本机IP，返回得到本机IP的个数
	long n=socketBase::getLocalHostIP(vec);
	for(int i=0;i<n;i++) buffer.len()+=sprintf(buffer.str()+buffer.len(),"%s ",vec[i].c_str());
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"</localip>");
	
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<ipfilter>");
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<access>%d</access>",vidcsvr.m_ipaccess);
	if(buffer.Space()<(vidcsvr.m_ipRules.length()+64)) buffer.Resize(buffer.size()+(vidcsvr.m_ipRules.length()+64));
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<ipaddr>%s</ipaddr>",vidcsvr.m_ipRules.c_str());
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"</ipfilter>");
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"</vidcs_status>");
	//列出当前已连接的vIDCc客户端
	vidcsvr.xml_list_vidcc(buffer);
	//获取代理服务的参数设置和状态---- end ---------------------------------------------

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
//vIDCs服务已连接vIDCc管理
bool webServer::httprsp_vidccs(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp)
{
	MyService *ptrService=MyService::GetService();
	vidcManager *pvidc=&ptrService->m_vidcManager;
	vidcServerEx &vidcsvr=pvidc->m_vidcsvr;
	const char *ptr_cmd=httpreq.Request("vid");
	long vidccID=(ptr_cmd)?atol(ptr_cmd):0;
	ptr_cmd=httpreq.Request("cmd");
	cBuffer buffer(1024);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<?xml version=\"1.0\" encoding=\"gb2312\" ?><xmlroot>");

	if(strcasecmp(ptr_cmd,"info")==0) //获取指定的连接vIDCc的信息
	{
		vidcsvr.xml_info_vidcc(buffer,vidccID);
	}
	else if(strcasecmp(ptr_cmd,"dele")==0) //强制断开某vIDCc和本vIDCs服务的连接
	{
		vidcsvr.DisConnect(vidccID);
		vidcsvr.xml_list_vidcc(buffer);
	}
	else if(strcasecmp(ptr_cmd,"sets")==0) //设置对某个vidcc映射的服务进行日志记录
	{
		const char *ptr=httpreq.Request("blogd");
		bool b=(ptr && atoi(ptr)==1)?true:false;
		vidcsvr.setLogdatafile(vidccID,b);
		vidcsvr.xml_info_vidcc(buffer,vidccID);
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

//vidc配置的导入导出
bool webServer::httprsp_vidcini(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp)
{
	MyService *ptrService=MyService::GetService();
	vidcManager *pvidc=&ptrService->m_vidcManager;
	const char *ptr_cmd=httpreq.Request("cmd");
	cBuffer buffer(512);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<?xml version=\"1.0\" encoding=\"gb2312\" ?><xmlroot>");

	if(strcasecmp(ptr_cmd,"out")==0) //导出vidc配置
	{
		std::string strini;
		pvidc->saveAsstring(strini);
		if(buffer.Space()<(strini.length()+64)) buffer.Resize(buffer.size()+(strini.length()+64));
		if(buffer.str())
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<settings><![CDATA[%s]]></settings>",strini.c_str());
	}
	else if(strcasecmp(ptr_cmd,"in")==0) //导入vidc配置
	{
		const char *ptr=httpreq.Request("ini");
		if(ptr){
			pvidc->initSetting();
			pvidc->parseIni((char *)ptr,0);
			pvidc->saveIni(); //保存vidc配置参数
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

