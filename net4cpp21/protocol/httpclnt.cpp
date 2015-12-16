/*******************************************************************
   *	httpclnt.cpp
   *    DESCRIPTION:HTTP协议客户端实现
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    LAST MODIFY DATE:2006-02-08
   *
   *	net4cpp 2.1
   *	HTTP/1.1
   *******************************************************************/

#include "../include/sysconfig.h"
#include "../include/httpclnt.h"
#include "../include/cLogger.h"

using namespace std;
using namespace net4cpp21;

//发送http请求 lstartRange,lendRange告诉web服务请求文件的范围
//strurl格式 http[s]://[username:password@]host[:port]/...
//例如 http://127.0.0.1
//	   https://aa:bb@17.0.0.1:81/aa.htm
//成功返回SOCKSERR_OK
SOCKSRESULT httpClient::send_httpreq_headX(const char *strurl,long lTimeOut,long lstartRange,long lendRange)
{//发送http响应头，不等待返回
	if(strurl==NULL) return SOCKSERR_PARAM;
	while(*strurl==' ') strurl++;//删除前导空格
	if(strncasecmp(strurl,"http://",7) && strncasecmp(strurl,"https://",8) ) return SOCKSERR_PARAM;

	std::string webhost,struser,strpswd;
	int webport=(strurl[4]==':')?HTTP_SERVER_PORT:HTTPS_SERVER_PORT;
	int iOffset=(strurl[4]==':')?7:8;
	const char *ptrURL=strchr(strurl+iOffset,'/');
	if(ptrURL) *(char *)ptrURL=0;
	//先尝试分离访问帐号和密码
	const char *ptr1,*ptr=strchr(strurl+iOffset,'@');
	if(ptr){//设置了访问帐号和密码
		*(char *)ptr=0;
		if( (ptr1=strchr(strurl+iOffset,':')) )
		{
			*(char *)ptr1=0;
			struser.assign(strurl+iOffset);
			strpswd.assign(ptr1+1);
			*(char *)ptr1=':';
		}else struser.assign(strurl+iOffset);
		*(char *)ptr='@';
		iOffset=ptr-strurl+1;
	}//?if(ptr)
	//分离出http服务的地址:端口和实际的URL
	if( (ptr=strchr(strurl+iOffset,':')) )
	{ 
		webport=atoi(ptr+1);
		webhost.assign(strurl+iOffset,ptr-strurl-iOffset);
	}else webhost.assign(strurl+iOffset);
	if(ptrURL) *(char *)ptrURL='/';
	
	m_httpreq.set_requestRange(lstartRange,lendRange);
	if(struser!="") m_httpreq.set_Authorization(struser.c_str(),strpswd.c_str());
	std::map<std::string,std::string> &header=m_httpreq.Header();
	header["Host"]=webhost; //设置Host信息
	//要进行socket连接,连接指定的web服务
	SOCKSRESULT sr=this->Connect(webhost.c_str(),webport,lTimeOut);
	if(sr<=0)
	{
		RW_LOG_DEBUG("[httpclnt] Failed to connect HTTP server(%s:%d),error=%d\r\n",webhost.c_str(),webport,sr);
		return SOCKSERR_CONN;
	}
#ifdef _SURPPORT_OPENSSL_
	else if(strurl[4]!=':')
	{ //访问的是http SSL服务
		if(!this->ifSSL()) this->initSSL(false,NULL); //初始化SSL客户端
		if(!this->SSL_Associate()){ this->Close(); return SOCKSERR_SSLASSCIATE; }
	}
#endif
	//发送请求
	return (ptrURL==NULL)?m_httpreq.send_req(this,"/"):m_httpreq.send_req(this,ptrURL);
}

//返回http响应码，如果发生错误返回<0
SOCKSRESULT httpClient::send_httpreq(const char *strurl,long lstartRange,long lendRange)
{
	SOCKSRESULT sr=send_httpreq_head(strurl,lstartRange,lendRange);
	if(sr!=SOCKSERR_OK) return SOCKSERR_HTTP_SENDREQ;
	return m_httprsp.recv_rspH(this);
}

//添加http请求的其他相关数据
void httpClient::add_reqHeader(const char *szname,const char *szvalue)
{
	if(szname==NULL) return;
	std::map<std::string,std::string> &header=m_httpreq.Header();
	std::string val; if(szvalue) val.assign(szvalue);
	header[szname]=val; return;
}
void httpClient::add_reqCookie(const char *szname,const char *szvalue)
{
	if(szname==NULL) return;
	std::map<std::string,std::string> &cookies=m_httpreq.Cookies();
	std::string val; if(szvalue) val.assign(szvalue);
	cookies[szname]=val; return;
}
void httpClient::add_reqPostdata(const char *szname,const char *szvalue)
{
	if(szname==NULL) return;
	std::map<std::string,std::string> &form=m_httpreq.Form();
	std::string val; if(szvalue) val.assign(szvalue);
	form[szname]=val; return;
}

void httpClient::set_reqPostdata(const char *buf,long buflen)
{
	m_httpreq.SetPostData(buf,buflen);
}

/*
//返回http响应码，如果发生错误返回<0
SOCKSRESULT httpClient::send_httpreq(const char *strurl,long lstartRange,long lendRange)
{
	if(strurl==NULL) return SOCKSERR_PARAM;
	while(*strurl==' ') strurl++;//删除前导空格
	if(strncasecmp(strurl,"http://",7) && strncasecmp(strurl,"https://",8) ) return SOCKSERR_PARAM;

	std::string webhost,struser,strpswd;
	int webport=(strurl[4]==':')?HTTP_SERVER_PORT:HTTPS_SERVER_PORT;
	int iOffset=(strurl[4]==':')?7:8;
	const char *ptrURL=strchr(strurl+iOffset,'/');
	if(ptrURL) *(char *)ptrURL=0;
	//先尝试分离访问帐号和密码
	const char *ptr1,*ptr=strchr(strurl+iOffset,'@');
	if(ptr){//设置了访问帐号和密码
		*(char *)ptr=0;
		if( (ptr1=strchr(strurl+iOffset,':')) )
		{
			*(char *)ptr1=0;
			struser.assign(strurl+iOffset);
			strpswd.assign(ptr1+1);
			*(char *)ptr1=':';
		}else struser.assign(strurl+iOffset);
		*(char *)ptr='@';
		iOffset=ptr-strurl+1;
	}//?if(ptr)
	//分离出http服务的地址:端口和实际的URL
	if( (ptr=strchr(strurl+iOffset,':')) )
	{ 
		webport=atoi(ptr+1);
		webhost.assign(strurl+iOffset,ptr-strurl-iOffset);
	}else webhost.assign(strurl+iOffset);
	if(ptrURL) *(char *)ptrURL='/';
	
	m_httpreq.set_requestRange(lstartRange,lendRange);
	if(struser!="") m_httpreq.set_Authorization(struser.c_str(),strpswd.c_str());
	std::map<std::string,std::string> &header=m_httpreq.Header();
	header["Host"]=webhost; //设置Host信息
	//要进行socket连接,连接指定的web服务
	SOCKSRESULT sr=this->Connect(webhost.c_str(),webport);
	if(sr<=0)
	{
		RW_LOG_DEBUG("[httpclnt] Failed to connect HTTP server(%s:%d),error=%d\r\n",webhost.c_str(),webport,sr);
		return SOCKSERR_CONN;
	}
#ifdef _SURPPORT_OPENSSL_
	else if(strurl[4]!=':')
	{ //访问的是http SSL服务
		if(!this->ifSSL()) this->initSSL(false,NULL); //初始化SSL客户端
		if(!this->SSL_Associate()){ this->Close(); return SOCKSERR_SSLASSCIATE; }
	}
#endif
	//发送请求
	sr=(ptrURL==NULL)?m_httpreq.send_req(this,"/"):m_httpreq.send_req(this,ptrURL);
	if(sr!=SOCKSERR_OK) return SOCKSERR_HTTP_SENDREQ;
	return m_httprsp.recv_rspH(this);
}
*/

