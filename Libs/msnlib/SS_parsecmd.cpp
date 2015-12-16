/*******************************************************************
   *	SS_parsecmd.cpp
   *    DESCRIPTION:处理从SS服务器收到的命令
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
#include "../../include/cLogger.h"
#include "msnlib.h"

using namespace std;
using namespace net4cpp21;

//从SS收到加入聊天会话信息
//格式和HTTP request协议类似，\r\n\r\n分割消息头和消息体
//例如:
// MSG yycnet@hotmail.com yyc:) 91\r\n
// MIME-Version: 1.0\r\n
// Content-Type: text/x-msmsgscontrol\r\n
// TypingUser: yycnet@hotmail.com\r\n
// \r\n
//或
// MSG yycnet@hotmail.com yyc:) 139\r\n
// MIME-Version: 1.0\r\n
// Content-Type: text/plain; charset=UTF-8\r\n
// X-MMS-IM-Format: FN=%E5%AE%8B%E4%BD%93; EF=; CO=0; CS=86; PF=0\r\n
// \r\n
// gdgdfggdgdgfd

//email --- 发送此msg消息的联系人
unsigned long msnMessager :: sscmd_msg(cContactor *pcon,const char *msg_email,char *pcmd,int cmdlen)
{
	char *pBodyData=NULL;//消息体
	int bodyDataLen=0; //消息体长度
	const char *ptr_ContentType=NULL;//消息体类型
	char *ptr_fmtFonts=NULL; //聊天字体格式
	char *ptr_p4Context=NULL;//聊天人显示名称
	const char *ptr_TypingUser=NULL;//正输入的用户email
	const char *ptr_P2pDest=NULL;//指向P2P-Dest内容
	
	const char *ptr_MSNProxy=NULL; //为MSN代理功能自定义的标签

//	RW_LOG_PRINT(LOGLEVEL_DEBUG,"MSG len=%d, %s.\r\n",cmdlen,pcmd);
	//开始解析消息头-----------start-------------------------
	char *tmpptr,*ptr,*pStart=pcmd;
	while( (ptr=strchr(pStart,'\r')) )
	{
		*ptr=0;
		if( (tmpptr=strchr(pStart,':')) )
		{
			*tmpptr=0;
			if(strcmp(pStart,"Content-Type")==0)
				ptr_ContentType=tmpptr+2;
			else if(strcmp(pStart,"X-MMS-IM-Format")==0)
				ptr_fmtFonts=tmpptr+2;
			else if(strcmp(pStart,"P4-Context")==0)
				ptr_p4Context=tmpptr+2;
			else if(strcmp(pStart,"TypingUser")==0)
				ptr_TypingUser=tmpptr+2;
//			else if(strcmp(pStart,"P2P-Dest")==0)
//				ptr_P2pDest=tmpptr+2;
#ifdef __SURPPORT_MSNPROXY__
			else if(strcmp(pStart,"MSN-Proxy")==0)
				ptr_MSNProxy=tmpptr+2;
#endif
		}//?if( (tmpptr=strchr(pStart,':')) )
		int i=1; while(*(ptr+i)=='\r' || *(ptr+i)=='\n') i++; //跳过\r\n
		if(i>2){ //碰到了两个\r\n，下面的内容为消息体。消息体的内容由ContentType决定
			pBodyData=ptr+i;
			bodyDataLen=cmdlen-(pBodyData-pcmd);
			break; 
		} 
		pStart=ptr+i;
	}//?while(...
	if(ptr_ContentType==NULL) return 0;
	//解析消息头结束----------- end -------------------------

	//根据ContentType处理消息
	if(strcmp(ptr_ContentType,"text/x-msmsgscontrol")==0)
	{//收到一个输入控制消息,TypingUser:指明某个用户正在输入聊天信息
		onChatSession((HCHATSESSION)pcon,MSN_CHATSESSION_TYPING,ptr_TypingUser,0);
	}//?if(strcmp(ptr_ContentType,"text/x-msmsgscontrol")==0)

	else if(strncmp(ptr_ContentType,"text/plain",10)==0)
	{//收到一个聊天消息
		if(pBodyData==NULL) return 0;
#ifdef __SURPPORT_MSNPROXY__
		if(ptr_MSNProxy)
			onProxyChat((HCHATSESSION)pcon,ptr_MSNProxy,pBodyData,bodyDataLen);
		else
#endif
		{//-------------------------------------------------------
			if(strstr(ptr_ContentType+10,"charset=UTF-8")==NULL)
				RW_LOG_PRINT(LOGLEVEL_WARN,0,"[msnChat] text/plain is not UTF-8\r\n");
			
			onChat((HCHATSESSION)pcon,msg_email,pBodyData,bodyDataLen,ptr_fmtFonts,ptr_p4Context);
			if( pBodyData[0]!=0 && pcon->m_shell.isValidW() )
			{
				if(pBodyData[0]==1)
					pcon->m_shell.Write("\r\n",2);
				else pcon->m_shell.WriteCrLf(pBodyData,bodyDataLen);
			}//?if(pBodyDataW[0]!=0)
		}//---------------------------------------------------------
	}//?else if(strncmp(ptr_ContentType,"text/plain",10)==0)

	else if(strncmp(ptr_ContentType,"text/x-msmsgsinvite",10)==0)
	{//msnc0协议消息.文件传输采用msnftp。msnp10以后用msnc1协议
	//netmeeting,音频聊天以及msnftp都是使用的msnc0协议。yyc comment 2005-07-21
		msnc0_parse(pcon,msg_email,pBodyData); //,bodyDataLen
	}//?else if(strncmp(ptr_ContentType,"text/x-msmsgsinvite",10)==0)

	else if(strcmp(ptr_ContentType,"application/x-msnmsgrp2p")==0)
	{//msnc1协议 - msnp2p消息. 网络摄像头功能以及msnp9以后的文件/头像传输都是使用此协议
		//msnp2p消息由三部分组成 48字节的Binary stuff + option Data + 4字节的footer,见msnc1协议说明
		unsigned char *pheader=(unsigned char *)pBodyData;  
		unsigned long lfooter;//最后四个字节为footer,in Big Endian order
		*((char *)&lfooter)=*(pBodyData+bodyDataLen-1);
		*((char *)&lfooter+1)=*(pBodyData+bodyDataLen-2);
		*((char *)&lfooter+2)=*(pBodyData+bodyDataLen-3);
		*((char *)&lfooter+3)=*(pBodyData+bodyDataLen-4);
		char *ptrmsg=pBodyData+48; int msglen=bodyDataLen-48-4;
		msnc1_parse(pcon,msg_email,pheader,ptrmsg,lfooter);
	}//?else if(strcmp(ptr_ContentType,"application/x-msnmsgrp2p")==0)

//	else if(strcmp(ptr_ContentType,"text/x-msnmsgr-datacast")==0)
//	{//对方发送一个传情动漫
/*	格式 MIME-Version: 1.0\r\n
		 Content-Type: text/x-msnmsgr-datacast\r\n                    
		 Message-ID: {E1255EF3-88D3-4270-A9AB-294686282F41}\r\n      //唯一标识一个动漫消息ID
		 Chunks: 3\r\n                                               //表示此动漫数据要经过几个MSG块发送完，此处为3块。剩下每个MSG块的Message-ID和此MSG的Message-ID相同
		 \r\n
		 ID: 2\r\n													//ID=2表明此处为一个传情动漫,如果等于1则是一个闪屏震动
		 Data: <msnobj Creator="yycnet@hotmail.com" Size="23427" Type="8"...
*/
/*	如果一块无法传送完，下面块的内容如下
		 Message-ID: {E1255EF3-88D3-4270-A9AB-294686282F41}\r\n    //表示此块为那个动漫消息的后续块
		 Chunk: 1												   //表示此块为那个动漫消息的后续的第几块，共Chunks-1块
		 \r\n
		 <后续数据>...

		 Message-ID: {E1255EF3-88D3-4270-A9AB-294686282F41}\r\n    //表示此块为那个动漫消息的后续块
		 Chunk: 2												   //表示此块为那个动漫消息的后续的第几块，共Chunks-1块
		 \r\n
		 <后续数据>...
*/
/*	闪屏震动消息
		  MIME-Version: 1.0\r\n
		  Content-Type: text/x-msnmsgr-datacast\r\n
		  \r\n
		  ID: 1\r\n
		  \r\n\r\n
*/
//	}
	return 0;
}

/*
//头像获取流程
Sender                  SS                Recver
 <-------  发送获取头像邀请 --------
  ---------Acknowledged Message----->
  -------   发送同意响应 200 OK ---->
  <--------Acknowledged Message------
  ------- 发送准备发送数据消息 ----->
  <--------Acknowledged Message------             //发送端接收到接收端的发送准备应答后才能开始发送数据
  ------- 发送数据  ---------------->              field6 20 00 00 00
  <--------Acknowledged Message------              //当所有数据接收完毕后发送一个ACK消息
  <-------   Bye message ------------
  ---------Acknowledged Message----->			  //And finally if the Bye message is received by the SC and everything is fine, 
												  //it can send an Acknowledged Message back to the RC

  //BYE消息总是由RC(接收者/被邀请者)发送
  //the RC must send a Bye Message to the SC to say that the session can be closed.

文件获取流程
sender                                   Recver
  ------- 发送文件传输邀请 ---------->
  <--------Acknowledged Message-------
  <------ 同意接收 200 OK-------------
  ----------Acknowledged Message----->
  - invite(Direct-Connect handshake)->  ---				//Content-Type: application/x-msnmsgr-transreqbody
										  |  此步骤可有可无
  <--- ... ...Handshake end.. ...--->   ---
  ------ 发送文件数据 --------------->              field 30 00 00 01
  <--------Acknowledged Message-------              //当所有数据接收完毕后发送一个ACK消息
  -------   Bye message ------------->				//发送端会回送一个Bye消息
  --------- has sended bye ---------->              //filed6 40 00 000 00 和Got bye同样结构
  <-------- has got bye --------------				//filed6 40 00 000 00
  
*/

//处理msnc1协议 - 解析msnp2p部分
void msnMessager :: msnc1_parse(cContactor *pcon,const char *msg_email,unsigned char *pBinarystuff,
								char *ptrmsg,unsigned long lfooter)
{ //先解析48字节Binary stuff
	//The first field is a DWORD and is the SessionID, which is zero when the Clients are negotiating about the session
	long sessionID=*((long *)pBinarystuff);
	//The second field is a DWORD and is the Identifier which identifies the message, the first message you receive from the other Client is the BaseIndentifier, 
	//the other messages contains this Identifier +1 or -2 or something like that, the BaseIdentifier is random generated. The Identifier can be in range from 4 to a max of 4294967295, I think.
	unsigned long messageID=*((unsigned long *)(pBinarystuff+4));
	unsigned long dataOffset=*((unsigned long *)(pBinarystuff+8));//这个域应该8字节长
	unsigned long totalSize=*((unsigned long *)(pBinarystuff+16));//这个域应该8字节长
	unsigned long dataMessageSize=*((unsigned long *)(pBinarystuff+24));
	//The sixth field is a DWORD and is the Flag field, it's 0x0 when no flags are specified, 
	//0x2 if it's an reply to a received message, 0x8 if there is an error on the binary level, 
	//0x20 when the data is for User Display Images or Emoticons, 0x40 --- sended bye或者got bye
	//0x01000030 if it's the data of a file.
	unsigned long dwFlags=*((unsigned long *)(pBinarystuff+28));
	//The seventh field is a DWORD and is an important field, if the SessionID field is zero and the data doesn't contain the SessionID then this field contains the Identifier of the previous received message, 
	//most of the time the Flag field is 0x2 then. If the data contains the SessionID or if the SessionID field is non-zero then this field is just some random generated number.
	unsigned long field7=*((unsigned long *)(pBinarystuff+32));
	
//	RW_LOG_PRINT(LOGLEVEL_DEBUG,"[msnc1] sessionID=0x%x,messageID=0x%x,MessageSize=0x%x,dataOffset=0x%x,totalSize=0x%x,flags=0x%x,field7=0x%x,footer=0x%x\r\n",
//		sessionID,messageID,dataMessageSize,dataOffset,totalSize,dwFlags,field7,lfooter);
//	if(lfooter==0) RW_LOG_PRINT(LOGLEVEL_DEBUG,"[msnc1] msg=%s.\r\n",ptrmsg);

	//照协议上说明，如果lfooter=0x00 ---协商过程 0x01 --- for User Display Images and Emoticons, 0x02 --- for File Transfers
	
	//只有 "got bye"消息和ACK应答消息 的dataMessageSize=0
	if(dwFlags==0x02) return; //收到一个应答消息.忽略，不做任何处理
	if(dataMessageSize==0) return; //收到一个0长度消息，不做任何处理。譬如GOT/SENDED_BYE消息。
	
	if(lfooter!=0) //处于传输会话状态，此时sessionID不为0
	{
		if(sessionID==0) return; //{ printf("aaaaErr: lfooter=%d, sessionID==0\r\n",lfooter);	return; }
		std::map<long,cMsnc1 *>::iterator it=pcon->m_msnc1Maps.find(sessionID);
		cMsnc1 *pmsnc1=(it==pcon->m_msnc1Maps.end())?NULL:(*it).second;
		if(pmsnc1==NULL) return; //{ printf("aaaaErr: sessionID=0x%x,pmsnc1=NULL\r\n",sessionID);	return; }
		
		if(lfooter==MSNINVITE_TYPE_PICTURE) //0x01) //User Display Images and Emoticons
		{
			if(dwFlags==0) //准备开始发送Display Images and Emoticons数据，注意接收
			{//打开文件准备写
				pmsnc1->beginWrite();
				pmsnc1->sendmsg_ACK(pBinarystuff); //回应ACK消息
				return;
			}
			else if(dwFlags==0x20) //Display Images and Emoticons数据
			{
				pmsnc1->writeFile(ptrmsg,dataMessageSize);
				if((dataMessageSize+dataOffset)<totalSize) return;//数据未接收完毕
			}//?else if(dwFlags==0x20)
		}//?if(lfooter==0x01)
		else if(lfooter==MSNINVITE_TYPE_FILE) //0x02//for File Transfers
		{
			if(dwFlags==0x01000030) //文件数据包
			{//此消息包含地是文件传输数据.totalSize是文件大小
				pmsnc1->writeFile(ptrmsg,dataMessageSize);
				if((dataMessageSize+dataOffset)<totalSize) return;//数据未接收完毕
			}//?if(dwFlags==0x01000030) //文件数据包
		}//?else if(lfooter==0x02)
/*		else if(lfooter==MSNINVITE_TYPE_ROBOT)
		{//ptrmsg格式:
			return;
		} */
		pmsnc1->endWrite();//写文件结束
		pmsnc1->sendmsg_ACK(pBinarystuff); //回应ACK消息

		//yyc add 2006-05-19
		onInvite((HCHATSESSION)pcon,pmsnc1->inviteType(),MSNINVITE_CMD_COMPLETED,pmsnc1);

		pcon->m_msnc1Maps.erase(it);
		if(lfooter==0x01){ 
			pmsnc1->sendmsg_BYE(); 
			std::map<std::string,cMsncx *>::iterator it1=pcon->m_msncxMaps.find(pmsnc1->m_callID);
			if(it1!=pcon->m_msncxMaps.end()){ pcon->m_msncxMaps.erase(it1); delete pmsnc1; }
		}//?if(lfooter==0x01)
		return;
	}//?if(lfooter!=0)

	if(sessionID!=0) return;//{ printf("aaaaErr: lfooter=0, sessionID==0x%x\r\n",sessionID);	return; }

	//------------------------------------------------------------------------------------------
	//----------------------------处理消息------------------------------------------------------
	//保证接受完整地消息，解析的总是一条完整得消息
	if(dataOffset!=0)
	{
		if(pcon->m_buffer.size()<totalSize) return;
		::memcpy(pcon->m_buffer.str()+pcon->m_buffer.len(),ptrmsg,dataMessageSize);
		pcon->m_buffer.len()+=dataMessageSize;
		if((dataMessageSize+dataOffset)<totalSize) return;
		ptrmsg=pcon->m_buffer.str(); 
		dataOffset=0; dataMessageSize=totalSize;
	}
	else if(dataMessageSize<totalSize)
	{//消息未接受完
		if(pcon->m_buffer.size()<totalSize){
			pcon->m_buffer.Resize(0);
			pcon->m_buffer.Resize(totalSize);
		}
		if(pcon->m_buffer.size()!=0){
			::memcpy(pcon->m_buffer.str(),ptrmsg,dataMessageSize);
			pcon->m_buffer.len()=dataMessageSize;
		}
		return;
	}//?else if(dataMessageSize<totalSize)
	
	const char *ptr_CallID=NULL;//指向Call-ID
	const char *ptr_ContentType=NULL;//指向Content-Type
	const char *ptr_Context=NULL; int ptr_Context_len=0;
	const char *ptr_SessionID=NULL;//指向 SessionID
	const char *ptr_AppID=NULL;
	const char *ptr_branch=NULL;
	//开始解析消息-----------start-------------------------
	char *tmpptr,*ptr,*pStart=ptrmsg;
	while( (ptr=strchr(pStart,'\r')) )
	{
		*ptr=0;
		if( (tmpptr=strchr(pStart,':')) )
		{
			*tmpptr=0;
			if(strcmp(pStart,"Call-ID")==0)
				ptr_CallID=tmpptr+2;
			else if(strcmp(pStart,"Content-Type")==0)
				ptr_ContentType=tmpptr+2;
			else if(strcmp(pStart,"SessionID")==0)
				ptr_SessionID=tmpptr+2;
			else if(strcmp(pStart,"AppID")==0)
				ptr_AppID=tmpptr+2;
			else if(strcmp(pStart,"Via")==0)
			{
				if( (ptr_branch=strstr(tmpptr+2,";branch=")) ) ptr_branch+=8;
			}
			else if(strcmp(pStart,"Context")==0)
			{
				ptr_Context=tmpptr+2; //Context是经过base64编码地要解码
				ptr_Context_len=strlen(ptr_Context);
				ptr_Context_len=cCoder::base64_decode((char *)ptr_Context,ptr_Context_len,(char *)ptr_Context);
				*((char *)ptr_Context+ptr_Context_len)=0;
//				RW_LOG_PRINT(LOGLEVEL_DEBUG,"Context Base64-decode(%d): %s.\r\n",ptr_Context_len,ptr_Context);
//				for(int ii=0;ii<ptr_Context_len;ii++){	printf("0x%x ",*((unsigned char *)ptr_Context+ii)); if(((ii+1)%16)==0) printf("\r\n");} 
			}
		}//?if( (tmpptr=strchr(pStart,':')) )
		pStart=ptr+1; while(*pStart=='\r' || *pStart=='\n') pStart++; //跳过\r\n
	}//?while(...
	//解析消息结束----------- end -------------------------
	if(ptr_ContentType==NULL) return;//{ printf("aaaaErr: ptr_ContentType=NULL.\r\n");	return; }
	if(ptr_CallID==NULL) return;//{ printf("aaaaErr: sessionID==0 && ptr_CallID==NULL.\r\n");	return; }

	cMsnc1 *pmsnc1=NULL;
	std::map<std::string,cMsncx *>::iterator it=pcon->m_msncxMaps.find(ptr_CallID);
	if(it!=pcon->m_msncxMaps.end()) pmsnc1=(cMsnc1 *)(*it).second;
	if(pmsnc1) pmsnc1->sendmsg_ACK(pBinarystuff); //回应ACK消息
	if(strncmp(ptrmsg,"BYE ",4)==0)
	{
		if(pmsnc1==NULL) return;
		RW_LOG_PRINT(LOGLEVEL_DEBUG,0,"[msnc1] Received BYE of x-msnmsgrp2p\r\n");
		pmsnc1->sendmsg_Got_BYE();
		sessionID=atol(pmsnc1->m_sessionID.c_str());

		pcon->m_msncxMaps.erase(it); delete pmsnc1;
		std::map<long,cMsnc1 *>::iterator it1=pcon->m_msnc1Maps.find(sessionID);
		if(it1!=pcon->m_msnc1Maps.end()) pcon->m_msnc1Maps.erase(it1);
	}
	else if(strncmp(ptrmsg,"MSNSLP/1.",9)==0)
	{
		if(pmsnc1==NULL) return;
		int respcode=atoi(ptrmsg+11);
		sessionID=atol(pmsnc1->m_sessionID.c_str());
		RW_LOG_PRINT(LOGLEVEL_DEBUG,"[msnc1] Received response of x-msnmsgrp2p,respcode=%d\r\n",respcode);
		if(respcode==200){ //成功接收响应
			pcon->m_msnc1Maps[sessionID]=pmsnc1;
			onInvite((HCHATSESSION)pcon,pmsnc1->inviteType(),MSNINVITE_CMD_ACCEPT,pmsnc1);
			if(pmsnc1->inviteType()==MSNINVITE_TYPE_FILE)
				m_threadpool.addTask((THREAD_CALLBACK *)&cMsnc1::sendThread,(void *)pmsnc1,THREADLIVETIME);
		}///if(respcode==200)
		else {
			onInvite((HCHATSESSION)pcon,pmsnc1->inviteType(),MSNINVITE_CMD_REJECT,pmsnc1);
			pcon->m_msncxMaps.erase(it);
			std::map<long,cMsnc1 *>::iterator it1=pcon->m_msnc1Maps.find(sessionID);
			if(it1!=pcon->m_msnc1Maps.end()) pcon->m_msnc1Maps.erase(it1);
			delete pmsnc1;
		}
	}//?else if(strncmp(ptrmsg,"MSNSLP/1.",9)==0)
	else if(strncmp(ptrmsg,"INVITE ",7)==0)
	{
		if(strcmp(ptr_ContentType,"application/x-msnmsgr-sessionreqbody")==0)
		{
			if(ptr_AppID==NULL || ptr_Context==NULL) return;
			int inviteTypeID=atoi(ptr_AppID);
			if( (pmsnc1=new cMsnc1(this,pcon,inviteTypeID))==NULL ) return;	
			if(ptr_SessionID) pmsnc1->m_sessionID.assign(ptr_SessionID);
			if(ptr_branch) pmsnc1->m_branch.assign(ptr_branch);
			pmsnc1->m_callID.assign(ptr_CallID);
			pmsnc1->sendmsg_ACK(pBinarystuff); //回应ACK消息
			if(inviteTypeID==MSNINVITE_TYPE_PICTURE) //某个用户发送请求获取本帐号的头像
				pmsnc1->m_offsetIdentifier-=3;
			
			//解析ptr_Context的内容
			long filesize=0;//传送文件大小
			if(inviteTypeID==MSNINVITE_TYPE_FILE)
			{//文件传输请求，解析要传输的文件名和文件大小，见cmsnc1::sendFile函数
				filesize=*((long *)(ptr_Context+8));
				//文件名是unicode编码
				int len=WideCharToMultiByte(CP_ACP,WC_COMPOSITECHECK|WC_DISCARDNS|WC_SEPCHARS|WC_DEFAULTCHAR,
					(unsigned short *)(ptr_Context+20),-1,(char *)ptr_Context,ptr_Context_len,NULL,NULL);
				*((char *)ptr_Context+len)=0;
				//设置文件名和大小，以便onInvite事件可以通过msncx对象获取文件名和大小，决定是否接收
				pmsnc1->filename().assign(ptr_Context);
				pmsnc1->filesize(filesize);
			}
/*			else if(inviteTypeID==MSNINVITE_TYPE_CAM)
			{//context的内容是unicode编码的UID字符串格式类似于{4BD96FC0-AB17-4425-A14A-439185962DC8}
				int len=WideCharToMultiByte(CP_ACP,WC_COMPOSITECHECK|WC_DISCARDNS|WC_SEPCHARS|WC_DEFAULTCHAR,
					(unsigned short *)(ptr_Context),-1,(char *)ptr_Context,ptr_Context_len,NULL,NULL);
				*((char *)ptr_Context+len)=0;
			}//?else if(inviteTypeID==INVITE_TYPE_CAM)
			else if(inviteTypeID==MSNINVITE_TYPE_ROBOT) //机器人邀请
			{//context的内容是unicode编码的<邀请类型>;1;<机器人名称>
				int len=WideCharToMultiByte(CP_ACP,WC_COMPOSITECHECK|WC_DISCARDNS|WC_SEPCHARS|WC_DEFAULTCHAR,
					(unsigned short *)(ptr_Context),-1,(char *)ptr_Context,ptr_Context_len,NULL,NULL);
				*((char *)ptr_Context+len)=0;
			}
*/			
			RW_LOG_PRINT(LOGLEVEL_DEBUG,"[msnc1] Received INVITE of x-msnmsgrp2p, type=%d\r\n",inviteTypeID);
			bool bAccept=onInvite((HCHATSESSION)pcon,inviteTypeID,
									MSNINVITE_CMD_INVITE,pmsnc1);
			if(bAccept)
			{
				pmsnc1->sendmsg_ACCEPT();//接受请求
				bool bValid=false;
				if(inviteTypeID==MSNINVITE_TYPE_PICTURE)
				{//准备发送本帐号的头像数据,context的内容是没经过mime编码的msnobj对象字符串
					if( (bValid=pmsnc1->sendPicture(m_photofile.c_str())) )
						m_threadpool.addTask((THREAD_CALLBACK *)&cMsnc1::sendThread,(void *)pmsnc1,THREADLIVETIME);
				}
				else if(inviteTypeID==MSNINVITE_TYPE_FILE)
				{	//对方可能进行Direct-Connect handshake
					//那样会先发送一个content-type==application/x-msnmsgr-transreqbody的invite
					if( (bValid=pmsnc1->beginWrite(ptr_Context,filesize)) )
					{
						sessionID=atol(pmsnc1->m_sessionID.c_str());
						pcon->m_msnc1Maps[sessionID]=pmsnc1;
					}
				}//?else if(inviteTypeID==INVITE_TYPE_FILE)
/*				else if(inviteTypeID==MSNINVITE_TYPE_ROBOT) //机器人邀请
				{
					RW_LOG_PRINT(LOGLEVEL_INFO,"[msnc1] Robot invite,context=%s\r\n",ptr_Context);
				} */
				else
					RW_LOG_PRINT(LOGLEVEL_INFO,"[msnc1] unknowed invite,AppID=%d\r\n",inviteTypeID);	
				if(bValid){ pcon->m_msncxMaps[ptr_CallID]=pmsnc1; pmsnc1=NULL; }
			}//?if(bAccept)
			else
				pmsnc1->sendmsg_REJECT();//拒绝请求
			delete pmsnc1; return;
		}//?f(strcmp(ptr_ContentType,...
		else
		{
			if(pmsnc1==NULL) return;//{ printf("aaaaErr: pmsnc1==NULL.\r\n");	return; }
/*			if(strcmp(ptr_ContentType,"application/x-msnmsgr-transreqbody")==0)
			{//Direct-Connect handshake INVITE
				//不支持直接连接
			}
			else if(strcmp(ptr_ContentType,"application/x-msnmsgr-transrespbody")==0)
			{
			}
*/
		}//?f(strcmp(ptr_ContentType,...else...
	}//?else if(strncmp(ptrmsg,"INVITE ",7)==0)
	
	return;
}

//解析msnc0消息协议
/*
MSG yycnet@hotmail.com yyc:) 29\r\n
MSG len=293, MIME-Version: 1.0\r\n
Content-Type: text/x-msmsgsinvite; charset=UTF-8\r\n
\r\n
Application-Name: NetMeeting\r\n
Application-GUID: {44BBA842-CC51-11CF-AAFA-00AA00B6015C}\r\n
Session-Protocol: SM1\r\n
Invitation-Command: INVITE\r\n
Invitation-Cookie: 25402056\r\n
Session-ID: {C4E9035F-CCEB-40F0-8F17-135FB734073B}\r\n
\r\n\r\n
*/
/*   音频聊天
MSG yycnet@hotmail.com yyc:) 491\r\n
MSG len=491, MIME-Version: 1.0\r\n
Content-Type: text/x-msmsgsinvite; charset=UTF-8\r\n
\r\n
Application-Name: 闊抽瀵硅瘽\r\n
Application-GUID: {02D3C01F-BF30-4825-A83A-DE7AF41648AA}\r\n
Session-Protocol: SM1\r\n
Context-Data: Requested:SIP_A,;Capabilities:SIP_A,;\r\n
Invitation-Command: INVITE\r\n
Avm-Support: 7\r\n
Avm-Request: 2\r\n
Invitation-Cookie: 26216456\r\n
Session-ID: {2175E8D4-7CAA-49DD-A520-C2786E891F6F}\r\n
Conn-Type: IP-Restrict-NAT\r\n
Sip-Capability: 1\r\n
Public-IP: 61.237.235.88\r\n
Private-IP: 192.168.0.12\r\n
UPnP: TRUE\r\n
\r\n\r\n
*/
void msnMessager :: msnc0_parse(cContactor *pcon,const char *msg_email,char *ptrmsg)
{
	std::map<std::string,cMsncx *> &msncxMaps=pcon->m_msncxMaps;

	const char *ptr_InviteCommand=NULL;//指向Invitation-Command
	const char *ptr_InviteCookie=NULL;//指向Invitation-Cookie
	const char *ptr_ApplicationName=NULL;//指向Application-Name 邀请类型描述
	const char *ptr_ApplicationGUID=NULL;//指向Application-GUID 邀请类型的UID
	
	const char *ptr_Connectivity=NULL;//指向Connectivity 邀请发起者是否是直接连接，即不在防火墙后面
	const char *ptr_ApplicationFile=NULL;//指向Application-File 文件传输的文件名
	const char *ptr_ApplicationFileSize=NULL;//指向Application-FileSize 文件传输的大小

	//接受邀请，数据连接的IP和端口
	const char *ptr_IPAddress=NULL;//指向IP-Address
	const char *ptr_Port=NULL;//指向Port
	const char *ptr_IPAddress_Internal=NULL;//指向IP-Address
	const char *ptr_PortX=NULL;//指向Port

	const char *ptr_AuthCookie=NULL;//指向AuthCookie
	//拒绝邀请
	const char *ptr_CancelCode=NULL; //指向Cancel-Code，拒绝的原因
	
//	RW_LOG_PRINT(LOGLEVEL_DEBUG,"[msnc0] %s.\r\n",ptrmsg);
	//开始解析消息-----------start-------------------------
	char *tmpptr,*ptr,*pStart=ptrmsg;
	while( (ptr=strchr(pStart,'\r')) )
	{
		*ptr=0;
		if( (tmpptr=strchr(pStart,':')) )
		{
			*tmpptr=0;
			if(strcmp(pStart,"Invitation-Command")==0)
				ptr_InviteCommand=tmpptr+2;
			else if(strcmp(pStart,"Invitation-Cookie")==0)
				ptr_InviteCookie=tmpptr+2;
			else if(strcmp(pStart,"Application-Name")==0)
				ptr_ApplicationName=tmpptr+2;
			else if(strcmp(pStart,"Application-GUID")==0)
				ptr_ApplicationGUID=tmpptr+2;
			else if(strcmp(pStart,"Cancel-Code")==0)
				ptr_CancelCode=tmpptr+2;
			else if(strcmp(pStart,"Application-File")==0)
				ptr_ApplicationFile=tmpptr+2;
			else if(strcmp(pStart,"Application-FileSize")==0)
				ptr_ApplicationFileSize=tmpptr+2;
			else if(strcmp(pStart,"AuthCookie")==0)
				ptr_AuthCookie=tmpptr+2;
			else if(strcmp(pStart,"IP-Address")==0)
				ptr_IPAddress=tmpptr+2;
			else if(strcmp(pStart,"Port")==0)
				ptr_Port=tmpptr+2;
			else if(strcmp(pStart,"IP-Address-Internal")==0)
				ptr_IPAddress_Internal=tmpptr+2;
			else if(strcmp(pStart,"PortX")==0)
				ptr_PortX=tmpptr+2;
			else if(strcmp(pStart,"Connectivity")==0)
				ptr_Connectivity=tmpptr+2;
		}//?if( (tmpptr=strchr(pStart,':')) )
		pStart=ptr+1; while(*pStart=='\r' || *pStart=='\n') pStart++; //跳过\r\n
	}//?while(...
	//解析消息结束----------- end -------------------------
	if(ptr_InviteCommand==NULL) return;
	if(ptr_InviteCookie==NULL) return;

	if(strcmp(ptr_InviteCommand,"INVITE")==0)
	{//邀请请求
		if(ptr_ApplicationGUID==NULL) return;//GUID代表邀请的类型
		int inviteType=MSNINVITE_TYPE_UNKNOW;
		if(strcmp(ptr_ApplicationGUID,"{5D3E02AB-6190-11d3-BBBB-00C04F795683}")==0)
			inviteType=MSNINVITE_TYPE_FILE;
		else if(strcmp(ptr_ApplicationGUID,"{44BBA842-CC51-11CF-AAFA-00AA00B6015C}")==0)
			inviteType=MSNINVITE_TYPE_NETMEET;
		else if(strcmp(ptr_ApplicationGUID,"{2175E8D4-7CAA-49DD-A520-C2786E891F6F}")==0)
			inviteType=MSNINVITE_TYPE_AUDIO;//音频聊天
		if(inviteType==MSNINVITE_TYPE_UNKNOW){
			RW_LOG_PRINT(LOGLEVEL_INFO,"[msnc0] unknowed invite,GUID=%s.\r\n",ptr_ApplicationGUID); 
			return;
		}
		cMsnc0 *pmsnc0=new cMsnc0(this,pcon,ptr_InviteCookie);
		if(pmsnc0==NULL) return;//{ printf("aaaaErr: new pmsnc0==NULL.\r\n");	return; }
		
		long filesize=0; std::string filename;
		if(inviteType==MSNINVITE_TYPE_FILE) //文件传输请求
		{
			filesize=(ptr_ApplicationFileSize)?atol(ptr_ApplicationFileSize):0;
			if(ptr_ApplicationFile){//进行utf8解码
				int len=cCoder::utf8_decode(ptr_ApplicationFile,strlen(ptr_ApplicationFile),(char *)ptr_ApplicationFile);
				*((char *)ptr_ApplicationFile+len)=0; filename.assign(ptr_ApplicationFile);
			}
		}//?if(inviteType==INVITE_TYPE_FILE)
		bool bAccept=onInvite((HCHATSESSION)pcon,inviteType,MSNINVITE_CMD_INVITE,pmsnc0);
		if(bAccept)
		{
			bool bListen=(m_Connectivity=='Y' && (ptr_Connectivity && strcmp(ptr_Connectivity,"N")==0) )?true:false;
			pmsnc0->sendmsg_ACCEPT(bListen);
			pmsnc0->beginWrite(filename.c_str(),filesize);
			//yyc add 2006-05-19 begin
			if( bListen && //启动侦听，等待对方连接
			    m_threadpool.addTask((THREAD_CALLBACK *)&cMsnc0::msnc0Thread,(void *)pmsnc0,THREADLIVETIME)!=0 )
				pmsnc0=NULL;
			else { msncxMaps[ptr_InviteCookie]=pmsnc0; pmsnc0=NULL; }
			//yyc add 2006-05-19 end
			//yyc remove 2006-05-19 begin 
//			msncxMaps[ptr_InviteCookie]=pmsnc0; pmsnc0=NULL;
//			if(bListen) //启动侦听，等待对方连接
//				m_threadpool.addTask((THREAD_CALLBACK *)&cMsnc0::msnc0Thread,(void *)pmsnc0,THREADLIVETIME)
			//yyc remove 2006-05-19 end
		}//?if(bAccept)
		else pmsnc0->sendmsg_REJECT("REJECT");
		delete pmsnc0; return;
	}//?if(strcmp(ptr_InviteCommand,"INVITE")==0)
	else if(strcmp(ptr_InviteCommand,"ACCEPT")==0)
	{//确认接收的应答
		std::map<std::string,cMsncx *>::iterator it=msncxMaps.find(ptr_InviteCookie);
		if(it==msncxMaps.end()) return;
		cMsnc0 *pmsnc0=(cMsnc0 *)(*it).second;
		onInvite((HCHATSESSION)pcon,pmsnc0->inviteType(),MSNINVITE_CMD_ACCEPT,pmsnc0);
		bool bValid=false;
		if( (ptr_IPAddress_Internal || ptr_IPAddress) && ptr_Port )
		{
			const char *iphost=(ptr_IPAddress_Internal)?ptr_IPAddress_Internal:ptr_IPAddress;
			//设置要连接的对方数据传输侦听服务，用线程异步连接
			pmsnc0->setHostinfo(iphost,atoi(ptr_Port),ptr_AuthCookie);
			bValid=(m_threadpool.addTask((THREAD_CALLBACK *)&cMsnc0::msnc0Thread,(void *)pmsnc0,THREADLIVETIME)!=0);
		}//?if(ptr_IPAddress && ptr_Port )
		else if(pmsnc0->bSender() && m_Connectivity=='Y') //如果我是邀请者，且对方的响应没有地址信息
		{//本帐号开侦听服务端口，进行数据传输
			pmsnc0->sendmsg_ACCEPT(true);//启动一个侦听，等待对方连接
			bValid=(m_threadpool.addTask((THREAD_CALLBACK *)&cMsnc0::msnc0Thread,(void *)pmsnc0,THREADLIVETIME)!=0);
		}
		if(!bValid){//发生错误
			pmsnc0->sendmsg_REJECT("FAIL");
			msncxMaps.erase(it); delete pmsnc0;
		}//?if(!bValid)
		//yyc add 2006-05-19 begin
		else
			msncxMaps.erase(it);
		//yyc add 2006-05-19 end
	}//?else if(strcmp(ptr_InviteCommand,"ACCEPT")==0)
	else if(strcmp(ptr_InviteCommand,"CANCEL")==0)
	{//用户拒绝了请求
		std::map<std::string,cMsncx *>::iterator it=msncxMaps.find(ptr_InviteCookie);
		if(it==msncxMaps.end()) return;
		cMsnc0 *pmsnc0=(cMsnc0 *)(*it).second;
		onInvite((HCHATSESSION)pcon,pmsnc0->inviteType(),MSNINVITE_CMD_REJECT,pmsnc0);
		msncxMaps.erase(it); delete pmsnc0;
	}//?else if(strcmp(ptr_InviteCommand,"CANCEL")==0)
	return;
}
