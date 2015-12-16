/*******************************************************************
   *	upnp.cpp
   *    DESCRIPTION:upnp 类实现
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *	net4cpp 2.1
   *******************************************************************/
#include "../include/sysconfig.h"
#include "../include/upnp.h"
#include "../include/httpclnt.h"
#include "../include/cLogger.h"

using namespace std;
using namespace net4cpp21;

static char *UPNP_SEARCH_NAME[] = 
{
	"urn:schemas-upnp-org:service:WANPPPConnection:1",  //以服务名进行查找
	"urn:schemas-upnp-org:service:WANIPConnection:1",
	"urn:schemas-upnp-org:device:InternetGatewayDevice:1" //以设备名进行查找
}; //因为有些设备支持以服务名查找，有些支持设备名查找
bool GetProperty(const char *xml,const char *name,string &strret);

upnp :: upnp()
{
	m_bFound=false;
}

upnp :: ~upnp()
{
	Clear();
}

void upnp :: Clear()
{
	std::vector<UPnPInfo *> ::iterator it=m_upnpsets.begin();
	for(;it!=m_upnpsets.end();it++){
		UPnPInfo *p=*it;
		if( m_bFound && p->bsuccess) DeletePortMapping(*p);	
		delete p;
	}
	m_upnpsets.clear();
}

bool upnp :: Search()
{
	int UPNP_SEARCH_SIZE=sizeof(UPNP_SEARCH_NAME)/sizeof(UPNP_SEARCH_NAME[0]);
	unsigned long target_addr=socketBase::Host2IP(UPnP_MULTI_ADDR);
	int target_port=UPnP_MULTI_PORT;
	
	m_bFound=false;
	if(this->status()!=SOCKS_OPENED)
		this->Open(0,false,NULL);
	if(this->status()!=SOCKS_OPENED){
		RW_LOG_DEBUG(0,"[UPnP] Failed to Open UDP socket\r\n");
		return false;
	}
	
	char packet[256]; //组播消息
	int l,len=sprintf(packet, "M-SEARCH * HTTP/1.1\r\n"
							"HOST: %s:%d\r\n"
							"MAN: \"ssdp:discover\"\r\n"
							"MX: 6\r\n",UPnP_MULTI_ADDR,UPnP_MULTI_PORT);
	for(int i=0; i<UPNP_SEARCH_SIZE;i++)
	{
		l=sprintf(packet+len,"ST: %s\r\n\r\n",UPNP_SEARCH_NAME[i]);
		this->SetRemoteInfo(target_addr,target_port);
		SOCKSRESULT sr=this->Send(len+l,packet,-1);
		RW_LOG_DEBUG("[UPnP] Sended Search Packet(len=%d), return %d\r\n%s",(len+l),sr,packet);
	}

	return false;
}

void upnp :: onData()
{
	char buf[UPnP_MAX_MESAGE_SIZE]; 
	int buflen=this->Receive(buf,UPnP_MAX_MESAGE_SIZE-1,-1);
	if(buflen<=0) return; else buf[buflen]=0;
	RW_LOG_DEBUG("[UPnP] Received response ,len=%d\r\n%s",buflen,buf);
	
	// D-Link 504 竟然不是 HTTP/*.* 而是 HTTP*.*
	UINT32 maj_ver, min_ver, status_code;
	if (::sscanf(buf, "HTTP/%u.%u %u", &maj_ver, &min_ver, &status_code) != 3 &&
		::sscanf(buf, "HTTP%u.%u %u", &maj_ver, &min_ver, &status_code) != 3)
		return;
	if (status_code != HTTP_STATUS_OK) return;
	
	const char *ptr_beg=strstr(buf,"\r\nLOCATION:");
	if(ptr_beg==NULL) ptr_beg=strstr(buf,"\r\nLocation:");
	if(ptr_beg==NULL) ptr_beg=strstr(buf,"\r\nlocation:");
	if(ptr_beg==NULL) return; else ptr_beg+=11;
	while( *ptr_beg==' ') ptr_beg++; //去掉空格
	const char *ptr_end=strchr(ptr_beg,'\r');
	if(ptr_end==NULL) return;
	m_strLocation.assign(ptr_beg,ptr_end-ptr_beg);
	RW_LOG_DEBUG("[UPnP] Found Loaction: %s\r\n",m_strLocation.c_str());
	//获取ST的值
	if( (ptr_beg=strstr(buf,"\r\nST:")) ){
		ptr_beg+=5;
		while( *ptr_beg==' ') ptr_beg++; //去掉空格
		if( (ptr_end=strchr(ptr_beg,'\r')) )
			m_targetName.assign(ptr_beg,ptr_end-ptr_beg);
	}else m_targetName="";
	
	std::string strXml; //获取设备描述xml
	if(!GetDevXML(strXml)) return;
	//获取控制url地址
	string sverviceType=string("<serviceType>")+m_targetName+string("</serviceType>");
	ptr_beg=strstr(strXml.c_str(),sverviceType.c_str());
	if(ptr_beg){
		ptr_beg+=sverviceType.length();
		if( (ptr_end=strstr(ptr_beg,"</service>")) )
		{
			*(char *)ptr_end=0; m_control_url="";
			GetProperty(ptr_beg,"controlURL",m_control_url);
			*(char *)ptr_end='<';
		}
	}//?if(ptr_beg)
	if(m_control_url=="") return;
	if( strncasecmp(m_control_url.c_str(),"http://",7)!=0 && 
		strncasecmp(m_control_url.c_str(),"https://",8)!=0 )
	{
		string base_url;
		GetProperty(strXml.c_str(),"URLBase",base_url);
		if(base_url==""){
			const char *ptr=(m_strLocation[4]==':')?
				strchr(m_strLocation.c_str()+7,'/'):  //+7 跳过http://
				strchr(m_strLocation.c_str()+8,'/');
			if(ptr) base_url.assign(m_strLocation.c_str(),ptr-m_strLocation.c_str());
		}
		if(base_url[base_url.length()-1]=='/') base_url.erase(base_url.length()-1);
		if(m_control_url[0]=='/')
			 m_control_url=base_url+m_control_url;
		else m_control_url=base_url+string("/")+m_control_url;
	}
	GetProperty(strXml.c_str(),"friendlyName",m_friendlyName);
	GetProperty(strXml.c_str(),"manufacturer",m_manufacturer);
	this->Close(); m_bFound=true; //成功
	RW_LOG_DEBUG("[UPnP] Success to search UPnP %s\r\n",m_targetName.c_str());
	RW_LOG_DEBUG("[UPnP] controlURL: %s\r\n",m_control_url.c_str());
	RW_LOG_DEBUG("[UPnP] friendlyName: %s\r\n",m_friendlyName.c_str());
	RW_LOG_DEBUG("[UPnP] manufacturer: %s\r\n",m_manufacturer.c_str());
	
	//查找到UPnP设备后，进行映射
	std::vector<UPnPInfo *> ::iterator it=m_upnpsets.begin();
	for(;it!=m_upnpsets.end();it++){
		UPnPInfo *p=*it;
		p->bsuccess=AddPortMapping(*p);
	}
	return;
}

bool upnp :: GetDevXML(std::string &strXml)
{
	//获取Location指定的xml文件
	httpClient httpsock; int iret;
TRANS302:
	httpsock.cls_httpreq();
	iret=httpsock.send_httpreq(m_strLocation.c_str());
	if(iret==302) //转向
	{
		httpResponse & resp=httpsock.Response();
		const char *ptr=resp.Header("Location");
		RW_LOG_DEBUG("[UPnP] Transfer to %s.\r\n",ptr);
		if(ptr==NULL) return false;
		m_strLocation.assign(ptr); goto TRANS302;
	}
	else if(iret==200) //响应成功
	{
		httpResponse & resp=httpsock.Response();
		resp.recv_remainder(&httpsock,-1);//接收完整的http响应体
//		RW_LOG_DEBUG("[UPnP] Receive XML: %d / %d\r\n%s\r\n",
//				resp.lReceivedContent(),resp.lContentLength(),resp.szReceivedContent());
		if(!resp.ifReceivedAll() || resp.lContentLength()==0 ) return false;//未接收完
//		if(resp.get_mimetype()!=MIMETYPE_XML) return false;
		
		strXml.assign(resp.szReceivedContent(),resp.lReceivedContent());
		return true;
	}
	return false;
}
//获取公网IP地址
bool upnp :: GetWanIP(std::string &strRet)
{
	std::string reqName("GetExternalIPAddress");
	strRet.assign("NewExternalIPAddress");
	return invoke_property(reqName,strRet);
}

bool upnp :: AddPortMapping(bool bTCP,const char *internalIP,int internalPort,int externPort,const char *desc)
{
	UPnPInfo *p=new UPnPInfo;
	if(p==NULL) return false;

	p->budp=!bTCP;
	p->appsvr.assign(internalIP);
	p->appport=internalPort;
	p->mapport=externPort;
	if(desc) p->appdesc.assign(desc);
	p->retmsg=(!m_bFound)?"Not find UPnP":""; 
	m_upnpsets.push_back(p);	
	p->bsuccess=(!m_bFound)?false:AddPortMapping(*p);
	
	return p->bsuccess;
}

bool upnp :: DeletePortMapping(bool bTCP,int externPort)
{
	std::vector<UPnPInfo *> ::iterator it=m_upnpsets.begin();
	for(;it!=m_upnpsets.end();it++){
		UPnPInfo *p=*it;
		if(!p->budp==bTCP && p->mapport==externPort) break;
	}
	if(it==m_upnpsets.end()) return false;
	if((*it)->bsuccess && m_bFound)
		DeletePortMapping(*(*it));
	m_upnpsets.erase(it);
	return true;
}


bool upnp :: AddPortMapping(UPnPInfo &info) 
{
	if(info.mapport<=0) info.mapport=info.appport;
	if(info.appport<=0){
		info.retmsg="invalid internal port";
		return false;
	} 
	unsigned long ipaddr=socketBase::Host2IP(info.appsvr.c_str());
	if(ipaddr==INADDR_NONE){
		info.retmsg="invalid internal IP";
		return false;
	} 
	if( ipaddr==socketBase::Host2IP("127.0.0.1") )
		ipaddr=socketBase::Host2IP( socketBase::getLocalHostIP());


	std::string strCmd="AddPortMapping";
	std::map<std::string,std::string> strArgs;
	char buf[16];

	strArgs["NewRemoteHost"]="";
	strArgs["NewLeaseDuration"]="0"; 
	strArgs["NewEnabled"]="1";

	strArgs["NewProtocol"]=(!info.budp)?"TCP":"UDP";
	sprintf(buf,"%d",info.appport);
	strArgs["NewInternalPort"]=buf;
	sprintf(buf,"%d",info.mapport);
	strArgs["NewExternalPort"]=buf; ;
	strArgs["NewInternalClient"]=socketBase::IP2A(ipaddr);
	strArgs["NewPortMappingDescription"]=info.appdesc;
	
	info.bsuccess=invoke_command(strCmd,strArgs);
	info.retmsg=strCmd;
	return info.bsuccess;
}

bool upnp :: DeletePortMapping(UPnPInfo &info)
{
	std::string strCmd="DeletePortMapping";
	std::map<std::string,std::string> strArgs;
	
	strArgs["NewRemoteHost"]="";
	strArgs["NewProtocol"]=(!info.budp)?"TCP":"UDP";
	char buf[16]; sprintf(buf,"%d",info.mapport);
	strArgs["NewExternalPort"]=buf; ;

	return invoke_command(strCmd,strArgs);
}

/////////////////////////////////////////////////////
// invoke_command
//
// request packet:
//		POST url  HTTP/1.1
//		HOST: ip:port
//		Content-Length: length
//		Content-Type: text/xml; charset="utf-8"
//		SOAPACTION: "urn:schemas-upnp-org:service:serviceType:v#actionName"
//
//		<s:Envelope
//         xmlns:s="http://schemas.xmlsoap.org/soap/envelope/"
//		   s:encodingStyle="http://schemas.xmlsoap.org/soap/encoding/">
//		     <s:Body>
//		        <u:actionName xmlns:u="urn:schemas-upnp-org:service:serviceType:v">
//                   <argumentName>argumentValue</argumentName>
//              </u:actionName>
//		     </s:Body>
//		</s:Envelope>
//
// respond packet:
//     (retrun a HTTP packet with XML data)
//
bool upnp :: invoke_command(std::string &strCmd,std::map<std::string,std::string> &strArgs)
{
	if(!m_bFound || m_control_url=="") return false;

	string xml_data;
	xml_data   = string("<s:Envelope\r\n    xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"\r\n    ");
	xml_data  += string("s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n");
	xml_data  += string("  <s:Body>\r\n    <u:")+strCmd;
	xml_data  += string(" xmlns:u=\"")+m_targetName+string("\">\r\n");
	
	std::map<std::string,std::string>::iterator it=strArgs.begin();
	for(;it!=strArgs.end();it++)
	{
		xml_data  += string("<")+(*it).first+string(">");
		xml_data  += (*it).second;
		xml_data  += string("</")+(*it).first+string(">\r\n");
	}

	xml_data  += string("    </u:")+strCmd;
	xml_data  += ">\r\n  </s:Body>\r\n</s:Envelope>\r\n\r\n";
	
	//发送http请求
	httpClient httpsock; 
	httpsock.add_reqHeader("Content-Type","text/xml");
	string s=m_targetName+string("#")+strCmd;
	httpsock.add_reqHeader("SOAPACTION",s.c_str());
	httpsock.set_reqPostdata(xml_data.c_str(),xml_data.length());
	SOCKSRESULT sr=httpsock.send_httpreq(m_control_url.c_str());

	httpResponse & resp=httpsock.Response();
	resp.recv_remainder(&httpsock,-1);//接收完整的http响应体
	RW_LOG_DEBUG("[UPnP] Receive XML: %d / %d\r\n%s\r\n",
		resp.lReceivedContent(),resp.lContentLength(),resp.szReceivedContent());
//	if(!resp.ifReceivedAll() || resp.lContentLength()==0 ) return false;//未接收完

	strCmd.assign(resp.szReceivedContent(),resp.lReceivedContent());
	return (sr==200);
}
//属性获取 ,reqName : 请求属性名  rspName: 响应名称
bool upnp :: invoke_property(std::string &reqName,std::string &rspName)
{
	if(!m_bFound || m_control_url=="") return false;

	string xml_data;
	xml_data   = string("<s:Envelope\r\n    xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\"\r\n    ");
	xml_data  += string("s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">\r\n");
	xml_data  += string("  <s:Body>\r\n    <u:")+reqName;
	xml_data  += string(" xmlns:u=\"")+m_targetName+string("\">\r\n");
	xml_data  += string("    </u:")+reqName;
	xml_data  += ">\r\n  </s:Body>\r\n</s:Envelope>\r\n\r\n";
	
	//发送http请求
	httpClient httpsock; 
	httpsock.add_reqHeader("Content-Type","text/xml");
	string s=m_targetName+string("#")+reqName;
	httpsock.add_reqHeader("SOAPACTION",s.c_str());
	httpsock.set_reqPostdata(xml_data.c_str(),xml_data.length());
	SOCKSRESULT sr=httpsock.send_httpreq(m_control_url.c_str());
	if(sr!=200) return false;
	httpResponse & resp=httpsock.Response();
	resp.recv_remainder(&httpsock,-1);//接收完整的http响应体
	RW_LOG_DEBUG("[UPnP] Receive XML: %d / %d\r\n%s\r\n",
		resp.lReceivedContent(),resp.lContentLength(),resp.szReceivedContent());
	if(!resp.ifReceivedAll() || resp.lContentLength()==0 ) return false;//未接收完
	
	return GetProperty(resp.szReceivedContent(),rspName.c_str(),rspName);
}

//-----------------------private function----------------------------------
bool GetProperty(const char *xml,const char *name,string &strret)
{
	string name_beg=string("<")+string(name);
	string name_end=string("</")+string(name);
	const char *ptr,*ptr_beg=strstr(xml,name_beg.c_str());
	if(ptr_beg==NULL) return false; else ptr_beg+=name_beg.length();
	if( (ptr=strchr(ptr_beg,'>'))==NULL ) return false;
	while( ptr_beg<ptr){ if(*ptr_beg++!=' ') return false; }
	ptr_beg=ptr+1;

	const char *ptr_end=strstr(ptr_beg,name_end.c_str());
	if(ptr_end==NULL) return false; 
	strret.assign(ptr_beg,ptr_end-ptr_beg);
	return true;
}

