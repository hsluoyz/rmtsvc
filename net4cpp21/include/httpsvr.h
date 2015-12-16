/*******************************************************************
   *	httpsvr.h
   *    DESCRIPTION:HTTP协议服务端声明
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

#ifndef __YY_HTTP_SERVER_H__
#define __YY_HTTP_SERVER_H__

#include "httpdef.h"
#include "httpreq.h"
#include "httprsp.h"
#include "socketSvr.h"

namespace net4cpp21
{
	
	class httpSession
	{
		char m_sessionID[24];
		std::map<std::string,std::string> m_maps;
	public:
		time_t m_startTime;//创建session的起始时间
		time_t m_lastTime;//最后一次访问此session时间
	public:
		static const long SESSION_VALIDTIME;
		static const char SESSION_IDNAME[];
		httpSession();
		virtual ~httpSession(){}
		bool isValid(time_t tNow) { return (tNow-m_lastTime)<SESSION_VALIDTIME; }
		const char *sessionID() { return m_sessionID;}
		bool SetSessionID(const char *strID);
		std::string& operator[](const std::string& key) { return m_maps[key]; }
	};

	class httpServer : public socketSvr
	{
	public:
		httpServer();
		virtual ~httpServer();
		//设置web服务的主目录以及虚目录
		bool setvpath(const char *vpath,const char *rpath,long lAccess);
	protected:
		virtual bool onHttpReq(socketTCP *psock,httpRequest &httpreq,httpSession &session,
			std::map<std::string,std::string>& application,httpResponse &httprsp){ return false; }

		//当有一个新的客户连接此服务触发此函数
		virtual void onAccept(socketTCP *psock);
		//如果当前连接数大于当前设定的最大连接数则触发此事件
		virtual void onTooMany(socketTCP *psock);
		virtual void onIdle(void);
	protected:
		void httprsp_fileNoFind(socketTCP *psock,httpResponse &httprsp);
		void httprsp_listDenied(socketTCP *psock,httpResponse &httprsp);
		void httprsp_accessDenied(socketTCP *psock,httpResponse &httprsp);
		void httprsp_listDir(socketTCP *psock,std::string &strPath,httpRequest &httpreq,httpResponse &httprsp);

		void httprsp_Redirect(socketTCP *psock,httpResponse &httprsp,const char *url);
		void httprsp_Redirect(socketTCP *psock,httpResponse &httprsp,const char *url,int iSeconds);
		void httprsp_NotModify(socketTCP *psock,httpResponse &httprsp);
		httpSession *GetSession(const char *sessionID)
		{ 
			if(sessionID==NULL) return NULL;
			std::map<std::string,httpSession *>::iterator it=m_sessions.find(sessionID);
			return (it!=m_sessions.end())?(*it).second:NULL;
		}
	private:
		long cvtVPath2RPath(std::string &vpath);

		cMutex m_mutex;
		std::map<std::string,httpSession *> m_sessions;
		std::map<std::string,std::string> m_application;

		std::map<std::string,std::pair<std::string,long> > m_dirAccess;//目录访问权限
			//first --- string : http的虚目录路径，例如/ 或 /aa/，虚目录不区分大小写全部转化为小写
			//second --- pair : 此http虚目录对应的实际目录和目录的访问权限，实际目录必须为\结尾(win平台)
	};
}//?namespace net4cpp21

#endif
