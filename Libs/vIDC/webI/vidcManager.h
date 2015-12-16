/*******************************************************************
   *	vidcManager.h
   *    DESCRIPTION:vIDC集合管理类
   *
   *    AUTHOR:yyc
   *	http://hi.baidu.com/yycblog/home
   *
   *******************************************************************/

#ifndef __YY_VIDC_MANAGER_H__
#define __YY_VIDC_MANAGER_H__

#include "../../../net4cpp21/include/sysconfig.h"
#include "../../../net4cpp21/include/cCoder.h"
#include "../../../net4cpp21/include/cLogger.h"
#include "../../../net4cpp21/include/upnp.h"

#include "vidcManager_def.h"

class vidcManager
{
public:
	vidcManager();
	~vidcManager();
	void Destroy();
	void mtcpl_Start();
	//获取本机ip
	void xml_list_localip(cBuffer &buffer);

	void xml_list_mtcp(cBuffer &buffer);
	void xml_info_mtcp(cBuffer &buffer,const char *mapname);
	void xml_start_mtcp(cBuffer &buffer,const char *mapname);
	void xml_stop_mtcp(cBuffer &buffer,const char *mapname);
	void xml_dele_mtcp(cBuffer &buffer,const char *mapname);

	void xml_list_mudp(cBuffer &buffer);
	void xml_info_mudp(cBuffer &buffer,const char *mapname);
	void xml_start_mudp(cBuffer &buffer,const char *mapname);
	void xml_stop_mudp(cBuffer &buffer,const char *mapname);
	void xml_dele_mudp(cBuffer &buffer,const char *mapname);

	bool readIni(); //从注册表读配置
	bool saveIni(); //往注册表写配置
	bool parseIni(char *pbuffer,long lsize);
	bool parseCommand(const char *pstart);
	bool saveAsstring(std::string &strini);
	void initSetting(); //初始化所有配置
	bool docmd_sslc(const char *strParam);
	bool docmd_mtcpl(const char *strParam);
	void docmd_mdhrsp(const char *strParam);
	void docmd_mdhreq(const char *strParam);
	void docmd_iprules(const char *strParam);
	void docmd_vidcs(const char *strParam);
	void docmd_vidcc(const char *strParam);
	void docmd_mtcpr(const char *strParam);
	void docmd_upnp(const char *strParam);
	
public:
	vidcServerEx m_vidcsvr; //vidc服务
	vidccSets m_vidccSets; //vidc客户端集合
	upnp	m_upnp;
private:
	std::map<std::string,mportTCP *> m_tcpsets; //TCP服务映射集合

};


#endif
