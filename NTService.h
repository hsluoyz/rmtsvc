
#ifndef NTService_h
#define NTService_h

class CNTService 
{
private:	// forbidden functions
	CNTService( const CNTService & );
	CNTService & operator=( const CNTService & );
public:
	CNTService(LPCTSTR ServiceName, LPCTSTR DisplayName = 0);
	virtual ~CNTService();
	BOOL	RegisterService(int argc, char ** argv);
	BOOL	InstallService();
	BOOL	RemoveService();
	BOOL	StartupService();
	BOOL	EndService();
	BOOL	SetServiceConfig();
	BOOL	GetServiceConfig(QUERY_SERVICE_CONFIG *psc);
	LPCTSTR GetServiceName() { return m_lpServiceName; }

protected:
	virtual void	Run(DWORD argc, LPTSTR * argv) = 0;
	virtual void	Stop() = 0;
	virtual void	Stop_Request(){}
	virtual void	Pause(){}
	virtual void	Continue(){}
	virtual void	Shutdown(){}

	BOOL	StartDispatcher();
	BOOL	DebugService(int argc, char **argv);
	LPTSTR	GetLastErrorText(LPTSTR Buf, DWORD Size);
	void	AddToMessageLog(LPTSTR	Message,
							WORD	EventType = EVENTLOG_ERROR_TYPE,
							DWORD	dwEventID = DWORD(-1) );
	BOOL	ReportStatus(DWORD CurState,DWORD WaitHint=3000,DWORD ErrExit = 0);

private:
	static void WINAPI	ServiceCtrl(DWORD CtrlCode);
	static void WINAPI	ServiceMain(DWORD argc, LPTSTR * argv);
	static BOOL WINAPI	ControlHandler(DWORD CtrlType);

	BOOL OpenNTService(SC_HANDLE &schSCManager,SC_HANDLE &schService);
	void CloseNTService(SC_HANDLE &schSCManager,SC_HANDLE &schService);

protected:	// data members
	LPCTSTR					m_lpServiceName;
	LPCTSTR					m_lpDisplayName;
	LPCTSTR					m_lpServiceDesc;//·þÎñÃèÊö
	DWORD					m_dwCheckPoint;
	BOOL					m_bDebug;			// TRUE if -d was passed to the program
	SERVICE_STATUS			m_ssStatus;			// current status of the service
	SERVICE_STATUS_HANDLE	m_sshStatusHandle;
	DWORD					m_dwControlsAccepted;	// bit-field of what control requests the
													// service will accept
													// (dflt: SERVICE_ACCEPT_STOP)
	BOOL					m_bWinNT;			// TRUE, if this is running on WinNT FALSE on Win95

	// parameters to the "CreateService()" function:
	DWORD			m_dwDesiredAccess;		// default: SERVICE_ALL_ACCESS
	DWORD			m_dwServiceType;		// default: SERVICE_WIN32_OWN_PROCESS
	DWORD			m_dwStartType;			// default: SERVICE_AUTO_START
	LPCTSTR			m_pszStartName;			// default: NULL
	LPCTSTR			m_pszPassword;			// default: NULL
};
// Retrieve the one and only CNTService object:
CNTService * AfxGetService();

#endif	// NTService_h

