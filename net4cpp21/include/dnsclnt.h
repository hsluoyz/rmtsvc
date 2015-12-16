/*******************************************************************
   *	dnsclnt.h
   *    DESCRIPTION:DNS协议客户端声明
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-11-02
   *
   *	net4cpp 2.1
   *	域名系统（DNS）是一种用于TCP/IP应用程序的分布式数据库，
   *	它提供主机名字和IP地址之间的转换信息。通常，网络用户通
   *	过UDP协议和DNS服务器进行通信，而服务器在特定的53端口监听，并返回用户所需的相关信息。
   *******************************************************************/
#ifndef __YY_DNS_CLINET_H__
#define __YY_DNS_CLINET_H__

#include "dnsdef.h"
#include "socketUdp.h"

namespace net4cpp21
{
	
	//MX的response中rdata的数据结构
	typedef struct dns_rdata_mx
	{
		unsigned short priority; //优先级别
		std::string mxname; //解析的邮件交换服务器名称
	}DNS_RDATA_MX,*PDNS_RDATA_MX;

	class dnsClient : public socketUdp
	{
	public:
		dnsClient();
		virtual ~dnsClient(){}
		void setTimeout(time_t s);
		//域名查询 域名--->IP,返回DNS_RCODE_ERR_OK(0)成功
		SOCKSRESULT Query(const char *names,const char *dnssvr,int dnsport=DNS_SERVER_PORT);
		//反向域名解析,返回DNS_RCODE_ERR_OK(0)成功
		SOCKSRESULT IQuery(const char *ip,const char *dnssvr,int dnsport=DNS_SERVER_PORT);
		//邮件交换器查询,返回DNS_RCODE_ERR_OK(0)成功
		//一个MX记录是由一个2字节的指示该邮件交换器的优先级值及不定长的邮件交换器名组成的
		SOCKSRESULT Query_MX(const char *names,const char *dnssvr,int dnsport=DNS_SERVER_PORT);
		//获取查询返回的dns头信息
		PDNS_HEADER resp_dnsh() { return &m_dnsh; }
		//获取查询返回的dns 第index个查询信息
		PDNS_QUERY resp_dnsq(unsigned short index=0);

		//获取查询返回的第index结果
		PDNS_RESPONSE resp_dnsr(unsigned short index=0);
		//解析DNS_RESPONSE的rdata域的数据
		unsigned long parse_rdata_Q(PDNS_RESPONSE pdnsr=NULL);
		const char * parse_rdata_IQ(PDNS_RESPONSE pdnsr=NULL);
		const char * parse_rdata_MX(PDNS_RDATA_MX pmx,PDNS_RESPONSE pdnsr=NULL);

	private:
		unsigned short m_msgID; //消息ID
		time_t m_lTimeout;//最大等待超时返回s
		
		std::string m_strnames; //用来临时保存解析的names
		char m_buffer[DNS_MAX_PACKAGE_SIZE];
		DNS_HEADER m_dnsh;
		DNS_QUERY m_dnsq;
		DNS_RESPONSE m_dnsr;
	};

}//?namespace net4cpp21

#endif
