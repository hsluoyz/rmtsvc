/*******************************************************************
   *	vidcsdef.h
   *    DESCRIPTION:声明定义头文件
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-06-03
   *	
   *******************************************************************/
#ifndef __YY_VIDCCDEF_H__
#define __YY_VIDCCDEF_H__

#include "mportdef.h"

typedef struct _VIDCSINFO
{
	std::string m_vidcsHost;
	int m_vidcsPort;
	std::string m_vidcsPswd; //访问vIDCs的密码
	bool m_bAutoConn;

	long m_vidccID; //连接上vDCs后，vIDCs分配的ID标识
	std::string m_vidcsIPList; //vIDCs返回的vidcs主机的IP地址列表
	int m_vidcsVer; //vIDCs的版本
}VIDCSINFO;

class mapInfo
{
public:
	VIDC_MAPTYPE m_mapType;
	std::string m_appsvr;//应用服务地址和端口，可,分割多个。
	std::string m_appdesc;
	SSLTYPE m_ssltype;
	bool m_sslverify;  //是否需要进行客户端证书验证，仅仅ssltype==TCPSVR_SSLSVR有效
	int m_proxyType; //支持的代理类型
	std::string m_proxyuser;
	std::string m_proxypswd;
	bool m_proxyauth; //代理是否需要验证
	MPORTTYPE m_apptype;
	int m_mportBegin;  //要求映射端口范围
	int m_mportEnd;
	char m_bindLocalIP[16]; //要求绑定的本地IP
	bool m_bAutoMap; //当连接上vIDCs后是否自动映射
	long m_ipaccess;
	std::string m_ipRules;//ip访问规则 
	std::string m_clicert; //客户验证证书信息
	std::string m_clikey;
	std::string m_clikeypswd;
	int m_mappedPort; //实际映射的端口
	bool m_mappedSSLv; //映射后的服务端口是否需要进行客户证书验证
	
	unsigned long m_maxconn; //限制最大连接，限制最大带宽 kb/s
	unsigned long m_maxratio;
	std::vector<std::string> m_hrspRegCond;
	std::vector<std::string> m_hreqRegCond;
	mapInfo(){
		m_mapType=VIDC_MAPTYPE_TCP;
		m_appsvr="";
		m_appdesc="";
		m_ssltype=TCPSVR_TCPSVR;
		m_sslverify=false;
		m_proxyType=7;
		m_proxyuser="";
		m_proxypswd="";
		m_proxyauth=false;
		m_apptype=MPORTTYPE_UNKNOW;
		m_mportBegin=m_mportEnd=0;
		m_bindLocalIP[0]=0;
		m_bAutoMap=false;
		m_ipaccess=1;
		m_ipRules="";
		m_clicert="";
		m_clikey="";
		m_clikeypswd="";
		m_mappedPort=0;
		m_mappedSSLv=false;

		m_maxconn=0;
		m_maxratio=0;
	}
	~mapInfo(){}
};


#endif
