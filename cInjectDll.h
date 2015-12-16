   /*******************************************************************
   *	cInjectDll.h
   *    DESCRIPTION:远程DLL注射，执行指定的函数
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2003-02-25
   *
   *******************************************************************/

  
#ifndef __CINJECTDLL_H__
#define __CINJECTDLL_H__

typedef HINSTANCE (WINAPI *PLOADLIBRARY)(LPCTSTR );
typedef BOOL (WINAPI *PFREELIBRARY)(HINSTANCE);
typedef HMODULE (WINAPI* PGETMODULEHANDLE)(LPCTSTR );
typedef PVOID (WINAPI* PGETPROCADDRESS)(HINSTANCE,LPCSTR);
typedef DWORD (WINAPI* PGETLASTERROR)(VOID);
typedef VOID (WINAPI* PEXITTHREAD)(DWORD);
typedef BOOL (WINAPI* PVIRTUALFREE)(LPVOID,SIZE_T,DWORD);
typedef BOOL (WINAPI *PCLOSEHANDLE)(HANDLE);
typedef HANDLE (WINAPI *PCREATEFILE)(LPCTSTR,DWORD,DWORD,LPSECURITY_ATTRIBUTES,DWORD,DWORD,HANDLE);
typedef BOOL (WINAPI *PWRITEFILE)(HANDLE,LPCVOID,DWORD,LPDWORD,LPOVERLAPPED);

#define MAXINJECTCODESIZE 1024 //注射代码最大字节数
//远程执行某dll中的函数类型声明
typedef DWORD (WINAPI *PCALLBACKFUNC)(PVOID);

typedef struct _INJECTLIBINFO
{
	PLOADLIBRARY pfnLoadLibrary;
	PFREELIBRARY pfnFreeLibrary;
	PGETPROCADDRESS pfnGetProcaddr;
	PGETLASTERROR pfnGetLastError;
	PEXITTHREAD pfnExitThread;
	PVIRTUALFREE pfnVirtualFree;
	PCLOSEHANDLE pfnCloseHandle;
	PCREATEFILE pfnCreateFile;
	PWRITEFILE pfnWriteFile;

	TCHAR szDllName[256];
	TCHAR szFuncName[256];
	bool bFree;//同步模式下，远程线程执行完毕后是否卸载远程线程加载的dll
			   //异步方式执行时bFree设置无效，远程线程执行完后都会释放加载的DLL
	HINSTANCE hModule;//[in|out] - 如果指定了hModule，则szDllName无效，不调用LoadLibrary加载dll
					  //否则加载szDllName指定dll到目标进程，同时同步执行模式下会返回加载DLL的模块句柄
	DWORD dwReturnValue;
	DWORD dwParam;//传递给调用进程的参数,即用户参数
} INJECTLIBINFO;
//远程线程执行函数类型声明
typedef DWORD (WINAPI *PREMOTEFUNC)(INJECTLIBINFO *pInfo);

class cInjectDll
{
private:
	INJECTLIBINFO m_InjectLibInfo;
	DWORD m_dwProcessId;//远程进程ID

	static DWORD WINAPI remoteThreadProc(INJECTLIBINFO *pInfo);
	//远程执行过程，异步调用方式
	static DWORD WINAPI remoteThreadProc_syn(INJECTLIBINFO *pInfo);

	//返回值：0--远程函数执行成功，否则发生错误，返回错误号
	//如果szDllName==NULL,则szFunctionName为PREMOTEFUNC函数指针
	//BOOL ifSyn --- 是否异步执行远程线程
	DWORD _run(LPCTSTR szDllName,LPCTSTR szFunctionName,BOOL ifSyn,PVOID param=NULL,DWORD dwParamSize=0);
public:
	//修改本进程的权限
	static bool EnablePrivilege(LPCTSTR lpszPrivilegeName,bool bEnable);
	//-----------------------------------------------------
	// 获取目标函数的真正入口地址。
	// 在debug版中，有些函数的地址实际上是相应的跳转表地址
	// 我们用这个跳转表地址得到真正的入口地址
	//-----------------------------------------------------
	static PVOID GetFuncAddress(PVOID addr);
	//获取指定的远程进程的ID，根据名称
	static DWORD GetPIDFromName(LPCTSTR szRemoteProcessName);
	
private:
	static DWORD FindProcessID (LPCTSTR szRemoteProcessName);
public:
	cInjectDll(LPCTSTR szRemoteProcessName);
	~cInjectDll(){}
	DWORD Inject(LPCTSTR szRemoteProcessName); //设定注射的目标exe
	//同步执行远程线程
	//可设置m_InjectLibInfo.bFree决定远程线程执行完后是否释放加载的DLL，默认卸载
	//可读m_InjectLibInfo.hModule返回远程线程加载dll的模块句柄
	//返回值：0--远程函数执行成功，否则发生错误，返回错误号
	DWORD run(LPCTSTR szDllName,LPCTSTR szFunctionName,PVOID param=NULL,DWORD dwParamSize=0)
	{
		if(szDllName==NULL||szFunctionName==NULL) return (DWORD)(-1);
		return _run(szDllName,szFunctionName,FALSE,param,dwParamSize);
	}
	//异步执行远程线程
	//异步方式执行时bFree设置无效，远程线程执行完后都会释放加载的DLL
	//返回值：0--远程函数执行成功，否则发生错误，返回错误号
	DWORD run_syn(LPCTSTR szDllName,LPCTSTR szFunctionName,PVOID param=NULL,DWORD dwParamSize=0)
	{
		if(szDllName==NULL||szFunctionName==NULL) return (DWORD)(-1);
		return _run(szDllName,szFunctionName,TRUE,param,dwParamSize);
	}
	//远程执行本exe的某个函数
	//函数的定义原型为PREMOTEFUNC类型，函数中的任何系统API要显示调用
	DWORD Call(DWORD pid,PREMOTEFUNC pfunc,PVOID param,DWORD paramLen)
	{
		if(pid!=0) m_dwProcessId=pid;
		return _run(NULL,(LPCTSTR)pfunc,FALSE,param,paramLen);
	}	
	//卸载指定目标进程中的某个dll
	DWORD DeattachDLL(DWORD pid,HMODULE hmdl);

//-------------------2005-01-25 监视自身是否异常退出 begin-------------
	//监视进程自身,成功返回0，否则发生错误
	DWORD spySelf(HANDLE hEvent,DWORD dwCreationFlags,const char *commandline);
//-------------------2005-01-25 监视自身是否异常退出  end -------------
};


#endif

