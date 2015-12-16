/*******************************************************************
   *	socketIcmp.cpp
   *    DESCRIPTION:ICMP socket 类的实现
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
#include "../include/socketIcmp.h"
#include "../include/cLogger.h"

using namespace std;
using namespace net4cpp21;

socketIcmp :: socketIcmp()
{
	m_echo_seq=0;
	m_echo_id=rand();
}

inline void ConstructICMP(IcmpHeader &icmph)
{
	memset((void *)&icmph,0,sizeof(IcmpHeader));
	//Set the timestamp
	icmph.Originate_Timestamp=GetTickCount();
	return;
}

//发送Icmp Echo数据包
//ipDest --- 要发送的目的主机IP
SOCKSRESULT socketIcmp :: sendIcmp_echo(unsigned long ipDest)
{
	IcmpHeader icmph;
	ConstructICMP(icmph);

	icmph.i_type=ICMP_Echo;
	icmph.i_code=0;
	icmph.sICMP.sUS.us_id=htons(m_echo_id);
	icmph.sICMP.sUS.us_seq=htons(++m_echo_seq);
	
	//And the checksum
	icmph.Checksum=socketRaw::checksum((unsigned short*)&icmph,sizeof(IcmpHeader));
	//send it
	return sendIcmpPackage(ipDest,icmph);
}

//发送Icmp Echo响应数据包
SOCKSRESULT socketIcmp :: sendIcmp_reply(unsigned long ipDest,unsigned short usId, 
										unsigned short usSeq,unsigned long ulData)
{
	IcmpHeader icmph;
	ConstructICMP(icmph);
	
	icmph.i_type=ICMP_Echo_Reply;
	icmph.i_code=0;
	icmph.sICMP.sUS.us_id=htons(usId);
	icmph.sICMP.sUS.us_seq=htons(usSeq);
	icmph.Originate_Timestamp=htonl(ulData);
	//And the checksum
	icmph.Checksum=socketRaw::checksum((unsigned short*)&icmph,sizeof(IcmpHeader));
	//send it
	return sendIcmpPackage(ipDest,icmph);
}

//发送Icmp数据包
SOCKSRESULT socketIcmp :: sendIcmpPackage(unsigned long ipDest,IcmpHeader &icmph)
{
	if(m_socktype!=SOCKS_RAW)
		if(!create(IPPROTO_ICMP,"")) return SOCKSERR_INVALID;

	ConstructIPV4Header(IPPROTO_ICMP,IpV4_Min_Header_Length);
	LPIpV4Header lpipv4h=&m_IpV4Header;//IP Header
	lpipv4h->DestinationAddress=ipDest;
	char buffer[IP_MAX_PACKAGE_SIZE];
	int ilen=encode_ipv4((const char *)&icmph,sizeof(IcmpHeader),buffer);
	m_remoteAddr.sin_port=0;
	m_remoteAddr.sin_addr.s_addr=ipDest;
	return this->Send(ilen,buffer,-1);
}




