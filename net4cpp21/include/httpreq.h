/*******************************************************************
   *	httpreq.h
   *    DESCRIPTION:HTTP 请求解析对象
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2006-02-08
   *
   *	net4cpp 2.1
   *	
   *******************************************************************/

#ifndef __YY_HTTP_REQUEST_H__
#define __YY_HTTP_REQUEST_H__

#include "httpdef.h"
#include "Buffer.h"
#include "socketTcp.h"

#include <map>

namespace net4cpp21
{
	class httpRequest 
	{
	public:
		httpRequest();
		~httpRequest(){}
		HTTPREQ_TYPE get_reqType() const { return m_httpreq_iType; }
		DWORD get_httpVer() const { return m_httpreq_dwVer; }
		std::string& url() { return m_httpreq_strUrl; }
		bool ifReceivedAll() const { return m_httpreq_bReceiveALL; }
		bool bKeepAlive();
		time_t IfModifiedSince();
		void set_requestRange(long lstartpos,long lendpos);
		//可能会有多个范围,返回范围个数
		int get_requestRange(long *lpstartpos,long *lpendpos,int idx=0);
		void set_Authorization(const char *user,const char *pswd);
		HTTPAUTH_TYPE get_Authorization(std::string &user,std::string &pswd);
		//设置POST数据类型
		void set_contentType(HTTPREQ_CONTENT_TYPE itype,const char *lpBoundary);
		HTTPREQ_CONTENT_TYPE get_contentType(std::string *strBoundary);
		const char * get_contentCharset();
		long get_contentLen() const { return m_httpreq_lContentlen; }
		//返回提交数据指针，仅仅对非Form数据有意义
		cBuffer &get_contentData() { return m_httpreq_postdata; }
		void ifParseParams(bool b) { m_bParseParams=b; } //设置是否解析提交参数，用于HTTP 代理服务

		//获取http请求数据
		const char *Request(const char *reqname){
			if(reqname==NULL) return NULL;

			std::map<std::string,std::string>::iterator it=
				m_httpreq_params_GET.find(reqname);
			if(it!=m_httpreq_params_GET.end())
				return (*it).second.c_str();
			it=m_httpreq_params_POST.find(reqname);
			return (it!=m_httpreq_params_POST.end())?
				((*it).second.c_str()):NULL;
		}
		const char *Cookies(const char *cookiename)
		{
			std::map<std::string,std::string>::iterator it=
				(cookiename)?m_httpreq_COOKIE.find(cookiename):m_httpreq_COOKIE.end();
			return (it!=m_httpreq_COOKIE.end())?((*it).second.c_str()):NULL;
		}
		const char *Header(const char *pheader)
		{//获取指定的http请求头
			std::map<std::string,std::string>::iterator it=
				(pheader)?m_httpreq_HEADER.find(pheader):m_httpreq_HEADER.end();
			return (it!=m_httpreq_HEADER.end())?((*it).second.c_str()):NULL;
		}

		//---------------------------------------------------------------
		void init_httpreq(bool ifKeepHeader=false);//初始化http请求准备发送新的http请求
		std::map<std::string,std::string> &QueryString() { return m_httpreq_params_GET;}
		std::map<std::string,std::string> &Form() { return m_httpreq_params_POST;}
		std::map<std::string,std::string> &Cookies() { return m_httpreq_COOKIE; }
		std::map<std::string,std::string> &Header() { return m_httpreq_HEADER; }
	
		//编码http请求并发送,成功返回SOCKSERR_OK
		SOCKSRESULT send_req(socketTCP *psock,const char *lpszurl);
		void SetPostData(const char *buf,long buflen) //设置要发送的POST数据
		{
			m_httpreq_params_POST.clear(); //此时POST Param无效
			m_httpreq_postdata.len()=0; //清空原有的数据
			m_httpreq_postdata.Resize(buflen+1);
			if(m_httpreq_postdata.str()==NULL) return; 
			::memcpy(m_httpreq_postdata.str(),buf,buflen);
			m_httpreq_postdata.len()=buflen;
		}
		const char * encodeReqestH(unsigned long &lret)
		{
			SOCKSRESULT sr=send_req(NULL,NULL);
			if(sr!=SOCKSERR_OK) return NULL;
			lret=m_httpreq_postdata.len();
			return m_httpreq_postdata.str();
		}
		//--------------------------------------------------------------

		//接收并解码处理http请求
		SOCKSRESULT recv_reqH(socketTCP *psock,time_t timeout=HTTP_MAX_RESPTIMEOUT);
		bool recv_remainder(socketTCP *psock,long receiveBytes=-1); //接收剩余未接收完的数据
		//解析Basic帐号密码串
		static bool ParseAuthorizationBasic(const char *str,
										  std::string &username,std::string &password);
	private:
		HTTPREQ_TYPE ParseRequest(const char *httpreqH);
		HTTPREQ_TYPE ParseFirstRequestLine(const char *lpszLine);
		void EncodeFirstRequestLine(cBuffer &buf);
		void parseURL(const char *lpszurl);
		void encodeURL(cBuffer &buf);
		void parseParam(char *strParam,char delm,
							 std::map<std::string,std::string> &maps,const char *ptrCharset);
		void encodeParam(cBuffer &buf,char delm,
							 std::map<std::string,std::string> &maps);

	private:
		DWORD m_httpreq_dwVer; //http协议版本
		HTTPREQ_TYPE m_httpreq_iType; //http请求类型
		long m_httpreq_lContentlen;
		std::string m_httpreq_strUrl;
		
		//保存从URL提交的参数
		std::map<std::string,std::string> m_httpreq_params_GET;
		//保存从POST表单提交的参数
		std::map<std::string,std::string> m_httpreq_params_POST;
		std::map<std::string,std::string> m_httpreq_HEADER;
		std::map<std::string,std::string> m_httpreq_COOKIE;
		bool m_httpreq_bReceiveALL;//是否已经http请求完整接收
		cBuffer m_httpreq_postdata; //保存接收的部分post数据

		bool m_bParseParams; //是否进行参数解析，默认为真
							//主要是给代理服务使用，代理服务在接收处理HTTPS代理时不解析参数提高速度
	};

}//?namespace net4cpp21

#endif
