/*******************************************************************
   *	cCmdShell.cpp
   *    DESCRIPTION:控制台程序的输入输出重定向对象。
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-01-14
   *	net4cpp 2.1
   *******************************************************************/
#include "../include/sysconfig.h"
#include "../utils/cCmdShell.h"
#include "../utils/utils.h"

using namespace net4cpp21;
//设置默认的cmd命令解释程序和路径，有些网管会删除system32下的cmd.exe或改名
std::string cCmdShell::staCmdPath="cmd.exe";
cCmdShell::cCmdShell()
{
	memset((void *)&m_piProcInfo,0,sizeof(PROCESS_INFORMATION));
	memset((void *)&m_siStartInfo,0,sizeof(STARTUPINFO));
	m_siStartInfo.cb = sizeof(STARTUPINFO);

	m_hChildStdinRd=m_hChildStdinWr=NULL;
	m_hChildStdoutRd=m_hChildStdoutWr=NULL;
}

cCmdShell::~cCmdShell()
{
	destroy();
}

void cCmdShell::destroy()
{
	
	if(m_hChildStdoutWr){
		unsigned long dwWritten=0; 
		::WriteFile(m_hChildStdoutWr, "\r\nquit cmd!\r\n",13, &dwWritten, NULL);
		::CloseHandle(m_hChildStdoutWr);
	}
	if(m_hChildStdoutRd) ::CloseHandle(m_hChildStdoutRd);

	if(m_hChildStdinRd) ::CloseHandle(m_hChildStdinRd);
	if(m_hChildStdinWr) ::CloseHandle(m_hChildStdinWr);

	if(m_piProcInfo.hThread) ::CloseHandle(m_piProcInfo.hThread);
	if(m_piProcInfo.hProcess){
		::TerminateProcess(m_piProcInfo.hProcess,0);
		::CloseHandle(m_piProcInfo.hProcess);
	}

	memset((void *)&m_piProcInfo,0,sizeof(PROCESS_INFORMATION));
	memset((void *)&m_siStartInfo,0,sizeof(STARTUPINFO));
	m_siStartInfo.cb = sizeof(STARTUPINFO);

	m_hChildStdinRd=m_hChildStdinWr=NULL;
	m_hChildStdoutRd=m_hChildStdoutWr=NULL;
}

bool cCmdShell::create()
{
	BOOL bRet=FALSE;
	HANDLE hSaveStdin=NULL, hSaveStdout=NULL,hSaveStderr=NULL;
	bool bSetStdin=false,bSetStdout=false,bSetStderr=false;
	while(true){
		SECURITY_ATTRIBUTES saAttr;
		// Set the bInheritHandle flag so pipe handles are inherited.
		saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
		saAttr.bInheritHandle = true;
		saAttr.lpSecurityDescriptor = NULL;
		//保存标准输出句柄
		hSaveStdout = ::GetStdHandle(STD_OUTPUT_HANDLE);
		// Create a pipe for the child's STDOUT.
		if (! ::CreatePipe(&m_hChildStdoutRd, &m_hChildStdoutWr, &saAttr, 0)) break;
		// Set a write handle to the pipe to be STDOUT.
		if (! ::SetStdHandle(STD_OUTPUT_HANDLE, m_hChildStdoutWr)) break;
		else bSetStdout=true;
		// Save the handle to the current STDERR.
		hSaveStderr = ::GetStdHandle(STD_ERROR_HANDLE);
		if (! ::SetStdHandle(STD_ERROR_HANDLE, m_hChildStdoutWr)) break;
		else bSetStderr=true;

		// Save the handle to the current STDIN.
		hSaveStdin = ::GetStdHandle(STD_INPUT_HANDLE);
		if (! ::CreatePipe(&m_hChildStdinRd, &m_hChildStdinWr, &saAttr, 0)) break;
		if (! SetStdHandle(STD_INPUT_HANDLE, m_hChildStdinRd)) break;
		else bSetStdin=true;

		// Duplicate the write handle to the pipe so it is not inherited.
		HANDLE hChildStdDup;
		if(! ::DuplicateHandle(::GetCurrentProcess(), m_hChildStdinWr,
		::GetCurrentProcess(), &hChildStdDup, 0,
           FALSE,                  // not inherited
           DUPLICATE_SAME_ACCESS)
		   ) break;
		::CloseHandle(m_hChildStdinWr); m_hChildStdinWr=hChildStdDup; 

		//开始创建要进行重定向的子进程
		m_siStartInfo.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
		m_siStartInfo.hStdInput = m_hChildStdinRd;
		m_siStartInfo.hStdOutput = m_hChildStdoutWr;
		m_siStartInfo.hStdError = m_hChildStdoutWr; //m_hChildStderrWr;
		m_siStartInfo.wShowWindow = SW_HIDE;
		std::string strCmd=cCmdShell::staCmdPath;
		bRet=::CreateProcess(
		  NULL,          // executable module
		  (char *)strCmd.c_str(),
		  NULL,          // process security attributes
		  NULL,          // primary thread security attributes
		  TRUE,          // handles are inherited !!! Must
		  0,//CREATE_NEW_PROCESS_GROUP|CREATE_NEW_CONSOLE,             // creation flags
		  NULL,          // use parent's environment
		  NULL,//HomeDirectory.c_str(),        // current directory
		  &m_siStartInfo,  // STARTUPINFO pointer
		  &m_piProcInfo);  // receives PROCESS_INFORMATION
		break;
	}//?while(true);
	
	// After process creation, restore the saved STDIN and STDOUT( and STDERR [ebt]).
	if(bSetStdin) ::SetStdHandle(STD_INPUT_HANDLE, hSaveStdin);
	if(bSetStdout) ::SetStdHandle(STD_OUTPUT_HANDLE, hSaveStdout);
	if(bSetStderr) ::SetStdHandle(STD_ERROR_HANDLE, hSaveStderr);
	if(bRet) return true;
	destroy(); return false;
}

/*
//往shell的输出缓冲区中写入，可通过read读出
long cCmdShell::Fill(void *buf,long buflen)
{
	if(buf==NULL || buflen<=0) return 0;
	if(m_hChildStdoutWr==NULL) return -1;
	unsigned long dwWritten=0; 
	if(!::WriteFile(m_hChildStdoutWr, buf,buflen, &dwWritten, NULL) ) return -2;
	return (long)dwWritten;
}
*/
//-------------------------------------------------------

cCmdShellAsyn :: ~cCmdShellAsyn()
{
	this->destroy();
}

bool cCmdShellAsyn :: create()
{
	if(!cCmdShell::create()) return false;
	if( m_thread.start( (THREAD_CALLBACK *)&readThread,(void *)this) ) return true;
	this->destroy(); return false;
}

void cCmdShellAsyn :: destroy()
{
	cCmdShell::destroy();
	m_thread.join(); //等待线程结束
}

long cCmdShellAsyn ::Read(void *buf,long buflen)
{
	if(buf==NULL || buflen<=0) return 0;
	if(m_hChildStdoutRd==NULL) return -1;
	m_mutex.lock();
	if(buflen>m_buflen) buflen=m_buflen;
	if(buflen>0){
		memcpy(buf,m_buffer,buflen);
		if( (m_buflen-=buflen)> 0)
			memmove(m_buffer,m_buffer+buflen,m_buflen);
	}//?if(buflen>0)
	m_mutex.unlock();
	return buflen;
}

#define recvBufferLen 2048
void cCmdShellAsyn::readThread(cCmdShellAsyn *pshell)
{
	if(pshell==NULL) return;
	if(pshell->m_hChildStdoutRd==NULL) return;
	//为m_buffer分配缓冲空间
	pshell->m_buffer=new char[recvBufferLen];
	if(pshell->m_buffer==NULL) return;
	pshell->m_buflen=0; //初始化数据长度
	
	char buf[256]; int len=0;
	while(pshell->m_hChildStdoutRd)
	{
		if(len<=0){
			len=pshell->cCmdShell::Read(buf,255);
			if(len<0) break;
			if(len==0) continue;
			buf[len]=0;
		}//?如果len>0说明上次读取的还未写进缓冲区

		pshell->m_mutex.lock();
		if( (len+pshell->m_buflen)<recvBufferLen )
		{
			memcpy(pshell->m_buffer+pshell->m_buflen,buf,len);
			pshell->m_buflen+=len; len=0;
		}
		pshell->m_mutex.unlock();
		if(len>0) cUtils::usleep(200000) ;//休眠200ms，等待缓冲区被读取
	}//?while(pshell->m_hChildStdoutRd)

	pshell->cCmdShell::destroy(); 
	delete[] pshell->m_buffer;
	pshell->m_buffer=NULL; pshell->m_buflen=0;
	return;
}

