/*******************************************************************
   *	proxyserver.h 
   *    DESCRIPTION: proxy服务
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *	
   *******************************************************************/

#include "net4cpp21/include/sysconfig.h"
#include "net4cpp21/include/cCoder.h"
#include "net4cpp21/include/cLogger.h"

#include "net4cpp21/include//proxysvr.h"

using namespace std;
using namespace net4cpp21;

//代理服务配置参数信息
typedef struct _TPROXYSETTINGS
{
	int svrport; //服务端口
	std::string bindip; //代理服务绑定IP
	int svrtype; //代理服务支持的代理类型
	bool bAuth; //代理服务是否需要验证
	bool autorun;  //程序运行是否自动启动代理服务
	bool bCascade; //是否启用二级代理
	std::string cassvrip; //二级代理服务地址和端口
	int castype; //二级代理支持的代理类型
	bool casAuth; //二级代理是否需要验证
	std::string casuser; //访问二级代理的帐号和密码
	std::string caspswd;
	long ipaccess;   //访问本代理服务的IP过滤规则
	std::string ipRules;
	bool bLogdatafile; //是否记录代理转发数据
}PROXYSETTINGS;


typedef struct _TProxyUser
{
	std::string username;//帐号,帐号不区分大小写(帐号转换为小写)
	std::string userpwd;//密码,如果密码==""则，无需密码验证
	std::string userdesc;
	long ipaccess;
	std::string ipRules;//ip访问规则
	unsigned long maxratio;//最大带宽 K/s,如果=0则不限
	long maxLoginusers;//限制此帐号的最大同时登录用户数,<=0则不限制 
	time_t limitedTime;//限制此帐号只在某个日期之前有效，==0不限制
	long forbid; //是否禁用此帐号

	std::string strAccessDest;//允许或禁止访问的目的
	int bAccessDest; //上述指定的目的是禁止还是允许 0禁止否则允许
}TProxyUser;

class proxysvrEx : public proxyServer
{
public:
	proxysvrEx();
	virtual ~proxysvrEx(){}
	bool Start();
	void Stop();
	int deleUser(const char *ptr_user);
	bool modiUser(TProxyUser &puser);
	bool readIni();
	bool saveIni();
	void initSetting();
	bool parseIni(char *pbuffer,long lsize);
	bool saveAsstring(std::string &strini);

	PROXYSETTINGS m_settings;
	std::map<std::string,TProxyUser> m_userlist;
protected:
	void docmd_psets(const char *strParam);
	void docmd_cassets(const char *strParam);
	void docmd_puser(const char *strParam);
	void docmd_iprules(const char *strParam);
private:
	
};
