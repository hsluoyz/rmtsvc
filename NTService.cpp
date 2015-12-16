
#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <crtdbg.h> //_ASSERTE

#include "NTService.h"

static CNTService * gpTheService = 0;			// the one and only instance
CNTService * AfxGetService() { return gpTheService; }

#define SERVICE_CONTROL_STOP_REQUEST 0x00000080 //发送停止服务请求

/////////////////////////////////////////////////////////////////////////////
// class CNTService -- construction/destruction
CNTService :: CNTService( LPCTSTR lpServiceName, LPCTSTR lpDisplayName )
	: m_lpServiceName(lpServiceName)
	, m_lpDisplayName(lpDisplayName ? lpDisplayName : lpServiceName)
	, m_lpServiceDesc(0)
	, m_dwCheckPoint(0)
	, m_bDebug(FALSE)
	, m_sshStatusHandle(0)
	, m_dwControlsAccepted(SERVICE_ACCEPT_STOP|SERVICE_ACCEPT_SHUTDOWN)
	// parameters to the "CreateService()" function:
	, m_dwDesiredAccess(SERVICE_ALL_ACCESS)
	, m_dwServiceType(SERVICE_WIN32_OWN_PROCESS)
	, m_dwStartType(SERVICE_AUTO_START)
	, m_pszStartName(0)
	, m_pszPassword(0)
{
	_ASSERTE( gpTheService==0);
	gpTheService = this;
	// SERVICE_STATUS members that rarely change
	m_ssStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
	m_ssStatus.dwServiceSpecificExitCode = 0;

	OSVERSIONINFO vi;
	vi.dwOSVersionInfoSize=sizeof(vi);  // init this.
	GetVersionEx(&vi);      //lint !e534
	m_bWinNT = (vi.dwPlatformId == VER_PLATFORM_WIN32_NT);
}


CNTService :: ~CNTService() {
	_ASSERTE( gpTheService==0);
	gpTheService = 0;
}

/////////////////////////////////////////////////////////////////////////////

#define NEXT_ARG ((((*Argv)[2])==TEXT('\0'))?(--Argc,*++Argv):(*Argv)+2)
BOOL CNTService :: RegisterService( int argc, char ** argv ) 
{	
    DWORD Argc; LPTSTR * Argv;	
#ifdef UNICODE
    Argv = CommandLineToArgvW(GetCommandLineW(), &Argc );
#else
    Argc = (DWORD) argc;
    Argv = argv;
#endif
	BOOL (CNTService::* fnc)() = &CNTService::StartDispatcher;

    while( ++Argv, --Argc ) {
		if( Argv[0][0] == TEXT('-') ) {
			switch( Argv[0][1] ) {
				case TEXT('i'):	// install the service -i [DisplayName] [ServiceDesc]
					fnc = &CNTService::InstallService; 
					if(Argc>1 && Argv[1][0]!='-') m_lpDisplayName=(--Argc,*++Argv);
					if(Argc>1 && Argv[1][0]!='-') m_lpServiceDesc=(--Argc,*++Argv);
					break;
				case TEXT('u'):	// uninstall the service
					fnc = &CNTService::RemoveService;
					break;
				case TEXT('s'):	// start the service
					fnc = &CNTService::StartupService;
					break;
				case TEXT('e'):	// end the service
					fnc = &CNTService::EndService;
					break;
				case TEXT('d'):	// debug the service
					#ifdef UNICODE
						::GlobalFree(HGLOBAL)Argv);
					#endif
					return DebugService(argc, argv);
			}//?switch( Argv[0][1] )
		}//?if( Argv[0][0] == TEXT('-')
	}//?while

#ifdef UNICODE
	::GlobalFree(HGLOBAL)Argv);
#endif
	
	if(fnc==&CNTService::StartDispatcher)
	{//如果以服务方式启动失败,则Debug方式启动
		if(CNTService::StartDispatcher()) return TRUE;
		return DebugService(argc, argv);
	}//?尝试以服务方式启动
	return (this->*fnc)();
}
//重新设置服务类型和启动类型
BOOL CNTService :: SetServiceConfig()
{
	if( !m_bWinNT ) return FALSE; //不支持win95
	SC_HANDLE schSCManager=NULL,schService=NULL;
	if(!OpenNTService(schSCManager,schService)) return FALSE;
	
	BOOL bRet=::ChangeServiceConfig(schService, 
				m_dwServiceType,m_dwStartType,SERVICE_ERROR_NORMAL,
				NULL,NULL,NULL,NULL,NULL,NULL,NULL);
	
	CloseNTService(schSCManager,schService);
	return bRet;
}
//获取服务状态
BOOL CNTService :: GetServiceConfig(QUERY_SERVICE_CONFIG *psc)
{
	if( !m_bWinNT ) return FALSE;
	SC_HANDLE schSCManager =OpenSCManager(0,0,SC_MANAGER_ALL_ACCESS);
	if( schSCManager==NULL ) return FALSE;
	SC_HANDLE schService =::OpenService(schSCManager,m_lpServiceName,SERVICE_ALL_ACCESS);
	//bRet==TRUE ,服务已安装
	BOOL bRet=(schService || GetLastError()!=ERROR_SERVICE_DOES_NOT_EXIST);
	if(psc){
		if(schService){
			DWORD bytesNeeded=0;//进一步获取信息
			QueryServiceConfig( schService, NULL, 0, &bytesNeeded);
			DWORD lpqscBuf_Size=bytesNeeded;
			LPQUERY_SERVICE_CONFIG lpqscBuf=(LPQUERY_SERVICE_CONFIG)::malloc(lpqscBuf_Size);
			if(lpqscBuf && QueryServiceConfig( schService, lpqscBuf, lpqscBuf_Size,&bytesNeeded))
				bRet=TRUE,::CopyMemory((VOID *)psc,lpqscBuf,sizeof(QUERY_SERVICE_CONFIG));
			else bRet=FALSE; //获取数据失败
			if(lpqscBuf) ::free(lpqscBuf);
		}else bRet=FALSE; //获取数据失败
	}//?if(psc)
	::CloseServiceHandle(schService);
	::CloseServiceHandle(schSCManager);
	return bRet;
}

//安装服务
BOOL CNTService :: InstallService() 
{
	if( !m_bWinNT ) return FALSE; //不支持win95
	SC_HANDLE schSCManager =OpenSCManager(0,0,SC_MANAGER_ALL_ACCESS);
	if( !schSCManager ) return FALSE;

	TCHAR szPath[1024]; GetModuleFileName(0,szPath,1023);
	SC_HANDLE schService =	CreateService(
						schSCManager,m_lpServiceName,m_lpDisplayName,
						m_dwDesiredAccess,m_dwServiceType,m_dwStartType,
						SERVICE_ERROR_NORMAL,szPath,
						NULL,NULL,NULL,m_pszStartName,m_pszPassword);
	BOOL bRet = TRUE;
	if( schService ){
		_tprintf(TEXT("%s installed.\n"), m_lpDisplayName );
		SERVICE_DESCRIPTION sd; sd.lpDescription=(LPTSTR)m_lpServiceDesc;
		if(m_lpServiceDesc)	
			ChangeServiceConfig2(schService,SERVICE_CONFIG_DESCRIPTION,(LPVOID)&sd);
	} else if(GetLastError()==ERROR_SERVICE_EXISTS)
		_tprintf(TEXT("%s has been installed.\n"), m_lpDisplayName );
	else {
		TCHAR szErr[256]; bRet = FALSE;
		_tprintf(TEXT("CreateService failed - %s\n"), GetLastErrorText(szErr, 256));
	}
	CloseNTService(schSCManager,schService);
	return bRet;
}

BOOL CNTService :: RemoveService() 
{
	if( !m_bWinNT ) return TRUE; //不支持win95
	SC_HANDLE schSCManager=NULL,schService=NULL;
	if(!OpenNTService(schSCManager,schService)) return FALSE;
	//先尝试停止服务，以免服务正在运行
	BOOL bRet=::ControlService(schService, SERVICE_CONTROL_STOP, &m_ssStatus);
	if(bRet)
	{
		_tprintf(TEXT("Stopping %s."), m_lpDisplayName);
		while( ::QueryServiceStatus(schService, &m_ssStatus) ) {
			if( m_ssStatus.dwCurrentState == SERVICE_STOP_PENDING ) {
				::Sleep( 1000 ); _tprintf(TEXT("."));	
			} else break;
		}//?while
		if( m_ssStatus.dwCurrentState == SERVICE_STOPPED )
			_tprintf(TEXT("\n%s stopped.\n"), m_lpDisplayName);
        else
            _tprintf(TEXT("\n%s failed to stop.\n"), m_lpDisplayName);
	}else{
		DWORD dwErr=GetLastError();
		if(dwErr==ERROR_INVALID_SERVICE_CONTROL) //需要密码控制，不允许删除服务
			_tprintf(TEXT("%s forbid stopping\n"), m_lpDisplayName);
		else bRet=TRUE; //允许删除服务
	}
	if(bRet)
	{	// now remove the service
		if( !(bRet=DeleteService(schService)) ){
			TCHAR szErr[256];
			_tprintf(TEXT("DeleteService failed - %s\n"), GetLastErrorText(szErr,256));
		}else _tprintf(TEXT("%s removed.\n"), m_lpDisplayName);
	}else _tprintf(TEXT("DeleteService failed,%s not stopped\n"), m_lpDisplayName);

	CloseNTService(schSCManager,schService);
	return bRet;
}
// Start the service. Does the same as if the
// SCM launches the program. Note that this method
// will create a new instance of the program.
BOOL CNTService :: StartupService() 
{
	SC_HANDLE schSCManager=NULL,schService=NULL;
	if(!OpenNTService(schSCManager,schService)) return FALSE;
	
	// try to start the service
	_tprintf(TEXT("Starting up %s."), m_lpDisplayName);
	BOOL bRet=::StartService(schService, 0, 0);
	if(bRet){
		bRet=FALSE; Sleep(1000);
		while( ::QueryServiceStatus(schService, &m_ssStatus) ) 
		{
			if( m_ssStatus.dwCurrentState == SERVICE_START_PENDING ) {
				_tprintf(TEXT("."));
				Sleep( 1000 );
			} else break;
		}//?while( ::QueryServiceStatus
		if( m_ssStatus.dwCurrentState == SERVICE_RUNNING )
			bRet = TRUE, _tprintf(TEXT("\n%s started.\n"), m_lpDisplayName);
        else
            _tprintf(TEXT("\n%s failed to start.\n"), m_lpDisplayName);
	}else{// StartService failed
		TCHAR szErr[256];
		_tprintf(TEXT("\n%s failed to start: %s\n"), m_lpDisplayName, GetLastErrorText(szErr,256));
	}
	CloseNTService(schSCManager,schService);
	return bRet;
}
// EndService() stops a running service (if the service
// is running as a service! Does not end a service
// running as a console program (see DebugService()
// below))
BOOL CNTService :: EndService() 
{
	SC_HANDLE schSCManager=NULL,schService=NULL;
	if(!OpenNTService(schSCManager,schService)) return FALSE;
	// try to stop the service ERROR_SERVICE_CANNOT_ACCEPT_CTRL
	_tprintf(TEXT("Stopping %s."), m_lpDisplayName);
	if(::ControlService(schService, SERVICE_CONTROL_STOP_REQUEST, &m_ssStatus))
	{
		int iCount=0;//发送服务停止请求
		while( ::QueryServiceStatus(schService, &m_ssStatus) )
		{
			if(m_ssStatus.dwControlsAccepted & SERVICE_ACCEPT_STOP) break;
			::Sleep(200); if(++iCount>5) break;
		}
	}//?if(::ControlService
	BOOL bRet=::ControlService(schService, SERVICE_CONTROL_STOP, &m_ssStatus);
	if(bRet)
	{
		bRet=FALSE; ::Sleep(1000);
		while( ::QueryServiceStatus(schService, &m_ssStatus) ) {
			if( m_ssStatus.dwCurrentState == SERVICE_STOP_PENDING ) {
				_tprintf(TEXT("."));
				::Sleep( 1000 );
			} else break;
		}//?while
		if( m_ssStatus.dwCurrentState == SERVICE_STOPPED )
			bRet = TRUE, _tprintf(TEXT("\n%s stopped.\n"), m_lpDisplayName);
        else
            _tprintf(TEXT("\n%s failed to stop.\n"), m_lpDisplayName);
	}else{
		DWORD dwErr=GetLastError(); TCHAR szErr[256];
		if(dwErr==ERROR_INVALID_SERVICE_CONTROL)
			_tprintf(TEXT("\n%s forbid stopping\n"), m_lpDisplayName);
		else if(dwErr==ERROR_SERVICE_NOT_ACTIVE)
			_tprintf(TEXT("\n%s has been stopped\n"), m_lpDisplayName);
		else	
			_tprintf(TEXT("\n%s failed to stop(%d): %s\n"), m_lpDisplayName,dwErr, GetLastErrorText(szErr,256));
	}
	CloseNTService(schSCManager,schService);
	return bRet;
}


///////////////////////////////////////////////////////////////////////////////////////
/////////////////////////protected function/////////////////////////////////////////////
BOOL CNTService :: StartDispatcher() 
{
	if( GetStdHandle(STD_OUTPUT_HANDLE) )
		return FALSE; //如果StartService启动的程序肯定没有控制台
	if(!GetServiceConfig(NULL)) return FALSE; //未安装为服务
    // Default implementation creates a single threaded service.
	// Override this method and provide more table entries for
	// a multithreaded service (one entry for each thread).
	SERVICE_TABLE_ENTRY dispatchTable[] =
    {
        { LPTSTR(m_lpServiceName), (LPSERVICE_MAIN_FUNCTION)ServiceMain },
        { 0, 0 }
    };
	if(StartServiceCtrlDispatcher(dispatchTable)) return TRUE;
	TCHAR szBuf[256];
    AddToMessageLog(GetLastErrorText(szBuf,255));
	return FALSE;
}

BOOL CNTService :: DebugService(int argc, char ** argv) 
{
    DWORD dwArgc; LPTSTR *lpszArgv;
#ifdef UNICODE
    lpszArgv = CommandLineToArgvW(GetCommandLineW(), &(dwArgc) );
#else
    dwArgc   = (DWORD) argc;
    lpszArgv = argv;
#endif
	
	m_bDebug = TRUE;
	_tprintf(TEXT("Debugging %s - %s.\nPress Ctrl+c to end program.\n"), 
			m_lpServiceName,(m_lpServiceDesc)?m_lpServiceDesc:m_lpDisplayName);
	SetConsoleCtrlHandler(ControlHandler, TRUE);

    Run(dwArgc, lpszArgv);
	
#ifdef UNICODE
	::GlobalFree(HGLOBAL)lpszArgv);
#endif
	return TRUE;
}

//!! TCW MOD - added DWORD dwErrExit for error exit value. Defaults to zero
BOOL CNTService :: ReportStatus(DWORD dwCurrentState,DWORD dwWaitHint,DWORD dwErrExit ) 
{
	// when debugging we don't report to the SCM
	if(m_bDebug) return TRUE;
	if( dwCurrentState == SERVICE_START_PENDING)
        m_ssStatus.dwControlsAccepted = 0;
    else
        m_ssStatus.dwControlsAccepted = m_dwControlsAccepted;
	m_ssStatus.dwCurrentState = dwCurrentState;
    m_ssStatus.dwWin32ExitCode = NO_ERROR;
    m_ssStatus.dwWaitHint = dwWaitHint;
	//!! TCW MOD START - added code to support error exiting
	m_ssStatus.dwServiceSpecificExitCode = dwErrExit;
	if (dwErrExit!=0)
		m_ssStatus.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
	if( dwCurrentState == SERVICE_RUNNING || dwCurrentState == SERVICE_STOPPED )
         m_ssStatus.dwCheckPoint = 0;
    else m_ssStatus.dwCheckPoint = ++m_dwCheckPoint;
	// Report the status of the service to the service control manager.
	if( SetServiceStatus( m_sshStatusHandle, &m_ssStatus) ) return TRUE;
	AddToMessageLog(TEXT("SetServiceStatus() failed"));
	return FALSE;
}

void CNTService :: AddToMessageLog(LPTSTR lpszMsg, WORD wEventType, DWORD dwEventID) 
{
	HANDLE hEventSource = RegisterEventSource(0, m_lpServiceName);
	if( hEventSource == 0 ) return;
	LPCTSTR lpszMessage = lpszMsg;
	ReportEvent(
		hEventSource,	// handle of event source
		wEventType,		// event type
		0,				// event category
		dwEventID,		// event ID
		NULL,			// current user's SID
		1,				// strings in lpszStrings
		0,				// no bytes of raw data
		&lpszMessage,	// array of error strings
		0);
	::DeregisterEventSource(hEventSource);
}


LPTSTR CNTService :: GetLastErrorText( LPTSTR lpszBuf, DWORD dwSize ) 
{
    LPTSTR lpszTemp = 0;
    DWORD dwRet =	::FormatMessage(
					FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |FORMAT_MESSAGE_ARGUMENT_ARRAY,
					0,GetLastError(),
					LANG_NEUTRAL,
					(LPTSTR)&lpszTemp,
					0,0 );
    if( dwRet==0 || dwSize < (dwRet+2) )
        lpszBuf[0] = TEXT('\0');
    else _tcscpy(lpszBuf, lpszTemp);

    if( lpszTemp ) LocalFree(HLOCAL(lpszTemp));
    return lpszBuf;
}

///////////////////////////////////////////////////////////////////////////////////////
/////////////////////////private function/////////////////////////////////////////////
// class CNTService -- default handlers
void WINAPI CNTService :: ServiceMain(DWORD dwArgc, LPTSTR *lpszArgv) {
	_ASSERTE( gpTheService != 0 );

	// register our service control handler:
	gpTheService->m_sshStatusHandle =	RegisterServiceCtrlHandler(
											gpTheService->m_lpServiceName,
											CNTService::ServiceCtrl
										);

	if( gpTheService->m_sshStatusHandle )
		// report the status to the service control manager.
		if( gpTheService->ReportStatus(SERVICE_START_PENDING) ){
			gpTheService->Run( dwArgc, lpszArgv );}
		else gpTheService->ReportStatus(SERVICE_STOPPED);
}

void WINAPI CNTService :: ServiceCtrl(DWORD dwCtrlCode) {
	_ASSERTE( gpTheService != 0 );

	// Handle the requested control code.
	switch( dwCtrlCode ) {
		case SERVICE_CONTROL_STOP:
			// Stop the service.
			gpTheService->m_ssStatus.dwCurrentState = SERVICE_STOP_PENDING;
			gpTheService->Stop();
			break;

		case SERVICE_CONTROL_PAUSE:
			gpTheService->m_ssStatus.dwCurrentState = SERVICE_PAUSE_PENDING;
			gpTheService->Pause();
			break;

		case SERVICE_CONTROL_CONTINUE:
			gpTheService->m_ssStatus.dwCurrentState = SERVICE_CONTINUE_PENDING;
			gpTheService->Continue();
			break;

		case SERVICE_CONTROL_SHUTDOWN:
			gpTheService->Shutdown();
			break;

		case SERVICE_CONTROL_INTERROGATE:
			// Update the service status.
			gpTheService->ReportStatus(gpTheService->m_ssStatus.dwCurrentState);
			break;

		case SERVICE_CONTROL_STOP_REQUEST:
			gpTheService->Stop_Request();
			break;

		default:
			// invalid control code
			break;
	}
}

BOOL WINAPI CNTService :: ControlHandler(DWORD dwCtrlType) {
	_ASSERTE(gpTheService != 0);
	switch( dwCtrlType ) {
		case CTRL_BREAK_EVENT:  // use Ctrl+C or Ctrl+Break to simulate
		case CTRL_C_EVENT:      // SERVICE_CONTROL_STOP in debug mode
			_tprintf(TEXT("Stopping %s.\n"), gpTheService->m_lpDisplayName);
			gpTheService->Stop();
			return TRUE;
	}
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////
/////////////////////////private function/////////////////////////////////////////////
inline void CNTService :: CloseNTService(SC_HANDLE &schSCManager,SC_HANDLE &schService)
{
	::CloseServiceHandle(schService);
	::CloseServiceHandle(schSCManager);
}

BOOL CNTService :: OpenNTService(SC_HANDLE &schSCManager,SC_HANDLE &schService)
{
	schSCManager = ::OpenSCManager(
					0,0,// machine (NULL == local),database (NULL == default)
					SC_MANAGER_ALL_ACCESS);	// access required
	if( schSCManager )
	{
		schService =::OpenService(schSCManager,
					m_lpServiceName,SERVICE_ALL_ACCESS);
		if( schService ) return TRUE;
		TCHAR szErr[256];
		_tprintf(TEXT("OpenService failed - %s\n"), GetLastErrorText(szErr,256));
        ::CloseServiceHandle(schSCManager); schSCManager=0;
	}else{
		TCHAR szErr[256];
		_tprintf(TEXT("OpenSCManager failed - %s\n"), GetLastErrorText(szErr,256));
	}
	return FALSE;
}
