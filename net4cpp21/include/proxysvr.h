/*******************************************************************
   *	proxysvr.h
   *    DESCRIPTION:代理服务端声明
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2006-08-24
   *
   *	net4cpp 2.1
   *	
   *******************************************************************/

#ifndef __YY_PROXY_SERVER_H__
#define __YY_PROXY_SERVER_H__

#include "proxydef.h"
#include "socketSvr.h"

namespace net4cpp21
{
	class cProxysvr
	{
	public:
		cProxysvr();
		virtual ~cProxysvr(){}
		//设置代理服务支持的类型
		void setProxyType(int itype) { m_proxytype=itype; }
		void setProxyAuth(bool bAuth) { m_bProxyAuthentication=bAuth; }
		bool setCascade(const char *casHost,int casPort,int type,const char *user,const char *pswd);
		bool getIfLogdata() const { return m_bLogdatafile; }
		void setIfLogdata(bool b){ m_bLogdatafile=b; }
	protected:
		//获取帐号信息，返回指定的帐号对象
		PROXYACCOUNT *getAccount(const char *struser);
		//添加新帐号信息
		PROXYACCOUNT *newAccount(const char *struser);
		SOCKSRESULT delAccount(const char *struser);
		void onConnect(socketTCP *psock);//有一个用户连接上来
		//创建转发对任务线程
		virtual bool onTransferTask(THREAD_CALLBACK *pfunc,void *pargs)
		{
			return false;
		}
		//收到转发数据，用于数据分析处理
		virtual void onData(char *buf,long len,socketTCP *from,socketTCP *to)
		{ return; }
	private:
		std::pair<std::string,int> * GetCassvr(){ //获取二级代理设置
			std::pair<std::string,int> *p=NULL;
			int n=m_vecCassvr.size();
			if(n==1) p=&m_vecCassvr[0];
			else if(n>1){
				srand(clock());
				p=&m_vecCassvr[rand()%n]; //随机获取一个应用服务得信息
			}
			return p; 
		}

		PROXYACCOUNT * ifAccess(socketTCP *psock,const char *user,const char *pwd,int * perrCode=NULL);
		void doSock4req(socketTCP *psock);
		void doSock5req(socketTCP *psock);
		void doHttpsreq(socketTCP *psock);
		void transData(socketTCP *psock,socketTCP *peer,const char *sending_buf,long sending_size);
		static void transThread(void *pthreadParam);
	private:
		int m_proxytype;//本代理服务支持的代理类型
		bool m_bProxyAuthentication;//本服务是否需要验证
		//此代理服务的帐号信息
		std::map<std::string,PROXYACCOUNT> m_accounts;
		//二级代理相关参数
		bool m_bCascade; //是否支持二级代理,支持多个二级代理服务器，随机选择
		std::vector<std::pair<std::string,int> > m_vecCassvr;
//		std::string m_casProxysvr; //二级代理服务端口
//		int m_casProxyport; 
		int m_casProxytype; //二级代理支持的类型
		bool m_casProxyAuthentication; //二级代理是否需要验证
		std::pair<std::string,std::string> m_casAccessAuth;
		bool m_bLogdatafile; //是否记录代理服务转发的数据到日志文件
	};

	class proxyServer : public socketSvr,public cProxysvr
	{
	public:
		proxyServer();
		virtual ~proxyServer();
	private:
		//当有一个新的客户连接此服务触发此函数
		virtual void onAccept(socketTCP *psock)
		{
			cProxysvr::onConnect(psock);
			return;
		}
		//创建转发对任务线程
		virtual bool onTransferTask(THREAD_CALLBACK *pfunc,void *pargs)
		{
			return (m_threadpool.addTask(pfunc,pargs,THREADLIVETIME)!=0);
		}
	};
}//?namespace net4cpp21

#endif

