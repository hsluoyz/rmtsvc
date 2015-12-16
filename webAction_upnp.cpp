/*******************************************************************
   *	webAction_upnp.cpp web请求处理 - upnp管理
   *    DESCRIPTION:
   *
   *    AUTHOR:yyc
   *
   *    HISTORY:
   *
   *    DATE:
   *	
   *******************************************************************/
#include "rmtsvc.h"


bool webServer::httprsp_upnp(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp)
{
	MyService *ptrService=MyService::GetService();
	vidcManager *pvidc=&ptrService->m_vidcManager;
	upnp &u=pvidc->m_upnp;
	const char *ptr_cmd=httpreq.Request("cmd");
	
	cBuffer buffer(1024);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<?xml version=\"1.0\" encoding=\"gb2312\" ?><xmlroot>");
	
	if(strcasecmp(ptr_cmd,"find")==0)
	{ //查找UPnP设备
		u.Search(); Sleep(2000);
	}
	else if(strcasecmp(ptr_cmd,"stop")==0)
		u.Close();
	else if(strcasecmp(ptr_cmd,"add")==0)
	{
		const char *ptr_param=httpreq.Request("param");
		if(strncasecmp(ptr_param,"upnp ",5)==0)
			pvidc->docmd_upnp(ptr_param+5);
	}
	else if(strcasecmp(ptr_cmd,"del")==0)
	{
		bool btcp=true; int wport=0;
		const char *ptr_param=httpreq.Request("tcp");
		if(ptr_param && atoi(ptr_param)==0) btcp=false;
		if( (ptr_param=httpreq.Request("wport")) ) wport=atoi(ptr_param);
		u.DeletePortMapping(btcp,wport);
	}

	if(u.status()==SOCKS_OPENED)
		 buffer.len()+=sprintf(buffer.str()+buffer.len(),"<status>1</status>");
	else buffer.len()+=sprintf(buffer.str()+buffer.len(),"<status>0</status>");
	//返回UPnP设备情况
	if(u.bFound()){
		std::string strIP; u.GetWanIP(strIP);
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<wanip>%s</wanip>",strIP.c_str());
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<devname><![CDATA[%s]]></devname>",u.name());
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<manufacturer><![CDATA[%s]]></manufacturer>",u.manufacturer());

	}else{
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<wanip></wanip><devname></devname><manufacturer></manufacturer>");
	}
	
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<upnplist>");
	std::vector<UPnPInfo *> &upnpsets=u.upnpinfo();
	std::vector<UPnPInfo *> ::iterator it_upnpsets=upnpsets.begin();
	for(;it_upnpsets!=upnpsets.end();it_upnpsets++){	
		int l=(*it_upnpsets)->appdesc.length()+(*it_upnpsets)->retmsg.length();
		if(buffer.Space()<256+l) buffer.Resize(buffer.size()+256+l);
		if(buffer.str()==NULL) break;
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<upnpinfo>");
		UPnPInfo *p=*it_upnpsets;
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<ptype>%s</ptype>", ((p->budp)?"UDP":"TCP") );
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<wport>%d</wport>", p->mapport );
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<lport>%d</lport>", p->appport );
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<laddr>%s</laddr>", p->appsvr.c_str() );
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<result>%s</result>", ((p->bsuccess)?"成功":"失败") );
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<desc><![CDATA[%s]]></desc>", p->appdesc.c_str() );
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"<error><![CDATA[%s]]></error>", p->retmsg.c_str());
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"</upnpinfo>");
	}
	if(buffer.Space()<32) buffer.Resize(buffer.size()+32);
	if(buffer.str()) 
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"</upnplist></xmlroot>");
	
	httprsp.NoCache();//CacheControl("No-cache");
	//设置MIME类型，默认为HTML
	httprsp.set_mimetype(MIMETYPE_XML);
	//设置响应内容长度
	httprsp.lContentLength(buffer.len());
	httprsp.send_rspH(psock,200,"OK");
	
	if(buffer.str()) psock->Send(buffer.len(),buffer.str(),-1);
	return true;
}

//获取upnp设备的xml信息
bool webServer::httprsp_upnpxml(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp)
{
	MyService *ptrService=MyService::GetService();
	vidcManager *pvidc=&ptrService->m_vidcManager;
	upnp &u=pvidc->m_upnp;
	std::string strXml;
	u.GetDevXML(strXml);

	httprsp.NoCache();//CacheControl("No-cache");
	//设置MIME类型，默认为HTML
	httprsp.set_mimetype(MIMETYPE_XML);
	//设置响应内容长度
	httprsp.lContentLength(strXml.length());
	httprsp.send_rspH(psock,200,"OK");
	
	psock->Send(strXml.length(),strXml.c_str(),-1);
	return true;
}
