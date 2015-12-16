/*******************************************************************
   *	httpclnt.h
   *    DESCRIPTION:HTTP协议客户端声明
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

#ifndef __YY_HTTP_CLIENT_H__
#define __YY_HTTP_CLIENT_H__

#include "httpdef.h"
#include "httpreq.h"
#include "httprsp.h"
#include "proxyclnt.h"

namespace net4cpp21
{
	class httpClient : public socketProxy
	{
	public:
		httpClient(){}
		virtual ~httpClient(){}
		//添加http请求的其他相关数据
		void add_reqHeader(const char *szname,const char *szvalue);
		void add_reqCookie(const char *szname,const char *szvalue);
		void add_reqPostdata(const char *szname,const char *szvalue);
		void set_reqPostdata(const char *buf,long buflen);
		//清除httpreq对象的http相关数据
		void cls_httpreq(bool ifKeepHeader=false) { m_httpreq.init_httpreq(ifKeepHeader); return; }
		//发送http请求 lstartRange,lendRange告诉web服务请求文件的范围
		//成功返回SOCKSERR_OK  //不等待响应返回
		SOCKSRESULT send_httpreq_headX(const char *strurl,long lTimeOut,long lstartRange,long lendRange);
		SOCKSRESULT send_httpreq_head(const char *strurl,long lstartRange=0,long lendRange=-1)
		{//发送http响应头，不等待返回
			return send_httpreq_headX(strurl,-1,lstartRange,lendRange);
		}
		//成功返回http响应码, <0发生错误 0:未知的响应码
		SOCKSRESULT send_httpreq(const char *strurl,long lstartRange=0,long lendRange=-1);

		httpResponse & Response() { return m_httprsp; }
		long rspContentLen() { return m_httprsp.lContentLength(); }
		//保存http响应为指定的文件(不包含http响应头)
		//返回保存文件的大小，==0发生错误
		unsigned long save_httpresp(const char *filename)
		{
			return m_httprsp.save_resp(this,filename);
		}
		
	private:
		httpRequest m_httpreq; //http请求解析对象
		httpResponse m_httprsp; //http响应处理对象
	};
}//?namespace net4cpp21

#endif

