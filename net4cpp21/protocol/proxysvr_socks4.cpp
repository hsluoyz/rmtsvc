/*******************************************************************
   *	proxysvr.cp
   *    DESCRIPTION:代理服务端实现
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

#include "../include/sysconfig.h"
#include "../include/proxyclnt.h"
#include "../include/proxysvr.h"
#include "../utils/utils.h"
#include "../include/cLogger.h"

using namespace std;
using namespace net4cpp21;

#define SOCKS4_MAX_PACKAGE_SIZE 64
void cProxysvr :: doSock4req(socketTCP *psock)
{
	char buf[SOCKS4_MAX_PACKAGE_SIZE];
	int iret,recvlen=0;
	if(m_bProxyAuthentication) return; //socks4代理协议不支持认证,但代理服务要求认证

	while( (iret=psock->Receive(buf+recvlen,SOCKS4_MAX_PACKAGE_SIZE-recvlen,PROXY_MAX_RESPTIMEOUT))>0 )
	{
		recvlen+=iret;
		sock4req *psock4req=(sock4req *)buf;
		if(recvlen>1 && psock4req->CD!=0x01 && psock4req->CD!=0x02) break;
		if( recvlen<9 ) continue; //数据未接收完
		if((psock4req->IPAddr&0x00ffffff)==0) //socks4A请求
		{//支持socks4A协议 [版本] [命令] [端口] [0.0.0.X] [0] [hostdomain] [0]
			if(recvlen<(buf[8]+9)) continue; //数据未接收完
		}
		//否则接收了完整SOCKS4的协议数据,开始协议处理
		sock4ans ans; ans.CD =91; ans.VN =0;
		ans.Port =0;ans.IPAddr =0;

		char *hostip=NULL; int hostport=0; //要连接的目的服务ip和端口
		if((psock4req->IPAddr&0x00ffffff)==0) hostip=(char *)psock4req+9; //socks4A
		
		socketProxy peer; peer.setParent(psock);
		if(m_bCascade){ //设置了二级代理
			PROXYTYPE ptype=PROXY_SOCKS4;
			if((m_casProxytype & PROXY_SOCKS4)==0) //二级代理不支持SOCKS4代理
			{
				if(m_casProxytype & PROXY_HTTPS)
					ptype=PROXY_HTTPS;
				else if(m_casProxytype & PROXY_SOCKS5)
					ptype=PROXY_SOCKS5;
			}
			std::pair<std::string,int> * p=GetCassvr(); //获取二级代理服务设置
			if(m_casProxyAuthentication)
				peer.setProxy(ptype,p->first.c_str(),p->second,m_casAccessAuth.first.c_str(),m_casAccessAuth.second.c_str());
			else
				peer.setProxy(ptype,p->first.c_str(),p->second,"","");
		}//?if(m_bCascade)
		if(psock4req->CD==1) //CONNECT请求
		{//连接指定的远程主机,如果成功则建立数据转发对
			hostport=ntohs(psock4req->Port);
			
//			RW_LOG_DEBUG("[ProxySvr] SOCKS4 - Connecting %s:%d ... \r\n",((hostip)?hostip:socketBase::IP2A(psock4req->IPAddr)),hostport);
			
			if(m_bCascade) //设置了二级代理
			{//通过二级代理连接指定的应用服务
				if(hostip==NULL){
					strcpy(buf,socketBase::IP2A(psock4req->IPAddr));
					hostip=buf; }
				peer.Connect(hostip,hostport,PROXY_MAX_RESPTIMEOUT);
			}else{
				if(hostip) psock4req->IPAddr=socketBase::Host2IP(hostip); //域名/IP地址解析
				peer.SetRemoteInfo(psock4req->IPAddr,hostport);
				if( psock4req->IPAddr!=INADDR_NONE) 
					peer.socketTCP::Connect( NULL,0,PROXY_MAX_RESPTIMEOUT);
			}
			ans.CD=(peer.status()==SOCKS_CONNECTED)?90:92;
		}//?if(psock4req->CD==1)
		else if(psock4req->CD==2) //BIND请求
		{//客户端请求在socks 服务端建立一个临时侦听服务，等待指定hostip的连接到来
			if(m_bCascade) //设置了二级代理
			{
				std::string svrip; int svrport=hostport;
				if(hostip) svrip.assign(hostip); 
				else svrip.assign(socketBase::IP2A(psock4req->IPAddr));
				if( peer.Bind(svrip,svrport,PROXY_MAX_RESPTIMEOUT)>0 )
				{
					ans.CD=90; 
					ans.Port =htons(svrport); ans.IPAddr =peer.getRemoteip();
					psock->Send(8 /*sizeof(sock4ans)*/,(const char *)&ans,-1);
					ans.Port =0;ans.IPAddr =0;
					ans.CD=91; //等待对方连接，并发送第二个响应
					if(peer.WaitForBinded(PROXY_MAX_RESPTIMEOUT,false))
						ans.CD=90; //连接成功
					else ans.CD=92;
				}else ans.CD=92;
			}else if( (iret=peer.ListenX(0,FALSE,NULL)) > 0)
			{
				ans.CD=90; 
				ans.Port =htons(iret); ans.IPAddr =0; //psock->getLocalip();
				psock->Send(8 /*sizeof(sock4ans)*/,(const char *)&ans,-1);
				ans.Port =0;ans.IPAddr =0;
				ans.CD=91; //等待对方连接，并发送第二个响应
				if(peer.Accept(PROXY_MAX_RESPTIMEOUT,NULL)>0)
					ans.CD=90; //连接成功
			}else ans.CD=92;
		}//?else if(psock4req->CD==2)

		psock->Send(8 /*sizeof(sock4ans)*/,(const char *)&ans,-1);
		if(ans.CD==90) //SOCKS4命令成功,创建转发对
			transData(psock,&peer,NULL,0);
		else RW_LOG_PRINT(LOGLEVEL_WARN,"[ProxySvr] SOCKS4 failed - CD=%d, host:port=%s:%d\r\n",
					psock4req->CD,((hostip)?hostip:peer.getRemoteIP()),hostport);
		break;//跳出循环
	}//?while
	return;
}
