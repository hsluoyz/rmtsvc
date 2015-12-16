/*******************************************************************
   *	socketRaw.h
   *    DESCRIPTION:Raw socket 类的定义
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-12-10
   *	
   *	net4cpp 2.1
   *******************************************************************/
#ifndef __YY_SOCKET_RAW_H__
#define __YY_SOCKET_RAW_H__

#include "socketBase.h"
#include "ipdef.h"

namespace net4cpp21
{
	class socketRaw : public socketBase
	{
	public:
		socketRaw();
		virtual ~socketRaw(){}
		//Set the packet Time to live
		void SetTTL(unsigned char ucTTL) { m_TTL=ucTTL; return;}
		//Set source address for spoofing
		void SetSourceAddress(const char *lpSourceAddress)
		{
			if(lpSourceAddress) m_SourceAddress.assign(lpSourceAddress);
			return;
		}
		
		//生成校验和
		static unsigned short checksum(unsigned short *buffer, int size);
	private: //禁止copy和赋值
		socketRaw(socketRaw &sockRaw){ return; }
		socketRaw & operator = (socketRaw &sockRaw) { return *this; }

	protected:
		//解析IP 包,成功返回用户数据的在buf中的指针，错误的ip包则返回NULL
		//buf --- 收到的IP包字节流，len --- 字节流长度
		//当数据包解码成功后，包中真正数据的指针有此函数返回，数据的长度可由datalen函数返回
		char * decode_ipv4(char *buf,int len);
		unsigned short dataLen_ipv4();
		//将指定的用户数据编码为IP包字节流,返回编码后的字节流大小
		//data --- 用户数据指针, datalen --- 用户数据长度
		//encodebuf --- 存放编码后的ip包字节流
		int encode_ipv4(const char *data,int datalen,char *encodebuf);
		void ConstructIPV4Header(unsigned char  ucProtocol,unsigned char  ucHeaderLength);
		//bindip==NULL 绑定所有IP，==""绑定本机第一个ip
		//否则绑定指定的ip,创建Raw socket
		bool create(int IPPROTO_type,const char *bindip);
		//设置/清除指定fd的混杂模式
		//成功返回SOCKSERR_OK(0)否则错误
		SOCKSRESULT Set_Promisc(bool b);
		
		//Time to live
		unsigned char m_TTL;
		std::string m_SourceAddress;//源IP默认为本机第一个IP
		IpV4Header m_IpV4Header;//IP Header
		union
		{
			TcpHeader m_TcpHeader;
			UdpHeader m_UdpHeader;
		}m_ProtocolHeader; //Protocol Header

	}; 
}//?namespace net4cpp21

#endif

/*
		//得到IP头结构指针
		LPIpV4Header get_ipHeaderPtr()
		{
			return &m_IpV4Header;
		}
		LPTcpHeader get_tcpHeaderPtr()
		{
			if(m_IpV4Header.Protocol==IPPROTO_TCP) return &m_ProtocolHeader.m_TcpHeader;
			return NULL;
		}
		LPUdpHeader get_udpHeaderPtr()
		{
			if(m_IpV4Header.Protocol==IPPROTO_UDP) return &m_ProtocolHeader.m_UdpHeader;
			return NULL;
		} 
*/




