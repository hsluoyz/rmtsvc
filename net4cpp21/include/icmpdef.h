/*******************************************************************
   *	icmpdef.h
   *    DESCRIPTION:定义icmp协议所用到的常量、结构以及enum的定义
   *				
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-12-10
   *	
   *	net4cpp 2.1
   *******************************************************************/

#ifndef __YY_ICMPDEF_H__
#define __YY_ICMPDEF_H__

// ICMP data size
#define ICMP_DATA_SIZE 8

// ICMP Message unreachable
#define ICMP_Unreachable 3
#define ICMP_Unreachable_SIZE 8

#define ICMP_Unreachable_NET 0
#define ICMP_Unreachable_HOST 1
#define ICMP_Unreachable_PROTOCOL 2
#define ICMP_Unreachable_PORT 3
#define ICMP_Unreachable_FRAGMENTATION 4
#define ICMP_Unreachable_SOURCE 5

// ICMP Time exceeded
#define ICMP_Time 11

#define ICMP_Time_TRANSIT 0
#define ICMP_Time_FRAGMENT 1

// ICMP Parameter problem
#define ICMP_Parameter 12

#define ICMP_Parameter_ERROR 0

// ICMP Source quench
#define ICMP_Quench 4

// ICMP Redirect
#define ICMP_Redirect 5

#define ICMP_Redirect_NETWORK 0
#define ICMP_Redirect_HOST 1
#define ICMP_Redirect_SERVICE_NETWORK 2
#define ICMP_Redirect_SERVICE_HOST 3

// ICMP Echo
#define ICMP_Echo 8
#define ICMP_Echo_Reply 0

// ICMP Timestamp
#define ICMP_Timestamp 13
#define ICMP_Timestamp_Reply 14

// ICMP Information request
#define ICMP_Information 15
#define ICMP_Information_Reply 16

#define ICMP_Information_SIZE 8

typedef struct _IcmpHeader //定义ICMP首部
{
	unsigned char i_type;
	unsigned char i_code;
	unsigned short Checksum;
	union
	{
		struct {unsigned char uc1,uc2,uc3,uc4;} sUC;
		struct {unsigned short us_id,us_seq;} sUS; //id--识别号（一般用进程号作为识别号）
												   //seq--报文序列号
		unsigned long sUL;
	} sICMP;
	// Not standard field in header, but reserved nonetheless
	unsigned long Originate_Timestamp;
	unsigned long Receive_Timestamp;
    unsigned long Transmit_Timestamp;

	//Icmp数据包解码
	void decode(const char *ptrbuf,size_t buflen)
	{
		struct _IcmpHeader &icmph=*this;
		memset((void *)&icmph,0,sizeof(IcmpHeader));
		icmph.i_type=*ptrbuf; ptrbuf++;
		icmph.i_code=*ptrbuf; ptrbuf++;
		icmph.Checksum=*((unsigned short *)ptrbuf); ptrbuf+=2;
		icmph.Checksum=ntohs(icmph.Checksum);
						
		icmph.sICMP.sUS.us_id=*((unsigned short *)ptrbuf); ptrbuf+=2;
		icmph.sICMP.sUS.us_id=ntohs(icmph.sICMP.sUS.us_id);
						
		icmph.sICMP.sUS.us_seq=*((unsigned short *)ptrbuf); ptrbuf+=2;
		icmph.sICMP.sUS.us_seq=ntohs(icmph.sICMP.sUS.us_seq);
		
		icmph.Originate_Timestamp=*((unsigned int *)ptrbuf); ptrbuf+=4;
		icmph.Originate_Timestamp=ntohl(icmph.Originate_Timestamp);

		icmph.Receive_Timestamp=*((unsigned int *)ptrbuf); ptrbuf+=4;
		icmph.Receive_Timestamp=ntohl(icmph.Receive_Timestamp);

		icmph.Transmit_Timestamp=*((unsigned int *)ptrbuf); ptrbuf+=4;
		icmph.Transmit_Timestamp=ntohl(icmph.Transmit_Timestamp);
		return;
	}
	//Icmp数据包编码
	size_t encode(const char *data,size_t datalen,char *ptrbuf)
	{
		struct _IcmpHeader &icmph=*this;
		char *pstart_icmp=ptrbuf;
		*ptrbuf=icmph.i_type; ptrbuf++;
		*ptrbuf=icmph.i_code; ptrbuf++;
		
		icmph.Checksum=0;
		*((unsigned short *)ptrbuf)=htons(icmph.Checksum); ptrbuf+=2;
		*((unsigned short *)ptrbuf)=htons(icmph.sICMP.sUS.us_id); ptrbuf+=2;
		*((unsigned short *)ptrbuf)=htons(icmph.sICMP.sUS.us_seq); ptrbuf+=2;
		
		*((unsigned int *)ptrbuf)=htonl(icmph.Originate_Timestamp); ptrbuf+=4;
		*((unsigned int *)ptrbuf)=htonl(icmph.Receive_Timestamp); ptrbuf+=4;
		*((unsigned int *)ptrbuf)=htonl(icmph.Transmit_Timestamp); ptrbuf+=4;
		if(datalen>0)
		{
			::memcpy(ptrbuf,data,datalen);
			ptrbuf+=datalen;
		}
		//生成ICMP校验和
		icmph.Checksum=net4cpp21::socketRaw::checksum((unsigned short *)pstart_icmp,
					ptrbuf-pstart_icmp);
		*((unsigned short *)(pstart_icmp+2))=htons(icmph.Checksum); 
		return ptrbuf-pstart_icmp;
	}

}IcmpHeader,*LPIcmpHeader;


#endif
