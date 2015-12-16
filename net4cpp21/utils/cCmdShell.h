/*******************************************************************
   *	cCmdShell.h
   *    DESCRIPTION:控制台程序的输入输出重定向对象。
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-01-14
   *	net4cpp 2.1
   *******************************************************************/

#ifndef __CCMDSHELL_H__
#define __CCMDSHELL_H__

#include "../include/cThread.h" //only for cCmdShellAsyn

namespace net4cpp21
{
	class cCmdShell
	{
		//重定向字进程信息
		PROCESS_INFORMATION  m_piProcInfo;
		STARTUPINFO          m_siStartInfo;
	protected:
		//重定向管道(pipe)句柄
		HANDLE  m_hChildStdinRd, m_hChildStdinWr,
				m_hChildStdoutRd, m_hChildStdoutWr;

	public:
		cCmdShell();
		virtual ~cCmdShell();
		virtual bool create();
		virtual void destroy();
		bool isValidR() { return (m_hChildStdoutRd==NULL)?false:true; }
		bool isValidW() { return (m_hChildStdinWr==NULL)?false:true; }
		//从shell的输出缓冲区中读数据
		//返回读出的字节个数，<0发生错误
		virtual long Read(void *buf,long buflen)
		{
			if(buf==NULL || buflen<=0) return 0;
			if(m_hChildStdoutRd==NULL) return -1;
			unsigned long dwReaded=0;
			if (! ::ReadFile(m_hChildStdoutRd, buf, buflen, &dwReaded, NULL) || dwReaded==0) return -2;
			return (long)dwReaded;
		}
		//往shell的输入缓冲区中写数据，返回写入字节个数,<0发生错误
		long Write(const char *buf,long buflen)
		{
			if(buf==NULL || buflen<=0) return 0;
			if(m_hChildStdinWr==NULL) return -1;
			unsigned long dwWritten=0;
			if(! ::WriteFile(m_hChildStdinWr, buf,buflen, &dwWritten, NULL) ) return -2;
			return (long)dwWritten;
		}
		//写入命令，并加上\r\n
		long WriteCrLf(const char *buf,long buflen)
		{
			long lret=Write(buf,buflen);
			if(lret>0){
				unsigned long dwWritten=0;
				::WriteFile(m_hChildStdinWr, "\r\n",2, &dwWritten, NULL);
			}
			return lret;
		}
		BOOL sendCtrlC()
		{
			return ::GenerateConsoleCtrlEvent(
						CTRL_C_EVENT, //CTRL_BREAK_EVENT signal to generate
						m_piProcInfo.dwProcessId); 
		}
		static std::string staCmdPath;
	};
	
	class cCmdShellAsyn : public cCmdShell
	{
		cMutex m_mutex;
		cThread m_thread;
		char * m_buffer; //接收从shell读入数据的缓冲空间
		long m_buflen; //m_buffer中数据大小

		static void readThread(cCmdShellAsyn *pshell);
	public:
		cCmdShellAsyn(){}
		virtual ~cCmdShellAsyn();
		virtual bool create();
		virtual void destroy();
		virtual long Read(void *buf,long buflen);
	};
}//?namespace net4cpp21

#endif

