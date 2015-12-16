/*******************************************************************
   *	rmtsvc.h 
   *    DESCRIPTION: rmtsvc远程控制工具
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-12-30
   *	
   *******************************************************************/

#include "net4cpp21/include/sysconfig.h"
#include "net4cpp21/include/cCoder.h"
#include "net4cpp21/include/cLogger.h"
#include "net4cpp21/utils/utils.h"

#include "net4cpp21/include/httpsvr.h"
#ifdef _DEBUG
#pragma comment( lib, "libs/bin/net4cpp21_d" )
#elif defined _SURPPORT_OPENSSL_
#pragma comment( lib, "libs/bin/net4cpp21" )
#else
#pragma comment( lib, "libs/bin/net4cpp21_nossl" )
#endif

//#include "msnbot/msnbot.h"	//yyc remove MSN function
#include "ftpserver.h"
#include "proxyserver.h"
#include "telnetserver.h"
//-----------------------------vIDC--------------------------
#include "libs/vidc/webI/vidcManager.h"

using namespace std;
using namespace net4cpp21;

#define RMTSVC_ACCESS_NONE			0x0000
#define RMTSVC_ACCESS_ALL			0xFFFFFFFF
#define RMTSVC_ACCESS_SCREEN_ALL	0x0003
#define RMTSVC_ACCESS_SCREEN_VIEW	0x0001
#define RMTSVC_ACCESS_REGIST_ALL	0x000c
#define RMTSVC_ACCESS_REGIST_VIEW	0x0004
#define RMTSVC_ACCESS_SERVICE_ALL	0x0030
#define RMTSVC_ACCESS_SERVICE_VIEW	0x0010
#define RMTSVC_ACCESS_TELNET_ALL	0x00c0
#define RMTSVC_ACCESS_TELNET_VIEW	0x0040
#define RMTSVC_ACCESS_FILE_ALL		0x0300
#define RMTSVC_ACCESS_FILE_VIEW		0x0100
#define RMTSVC_ACCESS_FTP_ADMIN		0x0c00 //FTP配置管理权限
#define RMTSVC_ACCESS_VIDC_ADMIN	0x3000 //vIDC/Proxy管理权限

class webServer : public httpServer
{
public:
	webServer();
	virtual ~webServer(){}
	bool Start(); //启动服务
	void Stop();//停止服务
	
	void setRoot(const char *rpath,long lAccess,const char *defaultPage);
	void docmd_webs(const char *strParam);
	void docmd_webiprules(const char *strParam);
	void docmd_user(const char *strParam);

	int m_svrport;
	std::string m_bindip;
	bool m_bPowerOff; //是否无需权限就可执行关机重启动作
private:
	virtual bool onHttpReq(socketTCP *psock,httpRequest &httpreq,httpSession &session,
			std::map<std::string,std::string>& application,httpResponse &httprsp);

	bool setLastModify(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp);
	bool httprsp_version(socketTCP *psock,httpResponse &httprsp);
	bool httprsp_docommandEx(socketTCP *psock,httpResponse &httprsp,const char *strCommand);
	bool httprsp_telnet(socketTCP *psock,httpResponse &httprsp,long lAccess);
	bool httprsp_checkcode(socketTCP *psock,httpResponse &httprsp,httpSession &session);
	bool httprsp_login(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp,httpSession &session);
	bool httprsp_capSetting(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp,httpSession &session,bool bSetting);
	//设置仅仅获取指定窗口的屏幕
	bool httprsp_capWindow(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp,httpSession &session);
	bool httprsp_getpswdfromwnd(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp,httpSession &session);
	bool httprsp_GetClipBoard(socketTCP *psock,httpResponse &httprsp);
	bool httprsp_SetClipBoard(socketTCP *psock,httpResponse &httprsp,const char *strval);
	bool httprsp_msevent(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp,httpSession &session);
	bool httprsp_keyevent(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp);
	bool httprsp_command(socketTCP *psock,httpResponse &httprsp,const char *ptrCmd);
	bool httprsp_cmdpage(socketTCP *psock,httpResponse &httprsp,const char *ptrCmd);
	bool httprsp_capDesktop(socketTCP *psock,httpResponse &httprsp,httpSession &session);
	bool httprsp_sysinfo(socketTCP *psock,httpResponse &httprsp);
	bool httprsp_usageimage(socketTCP *psock,httpResponse &httprsp);
	bool httprsp_fport(socketTCP *psock,httpResponse &httprsp);
	bool httprsp_plist(socketTCP *psock,httpResponse &httprsp);
	bool httprsp_mlist(socketTCP *psock,httpResponse &httprsp,DWORD pid);
	bool httprsp_pkill(socketTCP *psock,httpResponse &httprsp,DWORD pid);
	bool httprsp_mdattach(socketTCP *psock,httpResponse &httprsp,DWORD pid,HMODULE hmdl,long count);
	bool httprsp_slist(socketTCP *psock,httpResponse &httprsp);
	bool sevent(const char *sname,const char *cmd);
	bool httprsp_reglist(socketTCP *psock,httpResponse &httprsp,const char *skey,int listWhat);
	bool httprsp_regkey_del(socketTCP *psock,httpResponse &httprsp,const char *spath,const char *skey);
	bool httprsp_regkey_add(socketTCP *psock,httpResponse &httprsp,const char *spath,const char *skey);
	bool httprsp_regitem_del(socketTCP *psock,httpResponse &httprsp,const char *spath,const char *sname);
	bool httprsp_regitem_add(socketTCP *psock,httpResponse &httprsp,const char *spath,
							const char *stype,const char *sname,const char *svalue);
	bool httprsp_regitem_md(socketTCP *psock,httpResponse &httprsp,const char *spath,
							const char *stype,const char *sname,const char *svalue);

	bool  httprsp_filelist(socketTCP *psock,httpResponse &httprsp,const char *spath,int listWhat,bool bdsphide);
	bool  httprsp_folder_del(socketTCP *psock,httpResponse &httprsp,const char *spath,const char *fname,bool bdsphide);
	bool  httprsp_folder_new(socketTCP *psock,httpResponse &httprsp,const char *spath,const char *fname,bool bdsphide);
	bool  httprsp_folder_ren(socketTCP *psock,httpResponse &httprsp,const char *spath,
									 const char *fname,const char *newname,bool bdsphide);
	bool  httprsp_file_del(socketTCP *psock,httpResponse &httprsp,const char *spath,const char *fname,bool bdsphide);
	bool  httprsp_file_ren(socketTCP *psock,httpResponse &httprsp,const char *spath,
									 const char *fname,const char *newname,bool bdsphide);
	bool  httprsp_file_run(socketTCP *psock,httpResponse &httprsp,const char *spath);
	
	bool  httprsp_profile_verinfo(socketTCP *psock,httpResponse &httprsp,const char *spath);
	bool  httprsp_profile(socketTCP *psock,httpResponse &httprsp,const char *spath,const char *prof);
	bool  httprsp_profolder(socketTCP *psock,httpResponse &httprsp,const char *spath,const char *prof);
	bool  httprsp_prodrive(socketTCP *psock,httpResponse &httprsp,const char *spath,const char *svolu);
	bool  httprsp_get_upratio(socketTCP *psock,httpResponse &httprsp,httpSession &session);
	bool  httprsp_upload(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp,httpSession &session);
	
	bool  httprsp_ftpsets(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp);
	bool  httprsp_ftpusers(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp);
	bool  httprsp_ftpini(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp);
	
	bool  httprsp_proxysets(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp);
	bool  httprsp_proxyusers(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp);
	bool  httprsp_proxyini(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp);
	//--------------------------vIDC--------------------------------------------------
	bool  httprsp_mportl(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp);
	bool  httprsp_mportr(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp);
	bool  httprsp_vidcini(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp);
	bool  httprsp_vidcsvr(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp);
	bool  httprsp_vidccs(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp);
	
	bool httprsp_upnp(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp);
	bool httprsp_upnpxml(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp);
	//从资源中获取指定的文件
	const char *GetFileFromRes(const char *filepath,DWORD &flength);

	std::string m_defaultPage; //默认文档
	int m_quality;//捕获桌面图像的质量
	DWORD m_dwImgSize;//捕获桌面图像的大小 0-桌面大小，否则指定大小HWORD=h,WWORD=w
	bool m_bGetFileFromRes;

	//first,访问者帐号 - 不区分大小写。保存时一律为小写
	//second.first - 访问者帐号 second.second - 访问者权限
	std::map<std::string,std::pair<std::string,long> > m_mapUsers;

	bool m_bSSLenabled; //启动SSL服务
	bool m_bSSLverify; //是否进行客户端证书验证
	//yyc add 2010-02-23 如果从ini中显示的配置了账号则不支持匿名方式
	bool m_bAnonymous; //是否允许匿名访问，默认为真
};

//---------------------------------------------------------------
//---------------------------------------------------------------
typedef struct _TaskTimer //定时任务结构
{
	long h,m;//定时的时分,如果是定时间隔任务则h指示定时间隔(秒)
	long type; //定时类型 't'-轮询间隔(s)执行 'd'每天定时执行
	long flag; //默认0
	std::string strTask; //定时任务
}TaskTimer;
class sockEvent : public socketBase
{
public:
	sockEvent(){ m_sockstatus=SOCKS_OPENED; }
	virtual ~sockEvent(){}
	virtual void Close(){ m_sockstatus=SOCKS_CLOSED;}
}; //用于程序结束时及时关闭所有的阻塞socket
#include "NTService.h"
class MyService : public CNTService 
{
	sockEvent m_hSockEvent; //用于程序结束时及时关闭所有的阻塞socket
	HANDLE m_hStop; //服务停止Event对象句柄
	HANDLE m_hStopEvent; //是否允许通过SCM或控制台停止服务事件
						//如果设置了停止密码，则根据停止密码创建命名事件
	bool m_bSpyself;//是否监视自身异常退出
	bool m_bFaceless; //默认双击运行程序是否不带控制台界面
	std::vector<TaskTimer> m_tasklist; //定时执行任务列表
	bool CreateTaskTime(const char *ptrAt,const char *strTask);
	void parseCommand(const char *strCommand);
	void docmd_sets(const char *strParam);
//*********************用户其他代码 statrt ****************************************
public:
	webServer m_websvr;
//	msnShell m_msnbot;	//yyc remove MSN function
	ftpsvrEx m_ftpsvr;
	proxysvrEx m_proxysvr;
	telServerEx m_telsvr;
	//---------------------------vIDC------------------------------
	vidcManager m_vidcManager; //vidc集合管理类
	std::string m_preCmdpage; //cmdpage命令行页命令处理前缀
//*********************用户其他代码  end  ****************************************
public:
	static const char *ServiceVers;
	static MyService *GetService() { return (MyService *)AfxGetService(); }
public:
	explicit MyService(LPCTSTR ServiceName, LPCTSTR ServiceDesc = 0);
	virtual ~MyService(){}
	
	void SetStopEvent(const char *stop_pswd);
	BOOL AutoSpy(const char *commandline);//启动自动监视
	socketBase *GetSockEvent(){ return &m_hSockEvent; }
private:
	//创建服务停止密码保护事件
	void CreateStopEvent(const char *stop_pswd);
	//重载函数
	virtual void	Run(DWORD argc, LPTSTR *argv); //服务/程序运行体
	virtual void	Stop();//服务停止处理
	virtual void	Stop_Request();
	virtual void	Shutdown(); //系统关机处理

};
//将一个相对路径名转换为一个绝对路径名
extern void getAbsolutfilepath(std::string &spath);
extern int splitString(const char *str,char delm,std::map<std::string,std::string> &maps);
extern int splitString(const char *str,char delm,std::vector<std::string> &vec,int maxSplit);
