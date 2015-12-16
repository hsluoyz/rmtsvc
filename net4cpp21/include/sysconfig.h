//sysconfig.h
#ifndef __YY_SYSCONFIG_H__
#define __YY_SYSCONFIG_H__

#ifdef WIN32 //windows系统平台
	#pragma warning(disable:4786)
	#pragma warning(disable:4503)
	#include <windows.h> //包含windows的头文件
	#define	MSG_NOSIGNAL    0  //windows下没有此定义
	
	#define strcasecmpW _wcsicmp
	#define strncasecmpW _wcsnicmp
	#define vsnprintfW _vsnwprintf
	#define stringlenW wcslen
	#define strprintfW swprintf
	#define fileopenW _wfopen

	#ifdef UNICODE
	#define strcasecmp strcasecmpW
	#define strncasecmp strncasecmpW
	#define vsnprintf vsnprintfW
	#define stringlen stringlenW
	#define strprintf strprintfW
	#define fileopen fileopenW
	#else	
	#define strcasecmp _stricmp
	#define strncasecmp _strnicmp
	#define vsnprintf _vsnprintf
	#define stringlen strlen
	#define strprintf sprintf
	#define fileopen fopen
	#endif
#elif defined MAC //暂时不支持
	typedef unsigned short WCHAR;
	//...
#else  //unix/linux平台
	//Sun unix下没有定义此常量，linux下在/usr/include/bits/socket.h中定义有此常量
	//EPIPE  The local end has been shut down on a connection oriented socket.  
	//In this case the  process  will  also receive a SIGPIPE unless MSG_NOSIGNAL is set.
	//如不保护，将导致程序发生broken pipe错误
	#define MSG_NOSIGNAL 0x4000	

	typedef wchar_t WCHAR;
	typedef unsigned __int64 DWORD64;
	typedef __int64 LONG64;
	
#endif //#ifdef WIN32 ...#else...

#endif //?#ifndef __YY_SYSCONFIG_H__

