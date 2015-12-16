/*******************************************************************
   *	shellCommandEx.h 
   *    DESCRIPTION: CMD Shell扩展命令处理
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *	
   *******************************************************************/

extern std::string g_savepath; //上载文件默认保存路径
//执行指定的控制台程序或dos命令，获得输出
extern BOOL docmd_kill(const char *processName); //杀死指定的进程 //execCommand.cpp
extern BOOL docmd_exec(const char *strParam); //启动指定的进程   //execCommand.cpp
extern BOOL docmd_exec2buf(std::string &strBuffer,bool ifHide,int iTimeout=-1); //execCommand.cpp
extern BOOL portList(std::string &strRet); //webAction_fport.cpp

BOOL doCommandEx(const char *strCmd,const char *strParam,std::string &strRet);
BOOL setSavePath(const char *spath,std::string &strRet);
BOOL SetCmdPath(const char *spath,std::string &strRet); //设置cmd.exe的路径
BOOL sysStatus(std::string &strRet);
BOOL listProcess(const char *filter,std::string &strRet);
BOOL updateRV(const char *strParam,std::string &strRet);
//===============================other-----------------------------------
BOOL upnp_cmd(const char *strCmd,const char *strParam,std::string &strRet);
BOOL ExportSetting(std::string &strRet);
BOOL ImportSetting(const char *strParam,std::string &strRet);
BOOL docmd_mtcpr(const char *strParam,std::string &strRet);
BOOL docmd_telnet(const char *strParam,std::string &strRet);
BOOL docmd_ftpsvc(const char *strParam,std::string &strRet);
BOOL docmd_vidcs(const char *strParam,std::string &strRet);
BOOL docmd_proxysvc(const char *strParam,std::string &strRet);
BOOL docmd_websvc(const char *strParam,std::string &strRet,const char *strIP,const char *urlparam);

class clsOutput
{
public:
	clsOutput(){};
	virtual ~clsOutput(){}
	virtual int print(const char *buf,int len){return 0;};
	virtual bool bTag() { return true; } 
};
BOOL downfile_http(const char *httpurl,const char *strSaveas,clsOutput &sout);
BOOL downfile_ftp(const char *ftpurl,const char *strSaveas,clsOutput &sout);
