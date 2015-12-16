/*******************************************************************
   *	cLogger.h
   *    DESCRIPTION:日志记录对象
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2004-10-10
   *	Last modify: 2005-09-01
   *	net4cpp 2.1
   *******************************************************************/
  
#ifndef __YY_CLOGGER_H__
#define __YY_CLOGGER_H__

#include <cstdio>
#include <string>
#include <map>

typedef bool (CB_LOG_OUTPUT)(const char *,long,long);
namespace net4cpp21
{
	typedef enum //输出调试信息级别
	{
		LOGLEVEL_NONE=-1,//不输出此信息
		LOGLEVEL_DEBUG=0,//调试信息
		LOGLEVEL_WARN,//输出警告信息
		LOGLEVEL_INFO,//输出用户记录信息
		LOGLEVEL_ERROR,//输出错误信息
		LOGLEVEL_FATAL //输出致命错误信息
	}LOGLEVEL;
	typedef enum //日志输出类型
	{
		LOGTYPE_NONE,
		LOGTYPE_STDOUT,//输出到标准输出设备上
		LOGTYPE_FILE,//输出到文件
		LOGTYPE_HWND,//输出到一个windows的窗体上
		LOGTYPE_SOCKET,//输出到一个socket管道
	}LOGTYPE;
	class cLogger
	{
		LOGTYPE m_logtype;//日志输出类型
		LOGLEVEL m_loglevel;//日志级别
		long m_hout;//输出句柄，对于LOGTYPE_FILE类型对应FILE *,LOGTYPE_HWND类型对应HWND，LOGTYPE_SOCKET类型对应socket句柄
		TCHAR m_fileopenType[4]; //默认未覆盖打开方式"w"
		bool m_bOutStdout;//是否同步输出到控制台
		bool m_bOutputTime;//是否打印输出时间
		CB_LOG_OUTPUT *m_pcallback;
		long	m_lcbparam; //回调函数用户参数
		static cLogger *plogger;//静态日志类实例指针
		cLogger(); //不允许创建此类实例
		
		void _dump(LPCTSTR buf,size_t len);
	public:
		~cLogger();
		static cLogger & getInstance(){ return *plogger; }
		//设置是否打印时间
		void setPrintTime(bool bPrint){ m_bOutputTime=bPrint; return; }
		void setOutStdout(bool b) { m_bOutStdout=b; return; }
		LOGLEVEL setLogLevel(LOGLEVEL ll){ 
			LOGLEVEL lOld=m_loglevel; m_loglevel=ll; return lOld; }
		//设置日志文件打开方式
		void setOpenfileType(TCHAR c) { m_fileopenType[0]=c; return; }
		void setCallback(CB_LOG_OUTPUT *pfunc,long param)
		{
			m_pcallback=pfunc; m_lcbparam=param;
		}
		//是否允许输出指定级别的日志
		bool ifOutPutLog(LOGLEVEL ll) { return ( (unsigned int)m_loglevel<=(unsigned int)ll ); }
		LOGTYPE LogType() { return m_logtype; }
		LOGTYPE setLogType(LOGTYPE lt,long lParam);
		void flush(){ 
			if(m_logtype==LOGTYPE_FILE && m_hout)
				::fflush((FILE *)m_hout); return;}
		void dump(LOGLEVEL ll,LPCTSTR fmt,...);
		void dump(LOGLEVEL ll,size_t len,LPCTSTR buf);
		//输出DEBUG级别的日志
		void debug(LPCTSTR fmt,...);
		void debug(size_t len,LPCTSTR buf);
		void dumpBinary(const char *buf,size_t len);
		void dumpMaps(std::map<std::string,std::string> &maps,const char *desc);
		void printTime(); //打印当前时间
	};
}//?namespace net4cpp21

//设置在打印消息前先打印时间
#define RW_LOG_SETPRINTTIME(b) \
{ \
	cLogger::getInstance().setPrintTime(b); \
}
//设置日志输出级别
#define RW_LOG_SETLOGLEVEL(ll) cLogger::getInstance().setLogLevel(ll);
//#define RW_LOG_SETLOGLEVEL(ll) \
//{ \
//	cLogger::getInstance().setLogLevel(ll); \
//}
//设置日志输出回调
#define RW_LOG_CALLBACK(pfunc,param) \
{ \
	cLogger::getInstance().setCallback(pfunc,param); \
}
//设置日志输出到指定的文件
#define RW_LOG_SETFILE(filename) \
{ \
	cLogger::getInstance().setLogType(LOGTYPE_FILE,filename); \
}
//设置文件日志为追加方式，而不是覆盖方式
#define RW_LOG_OPENFILE_APPEND() \
{ \
	cLogger::getInstance().setOpenfileType('a'); \
}
//设置日志输出到指定的窗口
#define RW_LOG_SETHWND(hWnd) \
{ \
	cLogger::getInstance().setLogType(LOGTYPE_HWND,hWnd); \
}
//设置日志输出到指定的tcp socket
#define RW_LOG_SETSOCK(sockfd) \
{ \
	cLogger::getInstance().setLogType(LOGTYPE_SOCKET,sockfd); \
}
//设置日志输出到标准输出设备stdout
#define RW_LOG_SETSTDOUT() \
{ \
	cLogger::getInstance().setLogType(LOGTYPE_STDOUT,0); \
}
//不输出日志
#define RW_LOG_SETNONE() \
{ \
	cLogger::getInstance().setLogType(LOGTYPE_NONE,0); \
}

#define RW_LOG_FFLUSH() \
{ \
	cLogger::getInstance().flush(); \
}

#define RW_LOG_LOGTYPE cLogger::getInstance().LogType
//检查是否允许输出指定级别的日志
#define RW_LOG_CHECK cLogger::getInstance().ifOutPutLog
//日志输出
#define RW_LOG_PRINT cLogger::getInstance().dump
#define RW_LOG_PRINTBINARY cLogger::getInstance().dumpBinary
#define RW_LOG_PRINTMAPS cLogger::getInstance().dumpMaps
//打印当前时间
#define RW_LOG_PRINTTIME cLogger::getInstance().printTime
//设置是否同步输出到控制台
#define RW_LOG_OUTSTDOUT cLogger::getInstance().setOutStdout

#define RW_LOG_DEBUG if(cLogger::getInstance().ifOutPutLog(LOGLEVEL_DEBUG)) cLogger::getInstance().debug
#endif


