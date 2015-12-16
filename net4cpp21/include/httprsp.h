/*******************************************************************
   *	httprsp.h
   *    DESCRIPTION:HTTP 响应解析对象
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

#ifndef __YY_HTTP_RESPONSE_H__
#define __YY_HTTP_RESPONSE_H__

#include "httpdef.h"
#include "Buffer.h"
#include "socketTcp.h"

#include <map>

namespace net4cpp21
{
	typedef struct _TNew_Cookie
	{
		std::string cookie_name;
		std::string cookie_value;
		std::string cookie_expires;
		std::string cookie_path;
	}TNew_Cookie; //新设置的cookie结构
	class httpResponse 
	{
	public:
		httpResponse();
		~httpResponse(){}
		bool ifReceivedAll() const { return m_httprsp_bReceiveALL; }
		int get_respcode() const { return m_respcode; }
		DWORD get_httpVer() const { return m_httprsp_dwVer; }
		long lContentLength() const { return m_httprsp_lContentlen; }
		void lContentLength(long l) { m_httprsp_lContentlen=l; }
		MIMETYPE_ENUM get_contentType();
		const char * get_contentCharset();
		MIMETYPE_ENUM get_mimetype();
		void set_mimetype(MIMETYPE_ENUM mt);
		//返回已经接收的部分content内容的字节长度和buffer指针
		long lReceivedContent() { return m_httprsp_data.len(); }
		const char *szReceivedContent() { return m_httprsp_data.str(); }
		
		TNew_Cookie *SetCookie(const char *cookiename)
		{
			std::map<std::string,TNew_Cookie>::iterator it=
				(cookiename)?m_httprsp_SETCOOKIE.find(cookiename):m_httprsp_SETCOOKIE.end();
			return (it!=m_httprsp_SETCOOKIE.end())?( &(*it).second ):NULL;
		}
		void SetCookie(const char *cookiename,const char *cookieval,const char *path);
		const char *Header(const char *pheader)
		{//获取指定的http请求头
			std::map<std::string,std::string>::iterator it=
				(pheader)?m_httprsp_HEADER.find(pheader):m_httprsp_HEADER.end();
			return (it!=m_httprsp_HEADER.end())?((*it).second.c_str()):NULL;
		}
		
		//---------------------------------------------------------------
		void init_httprsp();//初始化http响应准备发送/接收新的http响应
		std::map<std::string,std::string> &Header() { return m_httprsp_HEADER; }
		std::map<std::string,TNew_Cookie> &Cookies() { return m_httprsp_SETCOOKIE; }
		//-------------------解码处理http响应----------------------------
		SOCKSRESULT recv_rspH(socketTCP *psock,time_t timeout=HTTP_MAX_RESPTIMEOUT);
		bool recv_remainderX(socketTCP *psock,long receiveBytes,time_t timeout);
		bool recv_remainder(socketTCP *psock,long receiveBytes=-1){
			//接收剩余HTTP响应未接收完的Body数据
			return recv_remainderX(psock,receiveBytes,HTTP_MAX_RESPTIMEOUT);
		}
		//保存http响应为指定的文件(不包含http响应头)
		//返回保存文件的大小，==0发生错误
		unsigned long save_resp(socketTCP *psock,const char *filename);
		//--------------------编码发送http响应---------------------------
		void AddHeader(std::string &headName,std::string &headValue){ m_httprsp_HEADER[headName]=headValue;}
		//设置catch控制头
		//"No-cache" - Do not cache this page at all, even if for use by the same client
		//"No-store" - The response and the request that created it must not be stored on any cache, 
		//				whether shared or private. The storage inferred here is non-volatile storage, 
		//				such as tape backups. This is not an infallible security measure.
		//"Private" , "Public"
		void CacheControl(const char *str){ if(str) m_httprsp_HEADER["Cache-control"]=std::string(str);}
		void NoCache(); //禁止缓存
		
		//发送http响应头
		SOCKSRESULT send_rspH(socketTCP *psock,int respcode,const char *respDesc);
		//发送文件，成功返回SOCKSERR_OK
		SOCKSRESULT sendfile(socketTCP *psock,const char *filename,
			MIMETYPE_ENUM mt=MIMETYPE_UNKNOWED,long startPos=0,long endPos=-1);
		SOCKSRESULT sendfile(socketTCP *psock,const char *filename,
			MIMETYPE_ENUM mt,long* lpstartPos,long* lpendPos,int iRangeNums);

		static MIMETYPE_ENUM MimeType(const char *filename);
	private:
		//解析http响应头，返回响应码
		int ParseResponse(const char *httprspH);
		void parse_SetCookie(const char *strParam);
		void parseParam(char *strParam,char delm,
							 std::map<std::string,std::string> &maps);
		void encodeParam(cBuffer &buf,char delm,
							 std::map<std::string,std::string> &maps);
	private:
		int m_respcode;
		DWORD m_httprsp_dwVer; //http协议版本
		long m_httprsp_lContentlen;
		std::map<std::string,std::string> m_httprsp_HEADER;
		//httprsp响应接收的新设置的cookie信息
		std::map<std::string,TNew_Cookie> m_httprsp_SETCOOKIE;
		bool m_httprsp_bReceiveALL;//是否已经http请求完整接收
		cBuffer m_httprsp_data; //保存接收的部分/全部响应Body数据
	};

}//?namespace net4cpp21

#endif

/* yyc remove 2007-12-12
private:
	std::map<std::string,std::string> m_httprsp_COOKIE;
std::map<std::string,std::string> &Cookies() { return m_httprsp_COOKIE; }
const char *Cookies(const char *cookiename)
{
	std::map<std::string,std::string>::iterator it=
		(cookiename)?m_httprsp_COOKIE.find(cookiename):m_httprsp_COOKIE.end();
	return (it!=m_httprsp_COOKIE.end())?((*it).second.c_str()):NULL;
}

	if(!m_httprsp_COOKIE.empty()){
		std::map<std::string,std::string>::iterator it=m_httprsp_COOKIE.begin();
		for(;it!=m_httprsp_COOKIE.end();it++)
			outbuf.len()+=sprintf(outbuf.str()+outbuf.len(),"Set-Cookie: %s=%s; path=/\r\n",
			(*it).first.c_str(),(*it).second.c_str());
	} 
*/
