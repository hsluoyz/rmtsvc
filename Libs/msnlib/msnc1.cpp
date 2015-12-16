/*******************************************************************
*	msnc1.cpp
*    DESCRIPTION:msnc1协议处理类的实现。
*				
*    AUTHOR:yyc
*
*    HISTORY:
*
*    DATE:2005-07-05
*
*******************************************************************/

#include "../../include/sysconfig.h"
#include "../../include/cCoder.h"
#include "../../utils/utils.h"
#include "../../include/cLogger.h"
#include "msnlib.h"

using namespace std;
using namespace net4cpp21;

//MSNC1协议由  MSNC1协议头 + MSNP2P协议构成
//MSNP2P协议由 48字节binary stuff + MSNSLP协议构成
/* //MSNC1协议格式
MSNC1协议头			--------------|
48字节binary stuff  ---|          |
Optional Data       ---| msnp2p   | msnc1
4字节的footer       ---|          |
*/ //				--------------|

//MSNC1协议头 格式如下
// MSG yycnet@hotmail.com yyc:) 91  //接收时的起始行
// MSG <trID> D <content-length>    //发送时的起始行
//MIME-Version: 1.0\r\n
//Content-Type: application/x-msnmsgrp2p\r\n
//P2P-Dest: <dest-email>\r\n
//\r\n

//P2P Messages
//MSN Messenger 6 also introduces a new message type, the "application/x-msnmsgrp2p". 
//This message type enables the exchange of several features. The P2P Message is almost a normal message 
//but it has some binary stuff in it. The message has a 48-byte binary header in Little Endian order, 
//then optional plain text(MSNSLP) data and after that a 4-byte binary footer which is in Big Endian order.

//48字节binary stuff结构说明如下
//The 48-byte binary header consists of 6 DWORDs and 3 QWORDS, which are all in Little Endian order, where a DWORD is 32-bit unsigned integer. The first field is a DWORD and is the SessionID, which is zero when the Clients are negotiating about the session. 
//The second field is a DWORD and is the Identifier which identifies the message, the first message you receive from the other Client is the BaseIndentifier, the other messages contains this Identifier +1 or -2 or something like that, the BaseIdentifier is random generated. The Identifier can be in range from 4 to a max of 4294967295, I think. 
//The third field is a QWORD and is the Offset which is only being used if the data which is being sent is larger then 1202 bytes. For example: if you have some data which has a length 1496 bytes, then the first P2P Message contains an Offset field of 0 and the second contains an Offset of 1202 if the length of the data in the first message is 1202 bytes. 
//The fourth field is a QDWORD and represents the total size of the data which is being sended, if this message contains no data then the field should be the length of the data of the previous received message and the Flag field should be 0x2 then. 
//The fifth field is a DWORD and is the length of the data which is being transferred in this message. (不包括48字节binary stuff和4字节的footer)
//The sixth field is a DWORD and is the Flag field, it's 0x0 when no flags are specified, 0x2 if it's an reply to a received message, 0x8 if there is an error on the binary level, 0x20 when the data is for User Display Images or Emoticons, 0x01000030 if it's the data of a file. 
//The seventh field is a DWORD and is an important field, if the SessionID field is zero and the data doesn't contain the SessionID then this field contains the Identifier of the previous received message, most of the time the Flag field is 0x2 then. If the data contains the SessionID or if the SessionID field is non-zero then this field is just some random generated number. 
//The eigth field is a DWORD and is also important, because if it met the conditions explained with DWORD nine, then this field contains Field 9 of the previous received message. 
//The ninth field is a QWORD and is also important, because if it met the conditions explained with DWORD nine, then this field contains the total length of the data of the previous received message.

//Optional Data
//The plain text data is more SIP like(MSNSLP), you can also call this data (when it isn't file data) the request message. 

//Footer
//The 4-byte binary footer consists of 1 DWORD which is in Big Endian order and that field represents the ApplicationIdentifier (AppID). 
//If this field is zero, then the Clients are negotiating about the session. 
//The ApplicationIdentifier is 0x1 for User Display Images and Emoticons, 0x2 for File Transfers, 
//and for Games it's specified by the Start Menu code.


/* //MSNSLP协议
start-Line
message-header
CRLF
[ message-body ]
*/
/* //MSNSLP start-Line
Method SPACE MSNMSGR:buddy@mail.com SPACE MSNSLP-Version CRLF
或
MSNSLP-version SPACE Status-Code SPACE Reason-Phrase CRLF
*/
//Methods
//The client only uses two methods which are "INVITE" and "BYE". It uses INVITE to start a session and BYE to end a session. If you receive a second INVITE message in a session, then it's used to change the session parameters, you should accept it if everything is okay. 
//MSNSLP-Version
//For the MSNSLP v1.0 Protocol this field should be MSNSLP/1.0.
//In all other cases the "start-line" is a Status-Line, consisting of the protocol version followed by a numeric Status-Code and its associated textual phrase, with each element separated by SPACE characters. 
//Status Codes and Reason Phrases
//The Status-Code is a 3-digit integer result code that indicates the outcome of the attempt to understand and satisfy the request. Your client must read the Status-Code to determine whether this is a 200 or for example a 404.

//Message Header
//The "message-header" contains always the following fields in this order: "To", "From", "Via", "CSeq", "Call-ID", "Max-Forwards", "Content-Type" and "Content-Length". 
//When you receive an incoming message, you certainly need to check some fields whether or not they have the right values. If some data is missing or wrong, you should always send an error back.
//For further information read the error handling section of this document. The fields of the message header which you certainly should check are the "To", the "Content-Type" and the "Content-Length" field.

//Message Body
//The "message-body" depends on the type of data requested and there must always be a 0x00 character appended to it at the end. 
//For information about the content of the message body see the relevant sections.
cMsnc1 :: cMsnc1(msnMessager *pmsnmessager,cContactor *pcontact,int inviteType)
			:cMsncx(pmsnmessager,pcontact,inviteType)
{
	m_BaseIdentifier=rand()+0x11111111;
	m_offsetIdentifier=0;
}
cMsnc1 :: ~cMsnc1()
{ 
	m_cond.active();
}

//获取某个联系人的头像。 
bool cMsnc1 :: getPicture(const char *saveas)
{
	if(saveas==NULL || saveas[0]==0) return false;
	//从msnobject中获取头像的文件名和大小
	char msnobj_buf[512]; 
	int msnobj_len=sprintf(msnobj_buf,"%d",rand()+0x11111111);
	msnobj_buf[msnobj_len]=0; m_sessionID.assign(msnobj_buf);//生成sessionID
	msnobj_len=cCoder::mime_decode(m_pcontact->m_strMsnObj.c_str(),m_pcontact->m_strMsnObj.length(),msnobj_buf);
	msnobj_buf[msnobj_len]=0;
	const char *ptr=strstr(msnobj_buf,"Size=\"");
	m_filesize=(ptr)?atol(ptr+6):0;
	m_filename.assign(saveas);
	m_bSender=false;//RC
	m_inviteType=MSNINVITE_TYPE_PICTURE;
	return sendmsg_INVITE(msnobj_buf,msnobj_len);	
}
//发送本联系人的头像，filename --- 本联系人的头像文件
bool cMsnc1 :: sendPicture(const char *filename)
{
	if(filename==NULL || filename[0]==0 ) return false;
	FILE *fp=::fopen(filename,"rb");
	if(fp==NULL) return false;
	fseek(fp,0,SEEK_END); 
	m_filesize=ftell(fp); ::fclose(fp);
	const char *ptr=strrchr(filename,'\\');
	if(ptr){
		m_filename.assign(ptr+1);
		m_filepath.assign(filename,ptr-filename+1);
	}
	else
		m_filename.assign(filename);
	m_bSender=true;//SC
	m_inviteType=MSNINVITE_TYPE_PICTURE;
	return true;
}
//发送指定文件
bool cMsnc1 :: sendFile(const char *filename)
{
	if(filename==NULL || filename[0]==0 ) return false;
	FILE *fp=::fopen(filename,"rb");
	if(fp==NULL) return false;
	fseek(fp,0,SEEK_END); 
	m_filesize=ftell(fp); ::fclose(fp);
	const char *ptr=strrchr(filename,'\\');
	if(ptr){
		m_filename.assign(ptr+1);
		m_filepath.assign(filename,ptr-filename+1);
	}
	else
		m_filename.assign(filename);
	m_bSender=true;//SC
	m_inviteType=MSNINVITE_TYPE_FILE;

	char context_buf[638]; 
	int context_len=sprintf(context_buf,"%d",rand()+0x11111111);
	context_buf[context_len]=0; m_sessionID.assign(context_buf);//生成sessionID
	//生成context内容
	memset((void *)context_buf,0,638);
	context_buf[0]=0x7e; context_buf[1]=2; context_buf[4]=3; context_buf[16]=1;
	*((unsigned char *)context_buf+570)=0xff;
	*((unsigned char *)context_buf+571)=0xff;
	*((unsigned char *)context_buf+572)=0xff;
	*((unsigned char *)context_buf+573)=0xff;
	memcpy((void *)(context_buf+8),(const void *)&m_filesize,4);
	MultiByteToWideChar(CP_ACP,0,m_filename.c_str(),m_filename.length(),(unsigned short *)(context_buf+20),250);
	return sendmsg_INVITE(context_buf,638);
}
/*
//发送一个机器人邀请
bool cMsnc1 :: sendRobotInvite(const char *robotname)
{
	m_bSender=true;
	m_inviteType=MSNINVITE_TYPE_ROBOT;
	//生成context内容
	char context_buf[256]; int context_len;
	if(robotname==NULL || robotname[0]==0 || strlen(robotname)>64)
		 context_len=sprintf(context_buf,"%d;1;robot",MSNINVITE_TYPE_ROBOT);
	else context_len=sprintf(context_buf,"%d;1;%s",MSNINVITE_TYPE_ROBOT,robotname);
	context_buf[context_len]=0;
	unsigned short contextW_buf[256];//将单字节编码转换为unicode双字节编码
	int contextW_len=MultiByteToWideChar(CP_ACP,0,context_buf,context_len,contextW_buf,255);
	return sendmsg_INVITE((const char *)contextW_buf,contextW_len*sizeof(unsigned short));
}
*/
//Sending Acknowledgements
//When a client sends you a P2P-message, you should always reply with a sort of Acknowledgement Message, this message has 5 fields which are important. 
//The "Message Length" field contains the value from the "Message Length" of the Acknowledged message. 
//The "Flag" field contains the flag 0x2 to indicate that this is a Acknowledge message. 
//The "Acknowledged Session Identifier" field contains the Session Identifier of the message to which this is an Acknowledgement. 
//The "Acknowledged Unique Identifier" field contains the Unique Identifier (eigth field) of the message to which this is an Acknowledgement. 
//The "Acknowledged Data Size" field contains the value from the "Message Length" of the Acknowledged message. 
//The rest of the fields is always zero, except the Session Identifier field and the Identifier field of the message. Those are determined by the client. 
//pheader 指向接收MSG的 header
bool cMsnc1 :: sendmsg_ACK(unsigned char *pheader)
{
	char binary_stuff[48+4]; 
	unsigned long lfooter=0;
	::memset((void *)binary_stuff,0,48);
	//sessionID
	memcpy((void *)binary_stuff,(const void *)pheader,4);
	long Identifier=m_BaseIdentifier+m_offsetIdentifier++;
	if(m_offsetIdentifier==0) m_offsetIdentifier++;//跳过0
	//MessageID
	memcpy((void *)(binary_stuff+4),(const void *)&Identifier,4);
	//total size
	memcpy((void *)(binary_stuff+16),(const void *)(pheader+16),8);
	//flag
	binary_stuff[28]=(char)0x02;
	//ack msgid
	memcpy((void *)(binary_stuff+32),(const void *)(pheader+4),4);
	//ack unique id
	memcpy((void *)(binary_stuff+36),(const void *)(pheader+32),4);
	//ack data size
	memcpy((void *)(binary_stuff+40),(const void *)(pheader+16),8);

	//设置lfooter,in Big Endian order 
	binary_stuff[48]=*( ((char *)&lfooter)+3 );
	binary_stuff[48+1]=*( ((char *)&lfooter)+2 );
	binary_stuff[48+2]=*( ((char *)&lfooter)+1 );
	binary_stuff[48+3]=*( ((char *)&lfooter) );
	return (sendMSNC1(binary_stuff,52)!=0);
}

bool cMsnc1 :: sendmsg_Got_BYE()
{
	char binary_stuff[48+4]; 
	unsigned long lfooter=0;
	::memset((void *)binary_stuff,0,48);
	
	long lTmp=atol(m_sessionID.c_str());
	memcpy((void *)binary_stuff,(const void *)&lTmp,4); //sessionID
	long Identifier=m_BaseIdentifier+m_offsetIdentifier++;
	if(m_offsetIdentifier==0) m_offsetIdentifier++;//跳过0
	//MessageID
	memcpy((void *)(binary_stuff+4),(const void *)&Identifier,4);
	//flag
	binary_stuff[28]=(char)0x40;

	//设置lfooter,in Big Endian order 
	binary_stuff[48]=*( ((char *)&lfooter)+3 );
	binary_stuff[48+1]=*( ((char *)&lfooter)+2 );
	binary_stuff[48+2]=*( ((char *)&lfooter)+1 );
	binary_stuff[48+3]=*( ((char *)&lfooter) );
	return (sendMSNC1(binary_stuff,52)!=0);
}

bool cMsnc1 :: sendmsg_ACCEPT()
{
	char binary_stuff[48+512];
	char *pmsnslp=binary_stuff+48;
	const char *toEmail=m_pcontact->m_email.c_str();
	const char *fromEmail=m_pmsnmessager->thisEmail();

	int contentLength=sprintf(binary_stuff,"SessionID: %s\r\n\r\n",m_sessionID.c_str());
	binary_stuff[contentLength]=0; contentLength++; //内容最后补一个'\0'

	int len=sprintf(pmsnslp,"MSNSLP/1.0 200 OK\r\nTo: <msnmsgr:%s>\r\nFrom: <msnmsgr:%s>\r\n"
						"Via: MSNSLP/1.0/TLP ;branch=%s\r\nCSeq: 1\r\nCall-ID: %s\r\n"
						"Max-Forwards: 0\r\nContent-Type: application/x-msnmsgr-sessionreqbody\r\n"
						"Content-Length: %d\r\n\r\n%s"
						,toEmail,fromEmail
						,m_branch.c_str(),m_callID.c_str(),
						contentLength,binary_stuff);
	
	pmsnslp[len]=0; len++;//包含Content最后的'\0'
	
	unsigned long lfooter=0;
	::memset((void *)binary_stuff,0,48);
	long Identifier=m_BaseIdentifier+m_offsetIdentifier++;
	if(m_offsetIdentifier==0) m_offsetIdentifier++;//跳过0
	//MessageID
	memcpy((void *)(binary_stuff+4),(const void *)&Identifier,4);
	//totalSize
	memcpy((void *)(binary_stuff+16),(const void *)&len,4);
	//message size
	memcpy((void *)(binary_stuff+24),(const void *)&len,4);
	
	//设置lfooter,in Big Endian order 
	binary_stuff[48+len]=*( ((char *)&lfooter)+3 );
	binary_stuff[48+len+1]=*( ((char *)&lfooter)+2 );
	binary_stuff[48+len+2]=*( ((char *)&lfooter)+1 );
	binary_stuff[48+len+3]=*( ((char *)&lfooter) );
	
	return (sendMSNC1(binary_stuff,52+len)!=0);
}
bool cMsnc1 :: sendmsg_REJECT()
{
	char binary_stuff[48+512];
	char *pmsnslp=binary_stuff+48;
	const char *toEmail=m_pcontact->m_email.c_str();
	const char *fromEmail=m_pmsnmessager->thisEmail();

	int contentLength=sprintf(binary_stuff,"SessionID: %s\r\n\r\n",m_sessionID.c_str());
	binary_stuff[contentLength]=0; contentLength++; //内容最后补一个'\0'

	int len=sprintf(pmsnslp,"MSNSLP/1.0 603 Decline\r\nTo: <msnmsgr:%s>\r\nFrom: <msnmsgr:%s>\r\n"
						"Via: MSNSLP/1.0/TLP ;branch=%s\r\nCSeq: 1\r\nCall-ID: %s\r\n"
						"Max-Forwards: 0\r\nContent-Type: application/x-msnmsgr-sessionreqbody\r\n"
						"Content-Length: %d\r\n\r\n%s"
						,toEmail,fromEmail
						,m_branch.c_str(),m_callID.c_str(),
						contentLength,binary_stuff);
	
	pmsnslp[len]=0; len++;//包含Content最后的'\0'
	
	unsigned long lfooter=0;
	::memset((void *)binary_stuff,0,48);
	long Identifier=m_BaseIdentifier+m_offsetIdentifier++;
	if(m_offsetIdentifier==0) m_offsetIdentifier++;//跳过0
	//MessageID
	memcpy((void *)(binary_stuff+4),(const void *)&Identifier,4);
	//totalSize
	memcpy((void *)(binary_stuff+16),(const void *)&len,4);
	//message size
	memcpy((void *)(binary_stuff+24),(const void *)&len,4);
	
	//设置lfooter,in Big Endian order 
	binary_stuff[48+len]=*( ((char *)&lfooter)+3 );
	binary_stuff[48+len+1]=*( ((char *)&lfooter)+2 );
	binary_stuff[48+len+2]=*( ((char *)&lfooter)+1 );
	binary_stuff[48+len+3]=*( ((char *)&lfooter) );
	
	return (sendMSNC1(binary_stuff,52+len)!=0);
}

bool cMsnc1 :: sendmsg_ERROR()
{
	char binary_stuff[48+512];
	char *pmsnslp=binary_stuff+48;
	const char *toEmail=m_pcontact->m_email.c_str();
	const char *fromEmail=m_pmsnmessager->thisEmail();

	int len=sprintf(pmsnslp,"MSNSLP/1.0 500 Internal Error\r\nTo: <msnmsgr:%s>\r\nFrom: <msnmsgr:%s>\r\n"
						"Via: MSNSLP/1.0/TLP ;branch=%s\r\nCSeq: 1\r\nCall-ID: %s\r\n"
						"Max-Forwards: 0\r\nContent-Type: null\r\n"
						"Content-Length: 0\r\n\r\n"
						,toEmail,fromEmail
						,m_branch.c_str(),m_callID.c_str());
						
	pmsnslp[len]=0;
	
	unsigned long lfooter=0;
	::memset((void *)binary_stuff,0,48);
	long Identifier=m_BaseIdentifier+m_offsetIdentifier++;
	if(m_offsetIdentifier==0) m_offsetIdentifier++;//跳过0
	//MessageID
	memcpy((void *)(binary_stuff+4),(const void *)&Identifier,4);
	//totalSize
	memcpy((void *)(binary_stuff+16),(const void *)&len,4);
	//message size
	memcpy((void *)(binary_stuff+24),(const void *)&len,4);
	
	//设置lfooter,in Big Endian order 
	binary_stuff[48+len]=*( ((char *)&lfooter)+3 );
	binary_stuff[48+len+1]=*( ((char *)&lfooter)+2 );
	binary_stuff[48+len+2]=*( ((char *)&lfooter)+1 );
	binary_stuff[48+len+3]=*( ((char *)&lfooter) );
	return (sendMSNC1(binary_stuff,52+len)!=0);
}
bool cMsnc1 :: sendmsg_BYE()
{
	char binary_stuff[48+512];
	char *pmsnslp=binary_stuff+48;
	const char *toEmail=m_pcontact->m_email.c_str();
	const char *fromEmail=m_pmsnmessager->thisEmail();

	int len=sprintf(pmsnslp,"BYE MSNMSGR:%s MSNSLP/1.0\r\n"
						"To: <msnmsgr:%s>\r\nFrom: <msnmsgr:%s>\r\n"
						"Via: MSNSLP/1.0/TLP ;branch=%s\r\nCSeq: 0\r\nCall-ID: %s\r\n"
						"Max-Forwards: 0\r\nContent-Type: application/x-msnmsgr-sessionclosebody\r\n"
						"Content-Length: 3\r\n\r\n\r\n"
						,toEmail,toEmail,fromEmail
						,m_branch.c_str(),m_callID.c_str());
	
	pmsnslp[len]=0; len++;//包含Content最后的'\0'
	
	unsigned long lfooter=0;
	::memset((void *)binary_stuff,0,48);
	long Identifier=m_BaseIdentifier+m_offsetIdentifier++;
	if(m_offsetIdentifier==0) m_offsetIdentifier++;//跳过0
	//MessageID
	memcpy((void *)(binary_stuff+4),(const void *)&Identifier,4);
	//totalSize
	memcpy((void *)(binary_stuff+16),(const void *)&len,4);
	//message size
	memcpy((void *)(binary_stuff+24),(const void *)&len,4);
	
	//设置lfooter,in Big Endian order 
	binary_stuff[48+len]=*( ((char *)&lfooter)+3 );
	binary_stuff[48+len+1]=*( ((char *)&lfooter)+2 );
	binary_stuff[48+len+2]=*( ((char *)&lfooter)+1 );
	binary_stuff[48+len+3]=*( ((char *)&lfooter) );
	
	return (sendMSNC1(binary_stuff,52+len)!=0);
}
//-------------------------------------------------------------------------------
unsigned long cMsnc1 :: sendmsg_READY2TRANS()
{
	if(m_inviteType!=MSNINVITE_TYPE_PICTURE) return 0;
	char binary_stuff[48+4+4];
	char *pmsnslp=binary_stuff+48;

	pmsnslp[0]=pmsnslp[1]=pmsnslp[2]=pmsnslp[3]=0;
	int len=4;
	
	unsigned long lfooter=m_inviteType; 
	::memset((void *)binary_stuff,0,48);
	
	long sessionID=atol(m_sessionID.c_str());//sessionID
	memcpy((void *)(binary_stuff),(const void *)&sessionID,4);

	long Identifier=m_BaseIdentifier+m_offsetIdentifier++;
	if(m_offsetIdentifier==0) m_offsetIdentifier++;//跳过0
	//MessageID
	memcpy((void *)(binary_stuff+4),(const void *)&Identifier,4);
	//totalSize
	memcpy((void *)(binary_stuff+16),(const void *)&len,4);
	//message size
	memcpy((void *)(binary_stuff+24),(const void *)&len,4);
	
	//设置lfooter,in Big Endian order 
	binary_stuff[48+len]=*( ((char *)&lfooter)+3 );
	binary_stuff[48+len+1]=*( ((char *)&lfooter)+2 );
	binary_stuff[48+len+2]=*( ((char *)&lfooter)+1 );
	binary_stuff[48+len+3]=*( ((char *)&lfooter) );

	return sendMSNC1(binary_stuff,52+len);
}

//添加MSNC1协议头，然后发送MSNC1消息
//Splitting Messages
//When the content of the message between de binary header and footer is larger then 1202 characters 
//(or 1352 in case of a Direct Connection), you should split the data in seperate parts and add a message header, 
//binary header and a binary footer to it. 
unsigned long  cMsnc1 :: sendMSNC1(const char *ptr_msnp2p,long p2p_length)
{
	char buf[2048]; 
	const char *toEmail=m_pcontact->m_email.c_str();
	unsigned long trID=m_pmsnmessager->msgID();

	long msnc1_headerlen=sprintf(buf+32,"MIME-Version: 1.0\r\nContent-Type: application/x-msnmsgrp2p\r\nP2P-Dest: %s\r\n\r\n"
						,toEmail);
	long length_msnslp=p2p_length-48-4;
	if(length_msnslp<=1202)
	{
		::memcpy(buf+32+msnc1_headerlen,ptr_msnp2p,p2p_length);
		long datalen=p2p_length+msnc1_headerlen;
		int iret=sprintf(buf,"MSG %d D %d\r\n",trID,datalen);
		memmove(buf+(32-iret),buf,iret); 
		return (m_pcontact->m_chatSock.Send(datalen+iret,buf+(32-iret),-1)>0)?trID:0;
	}
	//split 消息
	const char *ptr_Binarystuff=ptr_msnp2p;
	const char *ptr_msnslp=ptr_msnp2p+48;
	long offset=0;
	unsigned long lfooter=*((unsigned long *)(ptr_msnp2p+p2p_length-4));
	//如果length_msnslp>1202要进行split
	while(length_msnslp>0)
	{
		long datalen=(length_msnslp>1202)?1202:length_msnslp;
		length_msnslp-=datalen;
		::memcpy(buf+32+msnc1_headerlen,ptr_Binarystuff,48);
		const char *pHeader=buf+32+msnc1_headerlen; 
		//修正binary 中的offset，dataMessageSize
		*((unsigned long *)(pHeader+8))=offset;
		*((unsigned long *)(pHeader+24))=datalen;
		::memcpy(buf+32+msnc1_headerlen+48,ptr_msnslp,datalen);
		ptr_msnslp+=datalen; offset+=datalen;
		datalen+=(msnc1_headerlen+48);
		*((unsigned long *)(buf+32+datalen))=lfooter;
		datalen+=4; 
		int iret=sprintf(buf,"MSG %d D %d\r\n",trID,datalen);
		memmove(buf+(32-iret),buf,iret); 
		if(m_pcontact->m_chatSock.Send(datalen+iret,buf+(32-iret),-1)<=0) return 0;
	}//?while(msglength>0)
	return trID;
}

inline long createUID(char *buf)
{
	long len=sprintf(buf,"{%4X%4X-%4X-%4X-%4X-%4X%4X%4X}",rand()+0x1111,rand()+0x1111,rand()+0x1111,
		rand()+0x1111,rand()+0x1111,rand()+0x1111,rand()+0x1111,rand()+0x1111);
	return len;
}
char *INVITE_GUID[]={
					"{A4268EEC-FEC5-49E5-95C3-F126696BDBF6}", //Display picture
					"{5D3E02AB-6190-11D3-BBBB-00C04F795683}", //FILE trans
					"",
					"{4BD96FC0-AB17-4425-A14A-439185962DC8}"  //网络摄像头
					};
//char *INVITE_ROBOT_GUID="{6A13AF9C-5308-4F35-923A-67E8DDA40C2F}"; //MSNINVITE_TYPE_ROBOT
bool cMsnc1 :: sendmsg_INVITE(const char *strcontext,int contextLen)
{
	char binary_stuff[48+1024];
	char contextBuf[1024];
	char *pmsnslp=binary_stuff+48;
	const char *toEmail=m_pcontact->m_email.c_str();
	const char *fromEmail=m_pmsnmessager->thisEmail();

	//先将strcontext进行base64编码
	unsigned int oldLineWidth=cCoder::m_LineWidth;
	cCoder::m_LineWidth=1024;
	int len=cCoder::base64_encode((char *)strcontext,contextLen,pmsnslp);
	pmsnslp[len]=0;
/*	if(m_inviteType==MSNINVITE_TYPE_ROBOT)
	{
		contextLen=sprintf(contextBuf,"EUF-GUID: %s\r\nSessionID: %s\r\nAppID: %d\r\n"
							"Context: %s\r\nRobot-Command: auto-accept#%s#\r\n\r\n",
							INVITE_ROBOT_GUID,m_sessionID.c_str(),m_inviteType,pmsnslp,fromEmail);
	}else */
		contextLen=sprintf(contextBuf,"EUF-GUID: %s\r\nSessionID: %s\r\nAppID: %d\r\n"
							"Context: %s\r\n\r\n",INVITE_GUID[m_inviteType-1],
							m_sessionID.c_str(),m_inviteType,pmsnslp);
						
	contextBuf[contextLen]=0; contextLen++; //包含Content最后的'\0'
	cCoder::m_LineWidth=oldLineWidth;

	//生成branch ID和 call ID
	if(m_branch==""){
		len=createUID(pmsnslp); 
		pmsnslp[len]=0; m_branch.assign(pmsnslp);
	}
	if(m_callID==""){
		createUID(pmsnslp); 
		pmsnslp[len]=0; m_callID.assign(pmsnslp);
	}
	
	len=sprintf(pmsnslp,"INVITE MSNMSGR:%s MSNSLP/1.0\r\n"
						"To: <msnmsgr:%s>\r\nFrom: <msnmsgr:%s>\r\n"
						"Via: MSNSLP/1.0/TLP ;branch=%s\r\nCSeq: 0\r\nCall-ID: %s\r\n"
						"Max-Forwards: 0\r\nContent-Type: application/x-msnmsgr-sessionreqbody\r\n"
						"Content-Length: %d\r\n\r\n%s"
						,toEmail,toEmail,fromEmail
						,m_branch.c_str(),m_callID.c_str()
						,contextLen,contextBuf);
	pmsnslp[len]=0; len++;//包含Content最后的'\0'
	
	unsigned long lfooter=0;
	::memset((void *)binary_stuff,0,48);
	long Identifier=m_BaseIdentifier+m_offsetIdentifier++;
	if(m_offsetIdentifier==0) m_offsetIdentifier++;//跳过0
	//MessageID
	memcpy((void *)(binary_stuff+4),(const void *)&Identifier,4);
	//totalSize
	memcpy((void *)(binary_stuff+16),(const void *)&len,4);
	//message size
	memcpy((void *)(binary_stuff+24),(const void *)&len,4);
	memcpy((void *)(binary_stuff+32),(const void *)&m_BaseIdentifier,4);

	//设置lfooter,in Big Endian order 
	binary_stuff[48+len]=*( ((char *)&lfooter)+3 );
	binary_stuff[48+len+1]=*( ((char *)&lfooter)+2 );
	binary_stuff[48+len+2]=*( ((char *)&lfooter)+1 );
	binary_stuff[48+len+3]=*( ((char *)&lfooter) );
	
	return (sendMSNC1(binary_stuff,52+len)!=0);
}

void cMsnc1 :: sendThread(cMsnc1 *pmsnc1)
{
	if(pmsnc1==NULL) return;
	pmsnc1->m_bDataThread_Running=true;
	RW_LOG_PRINT(LOGLEVEL_INFO,0,"msnc1:data-thread of msnMessager has been started.\r\n");
	socketTCP *pchatsession=&pmsnc1->m_pcontact->m_chatSock;
	
	std::string fullpath=pmsnc1->m_filepath+pmsnc1->m_filename;
	if( (pmsnc1->m_fp=::fopen(fullpath.c_str(),"rb")) )
	{ 
		char binary_stuff[1200+48+4]; int readLen=0;
		char *readBuf=binary_stuff+48;
		unsigned long offset=0;
		unsigned long lfooter=pmsnc1->m_inviteType; 
		::memset((void *)binary_stuff,0,48);
		long sessionID=atol(pmsnc1->m_sessionID.c_str());//sessionID
		memcpy((void *)(binary_stuff),(const void *)&sessionID,4);
		long Identifier=pmsnc1->m_BaseIdentifier+pmsnc1->m_offsetIdentifier++;
		if(pmsnc1->m_offsetIdentifier==0) pmsnc1->m_offsetIdentifier++;//跳过0
		//MessageID ,数据传输期间messageID保持不变
		memcpy((void *)(binary_stuff+4),(const void *)&Identifier,4);
		memcpy((void *)(binary_stuff+16),(const void *)&pmsnc1->m_filesize,4);

		std::map<unsigned long,cCond *> &conds=pmsnc1->m_pmsnmessager->m_conds;
		pmsnc1->m_cond.setArgs(0); 
		if(pmsnc1->m_inviteType==MSNINVITE_TYPE_FILE) 
		{ 
			binary_stuff[28]=0x30; binary_stuff[31]=0x01; 
			binary_stuff[29]=binary_stuff[30]=0; 
		}
		else if(pmsnc1->m_inviteType==MSNINVITE_TYPE_PICTURE)
		{
			binary_stuff[28]=0x20;
			//send the data preparation message,发送准备开始发送头像数据消息
			unsigned long trID=pmsnc1->sendmsg_READY2TRANS();
			//发送准备发送头像消息后要等待对方的应答才能发送头像数据，这里等待服务应答就可以.
			conds[trID]=&pmsnc1->m_cond; pmsnc1->m_cond.wait(5);//MSN_MAX_TIMEOUT);
			pmsnc1->m_pmsnmessager->eraseCond(trID); 
		}

		while(pchatsession->status()==SOCKS_CONNECTED)
		{
			readLen=fread(readBuf,sizeof(char),1200,pmsnc1->m_fp);
			if(readLen<=0) break;
//-------------------------------------------------------------------
			//offset
			memcpy((void *)(binary_stuff+8),(const void *)&offset,4);
			//message size
			memcpy((void *)(binary_stuff+24),(const void *)&readLen,4);
			offset+=readLen;
			//设置lfooter,in Big Endian order 
			binary_stuff[48+readLen]=*( ((char *)&lfooter)+3 );
			binary_stuff[48+readLen+1]=*( ((char *)&lfooter)+2 );
			binary_stuff[48+readLen+2]=*( ((char *)&lfooter)+1 );
			binary_stuff[48+readLen+3]=*( ((char *)&lfooter) );
			unsigned long trID=pmsnc1->sendMSNC1(binary_stuff,52+readLen); 
			if(trID){
				conds[trID]=&pmsnc1->m_cond; pmsnc1->m_cond.wait(5);//MSN_MAX_TIMEOUT);
				pmsnc1->m_pmsnmessager->eraseCond(trID); 
			}else break;
//-------------------------------------------------------------------
			if(readLen<1200) break; //文件读取完毕
		}//?while
		if(pmsnc1->m_inviteType==MSNINVITE_TYPE_FILE) pmsnc1->sendmsg_BYE();
		if(pmsnc1->m_fp){ ::fclose(pmsnc1->m_fp); pmsnc1->m_fp=NULL; }

		//yyc add 2006-05-19
		pmsnc1->m_pmsnmessager->onInvite((HCHATSESSION)pmsnc1->m_pcontact,
				pmsnc1->inviteType(),MSNINVITE_CMD_COMPLETED,pmsnc1);
	}//?if(rfp)
	else
		RW_LOG_PRINT(LOGLEVEL_INFO,"msnc1:data-thread failed to open %s.\r\n",fullpath.c_str());
	
	RW_LOG_PRINT(LOGLEVEL_INFO,0,"msnc1:data-thread of msnMessager has been ended.\r\n");
	pmsnc1->m_bDataThread_Running=false; return;
}

//----------------------------------------------------------------------------------
cMsncx :: cMsncx(msnMessager *pmsnmessager,cContactor *pcontact,int inviteType)
{
	m_pcontact=pcontact;
	m_pmsnmessager=pmsnmessager;
	m_inviteType=inviteType;//MSNINVITE_TYPE_UNKNOW;
	m_filesize=0;
	m_bSender=false;
	m_bDataThread_Running=false;
	m_fp=NULL;
}
cMsncx :: ~cMsncx()
{
	if(m_fp) ::fclose(m_fp); 
	while(m_bDataThread_Running) cUtils::usleep(100000);//100ms
}

bool cMsncx:: beginWrite(const char *filename,long filesize)
{
	m_filename.assign(filename);
	m_filesize=filesize;
	m_bSender=false;
	m_inviteType=MSNINVITE_TYPE_FILE;
	return beginWrite();
}

/* 文件传输
First, the Sending Client (SC) need to send the Receiving Client (RC) an Invitation to start a session. 
The Identifier field of that Invitation should contain a generated BaseIdentifier, 
the Identifier field of the following messages should be calculated from that BaseIdentifier. 
首先SC需要向RC发送一个开始邀请的Session请求(INVITE message),消息头的msgid域是一个自动产生的BaseIdentifier
接下来发送的消息头的msgid域的值由BaseIdentifier计算出来。
The Identifier fields of the next messages sent by the RC should be BaseIdentifier + 1, BaseIdentifier + 2 and so on.
RC发送的下一个消息头的msgid域的值应是BaseIdentifier + 1, BaseIdentifier + 2等等
The SC should on its turn send messages with BaseIdentifier + 1, BaseIdentifier + 2 and so on as values for the Identifier field. 
The only thing which is different here is that both Identifier fields will always be BaseIdentifier + a number.
*/
/*
//display picture 
EUF-GUID: {A4268EEC-FEC5-49E5-95C3-F126696BDBF6}
AppID: 1
context==msnobj对象
//file
EUF-GUID: {5D3E02AB-6190-11D3-BBBB-00C04F795683}
AppID: 2
context: base64编码 8~12字节为文件大小 20字节开始为文件名(unicode编码)
//video cam
EUF-GUID: {4BD96FC0-AB17-4425-A14A-439185962DC8}
AppID: 4
context: 
*/

//example
/* invite session请求
INVITE MSNMSGR:yycnet@163.com MSNSLP/1.0
To: <msnmsgr:yycnet@163.com>
From: <msnmsgr:yycnet@hotmail.com>
Via: MSNSLP/1.0/TLP ;branch={747242DA-ECB5-4753-9275-9711629C5D3B}
CSeq: 0
Call-ID: {22AEA6FA-0D9F-4C63-89E0-E4B38E584352}
Max-Forwards: 0
Content-Type: application/x-msnmsgr-sessionreqbody
Content-Length: 947

EUF-GUID: {5D3E02AB-6190-11D3-BBBB-00C04F795683}
SessionID: 65394293
AppID: 2
Context: fgIAAAMAAADCGQAAAAAAAAEAAABiAGIALgB0AHgAdAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA
AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA/////wAAAAAAAAAAAAAAAAAAA
*/
/* //请求文件传输
INVITE MSNMSGR:rmtsvc8@hotmail.com MSNSLP/1.0
To: <msnmsgr:rmtsvc8@hotmail.com>
From: <msnmsgr:yycnet@hotmail.com>
Via: MSNSLP/1.0/TLP ;branch={C14E0CE1-1825-4840-A06E-EBCAC648D906}
CSeq: 0
Call-ID: {F3149DEE-3C5F-49F2-A61E-3388572D9296}
Max-Forwards: 0
Content-Type: application/x-msnmsgr-transreqbody
Content-Length: 102

Bridges: TRUDPv1 TCPv1
NetID: -873796291
Conn-Type: IP-Restrict-NAT
UPnPNat: false
ICF: false
 */
/*[msnc1] msg=INVITE MSNMSGR:yycnet@163.com MSNSLP/1.0
To: <msnmsgr:yycnet@163.com>
From: <msnmsgr:yycnet@hotmail.com>
Via: MSNSLP/1.0/TLP ;branch={394BF77F-3982-4F2E-927F-7E2D00CE95C0}
CSeq: 0
Call-ID: {90E8771E-FEBB-4F42-939E-D7E3033C6C8C}
Max-Forwards: 0
Content-Type: application/x-msnmsgr-transreqbody
Content-Length: 156

Bridges: TRUDPv1 TCPv1
NetID: 1626074429
Conn-Type: IP-Restrict-NAT
UPnPNat: false
ICF: false
Hashed-Nonce: {3279AB80-FEB7-894A-88B8-78478A9E297E}*/
/* //invite session 结束
BYE MSNMSGR:yycnet@163.com MSNSLP/1.0
To: <msnmsgr:yycnet@163.com>
From: <msnmsgr:yycnet@hotmail.com>
Via: MSNSLP/1.0/TLP ;branch={25DDE471-B1BF-46BB-A9C9-E8E823413F20}
CSeq: 0
Call-ID: {22AEA6FA-0D9F-4C63-89E0-E4B38E584352}
Max-Forwards: 0
Content-Type: application/x-msnmsgr-sessionclosebody
Content-Length: 3

*/
