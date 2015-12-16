/*******************************************************************
   *	execCommand.cpp 
   *    DESCRIPTION:处理执行exec和kill命令
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-12-30
   *	
   *******************************************************************/

#include <windows.h>
#include <string>
#include "cInjectDll.h"

inline bool isDigest(const char *str)
{
	if(str==NULL) return false;
	while(*str!=0){if(*str>='0' && *str<='9') str++; else return false; }
	return true;
}
//杀死指定的进程
//命令格式: 
//	kill <pid/进程名>,<pid/进程名>,<pid/进程名>...
BOOL docmd_kill(const char *processName)
{
	if(processName==NULL || processName[0]==0)
		return FALSE;
	std::string tmpStr(processName); BOOL bRet=FALSE;
	char *pstr=(char *)(tmpStr.c_str());
	char *token = strtok(pstr, ",");
	while( token != NULL )
	{
		long pID_l=0;
		if(isDigest(token)) pID_l=atol(token);
		DWORD pID=(pID_l>0)?((DWORD)pID_l):
				  (cInjectDll::GetPIDFromName(token));
		if(pID==0 && strcmp(token,"ME")==0) pID=GetCurrentProcessId();
		if(pID)
		{
			HANDLE hProcess=::OpenProcess(PROCESS_ALL_ACCESS,TRUE,pID);
			if(hProcess)
			{
				::TerminateProcess(hProcess,0);
				::CloseHandle(hProcess);
				bRet=TRUE;
			}
		}//?if(token[0]!=0)
		/* Get next token: */
		token = strtok( NULL, ",");
	}//?while(...
	return bRet;
}

extern BOOL StartInteractiveClientProcess (
    LPTSTR lpszUsername,    // client to log on
    LPTSTR lpszDomain,      // domain of client's account
    LPTSTR lpszPassword,    // client's password
    LPTSTR lpCommandLine,    // command line to execute
	bool ifHide,
	long lwaittimeout
) ;
//启动指定的进程
//命令格式: 
//	exec [-hide] [-check <programname>] [-user <[Domain\]account:password>] [-wait <dwMilliseconds>] <full path program>
//-hide  - 是否后台运行指定的程序
//-check <programname> - 执行指定的程序前是否先检测<programname>是否已经运行，如果已运行则不执行指定的程序
//-user <[Domain\]account:password> - 以指定得帐号启动程序，仅仅在程序以服务方式运行时有效。
//			Domain域可以不指定，如果不指定则默认为本地帐号
//-wait <dwMilliseconds> - 成功启动进程后是否等待进程结束==0，不等待,<0等待进程结束,>0 等待指定得毫秒
//-shell //shell方式运行
//<full path program>  - 指定要执行的程序
BOOL docmd_exec(const char *strParam)
{
	if(strParam==NULL || strParam[0]==0) return FALSE;
	bool ifHide=false;//是否后台隐藏执行
	bool ifShell=false; //yyc add 2011-01-05
	std::string strcheck="";//是否判断程序已经运行，如果已经运行则仅仅执行一次
	std::string strAccount="",strPwd="",strDomain;
	long lwaittimeout=0;//是否等待进程结束
	char *ptr=(char *)strParam;
	while(ptr[0]=='-')
	{
		if(strncmp(ptr,"-hide ",6)==0)
		{	ifHide=true; ptr+=6; }
		else if(strncmp(ptr,"-shell ",7)==0)
		{	ifShell=true; ptr+=7; }
		else if(strncmp(ptr,"-check ",7)==0)
		{
			char *pos=(*(ptr+7)=='-')?NULL:strchr(ptr+7,' ');
			if(pos){
				*pos=0; strcheck.assign(ptr+7); *pos=' ';
				ptr=pos+1;
			}
			else ptr+=7;
		}
		else if(strncmp(ptr,"-wait ",6)==0)
		{//是否等待进程结束
			char *pos=(*(ptr+6)=='-')?NULL:strchr(ptr+6,' ');
			if(pos){
				*pos=0; lwaittimeout=atol(ptr+6); *pos=' ';
				ptr=pos+1;
			}
			else ptr+=6;
		}//?else if(strncmp(ptr,"-wait ",6)==0)
		else if(strncmp(ptr,"-user ",6)==0)
		{//以指定得用户运行，格式　-user domain\account:password
			char *pos=(*(ptr+6)=='-')?NULL:strchr(ptr+6,' ');
			if(pos){//指定了帐号
				*pos=0; std::string strUser;
				strUser.assign(ptr+6); *pos=' ';
				ptr=pos+1;
				//解析出domain account password
				const char *ptrBegin=strUser.c_str();
				const char *pTmp=strchr(ptrBegin,'\\');
				if(pTmp){
					*(char *)pTmp=0;
					strDomain.assign(ptrBegin);
					*(char *)pTmp='\\';
					ptrBegin=pTmp+1;
				}
				if( (pTmp=strchr(ptrBegin,':')) )
				{//指定了帐号密码
					*(char *)pTmp=0;
					strAccount.assign(ptrBegin);
					*(char *)pTmp=':';
					strPwd.assign(pTmp+1);
				}
				else //仅仅指定了帐号，密码为""
					strAccount.assign(ptrBegin);
			}//?if(pos){//指定了帐号
			else ptr+=6;
		}//?else if(strncmp(ptr,"-user ",6)==0)
		else ptr++;
	}//?while
	if(strcheck!=""){
		//是否判断程序已经运行,如果已经运行则不再执行
		DWORD pID=cInjectDll::GetPIDFromName(strcheck.c_str());
		if(pID) return TRUE;
	}
	
	if(ifShell){ //Shell方式打开指定的文件，不支持指定帐号运行
		unsigned long iret=(unsigned long)::ShellExecute(NULL,"open",
			ptr,NULL,NULL,((ifHide)?SW_HIDE:SW_SHOW) );
		return (iret>32)?TRUE:FALSE;
	}//?if(ifShell) //yyc add 2011-01-05

	if(strAccount!=""){ //用指定得帐号运行进程,必须是localsystem权限才行
		if(strDomain=="") strDomain.assign(".");
		BOOL b=StartInteractiveClientProcess((char *)strAccount.c_str(),(char *)strDomain.c_str(),
			(char *)strPwd.c_str(),ptr,ifHide,lwaittimeout);
		if( b )
		{
//			RW_LOG_PRINT(LOGLEVEL_WARN,"[EXEC] - success to execute %s as User(%s)!\r\n",
//				ptr,strAccount.c_str());
			return TRUE;
		}
//		else
//			RW_LOG_PRINT(LOGLEVEL_WARN,"[EXEC] - failed to execute %s as user(%s\\%s)!\r\n",
//			ptr,strDomain.c_str(),strAccount.c_str());
	}//?if(strAccount!=""){ //用指定得帐号运行进程

	STARTUPINFO si; PROCESS_INFORMATION pi;
	memset((void *)&pi,0,sizeof(PROCESS_INFORMATION));
	memset((void *)&si,0,sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.wShowWindow =(ifHide)?SW_HIDE:SW_NORMAL;
	if(::CreateProcess(NULL,ptr,NULL,NULL,FALSE,0,NULL, NULL,&si,&pi))
	{
		if(lwaittimeout!=0 && pi.hProcess != INVALID_HANDLE_VALUE)
		{
//			RW_LOG_PRINT(LOGLEVEL_WARN,"[EXEC] - success to execute %s,waiting %d!\r\n",ptr,lwaittimeout);
			if(lwaittimeout<0)
				WaitForSingleObject(pi.hProcess, INFINITE);
			else
				WaitForSingleObject(pi.hProcess, lwaittimeout);
		}//?if(lwaittimeout!=0)
//		else
//			RW_LOG_PRINT(LOGLEVEL_WARN,"[EXEC] - success to execute %s!\r\n",ptr);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return TRUE;
	}
//	RW_LOG_PRINT(LOGLEVEL_WARN,"[EXEC] - failed to execute %s!\r\n",ptr);
	return FALSE;
}

//执行指定的控制台程序或dos命令，获得输出
//[in|out] strBuffer - [in] 指向要执行的控制台程序或dos命令
//					 - [out] 返回执行的输出
//iTimeout -- 命令执行超时时间s, -1不限
BOOL docmd_exec2buf(std::string &strBuffer,bool ifHide,int iTimeout)
{
	STARTUPINFO si; PROCESS_INFORMATION pi;
	memset((void *)&pi,0,sizeof(PROCESS_INFORMATION));
	memset((void *)&si,0,sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.wShowWindow =(ifHide)?SW_HIDE:SW_NORMAL;
	
	char buffer[4096]; long buflen=0; BOOL bret=TRUE;
	HANDLE hChildStdoutRd=NULL,hChildStdoutWr=NULL; 
	//此时可设置标准输出指向一个特定的管道---------------------------
	SECURITY_ATTRIBUTES saAttr;
	// Set the bInheritHandle flag so pipe handles are inherited.
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = true;
	saAttr.lpSecurityDescriptor = NULL;
	try{// Create a pipe for the child's STDOUT.
		if(!::CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0))
			throw "[CMD] Failed to create stdout pipe.\r\n";
		si.hStdOutput = hChildStdoutWr;
		si.hStdError = hChildStdoutWr;
		char *lpCommandLine=(char *)strBuffer.c_str();
//		printf("%s\r\n",lpCommandLine);
		if(!::CreateProcess(NULL,lpCommandLine ,NULL,NULL,TRUE,0,NULL, NULL,&si,&pi))
			throw "[CMD] Failed to CreateProcess.\r\n";
		DWORD dwret=(iTimeout<0)?INFINITE:iTimeout*1000;
		dwret=WaitForSingleObject(pi.hProcess, dwret);	
		if(dwret!=WAIT_OBJECT_0)
		{
			::TerminateProcess(pi.hProcess,0);
			throw "[CMD] - The time-out interval elapsed executing command.\r\n";
		}else{ //读出管道中的数据
			unsigned long dw=0;
			if( ::WriteFile(hChildStdoutWr, " ",1, &dw, NULL) )
			{
				if (! ::ReadFile(hChildStdoutRd, buffer, 4095, &dw, NULL) || dw==0)
					throw "[CMD] - Failed to ReadFile\r\n";
				else buflen=dw;
			}//?if( ::WriteFile(...
			else throw "[CMD] Failed to WriteFile.\r\n";
		} //if(dwret!=WAIT_OBJECT_0)...end
		::CloseHandle(pi.hProcess);
		::CloseHandle(pi.hThread);
	}catch(const char *message)
	{
		buflen=sprintf(buffer,"%s",message);
		bret=FALSE;
	}
	if(buflen>0) strBuffer.assign(buffer,buflen);
	else strBuffer="";
//	printf("aaaastrBuffer:%s\r\n",strBuffer.c_str());
	if(hChildStdoutWr) ::CloseHandle(hChildStdoutWr);
	if(hChildStdoutRd) ::CloseHandle(hChildStdoutRd);
	return bret;
}
/* yyc remove 2009-03-26 测试飞信机器人时发现，原来的docmd_exec2buf频繁连续调用程序会崩溃
BOOL docmd_exec2buf(std::string &strBuffer,bool ifHide,int iTimeout)
{
	STARTUPINFO si; PROCESS_INFORMATION pi;
	memset((void *)&pi,0,sizeof(PROCESS_INFORMATION));
	memset((void *)&si,0,sizeof(STARTUPINFO));
	si.cb = sizeof(STARTUPINFO);
	si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.wShowWindow =(ifHide)?SW_HIDE:SW_NORMAL;
	
	HANDLE hChildStdoutRd=NULL,hChildStdoutWr=NULL;
	//保存标准输出句柄
	HANDLE hSaveStdout = ::GetStdHandle(STD_OUTPUT_HANDLE);
	HANDLE hSaveStderr = ::GetStdHandle(STD_ERROR_HANDLE);
//此时可设置标准输出指向一个特定的管道---------------------------
	SECURITY_ATTRIBUTES saAttr;
	// Set the bInheritHandle flag so pipe handles are inherited.
	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
	saAttr.bInheritHandle = true;
	saAttr.lpSecurityDescriptor = NULL;
	// Create a pipe for the child's STDOUT.
	if (! ::CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0))
	{
		strBuffer.assign("[CMD] Failed to create stdout pipe.\r\n");
		return FALSE;
	}
	// Set a write handle to the pipe to be STDOUT.
	::SetStdHandle(STD_OUTPUT_HANDLE, hChildStdoutWr);
	::SetStdHandle(STD_ERROR_HANDLE, hChildStdoutWr);
	si.hStdOutput = hChildStdoutWr;
	si.hStdError = hChildStdoutWr;
//此时可设置标准输出指向一个特定的管道---------------------------
	char buffer[4096]; 
	if(strBuffer.length()>4095) strBuffer[4095]=0; //保护
	sprintf(buffer,"%s",strBuffer.c_str());

//	printf("%s\r\n",buffer);

	BOOL bret=::CreateProcess(NULL,buffer ,NULL,NULL,TRUE,0,NULL, NULL,&si,&pi);
	if(bret)
	{
		DWORD dwret=(iTimeout<0)?INFINITE:iTimeout*1000;
		dwret=WaitForSingleObject(pi.hProcess, dwret);
		if(dwret!=WAIT_OBJECT_0)
		{
			bret=::TerminateProcess(pi.hProcess,0);
			strBuffer.assign("[CMD] - The time-out interval elapsed executing command.\r\n");
			char ss[32]; sprintf(ss,"kill it return %s\r\n",(bret)?"true":"false");
			strBuffer.append(ss);
			bret=FALSE;
		}
		else //?if(dwret!=WAIT_OBJECT_0)...else
		{//读出管道中的数据
			unsigned long dw=0;
			if( ::WriteFile(hChildStdoutWr, " ",1, &dw, NULL) )
			{
				if (! ::ReadFile(hChildStdoutRd, buffer, 4095, &dw, NULL) || dw==0)
				{
					strBuffer.assign("[CMD] - Failed to ReadFile\r\n"); 
					bret=FALSE;
				}else{
					buffer[dw]=0;
					strBuffer.assign(buffer);
				}
			}//?if( ::WriteFile(...
			else{
				strBuffer=""; bret=FALSE;
			}
		} //if(dwret!=WAIT_OBJECT_0)...end
		::CloseHandle(pi.hProcess);
		::CloseHandle(pi.hThread);
	}
	else
	{
		strBuffer.assign("[CMD] - failed to execute command!\r\n");
	}
//	if(hSaveStdout) 
	::SetStdHandle(STD_OUTPUT_HANDLE, hSaveStdout);
	::SetStdHandle(STD_OUTPUT_HANDLE, hSaveStderr);
	if(hChildStdoutWr) ::CloseHandle(hChildStdoutWr);
	if(hChildStdoutRd) ::CloseHandle(hChildStdoutRd);

	return bret;
}
*/