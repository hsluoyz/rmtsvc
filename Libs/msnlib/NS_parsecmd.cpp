/*******************************************************************
   *	NS_parsecmd.cpp
   *    DESCRIPTION:处理从NS服务器收到的命令
   *
   *    AUTHOR:yyc
   *
   *    HISTORY:
   *
   *    DATE:2005-06-28
   *	net4cpp 2.0
   *******************************************************************/

#include "../../include/sysconfig.h"
#include "../../include/cCoder.h"
#include "../../include/cLogger.h"
#include "msnlib.h"

using namespace std;
using namespace net4cpp21;
int splitstring(const char *str,char delm,std::vector<std::string> &vec,int maxSplit=0);

//当你登录成功后NS服务器会发送如下消息
// SBS 0 null\r\n     --- 不知何意
//MSG Hotmail Hotmail 514 ---本帐号登录信息,见MSG消息处理说明
// GTC A.   --- 不知何意
// BLP AL.  --- 指示当某个联系人加了你之后，如果你还没有加他，则默认是AL，即好像他可以access你
// PRP MFN yyc:). --- 本帐号的昵称
// PRP HSB 1.  --- 本帐号有MSN Space空间
// PRP MBE N.  --- 没有绑定移动消息的功能
// PRP WWE 0.  --- 不知何意
//接下来是LSG LST等消息

/*MSG Hotmail Hotmail 514
MIME-Version: 1.0
Content-Type: text/x-msmsgsprofile; charset=UTF-8
LoginTime: 1119866712                              -----In all known cases, immediately after sending the final USR, the NS will send a profile message. This is a message with a Content-Type of text/x-msmsgsprofile. This message has a large header with lots of fields, and no body. 
EmailEnabled: 1                                    -----Whether or not the user's account has email notification (currently just activated Hotmail and MSN.com accounts) - 1 or 0
MemberIdHigh: 94065                                -----High 32 bits of the Passport Unique ID 
MemberIdLow: 923558286                             -----Low 32 bits of the Passport Unique ID 
lang_preference: 2052                              -----Preferred language number 
preferredEmail:                                    -----User's primary email address 
country: CN                                        -----Two-digit country code 
PostalCode:                                        -----User's post-code (or zip code, in the U.S.) 
Gender:                                            -----User's gender (m, f, or U if unspecified) 
Kid: 0                                             -----Whether your account is a Kids Passport (0 or 1)
Age:
BDayPre:                                           -----Birthday Preference, Numbered 0 to 5 (see below) 
Birthday:
Wallet:                                            -----Whether you have an MS Wallet? (0 or 1) 
Flags: 1073759299                                  -----Unknown 
sid: 507                                           -----A number needed for Hotmail login 
kv: 6                                              -----Another number needed for Hotmail login 
MSPAuth: 6UrfLC338HpPz1Zhr2LoB3hFillQ7kZgG38veXiF4f9eUWpEEcPNoWUqhMGIpAbk9os5xg1ogC6kJ23B4uMHUcbmMmCshabw0Wnfw0RtqBOpo*Tnr*ZcyWlA$$
ClientIP: 61.237.234.103
ClientPort: 24695
ABCHMigrated: 1
*/
/*BDayPre can be numbered 0 to 5. These numbers indicate the following information: 
0 - No Information
1 - User is 18+, no specified Day or Month for Birthday.
2 - User is 18+, filled out full Day, Month and Year for Birthday.
3 - User is under 18
4 - User is under 13
5 - User is between the ages of 13 and 18 (or is 13 exactly)
*/
int contact_counts=0;//联系人个数 接收完搜有联系人信息后可发送上线状态
std::string last_contact_email=""; //最后一次收到LST的联系人email
//NS服务器发送的命令
unsigned long msnMessager :: nscmd_sbs(socketTCP *psock,const char *pcmd)
{
	return 0;
}
//NS服务器发送的命令 Profile Messages MSNP8及以后版本
//In all known cases, immediately after sending the final USR, the NS will send a profile message. This is a message with a Content-Type of text/x-msmsgsprofile. This message has a large header with lots of fields, and no body. 
//从NS的MSG Hotmail信息中解析本客户端的IP地址，以便知道本客户端是否是通过NAT或代理出去的
unsigned long msnMessager :: nscmd_msg(socketTCP *psock,const char *pcmd)
{
	if(last_contact_email==""){
		const char *ptr1,*ptr=strstr(pcmd,"ClientIP: ");
		m_Connectivity='N';
		if(ptr && (ptr1=strchr(ptr+10,'\r')) ){
			*(char *)ptr1=0; m_clientIP.assign(ptr+10);
			if(strcmp(psock->getLocalIP(),ptr+10)==0) m_Connectivity='Y';
			RW_LOG_PRINT(LOGLEVEL_WARN,"%s ,Connectivity=%c.\r\n",ptr,m_Connectivity);
			*(char *)ptr1='\r';
		}//?if(ptr)
	}
//	else //后续的消息也会有MSG消息出现，例如通知用户状态时
// ILN 10 NLN tanhuijian@hotmail.com tanago....\r\n
//UBX tanhuijian@hotmail.com 68\r\n
//<Data><PSM>闆ㄥ鍐嶆潵浜?/PSM><CurrentMedia></CurrentMedia></Data>
//MSG Hotmail Hotmail 290\r\n
//MIME-Version: 1.0\r\n
//Content-Type: text/x-msmsgsinitialmdatanotification; charset=UTF-8\r\n...
	return 0;
}
//NS服务器对SYN命令的响应
//格式 SYN trID Time1 Time2 listnumbers groupnumbers
//例如: SYN 8 1 0 0 0
//例如: SYN 8 2005-06-24T02:53:13.8170000-07:00 2005-06-19T19:39:19.9730000-07:00 107 4
unsigned long msnMessager :: nscmd_syn(socketTCP *psock,const char *pcmd)
{
	std::vector<std::string> v;
	int iret=splitstring(pcmd,' ',v);
	int group_counts=0;
	contact_counts=0; 
	last_contact_email="";
	if(iret>=6){//得到总的联系人个数和组个数
		contact_counts=atoi(v[4].c_str());
		group_counts=atoi(v[5].c_str());
		onSYN(contact_counts,group_counts);
		if(contact_counts==0) onSIGN();
	}
	return (unsigned long)atol(v[1].c_str());
}
//NS服务器发送的命令  MSNP11 Challenge
//MSN Messenger 7.0.0813 uses: 
char *szClientID="PROD0101{0RM?UBW";  
char *szClientCode="CFHUR$52U_{VIX5T"; 
unsigned long msnMessager :: nscmd_chl(socketTCP *psock,const char *pcmd)
{
	std::vector<std::string> v;
	int iret=splitstring(pcmd,' ',v);
	if(iret<3) return 0;
	msnMessager::MSNP11Challenge(v[2],szClientID,szClientCode);
	psock->Send("QRY %d %s 32\r\n%s",msgID(),szClientID,v[2].c_str());
	return 0;
}
//NS服务器发送的命令. 收到联系人组信息
//格式:LSG 组名 组ID  //组名可能mime编码+utf8编码
//例如:LSG 鍚屼簨 582cbb4e-8695-4028-8b72-5b947bbb543d
unsigned long msnMessager :: nscmd_lsg(socketTCP *psock,const char *pcmd)
{
	std::vector<std::string> v;
	int iret=splitstring(pcmd,' ',v);
	if(iret<3) return 0;
	//先对组名进行mime解码
	iret=cCoder::mime_decode(v[1].c_str(),v[1].length(),(char *)v[1].c_str());
	v[1][iret]=0; 
	//然后进行utf8解码
	wchar_t *gnameW=new wchar_t[iret+1];
	if(gnameW==NULL) return 0;
	iret=cCoder::utf8_decodeW(v[1].c_str(),iret,gnameW);
	m_groups[v[2]]=std::wstring(gnameW);
	onLSG(gnameW,v[2].c_str());

	delete[] gnameW; return 0;
}
//NS服务器发送的命令. 收到联系人信息
//格式 LST N=email [F=nick] [C=xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx] 标识 [组ID]\r\n
//nick --- utf8编码 ，mime编码
//the C= hex string is a GUID (globally unique identifier) that is used to identify the contact in the ADC and REM commands.
//组ID --- xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx.可能隶属多个组，各个组ID之间以,分割
//标识 the principle is a part of, in the same format as MSNP8 - bitwise number where 1=Forward, 2=Allow, 4=Block ,8=Reverse. composed of:FL: 1 AL: 2 BL: 4 RL: 8
//		FL --- 说名此联系人在本帐号的联系人列表中
//		AL --- 本帐号允许此联系人看到
//		BL --- 本帐号阻止了这个联系人
//		RL --- 说明本帐号在这个联系人的列表中.
//If the contact is not in a group then the last parameter is omitted. When in more than one, comes as comma-separated values (x...-xxxx-...x,x...-xxxx-...x,...). 
//If the contact is not part of the Forward list at all, then the parameters F= and C= are omitted too, leaving only a N= and lists value. 
//范例:
//LST N=jackhuo@hotmail.com F=:[%20Jackhuo%20(li) C=78da23a8-d5dc-483d-aabf-098fc487880d 11 45aaff9a-5ffd-408d-a12a-ce3c522fd327
unsigned long msnMessager :: nscmd_lst(socketTCP *psock,const char *pcmd)
{
	std::vector<std::string> v;
	int iret=splitstring(pcmd,' ',v);
	if(iret<3) return 0;
	last_contact_email=::_strlwr((char *)(v[1].c_str()+2));
	cContactor *pcon=_newContact(last_contact_email.c_str(),NULL);
	if(pcon==NULL) return 0; bool ifFlags=true;
	RW_LOG_DEBUG("[msnlib] <-- LST N=%s, contact_counts=%d\r\n",pcon->m_email.c_str(),contact_counts);
	for(int i=2; i<iret;i++){
		if(v[i][0]=='F' && v[i][1]=='=')
		{//先对nick进行mime解码
			int len; wchar_t *nickW=NULL;
			len=cCoder::mime_decode(v[i].c_str(),v[i].length(),(char *)v[i].c_str());
			//然后进行utf8解码
			if( (nickW=new wchar_t[len+1]) ){
				cCoder::utf8_decodeW(v[i].c_str(),len,nickW);
				pcon->m_nick.assign(nickW+2);
				wchar2chars(pcon->m_nick.c_str(),(char *)v[i].c_str(),v[i].length());
				pcon->m_nick_char.assign(v[i].c_str());
				delete[] nickW;
			}
//	printf(" F=%S",pcon->m_nick.c_str());
		}
		else if(v[i][0]=='C' && v[i][1]=='=')
		{
			pcon->m_uid.assign(v[i].c_str()+2);
//	printf(" C=%s",pcon->m_uid.c_str());
		}
		else if(ifFlags) //先出现的为标志
		{
			pcon->m_flags=atoi(v[i].c_str());
			ifFlags=false;
//	printf(" %s",v[i].c_str());
		}
		else
		{//可能隶属多个组，各个组ID之间以,分割
			const char *ptr=strchr(v[i].c_str(),',');
			if(ptr==NULL)
				pcon->m_gid=v[i];
			else{
				*(char *)ptr=0;
				pcon->m_gid.assign(v[i].c_str());
				*(char *)ptr=',';
			}
//	printf(" %s",v[i].c_str());
		}
	}//?for(...
//	printf("\r\n");
	onLST(pcon->m_email.c_str(),pcon->m_nick.c_str(),pcon->m_flags);
	if(--contact_counts==0){//收完所有联系人信息
		onSIGN();
	}else if(contact_counts<0) contact_counts=0;
	return 0;
}
//NS服务器发送的命令
//格式: BPR [PHH|PHW|PHM|HSB|MOB] [tel:<电话>] <其他>
//BPR PHH tel:86%2068036142 0  //联系人的家庭电话 后面的数字不知何意
//BPR PHW tel:86%2065188989 0  //联系人的办公电话 后面的数字不知何意
//BPR PHM tel:86%2013301338823 0 //联系人的移动电话 后面的数字不知何意
//BPR MOB Y //指示此人可接收移动消息
//BPR HSB 1 //It is used to indicate that the principle has an updated MSN Space blog. 
//BPR总是设定最近接收的LST的联系人的属性
//This command is sent by the notification server immediately after it sends a LST command while synchronising. It contains no way of identifying the principle that it refers to, so you must assume that it is for the most recently sent LST. 
//New in MSNP11 is the HSB setting. Other values are PHM (Phone Mobile), PHW (Phone Work), PHH (Phone Home) and MOB (Mobile). 
unsigned long msnMessager :: nscmd_bpr(socketTCP *psock,const char *pcmd)
{
	std::vector<std::string> v;
	int iret=splitstring(pcmd,' ',v);
	if(iret<3) return 0;
	std::map<std::string, cContactor *>::iterator it=m_contacts.find(last_contact_email);
	if(it==m_contacts.end()) return 0;
	cContactor *pcon=(*it).second; if(pcon==NULL) return 0;
	if(v[1]=="PHH")
		pcon->m_bpr_phh=v[2];
	else if(v[1]=="PHW")
		pcon->m_bpr_phw=v[2];
	else if(v[1]=="PHM")
		pcon->m_bpr_phm=v[2];
	else if(v[1]=="MOB")
		pcon->m_bpr_mob=(v[2]=="Y")?'Y':'N';
	else if(v[1]=="HSB")
		pcon->m_bpr_hsb=(v[2]=="1")?1:0;
	return 0;
}

//Setting your display name is now done (since MSNP10) with the PRP command:
//例如：PRP 9 MFN My%20New%20Name\r\n
//As usual the 9 is the TrID, MFN probably stands for "My Friendly Name", and your new display name must, as always, be urlencoded. This new method obsoletes the REA command.
//PRP was already used to modify phone numbers too, with this syntax:
//PRP <TrID> <Kind> <Url-Encoded Phone Number>
//Where Trid is the transaction ID, kind can be PHH for home, PHW for work, and PHM for cell phone. The last parameter is the phone number encoded in url format. The response is always the same string you sent. Like: 
//>>> PRP 9 PHW 55%2036212222\r\n
//<<< PRP 9 PHW 55%2036212222\r\n
//>>> PRP 10 PHM 55%2088765432\r\n
//<<< PRP 10 PHM 55%2088765432\r\n
//>>> PRP 11 PHM \r\n
//<<< PRP 11 PHM \r\n
unsigned long msnMessager :: nscmd_prp(socketTCP *psock,const char *pcmd)
{
	std::vector<std::string> v;
	RW_LOG_PRINT(LOGLEVEL_DEBUG,"[msnlib] <--- %s\r\n",pcmd);
	int iret=splitstring(pcmd,' ',v);
	unsigned long trID=(unsigned long)atol(v[1].c_str());
	if(iret<3) return trID;
	if(trID==0){//发过来的是本帐号的PRP信息
		if(v[1]=="MFN"){
			int len; wchar_t *nickW=NULL;
			len=cCoder::mime_decode(v[2].c_str(),v[2].length(),(char *)v[2].c_str());
			//然后进行utf8解码
			if( (nickW=new wchar_t[len+1]) ){
				cCoder::utf8_decodeW(v[2].c_str(),len,nickW);
				m_curAccount.m_nick.assign(nickW);
				wchar2chars(m_curAccount.m_nick.c_str(),(char *)v[2].c_str(),v[2].length());
				m_curAccount.m_nick_char.assign(v[2].c_str());
				delete[] nickW;
			}
		}
		else if(v[1]=="HSB"){
			m_curAccount.m_bpr_hsb=(v[2]=="1")?1:0;
		}
	}//?if(trID==0)
	else{	
	}
	return trID;
}
//NS服务器对CHG命令的响应
//ILN 本客户端上线后，NS服务器发送的联系人状态
//格式 ILN TrID status email nick clientID...
//范例 ILN 9 NLN yycnet@hotmail.com yyc:) 805306420 %3Cmsnobj%20Creator%3D%22yycnet%40hotmail.com%22%20Size%3D%2216316%22%20Type%3D%223%22%20Location%3D%22TFRB.tmp%22%20Friendly%3D%22AAA%3D%22%20SHA1D%3D%22CWKlKODMVbRMwdc1yYpgQN4%2BsAA%3D%22%20SHA1C%3D%22JCt3eGOZWUAWssdVjOmn0ZPd0nQ%3D%22%2F%3E
unsigned long msnMessager :: nscmd_iln(socketTCP *psock,const char *pcmd)
{
	std::vector<std::string> v;
	int iret=splitstring(pcmd,' ',v);
	unsigned long trID=(unsigned long)atol(v[1].c_str());
	if(iret<4) return trID;
	last_contact_email=v[3]; ::_strlwr((char *)last_contact_email.c_str());
	std::map<std::string, cContactor *>::iterator it=m_contacts.find(last_contact_email);
	if(it==m_contacts.end()) return trID;
	cContactor *pcon=(*it).second; if(pcon==NULL) return trID;
	pcon->m_status=v[2];
	if(iret>=5){//nick
		int len; wchar_t *nickW=NULL;
		len=cCoder::mime_decode(v[4].c_str(),v[4].length(),(char *)v[4].c_str());
		//然后进行utf8解码
		if( (nickW=new wchar_t[len+1]) ){
			cCoder::utf8_decodeW(v[4].c_str(),len,nickW);
			pcon->m_nick.assign(nickW);
			wchar2chars(pcon->m_nick.c_str(),(char *)v[4].c_str(),v[4].length());
			pcon->m_nick_char.assign(v[4].c_str());
			delete[] nickW;
		}
	}
	if(iret>=6) pcon->m_clientID=(unsigned long)atol(v[5].c_str());
	if(iret>=7) pcon->m_strMsnObj=v[6].c_str();

	if(pcon->m_status!="FLN") onLine((HCHATSESSION)pcon,last_contact_email.c_str()); //某个用户上线
	onNLN((HCHATSESSION)pcon,last_contact_email.c_str(),(0x80000000 | 0x0f) );
	return trID;
}
//NS服务器发送的命令. 好友改变/匿称状态
//NLN 当联系人状态改变或名称改变后发送的消息
//格式 NLN status email nick clientID...
//范例 NLN BSY yycnet@hotmail.com yyc:) 805306420 %3Cmsnobj%20Creator%3D%22yycnet%40hotmail.com%22%20Size%3D%2216316%22%20Type%3D%223%22%20Location%3D%22TFRB.tmp%22%20Friendly%3D%22AAA%3D%22%20SHA1D%3D%22CWKlKODMVbRMwdc1yYpgQN4%2BsAA%3D%22%20SHA1C%3D%22JCt3eGOZWUAWssdVjOmn0ZPd0nQ%3D%22%2F%3E
unsigned long msnMessager :: nscmd_nln(socketTCP *psock,const char *pcmd)
{
	std::vector<std::string> v;
	int iret=splitstring(pcmd,' ',v);
	if(iret<3) return 0;
	::_strlwr((char *)v[2].c_str());
	std::map<std::string, cContactor *>::iterator it=m_contacts.find(v[2]);
	if(it==m_contacts.end()) return 0;
	cContactor *pcon=(*it).second; if(pcon==NULL) return 0;
	long flags=0; bool bOnline=false;
	if(pcon->m_status!=v[1]) 
	{ 
		bOnline=(pcon->m_status=="FLN");
		pcon->m_status=v[1]; flags|=1;
		
	} //状态改变
	if(iret>=4){//nick
		int len; wchar_t *nickW=NULL;
		len=cCoder::mime_decode(v[3].c_str(),v[3].length(),(char *)v[3].c_str());
		//然后进行utf8解码
		if( (nickW=new wchar_t[len+1]) ){
			cCoder::utf8_decodeW(v[3].c_str(),len,nickW);
			if(wcscmp(pcon->m_nick.c_str(),nickW)!=0) //昵称改变
			{
				pcon->m_nick.assign(nickW);
				flags|=2; //昵称改变
				wchar2chars(pcon->m_nick.c_str(),(char *)v[3].c_str(),v[3].length());
				pcon->m_nick_char.assign(v[3].c_str());
			}delete[] nickW;
		}
	}
	if(iret>=5){
		unsigned long id=(unsigned long)atol(v[4].c_str());
		if(pcon->m_clientID!=id) flags|=4;
		pcon->m_clientID=id;
	}
	if(iret>=6 && pcon->m_strMsnObj!=v[5]){
		pcon->m_strMsnObj=v[5];
		flags|=8;//头像改变
	}
	if(bOnline) onLine((HCHATSESSION)pcon,v[2].c_str()); //某个用户上线
	onNLN((HCHATSESSION)pcon,v[2].c_str(),flags);
	return 0;
}
//好友离线 //FLN yycnet@hotmail.com
//如果某个联系人阻止了你，你也会收到对方下线的消息
//当对方取消阻止时你将会收到一个NLN消息
unsigned long msnMessager :: nscmd_fln(socketTCP *psock,const char *pcmd)
{
	std::vector<std::string> v;
	int iret=splitstring(pcmd,' ',v);
	if(iret<2) return 0;
	::_strlwr((char *)v[1].c_str());
	std::map<std::string, cContactor *>::iterator it=m_contacts.find(v[1]);
	if(it==m_contacts.end()) return 0;
	cContactor *pcon=(*it).second; if(pcon==NULL) return 0;
	pcon->m_status="FLN"; pcon->m_chatSock.Close();
	offLine((HCHATSESSION)pcon,v[1].c_str()); //某个用户下线
	return 0;
}
//NS服务器发送的命令或REM命令的响应
//格式 REM 0 RL email
//范例 REM 0 RL yycnet@hotmail.com
//	   REM 207 FL 7bae0ef9-575b-42d5-b13d-61ebd3e69ab8
unsigned long msnMessager :: nscmd_rem(socketTCP *psock,const char *pcmd)
{
	std::vector<std::string> v;
	int iret=splitstring(pcmd,' ',v);
	unsigned long trID=(unsigned long)atol(v[1].c_str());
	if(iret<4) return trID;
	cContactor *pcon=NULL;
	if(trID && v[2]=="FL") //从本地删除联系人的响应
	{
		const char *ptr=strchr(v[3].c_str(),'@'); //判断是邮件还是uid
		std::map<std::string, cContactor *>::iterator it1;
		if(ptr)
		{
			::_strlwr((char *)v[3].c_str());
			it1=m_contacts.find(v[3]);
		}
		else{
			it1=m_contacts.begin();
			for(;it1!=m_contacts.end();it1++) if((*it1).second->m_uid==v[3]) break;
		}
		if(it1!=m_contacts.end()){
			if( (pcon=(*it1).second) ){
				pcon->m_flags &=0xfffffffe;
				pcon->m_uid="";
			}
		}
		return trID;
	}//?if(trID && v[2]=="FL")
	::_strlwr((char *)v[3].c_str());
	std::map<std::string, cContactor *>::iterator it=m_contacts.find(v[3]);
	if(it==m_contacts.end()) return trID;
	if( (pcon=(*it).second)==NULL ) return trID; 
	if(v[2]=="AL")
		pcon->m_flags &=0xfffffffd;
	else if(v[2]=="BL")
		pcon->m_flags &=0xfffffffb;
	else if(v[2]=="RL")
	{
		pcon->m_flags &=0xfffffff7;
		if(trID==0) //NS服务器发送过来的某个用户删除了本帐号的消息
		{
			iret=onREM((HCHATSESSION)pcon,v[3].c_str());
			if( (iret & 1) )//删除此帐号
				remEmail(v[3].c_str(),false);
			if( (iret & 2) ) //阻止此帐号
				blockEmail(v[3].c_str());
		}		
	}//?else if(v[2]=="RL")
	else if(v[2]=="FL"){
		pcon->m_flags &=0xfffffffe;
		pcon->m_uid="";
	}
	return trID;
}
//NS服务器发送的命令或ADC命令的响应
//某个用户添加了你或ADC命令的NS响应返回
//ADC 0 RL N=yycnet@hotmail.com F=yyc:)
//ADC 196 FL N=yycnet@163.com F=yycnet@163.com C=7bae0ef9-575b-42d5-b13d-61ebd3e69ab8
unsigned long msnMessager :: nscmd_adc(socketTCP *psock,const char *pcmd)
{
	std::vector<std::string> v;
	int iret=splitstring(pcmd,' ',v);
	unsigned long trID=(unsigned long)atol(v[1].c_str());
	if(iret<4) return trID; 
	const char *strEmail=::_strlwr((char *)v[3].c_str()+2);
	if(trID){//ADC命令的NS响应返回
		std::map<std::string, cContactor *>::iterator it=m_contacts.find(strEmail);
		if(it==m_contacts.end()) return trID; 
		cContactor *pcon=(*it).second; if(pcon==NULL) return trID; 
		if(v[2]=="FL"){
			pcon->m_flags |=0x1;
			if(iret>=5 && v[4][0]=='F' && v[4][1]=='=')
			{
				int len; wchar_t *nickW=NULL;
				len=cCoder::mime_decode(v[4].c_str(),v[4].length(),(char *)v[4].c_str());
				//然后进行utf8解码
				if( (nickW=new wchar_t[len+1]) ){
					cCoder::utf8_decodeW(v[4].c_str(),len,nickW);
					pcon->m_nick.assign(nickW+2);
					wchar2chars(pcon->m_nick.c_str(),(char *)v[4].c_str(),v[4].length());
					pcon->m_nick_char.assign(v[4].c_str());
					delete[] nickW;
				}
			}
			if(iret>=6 && v[5][0]=='C' && v[5][1]=='=')
				pcon->m_uid.assign(v[5].c_str()+2);
		}//?if(v[2]=="FL"){
		else if(v[2]=="AL")
			pcon->m_flags |=0x2;
		else if(v[2]=="BL")
			pcon->m_flags |=0x4;
		else if(v[2]=="RL")
			pcon->m_flags |=0x8;
		onADD((HCHATSESSION)pcon,strEmail);
	}//?if(trID)
	else{ //服务器发送的某个联系人添加了本帐号的消息
		if(v[2]!="RL") return trID; 
		cContactor *pcon=_newContact(strEmail,NULL);
		if(pcon==NULL) return trID; pcon->m_flags |=0x08;
		if(iret>=5 && v[4][0]=='F' && v[4][1]=='=')
		{//先对nick进行mime解码
			int len; wchar_t *nickW=NULL;
			len=cCoder::mime_decode(v[4].c_str(),v[4].length(),(char *)v[4].c_str());
			//然后进行utf8解码
			if( (nickW=new wchar_t[len+1]) ){
				cCoder::utf8_decodeW(v[4].c_str(),len,nickW);
				pcon->m_nick.assign(nickW+2);
				wchar2chars(pcon->m_nick.c_str(),(char *)v[4].c_str(),v[4].length());
				pcon->m_nick_char.assign(v[4].c_str());
				delete[] nickW;
			}
		}
		( onADC((HCHATSESSION)pcon,strEmail) )? addEmail(strEmail,0): blockEmail(strEmail);
	}//?if(trID)...else
	return trID; 
}

unsigned long msnMessager :: nscmd_ubx(socketTCP *psock,const char *email,const char *pdata)
{
	return 0;
}

//从NS收到聊天请求命令
//格式：RNG 178257 207.46.108.53:1863 CKI 1090569642.21212 yycnet@hotmail.com yyc:)
//RNG 17485110 207.46.108.87:1863 CKI 1090571470.9784 yycnet@hotmail.com yyc:)
unsigned long msnMessager :: nscmd_rng(socketTCP *psock,const char *pcmd)
{
	std::vector<std::string> v;
	int iret=splitstring(pcmd,' ',v);
//	if(RW_LOG_CHECK(LOGLEVEL_WARN))
//		RW_LOG_PRINT(LOGLEVEL_WARN,"[RNG - %d]:\r\n\t%s\r\n",iret,pcmd);
	if(iret<6) return 0; 
	::_strlwr((char *)v[5].c_str());
	std::map<std::string, cContactor *>::iterator it=m_contacts.find(v[5]);
	if(it==m_contacts.end()) return 0;
	cContactor *pcon=(*it).second; if(pcon==NULL) return 0;
	if(pcon->m_chatSock.status()==SOCKS_CONNECTED) return 0;

	if( !connectSvr(pcon->m_chatSock,v[2].c_str(),0) ) return 0;
	std::pair<msnMessager *,cContactor *> *pp=new std::pair<msnMessager *,cContactor *>(this,pcon);
	if(pp){
		if(m_threadpool.addTask((THREAD_CALLBACK *)&sessionThread,(void *)pp,THREADLIVETIME))
		{
			pcon->m_chatSock.Send("ANS %d %s %s %s\r\n",msgID(),m_curAccount.m_email.c_str(),
				v[4].c_str(),v[1].c_str());
//			if(RW_LOG_CHECK(LOGLEVEL_WARN))
//				RW_LOG_PRINT(LOGLEVEL_WARN,"[RNG - ANS]: Success to Connect %s\r\n",v[5].c_str());
			return 0;
		} else delete pp;
	}
	pcon->m_chatSock.Close(); return 0;
}



