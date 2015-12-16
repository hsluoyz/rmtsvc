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
#include "../include/socketUdp.h"
#include "../utils/utils.h"
#include "../include/cLogger.h"

using namespace std;
using namespace net4cpp21;


#define SOCKS5_MAX_PACKAGE_SIZE 1500
void transData_UDP(socketTCP *psock,socketUdp &sockUdp,unsigned long clntIP,int clntPort);
void cProxysvr :: doSock5req(socketTCP *psock)
{
	char buf[SOCKS5_MAX_PACKAGE_SIZE];
	int iret,recvlen=0; int istep=0; //socks5协商步骤
	PROXYACCOUNT *ptr_proa=NULL;
	while( (iret=psock->Receive(buf+recvlen,SOCKS5_MAX_PACKAGE_SIZE-recvlen,PROXY_MAX_RESPTIMEOUT))>0 )
	{
		if( (recvlen+=iret)<2 ) continue; //数据未接收完
		if(istep==0) //第一步socks5协商请求
		{
			sock5req *psock5req=(sock5req *)buf;
			if(recvlen<(psock5req->nMethods+2)) continue; //数据未接收完
			//接收完毕,开始处理消息
			sock5ans ans; ans.Ver =5;
			ans.Method=(m_bProxyAuthentication)?2:0;//2要求认证,等待密码认证信息否则不要求认证
			psock->Send(2 /*sizeof(sock5ans)*/,(const char *)&ans,-1);
			istep=(m_bProxyAuthentication)?1:2; //步骤1等待密码验证
			recvlen=0; continue; //清空接收的数据,继续
		}
		else if(istep==1) //第二步socks5密码验证
		{
			authreq *pauthreq=(authreq *)buf;
			if(pauthreq->Ver!=1) break; //错误的协议数据
			if( recvlen<4 ) continue; //数据未接收完
			if(recvlen<(pauthreq->Ulen+3) ) continue; //数据未接收完
			unsigned char plen=*(unsigned char *)(buf+2+pauthreq->Ulen); //获取密码长度
			if(recvlen<(pauthreq->Ulen+plen+3) ) continue; //数据未接收完
			//接收完毕,开始处理消息
			pauthreq->Name[pauthreq->Ulen]=0;
			char *strPass=(char *)(buf+2+pauthreq->Ulen+1);
			strPass[plen]=0;
//			pauthreq->Pass[pauthreq->PLen]=0; //error yyc modify 2007-01-10
			authans auans;auans.Ver=1;
			ptr_proa=ifAccess(psock,pauthreq->Name,strPass,NULL);
			if(ptr_proa) //认证通过
				auans.Status=0;  //成功 //认证通过
			else auans.Status=1; //认证不通过
			psock->Send(2 /*sizeof(authans)*/,(const char *)&auans,-1);
			if(auans.Status!=0) break;
			istep=2; //步骤2
			recvlen=0; continue; //清空接收的数据,继续
		}//?else if(istep==1)
		else if(istep==2) //第三步处理命令请求
		{
			sock5req1 *psock5req1=(sock5req1 *)buf;
			char *hostip; int hostport=0;
			unsigned long IPAddr=INADDR_NONE;
			sock5ans1 ans1;ans1.Ver=5;ans1.Rsv=0;ans1.Atyp=1;
			if( recvlen<10 ) continue; //数据未接收完
			if(psock5req1->Atyp==1) //IP address
			{
				IPAddr =*((unsigned long *)psock5req1->other);
				hostport=ntohs(*((unsigned short *)(psock5req1->other+4)));
				hostip=NULL;
			}
			else if(psock5req1->Atyp==3) //domain name
			{
				int datalen=6+psock5req1->other[0]+1;
				if( recvlen<datalen ) continue; //数据未接收完
				hostport=ntohs(*((unsigned short *)(psock5req1->other+psock5req1->other[0]+1)));
				hostip=psock5req1->other+1; hostip[ psock5req1->other[0] ]=0;
			}
			else //不支持的地址类型
			{
				ans1.Rep=8;//Address type not supported
				psock->Send(10 /*sizeof(sock5ans1)*/,(const char *)&ans1,-1);
				break; 
			}
			socketProxy peer; peer.setParent(psock);
			if(m_bCascade){ //设置了二级代理
				PROXYTYPE ptype=PROXY_SOCKS5;
				if((m_casProxytype & PROXY_SOCKS5)==0) //二级代理不支持SOCKS5代理
				{
					if(m_casProxytype & PROXY_HTTPS)
						ptype=PROXY_HTTPS;
					else if(m_casProxytype & PROXY_SOCKS4)
						ptype=PROXY_SOCKS4;
				}
				std::pair<std::string,int> * p=GetCassvr(); //获取二级代理服务设置
				if(m_casProxyAuthentication)
					peer.setProxy(ptype,p->first.c_str(),p->second,m_casAccessAuth.first.c_str(),m_casAccessAuth.second.c_str());
				else
					peer.setProxy(ptype,p->first.c_str(),p->second,"","");
			}//?if(m_bCascade)
			if(psock5req1->Cmd ==1) //tcp connect
			{//连接指定的远程主机,如果成功则建立数据转发对
				bool bAccessDest=true; //是否允许访问目的服务
				if(ptr_proa && ptr_proa->m_dstRules.rules()>0)
				{
					if(hostip) IPAddr=socketBase::Host2IP(hostip);
					bAccessDest=ptr_proa->m_dstRules.check(IPAddr,hostport,RULETYPE_TCP);
				}
				if(bAccessDest)
				{
//					RW_LOG_DEBUG("[ProxySvr] SOCKS5 - Connecting %s:%d ... \r\n",((hostip)?hostip:socketBase::IP2A(IPAddr)),hostport);

					if(m_bCascade) //设置了二级代理
					{//通过二级代理连接指定的应用服务
						if(hostip==NULL){
							strcpy(buf,socketBase::IP2A(IPAddr));
							hostip=buf; }
						peer.Connect(hostip,hostport,PROXY_MAX_RESPTIMEOUT); 
					}else{//域名/IP地址解析
						if(hostip) IPAddr=socketBase::Host2IP(hostip);
						peer.SetRemoteInfo(IPAddr,hostport);
						if( IPAddr!=INADDR_NONE) 
							peer.socketTCP::Connect( NULL,0,PROXY_MAX_RESPTIMEOUT);
					}//?if(m_bCascade)...else...
				}
				if(peer.status()==SOCKS_CONNECTED){
					ans1.Rep=0;
					ans1.IPAddr=peer.getRemoteip();
					ans1.Port=htons(peer.getRemotePort());
				}else ans1.Rep=5;
			}//?if(psock5req1->Cmd ==1)
			else if(psock5req1->Cmd ==2) //tcp bind
			{//客户端请求在socks 服务端建立一个临时侦听服务，等待指定hostip的连接到来
				if(m_bCascade) //设置了二级代理
				{
					std::string svrip; int svrport=hostport;
					if(hostip) svrip.assign(hostip); 
					else svrip.assign(socketBase::IP2A(IPAddr));
					if( peer.Bind(svrip,svrport,PROXY_MAX_RESPTIMEOUT)>0 )
					{
						ans1.Rep=0;  ans1.Port=htons(svrport);
						ans1.IPAddr=peer.getRemoteip();//...//发送成功响应 
						psock->Send(10 /*sizeof(sock5ans1)*/,(const char *)&ans1,-1);
						ans1.Port=0; ans1.IPAddr=0;
						ans1.Rep=1; //等待对方连接，并发送第二个响应
						if(peer.WaitForBinded(PROXY_MAX_RESPTIMEOUT,false))
						{
							ans1.Port=htons(peer.getRemotePort()); //...可能应该为实际的端口/IP???
							ans1.IPAddr=peer.getRemoteip(); //...
							ans1.Rep=0; //连接成功
						}
					}else ans1.Rep=1;
				}else if( (iret=peer.ListenX(0,FALSE,NULL)) > 0)
				{
					ans1.Rep=0;  ans1.Port=htons(iret);
					ans1.IPAddr=0; //psock->getLocalip();//发送成功响应 
					psock->Send(10 /*sizeof(sock5ans1)*/,(const char *)&ans1,-1);
					ans1.Port=0; ans1.IPAddr=0;
					ans1.Rep=1; //等待对方连接，并发送第二个响应
					if(peer.Accept(PROXY_MAX_RESPTIMEOUT,NULL)>0)
					{
						ans1.Port=htons(peer.getRemotePort());
						ans1.IPAddr=peer.getRemoteip();
						ans1.Rep=0; //连接成功
					}
				}else ans1.Rep=1; //一般性错误
			}//?else if(psock5req1->Cmd ==2)
			else if(psock5req1->Cmd ==3) //udp代理
			{
				socketUdp sockUdp; //打开一个UDP端口用来代理转发数据
				SOCKSRESULT sr=sockUdp.Open(0,false,NULL);
				if(sr>0){ //UDP端口打开成功
					std::string svrip; int svrport=sr; //二级代理返回的代理UDP端口和IP
					if(!m_bCascade || peer.UdpAssociate(svrip,svrport,PROXY_MAX_RESPTIMEOUT)>0)
					{//如果没有设置二级代理，或者向二级代理发送UDP代理命令成功
						ans1.Rep=0; ans1.Port=htons(sr); //成功
						ans1.IPAddr=psock->getLocalip();
						psock->Send(10 /*sizeof(sock5ans1)*/,(const char *)&ans1,-1);
						//获取UDP代理客户端IP和端口，即clntIP,clntPort
						if(hostip) IPAddr=socketBase::Host2IP(hostip);
						if(IPAddr==INADDR_ANY || IPAddr==INADDR_NONE) IPAddr=psock->getRemoteip();
						if(m_bCascade){
							peer.setRemoteInfo(svrip.c_str(),svrport);
							sockUdp.setParent(&peer);
						}
						transData_UDP(psock,sockUdp,IPAddr,hostport); //开始UDP数据代理转发
						break;
					}//?if(!m_bCascade ||
				}//?if(sr>0)
				ans1.Rep=1; //常见的Socks故障 //ans1.Rep=7;//暂时先不支持
			}//?else if(psock5req1->Cmd ==3) //udp代理
			else ans1.Rep=7;//不支持的命令
			psock->Send(10 /*sizeof(sock5ans1)*/,(const char *)&ans1,-1);
			if(ans1.Rep==0) //SOCKS5命令成功,创建转发对
				transData(psock,&peer,NULL,0);
			else RW_LOG_PRINT(LOGLEVEL_WARN,"[ProxySvr] SOCKS5 failed - Cmd=%d, Atyp=%d, Rep=%d, host:port=%s:%d\r\n",
					psock5req1->Cmd,psock5req1->Atyp,ans1.Rep,((hostip)?hostip:peer.getRemoteIP()),hostport);
			break;//跳出循环,结束处理
		}//?else if(istep==2)
		else break; //错误的步骤
	}//?while

	if(ptr_proa) ptr_proa->m_loginusers--;
}
//yyc 2007-02-12 增加socks5 UDP代理的处理
//UDP数据转发
#define UDPPACKAGESIZE 8192
#define SOCKS5UDP_SIZE 10  //sizeof(sock5udp)==12
void transData_UDP(socketTCP *psock,socketUdp &sockUdp,unsigned long clntIP,int clntPort)
{
	if(psock==NULL) return;
	socketTCP *psvr=(socketTCP *)psock->parent();
	if(psvr==NULL) return;
	//是否指定了二级代理,peer为连接二级代理的socket
	socketProxy *peer=(socketProxy *)sockUdp.parent();
	
	char *buffer=new char[SOCKS5UDP_SIZE+UDPPACKAGESIZE];
	if(buffer==NULL) return;
	
	//开始UDP数据的代理转发
	RW_LOG_DEBUG(0,"[ProxySvr] socks5-UDP has been started\r\n");
	if(peer) //指定了二级代理
	{   //获取二级代理开的UDP代理端口
		int casUdpPort=peer->getRemotePort();
		unsigned long casUdpIP=peer->getRemoteip();
		RW_LOG_DEBUG("[ProxySvr] socks5-UDP : Cascade UDP %s:%d\r\n",socketBase::IP2A(casUdpIP),casUdpPort);
		while(psvr->status()==SOCKS_LISTEN)
		{
			//checkSocket会判断连接二级代理的连接是否异常
			int iret=sockUdp.checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ);
			if(iret==0){
				if(psock->checkSocket(0,SOCKS_OP_READ)<0) break;
				else if(peer->checkSocket(0,SOCKS_OP_READ)<0) break;
				else continue;
			}
			if(iret<0) break; //发生错误
			//读客户端发送的数据
			iret=sockUdp.Receive(buffer,UDPPACKAGESIZE,-1);
			if(iret<=0){//抛弃错误，仅仅打印错误信息
				RW_LOG_DEBUG("[ProxySvr] socks5-UDP : Failed to Receive,(%d - %d)\r\n",iret,sockUdp.getErrcode());
				continue; 
			}
			//如果是从代理客户端过来的UDP数据则转发给二级代理，
			if(sockUdp.getRemoteip()==clntIP && sockUdp.getRemotePort()==clntPort)
				sockUdp.SetRemoteInfo(casUdpIP,casUdpPort);
			else //否则转发给代理客户端
				sockUdp.SetRemoteInfo(clntIP,clntPort);
			iret=sockUdp.Send(iret,buffer,-1);

//			RW_LOG_DEBUG("CasCade - len=%d data:\r\n:%s.\r\n",iret,buffer+sizeof(socks5udp));
		}//?while
	}
	else //没有指定二级代理
	{
		char *buf=buffer+SOCKS5UDP_SIZE;
		socks5udp *pudp_pack=(socks5udp *)buffer;
		pudp_pack->Rsv=0; pudp_pack->Frag=0;
		pudp_pack->Atyp=0x01; //IPV4
		while(psvr->status()==SOCKS_LISTEN)
		{
			int iret=sockUdp.checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ);
			if(iret==0){
				if(psock->checkSocket(0,SOCKS_OP_READ)<0) break;
				else continue;
			}
			if(iret<0){
				RW_LOG_DEBUG("[ProxySvr] socks5-UDP : checkSocket() return %d\r\n",iret);
				break; //发生错误
			}
			//读客户端发送的数据
			iret=sockUdp.Receive(buf,UDPPACKAGESIZE,-1);
			if(iret<=0){//抛弃错误，仅仅打印错误信息
				RW_LOG_DEBUG("[ProxySvr] socks5-UDP : Failed to Receive,(%d - %d)\r\n",iret,sockUdp.getErrcode());
				continue; 
			}
			//如果是从代理客户端过来的UDP数据则转发给二级代理，
			if(sockUdp.getRemoteip()==clntIP && sockUdp.getRemotePort()==clntPort)
			{//从UDP数据包中解析出实际的IP地址和端口
				unsigned long IPAddr=INADDR_NONE; int hostport=0;
				socks5udp *pudp=(socks5udp *)buf; long udppackLen;
				if(pudp->Frag!=0) //非独立UDP包需进行碎片重组
				{//暂时不实现碎片重组功能，抛弃此包
					RW_LOG_DEBUG(0,"[ProxySvr] socks5-UDP : Received UDP frag,discard it\r\n");
					continue;
				}
				if(pudp->Atyp==1) //IP address
				{
					IPAddr =pudp->IPAddr;
					hostport=ntohs(pudp->Port);
					udppackLen=SOCKS5UDP_SIZE;
				}
				else if(pudp->Atyp==3) //domain name
				{
					char *other=(char *)&pudp->IPAddr;
					udppackLen=7+other[0]; //4+1+other[0]+2;
					hostport=ntohs(*((unsigned short *)(other+other[0]+1)));
					other[other[0]+1]='\0'; other++; //other指向域名
					IPAddr=socketBase::Host2IP(other);
				}else{
					RW_LOG_DEBUG(0,"[ProxySvr] socks5-UDP : Not surpport Address Type\r\n");
					break; //不支持的地址类型
				}
				sockUdp.SetRemoteInfo(IPAddr,hostport);
				iret=sockUdp.Send(iret-udppackLen,buf+udppackLen,-1);
//				RW_LOG_DEBUG("Client->UDP(%s:%d) - len=%d data:\r\n%s.\r\n",
//					socketBase::IP2A(IPAddr),hostport,iret-udppackLen,buf+udppackLen);
			}else{ //否则进行UDP封包然后转发给代理客户端
				pudp_pack->IPAddr=sockUdp.getRemoteip();
				pudp_pack->Port=htons(sockUdp.getRemotePort());
				sockUdp.SetRemoteInfo(clntIP,clntPort);
				sockUdp.Send(iret+SOCKS5UDP_SIZE,buffer,-1);
//				RW_LOG_DEBUG("UDP->Client(%s:%d) - len=%d+%d data:\r\n%s.\r\n",
//					socketBase::IP2A(clntIP),clntPort,SOCKS5UDP_SIZE,iret,buf);
			}
		}//?while
	}

	sockUdp.Close(); delete[] buffer; 
	RW_LOG_DEBUG(0,"[ProxySvr] socks5-UDP has been ended\r\n");
	return;
}

