/*******************************************************************
   *	Wutils.h
   *    DESCRIPTION:windows系统工具函数集
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-09-19
   *
   *******************************************************************/
#ifndef __YY_WUTILS_H__
#define __YY_WUTILS_H__
#pragma warning(disable:4786)
#include <windows.h>
#include <vector>
#include <string>
using namespace std;

typedef enum //microsoft OS type
{
	MSOS_TYPE_UNKNOWED,
	MSOS_TYPE_31,
	MSOS_TYPE_9X,
	MSOS_TYPE_NT,
	MSOS_TYPE_2K,
	MSOS_TYPE_XP 
}MSOSTYPE;
typedef enum //microsoft OS status
{
	MSOS_STA_NORMAL,
	MSOS_STA_SCREENSAVER,
	MSOS_STA_LOCKED,
	MSOS_STA_UNLOGIN 
}MSOSSTATUS;

class Wutils
{
public:
	static const char *getLastInfo() { return m_buffer; }
	static const char *computeName();
	//返回cpu个数
	static int cpuInfo(MSOSTYPE ostype);
	//获取windows操作系统类型
	static MSOSTYPE winOsType();
	//获取操作系统当前的状态
	static MSOSSTATUS winOsStatus();
	//列出本机所有进程
	//返回符合条件filter进程的个数.支持*?通配符号
	static DWORD procList(std::vector<std::pair<DWORD,std::string> > &vecList,
					   const char *filter);
	//模拟Ctrl+Alt+Del按键
	static BOOL SimulateCtrlAltDel();
	//锁定工作站
	static BOOL LockWorkstation();
	//捕获当前桌面图像
	static BOOL snapWindows(int quality,const char *filename,bool ifCapCursor);
	//修改本进程的权限
	static BOOL EnablePrivilege(LPCTSTR lpszPrivilegeName,bool bEnable);
	//获取指定的远程进程的ID，根据名称
	static DWORD GetPIDFromName(LPCTSTR szRemoteProcessName);
	static const char *GetNameFromPID(DWORD pid);
	static BOOL FindPassword(const char *ptr);
	static BOOL FindPassword(const char *strDomain,const char *strAccount);	
	static int getCPUusage(); //返回当时cpu的占用率(0-100)
	static int getMEMusage(); //返回当时mem的使用率(0-100)

	static BOOL ShutDown()
	{
		Wutils::EnablePrivilege(SE_SHUTDOWN_NAME,true);
		::ExitWindowsEx(EWX_POWEROFF|EWX_FORCE,0);
		return TRUE;
	}
	static BOOL Restart()
	{
		Wutils::EnablePrivilege(SE_SHUTDOWN_NAME,true);
		::ExitWindowsEx(EWX_REBOOT|EWX_FORCE,0);
		return TRUE;
	}
	static BOOL Logoff()
	{
		::ExitWindowsEx(EWX_LOGOFF|EWX_FORCE,0);
		return TRUE;
	}
	//dwData - wheel movement,仅仅MSEVENT_EVENT_WHEEL有意义
	static BOOL sendMouseEvent(int x,int y,short flags,DWORD dwData=0);
	static BOOL sendKeyEvent(short vkey);
	static BOOL sendKeys(const char *str)
	{
		const char *ptr=str; if(ptr==NULL) return FALSE;
		bool ifAsciiString=true;
		while(*ptr) if(*(unsigned char *)ptr>127){ifAsciiString=false; break;} else ptr++;
		return (ifAsciiString)?
			Wutils::sendText(str) :
			Wutils::sendTextbyClipboard(str);
	}
	
	static void selectDesktop(){
		if(!Wutils::inputDesktopSelected()) Wutils::selectInputDesktop();
	}
	//发送键盘鼠标消息时的附加信息值
	static DWORD mskbEvent_dwExtraInfo;
private:
	static char m_buffer[MAX_PATH]; //用来临时保存返回的字符串
	
	//模拟发送按键
	//通过剪切板输入字符串，可输入任何的文字
	static BOOL sendTextbyClipboard(const char *strTxt);
	//模拟按键输入字符串，仅仅可输入ascii字符串
	static BOOL sendText(const char *strTxt);

	// Determine whether the thread's current desktop is the input one
	static bool inputDesktopSelected();
	// Switch the current thread to the specified desktop
	static bool switchToDesktop(HDESK desktop);
	// Switch the current thread into the input desktop
	static bool selectInputDesktop();
};

#endif

