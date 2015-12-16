/*******************************************************************
   *	ftpserver.h 
   *    DESCRIPTION: FTP服务
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *	
   *******************************************************************/

#include "net4cpp21/include/sysconfig.h"
#include "net4cpp21/include/cCoder.h"
#include "net4cpp21/include/cLogger.h"

#include "net4cpp21/include/ftpsvr.h"

using namespace std;
using namespace net4cpp21;

//ftp服务参数结构
typedef struct _TFTPSetting
{
	bool autoStart;
	int svrport;
	bool ifSSLsvr;
	int dataportB;
	int dataportE;
	int maxUsers;
	int logEvent;
	std::string bindip;
	std::string tips; //欢迎词
}TFTPSetting;

typedef struct _TFTPUser
{
	std::string username;//帐号,帐号不区分大小写(帐号转换为小写)
	std::string userpwd;//密码,如果密码==""则，无需密码验证
	std::string userdesc;
	unsigned long maxupratio;//最大上载速率 K/s,如果=0则不限
	unsigned long maxdwratio;//最大下载速率 K/s,如果=0则不限
	unsigned long maxupfilesize;//最大上载文件大小 KBytes ,0--不限
	unsigned long maxdisksize;//限制最大磁盘使用空间 KBytes,0--不限
	unsigned long curdisksize;//当前已使用磁盘空间 KBytes.
	std::map<std::string,std::pair<std::string,long> > dirAccess;//目录访问权限,目录区分大小写
			//first --- string : ftp的虚目录路径,最后以/结束，例如/ 或 /aa/，
			//second --- pair : 此ftp虚目录对应的实际目录和目录的访问权限，实际目录必须为\结尾(win平台)
	long ipaccess;
	std::string ipRules;//ip访问规则
	long maxLoginusers;//限制此帐号的最大同时登录用户数,<=0则不限制 
	time_t limitedTime;//限制此帐号只在某个日期之前有效，==0不限制
	long pswdmode;
	long disphidden; //是否显示隐藏文件
	long forbid; //是否禁用此帐号
}TFTPUser;

class ftpsvrEx : public ftpServer
{
public:
	ftpsvrEx();
	virtual ~ftpsvrEx(){}
	bool Start();
	void Stop();
	int deleUser(const char *ptr_user);
	bool modiUser(TFTPUser &ftpuser);
	bool readIni();
	bool saveIni();
	void initSetting();
	bool parseIni(char *pbuffer,long lsize);
	bool saveAsstring(std::string &strini);

	TFTPSetting m_settings;
	std::map<std::string,TFTPUser> m_userlist;
protected:
	virtual void onLogEvent(long eventID,cFtpSession &session);
	void docmd_sets(const char *strParam);
	void docmd_ssls(const char *strParam);
	void docmd_ftps(const char *strParam);
	void docmd_user(const char *strParam);
	void docmd_vpath(const char *strParam);
	void docmd_iprules(const char *strParam);
private:
	
};
