/*******************************************************************
   *	IPRules.h
   *    DESCRIPTION: IP应用规则过滤类
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2004-10-10
   *	net4cpp 2.1
   *******************************************************************/

#include "include/sysconfig.h"  
#include "include/IPRules.h"
#include "utils/utils.h"

using namespace std;
using namespace net4cpp21;

bool iprules::check(unsigned long ip,unsigned short port,RULETYPE rt)
{
	std::vector<IPRule>::iterator it=m_rules.begin();
	for(;it!=m_rules.end();it++)
	{
		IPRule &iprule=*it;
		if( ( iprule.ruleType & rt )==0 ) continue;
		if( iprule.port_src!=0 && iprule.port_src!=port) continue;
		if( (iprule.IPMask_src & ip)==iprule.IPAddr_src )
		{
			return iprule.bEnabled;
		}
	}//?for(...
	return m_defaultEnabled;
}

bool iprules::check(unsigned long ip_fr,unsigned short port_fr,
					unsigned long ip_to,unsigned short port_to,
					RULETYPE rt)
{
	std::vector<IPRule>::iterator it=m_rules.begin();
	for(;it!=m_rules.end();it++)
	{
		IPRule &iprule=*it;
		if( ( iprule.ruleType & rt )==0 ) continue;
		if(iprule.bBidirection){
			if( ((iprule.port_src==0 || iprule.port_src==port_fr) && 
				(iprule.port_to==0 || iprule.port_to==port_to)) ||
				((iprule.port_src==0 || iprule.port_src==port_to) && 
				(iprule.port_to==0 || iprule.port_to==port_fr)) )
				NULL;
			else continue;
			if( ((iprule.IPMask_src & ip_fr)==iprule.IPAddr_src &&
				(iprule.IPMask_to & ip_to)==iprule.IPAddr_to )  ||
				((iprule.IPMask_src & ip_to)==iprule.IPAddr_src &&
				(iprule.IPMask_to & ip_fr)==iprule.IPAddr_to ) )
				return iprule.bEnabled;
			else continue;
		}else{
			if( (iprule.port_src==0 || iprule.port_src==port_fr) && 
				(iprule.port_to==0 || iprule.port_to==port_to) )
				NULL;
			else continue;
			if( (iprule.IPMask_src & ip_fr)==iprule.IPAddr_src &&
				(iprule.IPMask_to & ip_to)==iprule.IPAddr_to )
				return iprule.bEnabled;
			else continue;
		}
	}//?for(...
	return m_defaultEnabled;
}


//添加一条过滤规则
//格式 [ip] [mask] [port] [type] [enabled]
bool iprules::addRule(const char *strRule)
{
	if(strRule==NULL || strRule[0]==0) return false;
	IPRule iprule; int icount=0;
	const char *ptrBegin=cUtils::strTrimLeft(strRule);
	while(true)
	{
		const char *ptr=strchr(ptrBegin,' ');
		if(ptr) *(char *)ptr='\0';
		if(icount==0)
			iprule.IPAddr_src=inet_addr(ptrBegin);
		else if(icount==1)
			iprule.IPMask_src=inet_addr(ptrBegin);
		else if(icount==2)
			iprule.port_src=atoi(ptrBegin);
		else if(icount==3)
		{
			if(strcasecmp(ptrBegin,"RULETYPE_TCP")==0)
				iprule.ruleType=RULETYPE_TCP;
			else if(strcasecmp(ptrBegin,"RULETYPE_UDP")==0)
				iprule.ruleType=RULETYPE_UDP;
			else if(strcasecmp(ptrBegin,"RULETYPE_ICMP")==0)
				iprule.ruleType=RULETYPE_ICMP;
			else iprule.ruleType =RULETYPE_ALL;
		}
		else if(icount==4)
		{
			if(strcasecmp(ptrBegin,"true")==0)
				iprule.bEnabled=true;
			else if(strcasecmp(ptrBegin,"false")==0)
				iprule.bEnabled=false;
			else
				iprule.bEnabled=(atoi(ptrBegin)==0)?false:true;
		}
		if(ptr) *(char *)ptr=' ';
		if(++icount>4) break;
		if(ptr==NULL) break;
		ptrBegin=cUtils::strTrimLeft(ptr+1);
	}//?while
	if(iprule.IPAddr_src==INADDR_NONE) return false;
	m_rules.push_back(iprule);
	return true;
}
//添加多条过滤规则，各条过滤规则之间用,分割
//格式 [ip] [mask] [port] [type] [enabled],...
long iprules::addRules(const char *strRules)
{
	m_rules.clear();
	if(strRules==NULL || strRules[0]==0) return 0;
	const char *ptrBegin=cUtils::strTrimLeft(strRules);
	while(true)
	{
		const char *ptr=strchr(ptrBegin,',');
		if(ptr) *(char *)ptr='\0';
		addRule(ptrBegin);
		if(ptr) *(char *)ptr=',';
		if(ptr==NULL) break;
		ptrBegin=cUtils::strTrimLeft(ptr+1);
	}//?while
	return m_rules.size();
}

inline void splitIPstr(const char *str,unsigned long &ipaddr,unsigned long &ipmask,unsigned short &port)
{
	ipaddr=ipmask=0; port=0;
	if(str[0]==0) return;
	const char *ptr=strchr(str,':');
	if(ptr){ port=atoi(ptr+1); *(char *)ptr=0; }
	ipaddr=inet_addr(str);
	if(ptr) *(char *)ptr=':';
	ipmask=ipaddr;
	unsigned char *p1=(unsigned char *)&ipmask;
	for(int i=0;i<sizeof(ipmask);i++)
		if(*(p1+i)!=0) *(p1+i)=0xff;
	return;
}
//添加一条双向过滤规则
//格式 [ip:port]<->[ip:port] [type] [enabled]
//掩码为ip地址中非0的段，例如ip=192.168.1.0 则掩码为255.255.255
bool iprules::addRule_new(const char *strRule)
{

	if(strRule==NULL || strRule[0]==0) return false;
	IPRule iprule; int icount=0;
	const char *ptrBegin=cUtils::strTrimLeft(strRule);
	while(true)
	{
		const char *ptr=strchr(ptrBegin,' ');
		if(ptr) *(char *)ptr='\0';
		
		if(icount==0)
		{
			const char *p0=strstr(ptrBegin,"<->");
			if(p0){ //双向过滤规则
				*(char *)p0=0;
				splitIPstr(ptrBegin,iprule.IPAddr_src,iprule.IPMask_src,iprule.port_src);
				const char *p1=p0+3; while(*p1==' ') p1++;
				splitIPstr(p1,iprule.IPAddr_to,iprule.IPMask_to,iprule.port_to);
				iprule.bBidirection=true;
				*(char *)p0='<';
			}
			else if( (p0=strstr(ptrBegin,"->")) )
			{
				*(char *)p0=0;
				splitIPstr(ptrBegin,iprule.IPAddr_src,iprule.IPMask_src,iprule.port_src);
				const char *p1=p0+2; while(*p1==' ') p1++;
				splitIPstr(p1,iprule.IPAddr_to,iprule.IPMask_to,iprule.port_to);
				*(char *)p0='-';
			}
			else if( (p0=strstr(ptrBegin,"<-")) )
			{
				*(char *)p0=0;
				splitIPstr(ptrBegin,iprule.IPAddr_to,iprule.IPMask_to,iprule.port_to);
				const char *p1=p0+2; while(*p1==' ') p1++;
				splitIPstr(p1,iprule.IPAddr_src,iprule.IPMask_src,iprule.port_src);
				*(char *)p0='<';
			}
			else
				splitIPstr(ptrBegin,iprule.IPAddr_src,iprule.IPMask_src,iprule.port_src);
		}//?if(icount==0)
		else if(icount==1)
		{
			if(strcasecmp(ptrBegin,"RULETYPE_TCP")==0)
				iprule.ruleType=RULETYPE_TCP;
			else if(strcasecmp(ptrBegin,"RULETYPE_UDP")==0)
				iprule.ruleType=RULETYPE_UDP;
			else if(strcasecmp(ptrBegin,"RULETYPE_ICMP")==0)
				iprule.ruleType=RULETYPE_ICMP;
			else iprule.ruleType =RULETYPE_ALL;
		}
		else if(icount==2)
		{
			if(strcasecmp(ptrBegin,"true")==0)
				iprule.bEnabled=true;
			else if(strcasecmp(ptrBegin,"false")==0)
				iprule.bEnabled=false;
			else
				iprule.bEnabled=(atoi(ptrBegin)==0)?false:true;
		}

		if(ptr) *(char *)ptr=' ';
		if(++icount>2) break;
		if(ptr==NULL) break;
		ptrBegin=cUtils::strTrimLeft(ptr+1);
	}//?while
//	printf("aaaaaaaaaaaaaaaaaa\r\n");
//	printf("src-0x%x 0x%x:%d dst-0x%x 0x%x:%d \r\nbi=%d type=%d,b=%d\r\n",
//		iprule.IPMask_src,iprule.IPAddr_src,iprule.port_src ,
//		iprule.IPMask_to,iprule.IPAddr_to,iprule.port_to ,
//		iprule.bBidirection,iprule.ruleType,iprule.bEnabled);
	if(iprule.IPAddr_src==INADDR_NONE || iprule.IPAddr_to==INADDR_NONE) 
		return false;
	m_rules.push_back(iprule);
	return true;
}

//添加多条双向过滤规则，各条过滤规则之间用,分割
long iprules::addRules_new(const char *strRules)
{
	m_rules.clear();
	if(strRules==NULL || strRules[0]==0) return 0;
	const char *ptrBegin=cUtils::strTrimLeft(strRules);
	while(true)
	{
		const char *ptr=strchr(ptrBegin,',');
		if(ptr) *(char *)ptr='\0';
		addRule_new(ptrBegin);
		if(ptr) *(char *)ptr=',';
		if(ptr==NULL) break;
		ptrBegin=cUtils::strTrimLeft(ptr+1);
	}//?while
	return m_rules.size();
}

//解析指定的域名,only for IPV4
unsigned long Host2IP(const char *host)
{
	unsigned long ipAddr=inet_addr(host);
	if(ipAddr!=INADDR_NONE) return ipAddr;
	//指定的不是一个有效的ip地址可能是一个主机域名
	struct hostent * p=gethostbyname(host);
	if(p==NULL) return INADDR_NONE;
	ipAddr=(*((struct in_addr *)p->h_addr)).s_addr;
	return ipAddr;
}
//另外一种添加IP过滤规则的格式
//ipaccess : strIP指定的IP是否可访问 0:不可访问 1:可访问
//strIP格式: <ip>[:<端口>],<ip>[:<端口>],<ip>[:<端口>]
//例如: 192.168.0.12,192.169.1.*,192.166.*.5 
//          *仅用来匹配IP地址4段号码中的一个段.下列写法不支持192.168.1.1*
//		www.sina.com.cn,192.168.0.3
long iprules::addRules_new(RULETYPE rt,int ipaccess,const char *strIP)
{
	m_rules.clear();//清空所有过滤规则
	m_defaultEnabled=true;
	if(strIP==NULL || strIP[0]==0) return 0;
	m_defaultEnabled=(ipaccess==0)?true:false;
	
	const char *ptrBegin=strIP; unsigned long iport=0;
	const char *p1,*p2,*p3,*p4,*p0=strchr(ptrBegin,',');
	while(true)
	{
		if(p0) *(char *)p0=0;
		while(*ptrBegin==' ') ptrBegin++; //清除前导空格
		p4=strchr(ptrBegin,':'); //指向端口
		unsigned long iport=0;
		if(p4 && atoi(p4+1)>0 ) iport=atoi(p4+1);
		
		IPRule iprule; char ipr[64];
		//判断ptrBegin指向的是否为主机域名
		unsigned long IPAddr=Host2IP(ptrBegin);
		if(IPAddr!=INADDR_NONE) //指向的是域名
		{
			iprule.IPAddr_src=IPAddr;
			iprule.IPMask_src=INADDR_NONE; //255.255.255.255	
		}else{//指向的可能是IP
			p1=p2=p3=p4=NULL;
			p1=strchr(ptrBegin,'.');
			if(p1) p2=strchr(p1+1,'.');
			if(p2) p3=strchr(p2+1,'.');
			if(p1 && p2 && p3)
			{
				*(char *)p1=0; p1++;
				*(char *)p2=0; p2++;
				*(char *)p3=0; p3++;
			
				sprintf(ipr,"%s.%s.%s.%s",
					((ptrBegin[0]!='*')?ptrBegin:"0"),
					((p1[0]!='*')?p1:"0"),
					((p2[0]!='*')?p2:"0"),
					((p3[0]!='*')?p3:"0") );
				iprule.IPAddr_src=inet_addr(ipr);
				sprintf(ipr,"%d.%d.%d.%d",
					((ptrBegin[0]!='*')?255:0),
					((p1[0]!='*')?255:0),
					((p2[0]!='*')?255:0),
					((p3[0]!='*')?255:0) );
				iprule.IPMask_src=inet_addr(ipr);
				
				*(char *)(p1-1)='.';
				*(char *)(p2-1)='.';
				*(char *)(p3-1)='.';
			}
		}//?...else
		
		iprule.port_src=iport;
		iprule.ruleType=rt;
		iprule.bEnabled=(ipaccess==0)?false:true;
		if(iprule.IPAddr_src!=INADDR_NONE) m_rules.push_back(iprule);

		if(p4) *(char *)p4=':';
		if(p0==NULL) break; else *(char *)p0=',';
		ptrBegin=p0+1;
		p0=strchr(ptrBegin,',');
	}//?while
	return m_rules.size();
}
/*
long iprules::addRules_new(RULETYPE rt,int ipaccess,const char *strIP)
{
	m_rules.clear();//清空所有过滤规则
	m_defaultEnabled=true;
	if(strIP==NULL || strIP[0]==0) return 0;
	m_defaultEnabled=(ipaccess==0)?true:false;
	
	const char *ptrBegin=strIP;
	const char *p1,*p2,*p3,*p4,*p0=strchr(ptrBegin,',');
	while(true)
	{
		if(p0) *(char *)p0=0;
		p1=p2=p3=p4=NULL;
		p1=strchr(ptrBegin,'.');
		if(p1) p2=strchr(p1+1,'.');
		if(p2) p3=strchr(p2+1,'.');
		if(p3) p4=strchr(p3+1,':');
		if(p4) *(char *)p4++=0;
		if(p1 && p2 && p3)
		{
			*(char *)p1=0; p1++;
			*(char *)p2=0; p2++;
			*(char *)p3=0; p3++;
			
			IPRule iprule; char ipr[64];
			sprintf(ipr,"%s.%s.%s.%s",
				((ptrBegin[0]!='*')?ptrBegin:"0"),
				((p1[0]!='*')?p1:"0"),
				((p2[0]!='*')?p2:"0"),
				((p3[0]!='*')?p3:"0") );
			iprule.IPAddr_src=inet_addr(ipr);
			sprintf(ipr,"%d.%d.%d.%d",
				((ptrBegin[0]!='*')?255:0),
				((p1[0]!='*')?255:0),
				((p2[0]!='*')?255:0),
				((p3[0]!='*')?255:0) );
			iprule.IPMask_src=inet_addr(ipr);
			if(p4 && atoi(p4)>0)
				iprule.port_src=(unsigned short)atoi(p4);
			else iprule.port_src=0;
			iprule.ruleType=rt;
			iprule.bEnabled=(ipaccess==0)?false:true;
			if(iprule.IPAddr_src!=INADDR_NONE) m_rules.push_back(iprule);
			
			*(char *)(p1-1)='.';
			*(char *)(p2-1)='.';
			*(char *)(p3-1)='.';
			if(p4) *(char *)(p4-1)=':';
		}
		if(p0==NULL) break; else *(char *)p0=',';
		ptrBegin=p0+1;
		p0=strchr(ptrBegin,',');
	}//?while
	return m_rules.size();
}
*/