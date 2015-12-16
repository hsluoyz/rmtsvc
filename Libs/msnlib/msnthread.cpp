/*******************************************************************
   *	msnlib.cpp
   *    DESCRIPTION:msn协议处理类实现
   *
   *    AUTHOR:yyc
   *
   *    HISTORY:
   *
   *    DATE:2005-06-28
   *	
   *******************************************************************/

#include "../../include/sysconfig.h"
#include "../../include/cCoder.h"
#include "../../utils/utils.h"
#include "../../include/cLogger.h"
#include "msnlib.h"

using namespace std;
using namespace net4cpp21;

//处理本客户端和NS服务器的命令交互
void msnMessager :: receiveThread(msnMessager *pmsnmessager) 
{
	if(pmsnmessager==NULL) return;
	RW_LOG_PRINT(LOGLEVEL_WARN,0,"main-thread of msnMessager has been started.\r\n");
	socketTCP *psock=(socketTCP *)&pmsnmessager->m_curAccount.m_chatSock;

	time_t tStart=time(NULL);
	char buffer[MSN_MAX_NS_COMMAND_SIZE]; //接收客户端命令
	int buflen=0; //清空命令接收缓冲
	while( psock->status()==SOCKS_CONNECTED )
	{
		int iret=psock->checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ);
		if(iret<0) break; 
		if(iret==0){
			if( (time(NULL)-tStart)>=20/*MSN_MAX_RESPTIMEOUT*/ ){ 
				psock->Send(5,"PNG\r\n",-1); 
				tStart=time(NULL); 
			}
			continue; //没有数据
		}//?if(iret==0)
		//读客户端发送的数据
		iret=psock->Receive(buffer+buflen,MSN_MAX_NS_COMMAND_SIZE-buflen-1,-1);
		if(iret<0) break; //==0表明接收数据流量超过限制
		if(iret==0){ cUtils::usleep(MAXRATIOTIMEOUT); continue; }
		buflen+=iret; buffer[buflen]=0;
		//解析msn命令
		unsigned long trID=0; //NS返回响应ID
		char *tmpptr,*ptrCmd,*ptrBegin=buffer;
		while( (ptrCmd=strchr(ptrBegin,'\r')) )
		{
			*(char *)ptrCmd=0;//开始解析命令
			if(ptrBegin[0]==0) goto NextCMD; //不处理空行数据

//处理MSN NS服务器命令 ---- begin-------------------------------------------
			//注意:收到的命令是先进行utf8编码然后进行mime编码的
//			RW_LOG_DEBUG("[msnlib] recevied Command from NS: %s.\r\n",ptrBegin);
			if( strncmp(ptrBegin,"MSG ",4)==0 || 
				strncmp(ptrBegin,"GCF ",4)==0 ||
				strncmp(ptrBegin,"UBX ",4)==0 )
			{//处理多行消息,格式范例
				//MSG Hotmail Hotmail 514  其中514指本消息的后续待接收的字节数
				//GCF 9 Shields.xml 159 其中159指本消息的后续待接收的字节数(为Shields.xml文件内容)
				//UBX idazhi@hotmail.com 60 其中60指本消息的后续待接收的字节数
				if( (tmpptr=strrchr(ptrBegin,' ')) )
				{//获取消息长度 len
					int len=atoi(tmpptr+1); 
					//判断命令的结束符号是\r还是\r\n
					int nSkip=(*(ptrCmd+1)=='\n')?2:1;
					if( (buflen-(ptrCmd-buffer+nSkip))>=len) 
					{ //消息已经全部接收完毕
						char c=*(ptrCmd+len+nSkip); 
						*(ptrCmd+len+nSkip)=0;//保护要处理的字符串有结束符号

						//处理消息 //+nSkip跳过\r\n
						if(ptrBegin[0]=='M')
							pmsnmessager->nscmd_msg(psock,ptrCmd+nSkip);
						else if(ptrBegin[0]=='U')
						{
							*(char *)tmpptr=0; //获取email信息
							pmsnmessager->nscmd_ubx(psock,ptrBegin+4,ptrCmd+nSkip);
						}
//						else if(ptrBegin[0]=='G')
//							pmsnmessager->nscmd_gcf(psock,ptrCmd+nSkip); //不处理GCF消息

						*(ptrCmd+len+nSkip)=c;
						//跳过后续消息,注意while循环会ptrBegin=ptrCmd+1;因此要-1
						ptrCmd+=(len+nSkip-1);
					}
					else{ *ptrCmd='\r'; break; }//继续接收剩余的消息
				}//?if( (tmpptr=strrchr(ptrBegin,' ')) )
			}//?strncmp(ptrBegin,"MSG ",4)
			else if( atol(ptrBegin)!=0 ) 
			{//某个命令的错误返回 格式: 错误码 trID 说明\r\n
				if( (tmpptr=strchr(ptrBegin,' ')) ) 
					trID=(unsigned long)atol(tmpptr+1);
			}//?if( atol(ptrBegin)!=0 )
			else if(strncmp(ptrBegin,"SBS ",4)==0)
				pmsnmessager->nscmd_sbs(psock,ptrBegin);
			else if(strncmp(ptrBegin,"XFR ",4)==0)
				trID=(unsigned long)atol(ptrBegin+4);
			else if(strncmp(ptrBegin,"SYN ",4)==0)
				pmsnmessager->nscmd_syn(psock,ptrBegin);
			else if(strncmp(ptrBegin,"LSG ",4)==0)
				pmsnmessager->nscmd_lsg(psock,ptrBegin);
			else if(strncmp(ptrBegin,"LST ",4)==0)
				pmsnmessager->nscmd_lst(psock,ptrBegin);
			else if(strncmp(ptrBegin,"CHL ",4)==0)
				pmsnmessager->nscmd_chl(psock,ptrBegin);
			else if(strncmp(ptrBegin,"ILN ",4)==0)
				pmsnmessager->nscmd_iln(psock,ptrBegin);
			else if(strncmp(ptrBegin,"FLN ",4)==0)
				pmsnmessager->nscmd_fln(psock,ptrBegin);
			else if(strncmp(ptrBegin,"NLN ",4)==0)
				pmsnmessager->nscmd_nln(psock,ptrBegin);
			else if(strncmp(ptrBegin,"REM ",4)==0)
				trID=pmsnmessager->nscmd_rem(psock,ptrBegin);
			else if(strncmp(ptrBegin,"ADC ",4)==0)
				trID=pmsnmessager->nscmd_adc(psock,ptrBegin);
			else if(strncmp(ptrBegin,"RNG ",4)==0)
				trID=pmsnmessager->nscmd_rng(psock,ptrBegin);
			else if(strncmp(ptrBegin,"PRP ",4)==0)
				trID=pmsnmessager->nscmd_prp(psock,ptrBegin);
			else
				RW_LOG_DEBUG("[msnlib] recevied Command from NS: %s.\r\n",ptrBegin);

			if(trID!=0){
				pmsnmessager->m_mutex.lock();
				std::map<unsigned long,cCond *>::iterator it=pmsnmessager->m_conds.find(trID);
				if(it!=pmsnmessager->m_conds.end()){ 
					char *respbuf=(char *)((*it).second->getArgs());
					if(respbuf) strcpy(respbuf,ptrBegin);
					(*it).second->active(); 
				}
				pmsnmessager->m_mutex.unlock(); trID=0; 
			}//?if(trID!=0)
//处理MSN NS服务器命令 ----  end -------------------------------------------
			

NextCMD:	//移动ptrBegin到下一个命令数据起始
			ptrBegin=ptrCmd+1; 
			while(*ptrBegin=='\r' || *ptrBegin=='\n') ptrBegin++; //跳过\r\n
		}//?while
		//如果有未接收完的命令则移动
		if((iret=(ptrBegin-buffer))>0 && (buflen-iret)>0)
		{//如果ptrBegin-buf==0说明这是一个错误命令数据包
			buflen-=iret;
			memmove((void *)buffer,ptrBegin,buflen);
		} else buflen=0;
	}//?while
	
	psock->Close(); pmsnmessager->onSIGNOUT();
	RW_LOG_PRINT(LOGLEVEL_WARN,0,"main-thread of msnMessager has been ended\r\n");
	return;

}

//处理本客户端和SS服务器的命令交互
//每个sessionThread是一个chat session
void msnMessager :: sessionThread(void *param) 
{
	std::pair<msnMessager *,cContactor *> *pp=(std::pair<msnMessager *,cContactor *> *)param;
	msnMessager *pmsnmessager=pp->first;
	cContactor *pcon=pp->second; delete pp;
	if(pmsnmessager==NULL || pcon==NULL) return;
	srand(clock()); //保证此线程的的rand的随机，否则每个线程都会以默认的种子产生随机数，导致每次
	//进入此线程产生的随机数相同 2005-07-18 yyc comment
	socketProxy *pchatsock=&pcon->m_chatSock;
	RW_LOG_PRINT(LOGLEVEL_WARN,"session-thread(%s) of msnMessager has been started.\r\n",
		pcon->m_email.c_str());
	pmsnmessager->onChatSession((HCHATSESSION)pcon,MSN_CHATSESSION_CREATE,
		pcon->m_email.c_str(),0);
	
	pcon->m_chat_contacts=0;//参与此次交谈的会话的联系人个数
	time_t tStart=time(NULL);//会话开始时间
	time_t tTimeout=60;//10*MSN_MAX_RESPTIMEOUT;
	char buffer[MSN_MAX_SS_COMMAND_SIZE]; //接收客户端命令
	int buflen=0;//缓冲区中命令的长度
	while(pchatsock->status()==SOCKS_CONNECTED)
	{
		int iret=pchatsock->checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ);
		if(iret<0) break; //此socket发生错误
		if(iret==0){
			if(pcon->m_chat_contacts>0 && pcon->m_vecMessage.size()>0) 
			{//yyc add 2006-03-13
				int iSize=pcon->m_vecMessage.size();
				for(int i=0;i<iSize;i++)
				{
					std::pair<char *,long> &p=pcon->m_vecMessage[i];
					pchatsock->Send(p.second,p.first,-1);
					delete[] p.first;
				}
				pcon->m_vecMessage.clear();
			}//yyc add 2006-03-13 end
			continue; //没有数据
		}
		//有数据到达,接收数据
		iret=pchatsock->Receive(buffer+buflen,MSN_MAX_SS_COMMAND_SIZE-buflen-1,-1);
		if(iret<0)  break;  //==0表明接收数据流量超过限制
		if(iret==0){ cUtils::usleep(MAXRATIOTIMEOUT); continue; }
		buflen+=iret; buffer[buflen]=0;
		//处理MSN SS服务器命令
		unsigned long trID=0; //SS返回响应ID
		char *tmpptr,*ptrCmd,*ptrBegin=buffer;
//		RW_LOG_DEBUG("[msnlibXXXX] recevied Command from SS: %s.\r\n",ptrBegin);
		while( (ptrCmd=strchr(ptrBegin,'\r')) )
		{
			*(char *)ptrCmd=0;//开始解析命令
			if(ptrBegin[0]==0) goto NextCMD; //不处理空行数据

//处理MSN SS服务器命令 ---- begin-------------------------------------------
//			RW_LOG_DEBUG("[msnlib] recevied Command from SS: %s.\r\n",ptrBegin);
			if( strncmp(ptrBegin,"MSG ",4)==0 )
			{//处理多行消息,格式范例
				//MSG yycnet@hotmail.com yyc:) 91  其中91指本消息的后续待接收的字节数
				if( (tmpptr=strrchr(ptrBegin,' ')) )
				{//获取消息长度 len
					int len=atoi(tmpptr+1); 
					//判断命令的结束符号是\r还是\r\n
					int nSkip=(*(ptrCmd+1)=='\n')?2:1;
					if( (buflen-(ptrCmd-buffer+nSkip))>=len) 
					{ //消息已经全部接收完毕
						//让ptrBegin+4指向MSG消息的发送人Email，去掉后面的其他数据
						if( (tmpptr=strchr(ptrBegin+4,' ')) ) *tmpptr=0;

						char c=*(ptrCmd+len+nSkip); 
						*(ptrCmd+len+nSkip)=0;//保护要处理的字符串有结束符号
						//处理消息 //+nSkip跳过\r\n
						if(ptrBegin[0]=='M')
							pmsnmessager->sscmd_msg(pcon,ptrBegin+4,ptrCmd+nSkip,len); 

						*(ptrCmd+len+nSkip)=c;
						//跳过后续消息,注意while循环会ptrBegin=ptrCmd+1;因此要-1
						ptrCmd+=(len+nSkip-1);
					}
					else{ *ptrCmd='\r'; break; }//继续接收剩余的消息
				}//?if( (tmpptr=strrchr(ptrBegin,' ')) )
			}//?strncmp(ptrBegin,"MSG ",4)
			else if( atol(ptrBegin)!=0 ) 
			{//某个命令的错误返回 格式: 错误码 trID 说明\r\n
				if( (tmpptr=strchr(ptrBegin,' ')) ) 
					trID=(unsigned long)atol(tmpptr+1);
			}//?if( atol(ptrBegin)!=0 )
			else if(strncmp(ptrBegin,"IRO ",4)==0)
			{//如果不是我发起的聊天，则其他人进入是收到IRO
				//IRO <trID> 1 1 yycnet@hotmail.com yyc:)\r\n
				//解析出email
				char *ptrStart=NULL,*ptr=ptrBegin+4;
				int icount=0;
				while(*ptr){
					if(*ptr==' '){
						icount++;
						if(icount==3) 
							ptrStart=ptr+1;
						else if(icount==4){ *ptr=0; break; }
					}//?if(*ptr==' ')
					ptr++;
				}//?while
				if(ptrStart!=NULL)
					pmsnmessager->onChatSession((HCHATSESSION)pcon,MSN_CHATSESSION_JOIN,
							ptrStart,pcon->m_chat_contacts);
				pcon->m_chat_contacts++;
			}//?else if(strncmp(pcmdbuf,"IRO ",4)==0)
			else if(strncmp(ptrBegin,"JOI ",4)==0)
			{//如果是我发起的聊天，则其他人进入是收到JOI
				//JOI yycnet@hotmail.com yyc:)\r\n
				if( (tmpptr=strchr(ptrBegin+4,' ')) ) *tmpptr=0;
				pmsnmessager->onChatSession((HCHATSESSION)pcon,MSN_CHATSESSION_JOIN,
					ptrBegin+4,pcon->m_chat_contacts);
				pcon->m_chat_contacts++;
			}//?else if(strncmp(pcmdbuf,"JOI ",4)==0)
			else if(strncmp(ptrBegin,"BYE ",4)==0)
			{//某个用户退出了聊天
				pmsnmessager->onChatSession((HCHATSESSION)pcon,MSN_CHATSESSION_BYE,
					ptrBegin+4,pcon->m_chat_contacts);
				if( --pcon->m_chat_contacts<=0 ) goto EXIT1;
			}//?else if(strncmp(pcmdbuf,"BYE ",4)==0)
			else if(strncmp(ptrBegin,"ACK ",4)==0)
			{ //ACK trID\r\n
				trID=(unsigned long)atol(ptrBegin+4);
			}
			else
				RW_LOG_DEBUG("[msnlib] recevied Command from SS: %s.\r\n",ptrBegin);

			if(trID!=0){
				pmsnmessager->m_mutex.lock();
				std::map<unsigned long,cCond *>::iterator it=pmsnmessager->m_conds.find(trID);
				if(it!=pmsnmessager->m_conds.end()){ 
					char *respbuf=(char *)((*it).second->getArgs());
					if(respbuf) strcpy(respbuf,ptrBegin);
					(*it).second->active(); 
				}
				pmsnmessager->m_mutex.unlock(); trID=0;
			}//?if(trID!=0)
//处理MSN SS服务器命令 ----  end -------------------------------------------
			

NextCMD:	//移动ptrBegin到下一个命令数据起始
			ptrBegin=ptrCmd+1; 
			while(*ptrBegin=='\r' || *ptrBegin=='\n') ptrBegin++; //跳过\r\n
		}//?while
		
		//判断是否有未处理完的数据
		if( (iret=(ptrBegin-buffer))> 0)
		{
			if((buflen-iret)>0) //有数据未处理完
			{
				buflen-=iret;
				memmove((void *)buffer,ptrBegin,buflen);
			}
			else buflen=0; //数据已全部处理完
		}
		//如果ptrBegin-buf==0说明这是一个未接收完的包
/***********yyc remove 2006-09-01*********************************
	//如果有未接收完的命令则移动
		if((iret=(ptrBegin-buffer))>0 && (buflen-iret)>0)
		{//如果ptrBegin-buf==0说明这是一个错误命令数据包 此句错误见MSG处理
			buflen-=iret;
			memmove((void *)buffer,ptrBegin,buflen);
		} else buflen=0;
*****************************************************************/
	}//?while
EXIT1:
	pchatsock->Send(5,"OUT\r\n",-1);
	pchatsock->Close(); pcon->m_shell.destroy();
	pmsnmessager->onChatSession((HCHATSESSION)pcon,MSN_CHATSESSION_DESTROY,NULL,0);
	if(pcon->m_vecMessage.size()>0) //yyc add 2006-03-13
	{
		std::vector<std::pair<char *,long> >::iterator it=pcon->m_vecMessage.begin();
		for(;it!=pcon->m_vecMessage.end();it++) delete[] (*it).first;
		pcon->m_vecMessage.clear();
	}
	pcon->m_chat_contacts=0;
	std::map<std::string,cMsncx *>::iterator it=pcon->m_msncxMaps.begin();
	for(;it!=pcon->m_msncxMaps.end();it++) delete (*it).second; 
	pcon->m_msncxMaps.clear(); pcon->m_msnc1Maps.clear();

	RW_LOG_PRINT(LOGLEVEL_WARN,"session-thread(%s) of msnMessager has been ended\r\n",
		pcon->m_email.c_str());
	return;
}
//和cmd shell交互的线程
//读取cmd shell的输出并发送导chat客户端
void msnMessager :: shellThread(void *param)
{
	std::pair<msnMessager *,cContactor *> *pp=(std::pair<msnMessager *,cContactor *> *)param;
	msnMessager *pmsnmessager=pp->first;
	cContactor *pcon=pp->second; delete pp;
	if(pmsnmessager==NULL || pcon==NULL) return;
	RW_LOG_PRINT(LOGLEVEL_DEBUG,0,"shell-thread of msnMessager has been started.\r\n");
	

	char msgHeader[512]; //保留56字节的空间用于写入MSG命令头
	int headerlen=pmsnmessager->encodeChatMsgHead(msgHeader+56,512-57,NULL,NULL);
	socketProxy *pchatsock=&pcon->m_chatSock;
	char Buf[2048]; int bufLen=0; 
	bool bSend=false;
	time_t tSend=time(NULL);
	while(pchatsock->status()==SOCKS_CONNECTED)
	{
		cUtils::usleep(SCHECKTIMEOUT); //休眠200ms
		int iret=pcon->m_shell.Read(Buf+bufLen,2048-bufLen-1);
		if(iret<0) break; 
		bufLen+=iret; Buf[bufLen]=0;
		if(iret==0){
			if((time(NULL)-tSend)>=MSN_MAX_RESPTIMEOUT)
				bSend=true; //防止chatSession没有交互被关闭
			else if(bufLen>0) bSend=true;//立即发送数据
		}//?if(iret==0)
		else if(bufLen>=1500) bSend=true; //立即发送数据
		if(bSend){
			if(bufLen<=0)
				pmsnmessager->sendcmd_SS_Typing(pcon,NULL);
			else if( !pmsnmessager->sendcmd_SS_chatMsg(pcon,msgHeader,headerlen,Buf,bufLen) ) 
				break;
			bufLen=0; bSend=false;
			tSend=time(NULL);
		}
	}//?while

	RW_LOG_PRINT(LOGLEVEL_DEBUG,0,"shell-thread of msnMessager has been ended\r\n");
	return;
}
