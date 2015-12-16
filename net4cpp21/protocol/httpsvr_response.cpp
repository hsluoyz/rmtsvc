/*******************************************************************
   *	httpsvr_response.cpp
   *    DESCRIPTION:HTTP协议服务端实现
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:
   *
   *	net4cpp 2.1
   *	HTTP/1.1传输协议
   *******************************************************************/

#include "../include/sysconfig.h"
#include "../include/httpsvr.h"
#include "../utils/utils.h"
#include "../include/cLogger.h"

using namespace std;
using namespace net4cpp21;

void httpServer::httprsp_listDir(socketTCP *psock,std::string &strPath,httpRequest &httpreq,httpResponse &httprsp)
{
	const char *ptr,*strHost=httpreq.Header("Host");
	char buffer[1024]; std::string strPreUrl;
	std::string& strCurUrl=httpreq.url();
	char c=strCurUrl[strCurUrl.length()-1];
	if(c=='/') strCurUrl[strCurUrl.length()-1]=0;
	if( (ptr=strrchr(strCurUrl.c_str(),'/')) )
		strPreUrl.assign(strCurUrl.c_str(),ptr-strCurUrl.c_str());
	//设置MIME类型，默认为HTML
	httprsp.set_mimetype(MIMETYPE_HTML);
	if(httprsp.send_rspH(psock,200,"OK")<0) return;
	int buflen=sprintf(buffer,"<html><head><title>%s - %s/</title>"
							  "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=gb2312\">"
							  "</head>\r\n<body><H1>%s - %s/</H1><hr>"
				   "<pre><A HREF=\"%s/\">[To Parent Directory]</A><br><br>\r\n",strHost,strCurUrl.c_str(),
				   strHost,strCurUrl.c_str(),strPreUrl.c_str());
	
	if( psock->Send(buflen,buffer,-1)<0 ) return;
	
#ifdef WIN32
	if(strPath!="")
	{
		WIN32_FIND_DATA finddata;
		HANDLE hd=::FindFirstFile(strPath.c_str(), &finddata);
		if(hd==INVALID_HANDLE_VALUE) return;
		do{
			if(!(finddata.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN))
			{//判断是否显示隐藏文件
				//先将UTC时间转为本地时间
				::FileTimeToLocalFileTime(&finddata.ftLastWriteTime,&finddata.ftLastAccessTime);
				SYSTEMTIME systime; ::FileTimeToSystemTime(&finddata.ftLastAccessTime,&systime);
				if(finddata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				{
					if(strcmp(finddata.cFileName,".")==0 || strcmp(finddata.cFileName,"..")==0) continue;
					buflen=sprintf(buffer,"            %04d年%02d月%02d日    %02d:%02d%&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&lt;dir&gt; "
										  "<A HREF=\"%s/%s/\">%s</A><br>\r\n",
										  systime.wYear,systime.wMonth,systime.wDay,systime.wHour,systime.wMinute,
										  strCurUrl.c_str(),finddata.cFileName,finddata.cFileName);
					
				}else{
					buflen=sprintf(buffer,"             %04d年%02d月%02d日    %02d:%02d%13d "
										  "<A HREF=\"%s/%s\">%s</A><br>\r\n",
										  systime.wYear,systime.wMonth,systime.wDay,systime.wHour,systime.wMinute,
										  finddata.nFileSizeLow,strCurUrl.c_str(),finddata.cFileName,finddata.cFileName);
				}
				if( psock->Send(buflen,buffer,-1)<0 ) break;
			}
		}while(::FindNextFile(hd,&finddata));
		::FindClose(hd);
	}
#else
	psock->Send(26," Only Surpport Windows OS!",-1);
#endif
	psock->Send(24,"</pre><hr></body></html>",-1);
	strCurUrl[strCurUrl.length()-1]=c;
	return;
}

void httpServer::httprsp_listDenied(socketTCP *psock,httpResponse &httprsp)
{
		const char rspContent[]=
"<html><head><title>Directory Listing Denied</title></head>\r\n"
"<body><h1>Directory Listing Denied</h1>This Virtual Directory does not allow contents to be listed.</body></html>";
	//设置MIME类型，默认为HTML
	httprsp.set_mimetype(MIMETYPE_HTML);
	//设置响应内容长度
	httprsp.lContentLength(sizeof(rspContent)-1); 
	if(httprsp.send_rspH(psock,200,"OK")<0) return;
	psock->Send(sizeof(rspContent)-1,rspContent,-1);
	return;
}
//访问拒绝
void httpServer::httprsp_accessDenied(socketTCP *psock,httpResponse &httprsp)
{
		const char rspContent[]=
"<html><head><title>Access Denied</title></head>\r\n"
"<body><h1>Access Denied</h1>Service does not allow resources to be accessed.</body></html>";
	//设置MIME类型，默认为HTML
	httprsp.set_mimetype(MIMETYPE_HTML);
	//设置响应内容长度
	httprsp.lContentLength(sizeof(rspContent)-1); 
	if(httprsp.send_rspH(psock,200,"OK")<0) return;
	psock->Send(sizeof(rspContent)-1,rspContent,-1);
	return;
}

//文件未找到，返回404错误
void httpServer::httprsp_fileNoFind(socketTCP *psock,httpResponse &httprsp)
{
	const char rspContent[]=
"<html><head><title>Not Find</title></head>\r\n"
"<body><h1>Not Find</h1>File does not exist,please check URL.</body></html>";
	//设置MIME类型，默认为HTML
	httprsp.set_mimetype(MIMETYPE_HTML);
	//设置响应内容长度
	httprsp.lContentLength(sizeof(rspContent)-1); 
	if(httprsp.send_rspH(psock,404,"OK")<0) return;
	psock->Send(sizeof(rspContent)-1,rspContent,-1);
	return;
}
/*
void httpServer::httprsp_fileNoFind(socketTCP *psock,httpResponse &httprsp)
{
	const char rspContent[]=
"<html><head>\r\n"
"<style> a:link{font:9pt/11pt 宋体; color:FF0000} a:visited{font:9pt/11pt 宋体; color:#4e4e4e}\r\n"
"</style>\r\n"
"<title>无法找到网页</title>\r\n"
"<META HTTP-EQUIV=\"Content-Type\" Content=\"text-html; charset=gb2312\">\r\n"
"</head><body bgcolor=\"FFFFFF\">\r\n"
"<table width=\"410\" cellpadding=\"3\" cellspacing=\"5\">\r\n"
"  <tr>\r\n"
"    <td align=\"left\" valign=\"middle\" width=\"360\">\r\n"
"	<h1 style=\"COLOR:000000; FONT: 12pt/15pt 宋体\"><!--Problem-->无法找到网页</h1>\r\n"
"    </td></tr>\r\n"
"  <tr>\r\n"
"   <td width=\"400\" colspan=\"2\"> <font style=\"COLOR:000000; FONT: 9pt/11pt 宋体\">您正在搜索的网页可能已经删除、更名或暂时不可用。</font>\r\n"
"   </td></tr>\r\n"
"  <tr>\r\n"
"    <td width=\"400\" colspan=\"2\"> <font style=\"COLOR:000000; FONT: 9pt/11pt 宋体\">\r\n"
"	<hr color=\"#C0C0C0\" noshade>\r\n"
"<p>请尝试下列操作：</p>\r\n"
"	<ul><li>如果您在“地址”栏中键入了网页地址，请检查其拼写是否正确。<br></li>\r\n"
"	<li>单击<a href=\"javascript:history.back(1)\">后退</a>按钮尝试其他链接。</li></ul>\r\n"
"<h2 style=\"font:9pt/11pt 宋体; color:000000\">HTTP 404 - 无法找到文件<br> Internet 信息服务<BR></h2>\r\n"
"	<hr color=\"#C0C0C0\" noshade>\r\n"
"    </font></td></tr></table>\r\n"
"</body></html>";
	//设置MIME类型，默认为HTML
	httprsp.set_mimetype(MIMETYPE_HTML);
	//设置响应内容长度
	httprsp.lContentLength(sizeof(rspContent)-1); 
	if(httprsp.send_rspH(psock,404,"OK")<0) return;
	psock->Send(sizeof(rspContent)-1,rspContent,-1);
	return;
}
*/
void httpServer::httprsp_Redirect(socketTCP *psock,httpResponse &httprsp,const char *url)
{
	//设置响应内容长度
	httprsp.lContentLength(0);
	httprsp.AddHeader(string("Location"),string(url));
	httprsp.send_rspH(psock,302,"Object Moved");
	return;
}
void httpServer::httprsp_Redirect(socketTCP *psock,httpResponse &httprsp,const char *url,int iSeconds)
{
	char rspContent[256];
	int len=sprintf(rspContent,"<html><head>"
					"<META HTTP-EQUIV=REFRESH CONTENT=%d;URL=\"%s\">"
					"<title>redirect</title></head><body>...</body></html>"
					,iSeconds,url);
	//设置MIME类型，默认为HTML
	httprsp.set_mimetype(MIMETYPE_HTML);
	//设置响应内容长度
	httprsp.lContentLength(len);
	if(httprsp.send_rspH(psock,200,"OK")<0) return;
	psock->Send(len,rspContent,-1);
	return;
}

void httpServer::httprsp_NotModify(socketTCP *psock,httpResponse &httprsp)
{
	//设置响应内容长度
	httprsp.lContentLength(0);
	httprsp.send_rspH(psock,304,"Not Modified");
	return;
}

