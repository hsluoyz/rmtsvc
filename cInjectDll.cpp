/*******************************************************************
 *	cInjectDll.cpp
 *    DESCRIPTION:远程DLL注射，执行指定的函数
 *
 *    AUTHOR:yyc
 *
 *    http://hi.baidu.com/yycblog/home
 *
 *    DATE:2003-02-25
 *
 *******************************************************************/

#include <windows.h>
#include <tlhelp32.h> //枚举所有进程

#include "cInjectDll.h"

#ifdef UNICODE
LPCSTR LoadLibraryFuncStr = "LoadLibraryW";
LPCSTR GetModuleHandleFuncStr = "GetModuleHandleW";
LPCSTR CreateFileFuncStr="CreateFileW";
#else
LPCSTR LoadLibraryFuncStr = "LoadLibraryA";
LPCSTR GetModuleHandleFuncStr = "GetModuleHandleA";
LPCSTR CreateFileFuncStr="CreateFileA";
#endif
LPCSTR FreeLibraryFuncStr = "FreeLibrary";
LPCSTR GetProcAddressFuncStr = "GetProcAddress";
LPCSTR GetLastErrorFuncStr = "GetLastError";
LPCSTR ExitThreadFuncStr = "ExitThread";
LPCSTR VirtualFreeFuncStr = "VirtualFree";
LPCSTR CloseHandleFuncStr= "CloseHandle";
LPCSTR WriteFileFuncStr="WriteFile";

cInjectDll::cInjectDll(LPCTSTR szRemoteProcessName)
{
	if(szRemoteProcessName)
		 m_dwProcessId=GetPIDFromName(szRemoteProcessName);
	else m_dwProcessId=0;
	//初始化参数
	memset((void *)&m_InjectLibInfo,0,sizeof(INJECTLIBINFO));
    m_InjectLibInfo.pfnLoadLibrary = (PLOADLIBRARY)GetProcAddress(GetModuleHandle
("Kernel32.dll"),LoadLibraryFuncStr);
	m_InjectLibInfo.pfnFreeLibrary = (PFREELIBRARY)GetProcAddress(GetModuleHandle
("Kernel32.dll"),FreeLibraryFuncStr);
	m_InjectLibInfo.pfnGetProcaddr = (PGETPROCADDRESS)GetProcAddress(GetModuleHandle
("Kernel32.dll"),GetProcAddressFuncStr);
    m_InjectLibInfo.pfnGetLastError = (PGETLASTERROR)GetProcAddress(GetModuleHandle
("Kernel32.dll"),GetLastErrorFuncStr);
	
	m_InjectLibInfo.pfnExitThread = (PEXITTHREAD)GetProcAddress(GetModuleHandle
("Kernel32.dll"),ExitThreadFuncStr);
    m_InjectLibInfo.pfnVirtualFree = (PVIRTUALFREE)GetProcAddress(GetModuleHandle
("Kernel32.dll"),VirtualFreeFuncStr);
	
	m_InjectLibInfo.pfnCloseHandle = (PCLOSEHANDLE)GetProcAddress(GetModuleHandle
("Kernel32.dll"),CloseHandleFuncStr);
    m_InjectLibInfo.pfnCreateFile = (PCREATEFILE)GetProcAddress(GetModuleHandle
("Kernel32.dll"),CreateFileFuncStr);
	m_InjectLibInfo.pfnWriteFile = (PWRITEFILE)GetProcAddress(GetModuleHandle
("Kernel32.dll"),WriteFileFuncStr);
    
	m_InjectLibInfo.szDllName[0]=0;
	m_InjectLibInfo.szFuncName[0]=0;
	m_InjectLibInfo.bFree =true;
	m_InjectLibInfo.hModule =NULL;
}
//设定注射的目标exe
DWORD cInjectDll::Inject(LPCTSTR szRemoteProcessName)
{
	if(szRemoteProcessName)
		 m_dwProcessId=GetPIDFromName(szRemoteProcessName);
	return m_dwProcessId;
}
//卸载指定目标进程中的某个dll
DWORD cInjectDll::DeattachDLL(DWORD pid,HMODULE hmdl)
{
	m_dwProcessId=pid;
	m_InjectLibInfo.hModule=hmdl;
	m_InjectLibInfo.bFree=true;
	return _run(" "," ",FALSE,NULL,0);
}
//修改本进程的权限
bool  cInjectDll::EnablePrivilege(LPCTSTR lpszPrivilegeName,bool bEnable)
{
    HANDLE hToken;
    TOKEN_PRIVILEGES tp;
    LUID luid;

    if(!::OpenProcessToken(GetCurrentProcess(),TOKEN_ADJUST_PRIVILEGES |
        TOKEN_QUERY | TOKEN_READ,&hToken))
        return false;
    if(!::LookupPrivilegeValue(NULL, lpszPrivilegeName, &luid))
        return false;

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;
    tp.Privileges[0].Attributes = (bEnable) ? SE_PRIVILEGE_ENABLED : 0;

    ::AdjustTokenPrivileges(hToken,FALSE,&tp,NULL,NULL,NULL);

    ::CloseHandle(hToken);

    return (GetLastError() == ERROR_SUCCESS);
}

//-----------------------------------------------------
// 获取目标函数的真正入口地址。
// 在debug版中，有些函数的地址实际上是相应的跳转表地址
// 我们用这个跳转表地址得到真正的入口地址
//-----------------------------------------------------
PVOID cInjectDll::GetFuncAddress(PVOID addr)
{
#ifdef _DEBUG
	//check if instruction is relative jump (E9)
	if (0xE9 != *((UCHAR*)addr))
		return addr;
	// calculate base of relative jump
	ULONG base = (ULONG)((UCHAR*)addr + 5);
	// calculate offset 
	ULONG *offset = (ULONG*)((UCHAR*)addr + 1);
	return (PVOID)(base + *offset);
#else
	// in release, don't have to mess with jumps
	return addr;
#endif
}

//获取指定的远程进程的ID，根据名称
DWORD cInjectDll::GetPIDFromName(LPCTSTR szRemoteProcessName)
{
	if(szRemoteProcessName==NULL || szRemoteProcessName[0]==0)
		return 0;
	DWORD dwRet=0;

	//yyc add 2003-03-21 support NT ******************
	HINSTANCE hDll=::LoadLibrary("KERNEL32.dll");
	typedef HANDLE (WINAPI *pfnCreateToolhelp32Snapshot_D)(DWORD,DWORD);
	typedef BOOL (WINAPI *pfnProcess32Next_D)(HANDLE,LPPROCESSENTRY32);
	pfnCreateToolhelp32Snapshot_D pfnCreateToolhelp32Snapshot=
		(pfnCreateToolhelp32Snapshot_D)::GetProcAddress(hDll,"CreateToolhelp32Snapshot");
	pfnProcess32Next_D pfnProcess32Next=
		(pfnProcess32Next_D)::GetProcAddress(hDll,"Process32Next");
	if(pfnCreateToolhelp32Snapshot==NULL) //只有2k支持CreateToolhelp32Snapshot等函数
	{
		dwRet=FindProcessID(szRemoteProcessName);
	} //yyc add 2003-03-21 end******************
	else 
	{
		HANDLE hSnapShot=(*pfnCreateToolhelp32Snapshot)(TH32CS_SNAPPROCESS,0);
		PROCESSENTRY32* processInfo=new PROCESSENTRY32;
		processInfo->dwSize=sizeof(PROCESSENTRY32);
		while((*pfnProcess32Next)(hSnapShot,processInfo)!=FALSE)
		{
				if(_stricmp(szRemoteProcessName,processInfo->szExeFile)==0)
				{
					dwRet=processInfo->th32ProcessID;
					break;
				}
		}
		CloseHandle(hSnapShot);
		delete processInfo;
	}//?if(pfnCreateToolhelp32Snapshot==NULL)

	//yyc add 2003-03-21 support NT ******************
	::FreeLibrary(hDll);
	//yyc add 2003-03-21 end *************************	
	return dwRet;
}
//远程执行过程，同步调用方式
DWORD WINAPI cInjectDll::remoteThreadProc(INJECTLIBINFO *pInfo)
{
	HINSTANCE hDll=NULL;
	pInfo->dwReturnValue = 0;
	if(pInfo->hModule!=NULL)
		hDll=pInfo->hModule;
	else
		hDll = (HINSTANCE)pInfo->pfnLoadLibrary(pInfo->szDllName);
	if(hDll!=NULL)
	{
		PCALLBACKFUNC pfn=(PCALLBACKFUNC)pInfo->pfnGetProcaddr(hDll,pInfo->szFuncName);
		if(pfn!=NULL)
			pInfo->dwReturnValue=(*pfn)(&pInfo->dwParam);
		else
			pInfo->dwReturnValue=(DWORD)(-2);
		if(pInfo->bFree)
		{ pInfo->pfnFreeLibrary(hDll);hDll=NULL;}
		pInfo->hModule =hDll;
	}
	else
		pInfo->dwReturnValue = pInfo->pfnGetLastError();
	return 0;
}
//异步执行远程线程
//异步方式执行时bFree设置无效，远程线程执行完后都会释放加载的DLL
//INJECTLIBINFO::hMoudle返回总是为NULL，设置hMoudle无效
//远程执行过程，异步调用方式
DWORD WINAPI cInjectDll::remoteThreadProc_syn(INJECTLIBINFO *pInfo)
{
	pInfo->dwReturnValue = 0;
	HINSTANCE hDll = (HINSTANCE)pInfo->pfnLoadLibrary(pInfo->szDllName);
	if(hDll!=NULL)
	{
		PCALLBACKFUNC pfn=(PCALLBACKFUNC)pInfo->pfnGetProcaddr(hDll,pInfo->szFuncName);
		if(pfn!=NULL)
			pInfo->dwReturnValue=(*pfn)(&pInfo->dwParam);
		else
			pInfo->dwReturnValue=(DWORD)(-2);
		pInfo->pfnFreeLibrary(hDll);
	}
	//----------------------------------------------------
	// 如果用户选择异步方式调用，则远程线程必须释放自己
	//----------------------------------------------------
	PEXITTHREAD fnExitThread = pInfo->pfnExitThread;
	PVIRTUALFREE fnVirtualFree = pInfo->pfnVirtualFree;
	//----------------------------------------------------
	// 释放自己后调用ExitThread结束线程。
	// 下面的汇编代码相当于：
	// VirtualFree( pInfo, 0, MEM_RELEASE );
	// ExitThread( 0 );
	//----------------------------------------------------
	__asm {
		push 0;				// parameter of ExitThread
		push 0;				// return address of ExitThread
		push MEM_RELEASE;	// parameter of VirtualFree
		push 0;				// parameter of VirtualFree
		push pInfo;			// parameter of VirtualFree
		push fnExitThread;	// return address of VirtualFree
		push fnVirtualFree;
		ret;				// call VirtualFree
		}

	return 0;
}

//BOOL ifSyn --- 是否异步执行远程线程
//如果szDllName==NULL,则szFunctionName为PREMOTEFUNC函数指针
//如果是异步执行方式，则不等待远程线程执行结束，不会得到远程线程的返回值
//返回值：0--远程函数执行成功，否则发生错误，返回错误号
DWORD cInjectDll::_run(LPCTSTR szDllName,LPCTSTR szFunctionName,BOOL ifSyn,PVOID param,DWORD dwParamSize)
{
	if(m_dwProcessId==0 || szFunctionName==NULL) return (DWORD)(-1);
	DWORD dwRet=0;

    //提升本进程权限然后打开目的进程
    //当前用户必须具有调试权限
    cInjectDll::EnablePrivilege(SE_DEBUG_NAME,true);
    HANDLE hRemoteProcess = ::OpenProcess(PROCESS_ALL_ACCESS,false,m_dwProcessId);
	if(hRemoteProcess == NULL)
    {
        //RW_LOG_DEBUG("[INJECT] Failed to Open Process. Err =%d\n",GetLastError());
        return (DWORD)(-1);
    }
	//else RW_LOG_DEBUG("[INJECT] success to Open Process.\n");
	if(szDllName){
		lstrcpyn(m_InjectLibInfo.szDllName,szDllName,strlen(szDllName)+1);
		lstrcpyn(m_InjectLibInfo.szFuncName,szFunctionName,strlen(szFunctionName)+1);
	}
	else
	{
		m_InjectLibInfo.szDllName[0]=0;
		m_InjectLibInfo.szFuncName[0]=0;
	}
	
	DWORD cbParamSize=0;
	if(param!=NULL)
		cbParamSize=(dwParamSize==0)?(strlen((const char *)param)+1):dwParamSize;

	DWORD cbDataBuffer=sizeof(INJECTLIBINFO)+cbParamSize;
	DWORD cbSize=cbDataBuffer+MAXINJECTCODESIZE;
	LPBYTE p, c;//p其实地址 c代码开始地址

	//-----------------------------------------------------------
	// 在目标进程分配一段内存，供我们写入启动代码和必要的参数
	//-----------------------------------------------------------
	if((p = (LPBYTE)::VirtualAllocEx(hRemoteProcess,NULL,cbSize,MEM_COMMIT,PAGE_EXECUTE_READWRITE))!=NULL)
	{
		c = p+cbDataBuffer;
		//写入参数数据
		if(::WriteProcessMemory(hRemoteProcess,p,(LPVOID)&m_InjectLibInfo,sizeof(INJECTLIBINFO)/*cbDataBuffer*/,0)!=0)
		{
			if(cbParamSize>0)
			{
				//写入传递给调用函数的参数数据
				LPBYTE paramOffset=p+((PBYTE)&m_InjectLibInfo.dwParam-(PBYTE)&m_InjectLibInfo);
				::WriteProcessMemory(hRemoteProcess,paramOffset,param,cbParamSize,0);
			}
			//写入代码
			LPVOID remoteThreadAddr;
			if(szDllName)
			{
				if(!ifSyn)
					remoteThreadAddr=cInjectDll::GetFuncAddress(remoteThreadProc);
				else
					remoteThreadAddr=cInjectDll::GetFuncAddress(remoteThreadProc_syn);
			}
			else 
				remoteThreadAddr=cInjectDll::GetFuncAddress((PVOID)szFunctionName);
			
			if(::WriteProcessMemory(hRemoteProcess,c,remoteThreadAddr,cbSize-cbDataBuffer,0)!=0)
			{
				HANDLE hRemoteThread;
				if((hRemoteThread = ::CreateRemoteThread(hRemoteProcess,0,0,(PCALLBACKFUNC)c,(INJECTLIBINFO*)p,0,0))!=NULL)
				{
					//RW_LOG_DEBUG("[INJECT] success to CreateRemoteThread.\n");
					if(!ifSyn) //异步执行远程线程则什么也不作,dwRet=0
					{//同步执行远程线程
						//等待远程线程结束
						::WaitForSingleObject(hRemoteThread,INFINITE);
						//RW_LOG_DEBUG("[INJECT] RemoteThread ended!.\n");
						//读函数返回值
						DWORD dwReaded,dwReadSize=((PBYTE)&m_InjectLibInfo.hModule-(PBYTE)&m_InjectLibInfo);
						LPBYTE pOfset=p+dwReadSize; dwReadSize=sizeof(INJECTLIBINFO)-dwReadSize;
						dwRet=::ReadProcessMemory(hRemoteProcess,pOfset,(LPVOID)&m_InjectLibInfo.hModule,dwReadSize,&dwReaded);
//						RW_LOG_DEBUG("[INJECT] dwRet=%d,dwReadSize=%d,dwReaded=%d.Err=%d\n",dwRet,dwReadSize,dwReaded,GetLastError());
						if(cbParamSize>0)
						{
							LPBYTE paramOffset=p+((PBYTE)&m_InjectLibInfo.dwParam-(PBYTE)&m_InjectLibInfo);
							dwRet=::ReadProcessMemory(hRemoteProcess,paramOffset,param,cbParamSize,&dwReaded);
//							RW_LOG_DEBUG("[INJECT] dwRet=%d,cbParamSize=%d,dwReaded=%d.Err=%d\n",dwRet,cbParamSize,dwReaded,GetLastError());
						} 
						dwRet=m_InjectLibInfo.dwReturnValue;
//						RW_LOG_DEBUG("5555555555555555 dwret=%d\r\n",dwRet);
					}//?if(ifSyn)
					::CloseHandle( hRemoteThread );
				}
				else
				{
					//RW_LOG_DEBUG("[INJECT] Failed to CreateRemoteThread.Err =%d\n",GetLastError());
					dwRet=(DWORD)(-6);
				}
			}//?if(::WriteProcessMemory(hRemoteProcess,c,cInjectDll...
			else
			{
				//RW_LOG_DEBUG("[INJECT] Failed to Write Code to Remote Process.Err =%d\n",GetLastError());
				dwRet=(DWORD)(-5);
			}
		}//?if(::WriteProcessMemory(hRemoteProcess,p,(LPVOID)...
		else
		{
			//RW_LOG_DEBUG("[INJECT] Failed to Write Param to Remote Process.Err =%d\n",GetLastError());
			dwRet=(DWORD)(-4);
		}
		//释放分配的空间
		if(!ifSyn || dwRet!=0) //非异步执行方式或着异步执行发生错误
		::VirtualFreeEx( hRemoteProcess, p, 0, MEM_RELEASE );
	}//?if((p = (PBYTE)::VirtualAllocEx(hRemoteProcess
	else
	{
        //RW_LOG_DEBUG("[INJECT] Failed to Allocate Memory at Remote Process for Param.Err=%d\n",GetLastError()); 
		dwRet=(DWORD)(-3);
	}
	
	if( hRemoteProcess != NULL )
		::CloseHandle(hRemoteProcess);
	//恢复权限
    EnablePrivilege(SE_DEBUG_NAME,false);
	return dwRet;
}

//only for FindProcessID 声明
// Undocumented typedef's
typedef struct _QUERY_SYSTEM_INFORMATION
{
	DWORD GrantedAccess;
	DWORD PID;
	WORD HandleType;
	WORD HandleId;
	DWORD Handle;
} QUERY_SYSTEM_INFORMATION, *PQUERY_SYSTEM_INFORMATION;
typedef struct _PROCESS_INFO_HEADER
{
	DWORD Count;
	DWORD Unk04;
	DWORD Unk08;
} PROCESS_INFO_HEADER, *PPROCESS_INFO_HEADER;
typedef struct _PROCESS_INFO
{
	DWORD LoadAddress;
	DWORD Size;
	DWORD Unk08;
	DWORD Enumerator;
	DWORD Unk10;
	char Name [0x108];
} PROCESS_INFO, *PPROCESS_INFO;
typedef DWORD (__stdcall *PFNNTQUERYSYSTEMINFORMATION)  (DWORD, PVOID, DWORD, PDWORD);
typedef PVOID (__stdcall *PFNRTLCREATEQUERYDEBUGBUFFER) (DWORD, DWORD);
typedef DWORD (__stdcall *PFNRTLQUERYPROCESSDEBUGINFORMATION) (DWORD, DWORD, PVOID);
typedef void (__stdcall *PFNRTLDESTROYQUERYDEBUGBUFFER) (PVOID);
// Note that the following code eliminates the need
// for PSAPI.DLL as part of the executable.
DWORD cInjectDll::FindProcessID (LPCTSTR szRemoteProcessName)
{
	#define INITIAL_ALLOCATION 0x100
	//函数指针定义
	PFNNTQUERYSYSTEMINFORMATION pfnNtQuerySystemInformation;
	PFNRTLCREATEQUERYDEBUGBUFFER pfnRtlCreateQueryDebugBuffer;
	PFNRTLQUERYPROCESSDEBUGINFORMATION pfnRtlQueryProcessDebugInformation;
	PFNRTLDESTROYQUERYDEBUGBUFFER pfnRtlDestroyQueryDebugBuffer;
	//获取函数指针
	HINSTANCE hNtDll = ::LoadLibrary("NTDLL.DLL");
	if(hNtDll==NULL) return 0;
	pfnNtQuerySystemInformation =
		(PFNNTQUERYSYSTEMINFORMATION) GetProcAddress 
			(hNtDll, 
			"NtQuerySystemInformation");
	pfnRtlCreateQueryDebugBuffer =
		(PFNRTLCREATEQUERYDEBUGBUFFER) GetProcAddress 
			(hNtDll, 
			"RtlCreateQueryDebugBuffer");
	pfnRtlQueryProcessDebugInformation =
		(PFNRTLQUERYPROCESSDEBUGINFORMATION) GetProcAddress 
			(hNtDll, 
			"RtlQueryProcessDebugInformation");
	pfnRtlDestroyQueryDebugBuffer =
		(PFNRTLDESTROYQUERYDEBUGBUFFER) GetProcAddress 
			(hNtDll, 
			"RtlDestroyQueryDebugBuffer");
	//-----------------------------------------------------
	DWORD i,NumHandles,rc = 0;
	DWORD SizeNeeded = 0;
	PQUERY_SYSTEM_INFORMATION QuerySystemInformationP;
	PVOID InfoP = HeapAlloc (GetProcessHeap (),HEAP_ZERO_MEMORY,INITIAL_ALLOCATION);


	// Find how much memory is required.
	pfnNtQuerySystemInformation (0x10, InfoP, INITIAL_ALLOCATION, &SizeNeeded);
	HeapFree (GetProcessHeap (),0,InfoP);
	// Now, allocate the proper amount of memory.
	InfoP = HeapAlloc (GetProcessHeap (),HEAP_ZERO_MEMORY,SizeNeeded);
	DWORD SizeWritten = SizeNeeded;
	if(pfnNtQuerySystemInformation (0x10, InfoP, SizeNeeded, &SizeWritten))
		goto EXIT1;
	if((NumHandles = SizeWritten / sizeof (QUERY_SYSTEM_INFORMATION))==0) 
		goto EXIT1;

	QuerySystemInformationP =(PQUERY_SYSTEM_INFORMATION) InfoP;
	for (i = 1; i <= NumHandles; i++)
	{
		// "5" is the value of a kernel object type process.
		if (QuerySystemInformationP->HandleType == 5)
		{
			PVOID DebugBufferP =pfnRtlCreateQueryDebugBuffer (0, 0);
			if (pfnRtlQueryProcessDebugInformation (QuerySystemInformationP->PID,1,DebugBufferP) == 0)
			{
				PPROCESS_INFO_HEADER ProcessInfoHeaderP =(PPROCESS_INFO_HEADER) ((DWORD) DebugBufferP + 0x60);
				DWORD Count =ProcessInfoHeaderP->Count;
				PPROCESS_INFO ProcessInfoP =(PPROCESS_INFO) ((DWORD) ProcessInfoHeaderP + sizeof (PROCESS_INFO_HEADER));
				//if (strstr (_strupr (ProcessInfoP->Name), "WINLOGON") != 0)
				if (strstr (_strlwr (ProcessInfoP->Name), szRemoteProcessName) != 0)//yyc modify 2003-03-21
				{
					rc=QuerySystemInformationP->PID;
					if (DebugBufferP) pfnRtlDestroyQueryDebugBuffer(DebugBufferP);
					break;
					/*
					//不知道有什么用???
					DWORD i;
					DWORD dw = (DWORD) ProcessInfoP;
					for (i = 0; i < Count; i++)
					{
						dw += sizeof (PROCESS_INFO);
						ProcessInfoP = (PPROCESS_INFO) dw;
						if (strstr (_strupr (ProcessInfoP->Name), "NWGINA") != 0)
							goto EXIT1;//return (0);
						if (strstr (_strupr (ProcessInfoP->Name), "MSGINA") == 0)
							rc = QuerySystemInformationP->PID;
					}
					if (DebugBufferP) pfnRtlDestroyQueryDebugBuffer(DebugBufferP);
					goto EXIT1;
					*/
				}//?if(strstr (
			}
			if(DebugBufferP) pfnRtlDestroyQueryDebugBuffer(DebugBufferP);
		}//?if (pfnRtlQueryProcessDebug...
		DWORD dw = (DWORD) QuerySystemInformationP;
		dw += sizeof (QUERY_SYSTEM_INFORMATION);
		QuerySystemInformationP = (PQUERY_SYSTEM_INFORMATION) dw;
	}//?for
		
EXIT1:
	HeapFree (GetProcessHeap (),0,InfoP);
	//--------------------------------------------------
	if(hNtDll!=NULL) FreeLibrary(hNtDll);
	return (rc);
} // FindWinLogon

//---------------------------------------------------------
//-------------------2005-01-25 监视自身是否异常退出 begin-------------
typedef DWORD (WINAPI *PWAITFORSINGLEOBJECT)(HANDLE,DWORD);
typedef BOOL (WINAPI *PCREATEPROCESS)(LPCTSTR,LPTSTR,LPSECURITY_ATTRIBUTES ,LPSECURITY_ATTRIBUTES ,BOOL,
									  DWORD,LPVOID,LPCTSTR,LPSTARTUPINFO,LPPROCESS_INFORMATION);
LPCSTR WaitForSingleObjectFuncStr = "WaitForSingleObject";
#ifdef UNICODE
LPCSTR CreateProcessFuncStr = "CreateProcessW";
#else
LPCSTR CreateProcessFuncStr = "CreateProcessA";
#endif

typedef struct _spyParam
{
	PWAITFORSINGLEOBJECT pfnWaitForSingleObject;
	PCREATEPROCESS pfnCreateProcess;
	HANDLE hEvent;
	HANDLE hProcess;//要监视的进程句柄
	DWORD dwCreationFlags;//启动标志
	char pname[MAX_PATH];//要监视的进程路径名称，包括启动参数
}SPYPARAM;

DWORD WINAPI _spySelf(INJECTLIBINFO *pInfo)
{
	SPYPARAM * p=(SPYPARAM *)&pInfo->dwParam;
	if(p->hProcess)
	{
		STARTUPINFO si;
		PROCESS_INFORMATION pi;
		memset((void *)&si,0,sizeof(si));
		si.cb = sizeof(si);
		memset((void *)&pi,0,sizeof(pi));
		p->pfnWaitForSingleObject(p->hProcess,INFINITE);
		//判断是否为正常退出
		if(p->pfnWaitForSingleObject(p->hEvent,100)!=WAIT_OBJECT_0)
		{//异常退出
			if(p->pfnCreateProcess(NULL,p->pname,NULL,NULL,FALSE,p->dwCreationFlags,NULL,NULL,&si,&pi))
			{
				pInfo->pfnCloseHandle( pi.hProcess );
				pInfo->pfnCloseHandle( pi.hThread );
			}
		}//否则正常退出
	}//?if(hProcess)
	pInfo->pfnCloseHandle(p->hEvent);
	pInfo->pfnCloseHandle(p->hProcess);
	//----------------------------------------------------
	// 如果用户选择异步方式调用，则远程线程必须释放自己
	//----------------------------------------------------
	PEXITTHREAD fnExitThread = pInfo->pfnExitThread;
	PVIRTUALFREE fnVirtualFree = pInfo->pfnVirtualFree;
	//----------------------------------------------------
	// 释放自己后调用ExitThread结束线程。
	// 下面的汇编代码相当于：
	// VirtualFree( pInfo, 0, MEM_RELEASE );
	// ExitThread( 0 );
	//----------------------------------------------------
	__asm {
		push 0;				// parameter of ExitThread
		push 0;				// return address of ExitThread
		push MEM_RELEASE;	// parameter of VirtualFree
		push 0;				// parameter of VirtualFree
		push pInfo;			// parameter of VirtualFree
		push fnExitThread;	// return address of VirtualFree
		push fnVirtualFree;
		ret;				// call VirtualFree
		}

	return 0;
}

//监视进程自身
#include <stdio.h>
DWORD cInjectDll::spySelf(HANDLE hEvent,DWORD dwCreationFlags,const char *commandline)
{
	cInjectDll::EnablePrivilege(SE_DEBUG_NAME,true);
	
	HANDLE hTargetProcess=(m_dwProcessId!=0)?
		OpenProcess(PROCESS_DUP_HANDLE,false,m_dwProcessId):NULL;
	HANDLE hDupEvent=NULL,hDupProcess=NULL;
	HANDLE hProcess=GetCurrentProcess();
	if(hTargetProcess)
	{
		DuplicateHandle(hProcess, hEvent,hTargetProcess, 
			&hDupEvent , 0,FALSE,DUPLICATE_SAME_ACCESS);
		DuplicateHandle(hProcess, hProcess,hTargetProcess, 
			&hDupProcess , 0,FALSE,DUPLICATE_SAME_ACCESS);
	}
	
	cInjectDll::EnablePrivilege(SE_DEBUG_NAME,false);
	if(hDupEvent==NULL || hDupProcess==NULL) return 1;
	
	SPYPARAM p;
	DWORD dwret=::GetModuleFileName(NULL,p.pname,MAX_PATH);
	p.pname[dwret]=0;
	if(commandline){
		size_t l=strlen(commandline);
		if(l>0 && (dwret+l)<MAX_PATH ) strcpy(p.pname+dwret,commandline);
	}//?启动程序的命令行参数
	p.dwCreationFlags=dwCreationFlags;
	p.hProcess=hDupProcess;
	p.hEvent=hDupEvent;
	p.pfnWaitForSingleObject = (PWAITFORSINGLEOBJECT)GetProcAddress(GetModuleHandle
						("Kernel32.dll"),WaitForSingleObjectFuncStr);
	p.pfnCreateProcess = (PCREATEPROCESS)GetProcAddress(GetModuleHandle
						("Kernel32.dll"),CreateProcessFuncStr);	
	LPVOID paddr=GetFuncAddress(_spySelf);
	return _run(NULL,(LPCTSTR)paddr,true,(PVOID)&p,sizeof(SPYPARAM));
}


