/*******************************************************************
   *	sniffer.cpp
   *    DESCRIPTION:sniffer类实现(混杂模式)
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-12-10
   *	
   *	net4cpp 2.1
   *******************************************************************/


#include "../include/sysconfig.h"
#include "../include/sniffer.h"
#include "../include/icmpdef.h"
#include "../include/cLogger.h"

using namespace std;
using namespace net4cpp21;

sniffer :: ~sniffer()
{
	Close();
	m_thread.join();
}

void sniffer::Close()
{
	Set_Promisc(false); //先关闭混杂模式
	socketRaw::Close();
	return;
}
SOCKSRESULT sniffer :: sniff(const char *bindip)
{
	if(m_socktype!=SOCKS_RAW)
	{
		if(!create(IPPROTO_IP,(bindip)?bindip:""))
		{
			RW_LOG_PRINT(LOGLEVEL_ERROR,"[sniffer] failed to create RAW Socket.\r\n");
			return SOCKSERR_INVALID;
		}
	}
	//设置混杂模式
	SOCKSRESULT sr=Set_Promisc(true);
	if(sr==SOCKSERR_OK){
		if(m_thread.start((THREAD_CALLBACK *)&sniffer::sniffThread,(void *)this))
			return SOCKSERR_OK;
		else sr=SOCKSERR_THREAD;
	}
	else
		RW_LOG_PRINT(LOGLEVEL_ERROR,"Failed to Setting Promisc.error=%d/%d\r\n",sr,m_errcode);
	Close(); return sr;
}

//打开ip记录文件并模拟sniff
bool sniffer :: openLogfile(const char *logfile)
{
	if(logfile==NULL || logfile[0]==0) return false;
	FILE *fp=::fopen(logfile,"rb");
	if(fp)
	{
		char buf[4];
		::fread(buf,sizeof(char),4,fp);
		if(buf[0]=='.' && buf[1]=='i' && buf[2]=='p' && buf[3]=='p')
		{
			this->m_parent=(socketBase *)fp;
			if(m_thread.start((THREAD_CALLBACK *)&sniffer::sniffThread_fromfile,(void *)this))
				return true;
			else{  this->m_parent=NULL; ::fclose(fp); }
		}
		else ::fclose(fp);
	}
	return false;
}

//有数据到达,如果返回假则停止sniff
void sniffer :: onData(char *dataptr)
{
	if( !RW_LOG_CHECK(LOGLEVEL_DEBUG) ) return;
	if(m_IpV4Header.Protocol==IPPROTO_TCP)
	{
		RW_LOG_DEBUG("[Tcp ] %s:%d ---> ",socketBase::IP2A(m_IpV4Header.SourceAddress),
			m_ProtocolHeader.m_TcpHeader.SourcePort);
		RW_LOG_DEBUG("%s:%d\r\n",socketBase::IP2A(m_IpV4Header.DestinationAddress),
			m_ProtocolHeader.m_TcpHeader.DestinationPort);
		RW_LOG_DEBUG("\t%c%c%c%c%c%c TTL=%d Len=%d datalen=%d\r\n",
			(m_ProtocolHeader.m_TcpHeader.get_FIN()!=0)?'F':'-',
			(m_ProtocolHeader.m_TcpHeader.get_SYN()!=0)?'S':'-',
			(m_ProtocolHeader.m_TcpHeader.get_RST()!=0)?'R':'-',
			(m_ProtocolHeader.m_TcpHeader.get_PSH()!=0)?'P':'-',
			(m_ProtocolHeader.m_TcpHeader.get_ACK()!=0)?'A':'-',
			(m_ProtocolHeader.m_TcpHeader.get_URG()!=0)?'U':'-',
			m_IpV4Header.TimeToLive,m_IpV4Header.TotalLength,
			dataLen_ipv4());
//			RW_LOG_DEBUG("\t%s\r\n",dataptr);
	}
	else if(m_IpV4Header.Protocol==IPPROTO_UDP)
	{
		RW_LOG_DEBUG("[Udp ] %s:%d ---> ",socketBase::IP2A(m_IpV4Header.SourceAddress),
				m_ProtocolHeader.m_UdpHeader.SourcePort);
		RW_LOG_DEBUG("%s:%d\r\n",socketBase::IP2A(m_IpV4Header.DestinationAddress),
			m_ProtocolHeader.m_UdpHeader.DestinationPort);
		RW_LOG_DEBUG("\t       TTL=%d Len=%d datalen=%d\r\n",
			m_IpV4Header.TimeToLive,m_IpV4Header.TotalLength,
			dataLen_ipv4());
//			RW_LOG_DEBUG("\t%s\r\n",dataptr);
	}
	else if(m_IpV4Header.Protocol==IPPROTO_ICMP)
	{
		IcmpHeader icmph;
		icmph.decode(dataptr,dataLen_ipv4());
		RW_LOG_DEBUG("[Icmp] %s ---> ",socketBase::IP2A(m_IpV4Header.SourceAddress));
		RW_LOG_DEBUG("%s:\r\n",socketBase::IP2A(m_IpV4Header.DestinationAddress));
		RW_LOG_DEBUG("\t       Type=%d Code=%d id=%d seq=%d\r\n",
			icmph.i_type,icmph.i_code,icmph.sICMP.sUS.us_id,icmph.sICMP.sUS.us_seq);
	}
	else
	{
		RW_LOG_DEBUG("[%04d] %s ---> ",m_IpV4Header.Protocol,
			socketBase::IP2A(m_IpV4Header.SourceAddress));
		RW_LOG_DEBUG("%s:\r\n",socketBase::IP2A(m_IpV4Header.DestinationAddress));
	}
	return;
}
//数据接收线程
#define SNIFF_CHECKTIMEOUT 50000 //us,select的超时时间 50ms
void sniffer :: sniffThread(sniffer *psniffer)
{
	if(psniffer==NULL) return;
	char buffer[IP_MAX_PACKAGE_SIZE+2];
	char *buf=buffer+2;

	FILE *fp=NULL;
	if(psniffer->m_logfile!=""){
		fp=::fopen(psniffer->m_logfile.c_str(),"wb");
		buf[0]='.';buf[1]='i';buf[2]='p';buf[3]='p';
		if(fp) ::fwrite(buf,sizeof(char),4,fp);
	}
	if(fp)
		RW_LOG_DEBUG(0,"[sniffer] sniffThread has been started - recording\r\n");
	else
		RW_LOG_DEBUG(0,"[sniffer] sniffThread has been started\r\n");

	bool bCheck=((psniffer->m_rules.rules()<=0) && psniffer->m_rules.getDefaultEnable());
	char *dataptr=NULL; int ret=0;
	while(psniffer->status()==SOCKS_OPENED)
	{
		ret=psniffer->checkSocket(SNIFF_CHECKTIMEOUT,SOCKS_OP_READ);
		if(ret<0) break; //此socket发生错误
		if(ret==0) continue;
		//有数据到达
		ret=psniffer->Receive(buf,IP_MAX_PACKAGE_SIZE,-1);
		if(ret<0) break; //发生错误
		if(ret==0) continue;
		if( (dataptr=psniffer->decode_ipv4(buf,ret)) )
		{
			if(bCheck)
			{
				if(fp){
					*((unsigned short *)buffer)=(unsigned short)ret;
					::fwrite(buffer,sizeof(char),ret+2,fp);
				}
				psniffer->onData(dataptr);
			}
			else
			{
				RULETYPE rt=RULETYPE_ALL;
				unsigned long ip_fr=psniffer->m_IpV4Header.SourceAddress;
				unsigned long ip_to=psniffer->m_IpV4Header.DestinationAddress;
				unsigned short port_fr=0,port_to=0;
				if(psniffer->m_IpV4Header.Protocol==IPPROTO_TCP)
				{
					rt=RULETYPE_TCP;
					port_fr=psniffer->m_ProtocolHeader.m_TcpHeader.SourcePort;
					port_to=psniffer->m_ProtocolHeader.m_TcpHeader.DestinationPort;
				}
				else if(psniffer->m_IpV4Header.Protocol==IPPROTO_UDP)
				{
					rt=RULETYPE_UDP;
					port_fr=psniffer->m_ProtocolHeader.m_TcpHeader.SourcePort;
					port_to=psniffer->m_ProtocolHeader.m_TcpHeader.DestinationPort;
				}
				else if(psniffer->m_IpV4Header.Protocol==IPPROTO_ICMP)
					rt=RULETYPE_ICMP;
				if(psniffer->m_rules.check(ip_fr,port_fr,ip_to,port_to,rt))
				{
					if(fp){
						*((unsigned short *)buffer)=(unsigned short)ret;
						::fwrite(buffer,sizeof(char),ret+2,fp);
					}
					psniffer->onData(dataptr);
				}
			}//?if(bCheck)...else
		}//?if( (dataptr
	}//?while(pasynsock->status()>=SOCKS_LISTEN)
	
	if(fp) ::fclose(fp);

	RW_LOG_DEBUG(0,"[sniffer] sniffThread has been ended\r\n");
	return;
}

void sniffer :: sniffThread_fromfile(sniffer *psniffer)
{
	if(psniffer==NULL) return;
	FILE *fp=(FILE *)psniffer->m_parent; psniffer->m_parent=NULL;
	if(fp==NULL) return;
	RW_LOG_DEBUG(0,"[sniffer] sniffThread has been started,reading from file\r\n");

	char buf[IP_MAX_PACKAGE_SIZE]; 
	unsigned short iplen;
	while(feof(fp)==0)
	{
		::fread((void *)&iplen,sizeof(unsigned short),1,fp);
		size_t ret=::fread(buf,sizeof(char),iplen,fp);
		char *dataptr=(ret>0)?psniffer->decode_ipv4(buf,ret):NULL;
		if(dataptr){
			RULETYPE rt=RULETYPE_ALL;
			unsigned long ip_fr=psniffer->m_IpV4Header.SourceAddress;
			unsigned long ip_to=psniffer->m_IpV4Header.DestinationAddress;
			unsigned short port_fr=0,port_to=0;
			if(psniffer->m_IpV4Header.Protocol==IPPROTO_TCP)
			{
				rt=RULETYPE_TCP;
				port_fr=psniffer->m_ProtocolHeader.m_TcpHeader.SourcePort;
				port_to=psniffer->m_ProtocolHeader.m_TcpHeader.DestinationPort;
			}
			else if(psniffer->m_IpV4Header.Protocol==IPPROTO_UDP)
			{
				rt=RULETYPE_UDP;
				port_fr=psniffer->m_ProtocolHeader.m_TcpHeader.SourcePort;
				port_to=psniffer->m_ProtocolHeader.m_TcpHeader.DestinationPort;
			}
			else if(psniffer->m_IpV4Header.Protocol==IPPROTO_ICMP)
				rt=RULETYPE_ICMP;
			if(psniffer->m_rules.check(ip_fr,port_fr,ip_to,port_to,rt))
			{
				psniffer->onData(dataptr);
			}
		}//?if(dataptr)
	}//?while(feof(fp)==0)

	::fclose(fp);
	RW_LOG_DEBUG(0,"[sniffer] sniffThread has been ended\r\n");
	return;
}
