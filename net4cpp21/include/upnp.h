/*******************************************************************
   *	upnp.h
   *    DESCRIPTION:upnp 类定义
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *	net4cpp 2.1
   *******************************************************************/

#ifndef __YY_UPNP_H__
#define __YY_UPNP_H__

#include "socketUdp.h"
#include "upnpdef.h"

namespace net4cpp21
{
	class upnp : public socketUdpAnsy
	{
	public:
		upnp();
		virtual ~upnp();
		bool bFound() { return m_bFound; } //是否寻找到可用的UPnP设备
		const char *name() { return m_friendlyName.c_str(); }
		const char *manufacturer() { return m_manufacturer.c_str(); }
		const char *controlURL() { return m_control_url.c_str(); }
		std::vector<UPnPInfo *> &upnpinfo() { return m_upnpsets; }
		bool Search(); //搜索支持upnp的NAT设备
		
		bool AddPortMapping(bool bTCP,const char *internalIP,int internalPort,int externPort,const char *desc);
		bool DeletePortMapping(bool bTCP,int externPort);
		bool GetWanIP(std::string &strRet); //获取公网IP地址
		bool GetDevXML(std::string &strXml);
		void Clear();
	protected:
		//有数据到达
		virtual void onData();
		bool AddPortMapping(UPnPInfo &info);
		bool DeletePortMapping(UPnPInfo &info);
		bool invoke_command(std::string &strCmd,std::map<std::string,std::string> &strArgs);
		bool invoke_property(std::string &reqName,std::string &rspName);

	private:
		std::string m_friendlyName;
		std::string m_manufacturer;
		std::string m_targetName;
		std::string m_control_url;
		std::string m_strLocation;
		bool m_bFound;

		std::vector<UPnPInfo *> m_upnpsets; //UPnP映射信息集合
	};
}//?namespace net4cpp21

#endif
