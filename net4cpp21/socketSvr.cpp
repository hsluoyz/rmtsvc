/*******************************************************************
   *	socketSvr.cpp
   *    DESCRIPTION:TCP 异步服务类类的实现
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-11-01
   *	
   *	net4cpp 2.1
   *******************************************************************/

#include "include/sysconfig.h"
#include "include/socketSvr.h"
#include "utils/utils.h"
#include "include/cLogger.h"

using namespace std;
using namespace net4cpp21;

socketSvr :: socketSvr()
{
	m_strSvrname="Listen-server";//侦听服务
	m_curConnection=0;
	m_maxConnection=0;
	m_lAcceptTimeOut=500; //500ms 异步Accept超时时间,设置默认accept延时等待时长
	//==-1则阻塞等待直到一个连接，实际测试发现如果设置为-1,假如Listenning socket有一个
	//对方未关闭的连接，此时即使关闭了Listen Socket但accept还是会一直阻塞。
	//因此m_lAcceptTimeOut最好不要设置为-1
	m_bReuseAddr=FALSE;
}

socketSvr :: ~socketSvr()
{
	Close();
	m_threadpool.join();//不在Close时等待线程结束，防止在回调中调用Close
}

//启动服务
SOCKSRESULT socketSvr::Listen(int port,BOOL bReuseAddr/*=FALSE*/,const char *bindIP/*=NULL*/)
{
	m_bReuseAddr=bReuseAddr;
	SOCKSRESULT sr=socketTCP::ListenX(port,m_bReuseAddr,bindIP);
	if(sr<=0){
		RW_LOG_DEBUG("Failed to Listen at %d,error=%d\r\n",port,sr);
		return sr;
	}
	//服务端口启动成功，开始启动Accept服务线程
	if( m_threadpool.addTask((THREAD_CALLBACK *)&listenThread,(void *)this,0)!=0 )
		return sr;
	
	Close();//服务线程启动失败
	return SOCKSERR_THREAD;
}

//启动服务
SOCKSRESULT socketSvr::Listen(int startport,int endport,BOOL bReuseAddr/*=FALSE*/,const char *bindIP/*=NULL*/)
{
	m_bReuseAddr=bReuseAddr;
	SOCKSRESULT sr=(startport==endport)?
				socketTCP::ListenX(startport,m_bReuseAddr,bindIP):
				socketTCP::ListenX(startport,endport,m_bReuseAddr,bindIP);
	if(sr<=0){
		RW_LOG_DEBUG("Failed to Listen at %d-%d,error=%d\r\n",startport,endport,sr);
		return sr;
	}
	//服务端口启动成功，开始启动Accept服务线程
	if( m_threadpool.addTask((THREAD_CALLBACK *)&listenThread,(void *)this,0)!=0 )
		return sr;
	
	Close();//服务线程启动失败
	return SOCKSERR_THREAD;
}

//侦听线程
void socketSvr :: listenThread(socketSvr *psvr)
{
	if(psvr==NULL) return;
	int svrPort=psvr->getLocalPort();
#ifdef _SURPPORT_OPENSSL_
	RW_LOG_DEBUG("[socketSvr] %s%s has been started,port=%d\r\n",
				psvr->m_strSvrname.c_str(),(psvr->ifSSL())?"(ssl)":"",svrPort);
#else
	RW_LOG_DEBUG("[socketSvr] %s has been started,port=%d\r\n",
				psvr->m_strSvrname.c_str(),svrPort);
#endif
	//yyc add 2007-03-29 如果本服务绑定端口时指定了可重用已绑定端口
	//并且本服务绑定了某IP,及没有绑定127.0.0.1。那么对不可访问的访问者
	//重定向到老的服务，即127.0.0.1:<本端口>
	bool bReirectOldSvr=( psvr->getLocalip()!=INADDR_ANY && psvr->m_bReuseAddr==SO_REUSEADDR)?true:false;
	socketTCP *psock=NULL;
	while(psvr->m_sockstatus==SOCKS_LISTEN)
	{
		if(psock==NULL)
		{
			if( (psock=new socketTCP())==NULL) 
			{
				RW_LOG_PRINT(LOGLEVEL_ERROR,0,"[listenThread] failed to new socketTCP()\r\n");
				break;
			}
		}
		SOCKSRESULT sr=psvr->Accept(psvr->m_lAcceptTimeOut,psock);
		if(sr==SOCKSERR_TIMEOUT){ psvr->onIdle(); continue; } //超时
		if(sr<0) break; //发生错误

		RW_LOG_DEBUG("[socketSvr] one connection (%s:%d) coming in\r\n",psock->getRemoteIP(),psock->getRemotePort());
		if(psvr->m_srcRules.check(psock->getRemoteip(),psock->getRemotePort(),RULETYPE_TCP))
		{
			if( psvr->m_threadpool.addTask((THREAD_CALLBACK *)&doacceptTask,(void *)psock,THREADLIVETIME)!=0 )
			{	psock=NULL; continue; }
		}
		else if(bReirectOldSvr) //将指定得请求重新定向到重用得服务上
		{
			if( psvr->m_threadpool.addTask((THREAD_CALLBACK *)&doRedirectTask,(void *)psock,THREADLIVETIME)!=0 )
			{	psock=NULL; continue; }
		}
		//yyc add 2007-03-29 ---------------------------------------
		psock->Close();
	}//?while
	
	if(psock) delete psock;
	RW_LOG_DEBUG("[socketSvr] %s has been ended,port=%d!\r\n",psvr->m_strSvrname.c_str(),svrPort);
}

//新连接到达处理任务
void socketSvr :: doacceptTask(socketTCP *psock)
{
	if(psock==NULL) return;
	socketSvr *psvr=(socketSvr *)psock->parent();
#ifdef _SURPPORT_OPENSSL_
	if(psock->ifSSL()) psock->SSL_Associate();
#endif
	if(psvr){
		psvr->m_curConnection++;
		if(psvr->m_maxConnection!=0 &&  psvr->m_curConnection > psvr->m_maxConnection)
			psvr->onTooMany(psock); 
		else
			psvr->onAccept(psock);
		psvr->m_curConnection--;
	}
	delete psock; return;
}

//yyc add 2007-03-29 允许对系统得某个服务端口进行重用
#define MAX_TRANSFER_BUFFER 2048
#define MAX_CONNECT_TIMEOUT 5
void socketSvr :: doRedirectTask(socketTCP *psock)
{
	if(psock==NULL) return;
	socketSvr *psvr=(socketSvr *)psock->parent();
	//连接重被复用的服务端口地址:127.0.0.1
	socketTCP *ppeer=new socketTCP;
	if(ppeer==NULL) { delete psock; return; }
	if( ppeer->Connect("127.0.0.1",psvr->getLocalPort(),MAX_CONNECT_TIMEOUT)>0)
		ppeer->setParent(psock);
	else { delete psock; delete ppeer; return; }

	RW_LOG_DEBUG("[socketSvr] Redirect old server(127.0.0.1:%d)\r\n",psvr->getLocalPort());

	if( psvr->m_threadpool.addTask((THREAD_CALLBACK *)&transThread,(void *)ppeer,THREADLIVETIME)==0 )
	{
		delete psock; delete ppeer;
		return;
	}

	char buf[MAX_TRANSFER_BUFFER]; //开始转发
	while( psock->status()==SOCKS_CONNECTED )
	{
		int iret=psock->checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ);
		if(iret<0) break; 
		if(iret==0) continue;
		//读客户端发送的数据
		iret=psock->Receive(buf,MAX_TRANSFER_BUFFER-1,-1);
		if(iret<0) break; //==0表明接收数据流量超过限制
		if(iret==0){ cUtils::usleep(MAXRATIOTIMEOUT); continue; }
		iret=ppeer->Send(iret,buf,-1);
		if(iret<0) break;
	}//?while
	ppeer->Close(); //等待transThread线程结束
	while(ppeer->parent()!=NULL) cUtils::usleep(SCHECKTIMEOUT);
	
	delete psock; delete ppeer;
	return;
}

//转发线程
void socketSvr :: transThread(socketTCP *psock)
{
	if(psock==NULL) return;
	socketTCP *ppeer=(socketTCP *)psock->parent();
	if(ppeer==NULL) return;
//	socketSvr *psvr=(socketSvr *)ppeer->parent();

	char buf[MAX_TRANSFER_BUFFER]; //开始转发
	while( psock->status()==SOCKS_CONNECTED )
	{
		int iret=psock->checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ);
		if(iret<0) break; 
		if(iret==0) continue;
		//读客户端发送的数据
		iret=psock->Receive(buf,MAX_TRANSFER_BUFFER-1,-1);
		if(iret<0) break; //==0表明接收数据流量超过限制
		if(iret==0){ cUtils::usleep(MAXRATIOTIMEOUT); continue; }
		iret=ppeer->Send(iret,buf,-1);
		if(iret<0) break;
	}//?while
	ppeer->Close(); 
	psock->setParent(NULL); //用于此判断转发线程是否结束
	return;
}
