/*******************************************************************
   *	webAction_sview.cpp web请求处理 - 服务浏览
   *    DESCRIPTION:
   *
   *    AUTHOR:yyc
   *
   *    HISTORY:
   *
   *    DATE:
   *	
   *******************************************************************/
#include "rmtsvc.h"

//列出本机所有服务
//buffer - 返回的xml文档,格式:
//<?xml version="1.0" encoding="gb2312" ?>
//<xmlroot>
//<service>
//<id>序号</id>
//<sname>服务名</sname>
//<status>服务状态</status>
//<rtype>启动类型</rtype>
//<stype>服务类型</stype>
//<sdisp>显示名称</sdisp>
//<sdesc>服务描述</sdesc>
//<spath>服务模块路径</spath>
//</service>
//...
//</xmlroot>
DWORD serviceList(cBuffer &buffer);
bool webServer::httprsp_slist(socketTCP *psock,httpResponse &httprsp)
{
	cBuffer buffer(1024);
	serviceList(buffer);
	httprsp.NoCache();//CacheControl("No-cache");
	//设置MIME类型，默认为HTML
	httprsp.set_mimetype(MIMETYPE_XML);
	//设置响应内容长度
	httprsp.lContentLength(buffer.len()); 
	httprsp.send_rspH(psock,200,"OK");
	psock->Send(buffer.len(),buffer.str(),-1);
	return true;
}

bool webServer::sevent(const char *sname,const char *cmd)
{
	if(sname==NULL || sname[0]==0) return false;
	//打开SCM获得SCM句柄
	SC_HANDLE schSCManager =	OpenSCManager(
			0,						// machine (NULL == local)
			0,						// database (NULL == default)
			SC_MANAGER_ALL_ACCESS	// access required
		);
	if( schSCManager==NULL ) return false;
	SC_HANDLE hService=OpenService(schSCManager,sname,SERVICE_ALL_ACCESS);
	if(hService==NULL){ ::CloseServiceHandle(schSCManager); return false; }
	
	SERVICE_STATUS	ssStatus; 
	if(strcmp(cmd,"run")==0) //启动指定的服务
	{
		if( ::StartService(hService, 0, 0) ) Sleep(1000);
	}
	else if(strcmp(cmd,"stop")==0) //停止指定的服务
	{
		if( ::ControlService(hService, SERVICE_CONTROL_STOP, &ssStatus) )
		{
			Sleep(1000); long lcount=5;
			while(--lcount>0 && QueryServiceStatus(hService, &ssStatus) ) 
			{
				if( ssStatus.dwCurrentState == SERVICE_STOP_PENDING )
					Sleep( 1000 );
				else break;
			}//?while
		}
	}
	else if(strcmp(cmd,"delete")==0) //删除卸载服务
	{
		if( ::ControlService(hService, SERVICE_CONTROL_STOP, &ssStatus) )
		{
			Sleep(1000); long lcount=5;
			while(--lcount>0 && QueryServiceStatus(hService, &ssStatus) ) 
			{
				if( ssStatus.dwCurrentState == SERVICE_STOP_PENDING )
					Sleep( 1000 );
				else break;
			}//?while
		}
		DeleteService(hService);
	}
	else if(strcmp(cmd,"forbid")==0) //禁用服务
	{
		ChangeServiceConfig(hService,SERVICE_NO_CHANGE,SERVICE_DISABLED,SERVICE_NO_CHANGE,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
	}
	else if(strcmp(cmd,"auto")==0) 
	{
		ChangeServiceConfig(hService,SERVICE_NO_CHANGE,SERVICE_AUTO_START,SERVICE_NO_CHANGE,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
	}
	else if(strcmp(cmd,"manual")==0)
	{
		ChangeServiceConfig(hService,SERVICE_NO_CHANGE,SERVICE_DEMAND_START,SERVICE_NO_CHANGE,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
	}
	::CloseServiceHandle(hService);
	::CloseServiceHandle(schSCManager);
	return true;
}

DWORD serviceList(cBuffer &buffer)
{
	//打开SCM获得SCM句柄
	SC_HANDLE schSCManager =	OpenSCManager(
			0,						// machine (NULL == local)
			0,						// database (NULL == default)
			SC_MANAGER_ALL_ACCESS	// access required
		);
	if( schSCManager==NULL ) return 0;
	ENUM_SERVICE_STATUS service,*lpservice;
	DWORD bytesNeeded,servicesReturned,resumeHandle=0;
	BOOL rc=::EnumServicesStatus(schSCManager,SERVICE_WIN32,SERVICE_STATE_ALL,&service,
		sizeof(service),&bytesNeeded,&servicesReturned,&resumeHandle);

	if( rc==FALSE && ::GetLastError()!=ERROR_MORE_DATA ){ ::CloseServiceHandle(schSCManager); return 0; }

	LPBYTE lpqsconfig_buffer=NULL;
	DWORD bytes=bytesNeeded+sizeof(ENUM_SERVICE_STATUS);
	if( (lpservice=(ENUM_SERVICE_STATUS *)::malloc(bytes))==NULL ){ ::CloseServiceHandle(schSCManager); return 0; }

	::EnumServicesStatus(schSCManager,SERVICE_WIN32,SERVICE_STATE_ALL,lpservice,
		bytes,&bytesNeeded,&servicesReturned,&resumeHandle);

	DWORD dwret=0;
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<?xml version=\"1.0\" encoding=\"gb2312\" ?><xmlroot>");
	while( servicesReturned-- >0 ) //lpservice->lpServiceName)
	{
		if(buffer.Space()<280) buffer.Resize(buffer.size()+280);
		buffer.len()+=sprintf(buffer.str()+buffer.len(),
			"<service><id>%d</id><sname>%s</sname>",++dwret,lpservice->lpServiceName);
		switch(lpservice->ServiceStatus.dwCurrentState)
		{
			case SERVICE_RUNNING:
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<status>已启动</status>");
				break;
			case SERVICE_STOPPED:
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<status>已停止</status>");
				break;
			case SERVICE_PAUSED:
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<status>已暂停</status>");
				break;
			case SERVICE_STOP_PENDING:
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<status>正停止</status>");
				break;
			case SERVICE_CONTINUE_PENDING:
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<status>已挂起</status>");
				break;
			case SERVICE_PAUSE_PENDING:
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<status>已挂起</status>");
				break;
			default:
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<status>---</status>");
				break;
		}//?switch
		SC_HANDLE hService=OpenService(schSCManager,lpservice->lpServiceName,SERVICE_ALL_ACCESS);
		if(hService==NULL){ buffer.len()+=sprintf(buffer.str()+buffer.len(),"</service>"); continue; }
		
		bytesNeeded=0;//进一步获取信息
		QueryServiceConfig( hService, NULL, 0, &bytesNeeded);
		DWORD lpqscBuf_Size=bytesNeeded;
		LPQUERY_SERVICE_CONFIG lpqscBuf=(LPQUERY_SERVICE_CONFIG)::malloc(lpqscBuf_Size);
		if(lpqscBuf && QueryServiceConfig( hService, lpqscBuf, lpqscBuf_Size,&bytesNeeded))
		{
			if(buffer.Space()<(bytesNeeded+100)) buffer.Resize(buffer.size()+(bytesNeeded+100));
			if( lpqscBuf->dwStartType==SERVICE_AUTO_START)
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<rtype>自动</rtype>");
			else if( lpqscBuf->dwStartType==SERVICE_DEMAND_START)
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<rtype>手动</rtype>");
			else if( lpqscBuf->dwStartType==SERVICE_DISABLED)
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<rtype>禁用</rtype>");
			else buffer.len()+=sprintf(buffer.str()+buffer.len(),"<rtype>---</rtype>");
			
			if( lpqscBuf->dwServiceType & SERVICE_WIN32_OWN_PROCESS)
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<stype>独立进程服务%s</stype>",
				(lpqscBuf->dwServiceType & SERVICE_INTERACTIVE_PROCESS)?",可交互":"");
			else if( lpqscBuf->dwServiceType & SERVICE_WIN32_SHARE_PROCESS)
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<stype>共享进程服务%s</stype>",
				(lpqscBuf->dwServiceType & SERVICE_INTERACTIVE_PROCESS)?",可交互":"");
			else if( lpqscBuf->dwServiceType & SERVICE_FILE_SYSTEM_DRIVER)
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<stype>文件系统驱动</stype>");
			else if( lpqscBuf->dwServiceType & SERVICE_KERNEL_DRIVER)
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<stype>系统内核驱动</stype>");
			else buffer.len()+=sprintf(buffer.str()+buffer.len(),"<stype>---</stype>");
			
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<sdisp>%s</sdisp>",lpqscBuf->lpDisplayName);
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<spath>%s</spath>",lpqscBuf->lpBinaryPathName);
			
			QueryServiceConfig2(hService,SERVICE_CONFIG_DESCRIPTION,NULL, 0, &bytesNeeded);
			if(bytesNeeded>lpqscBuf_Size)
			{
				::free(lpqscBuf);
				lpqscBuf=(LPQUERY_SERVICE_CONFIG)::malloc(bytesNeeded);
			} 
			lpqscBuf_Size=bytesNeeded;
			if(lpqscBuf)
			{
				QueryServiceConfig2(hService,SERVICE_CONFIG_DESCRIPTION,(LPBYTE)lpqscBuf, lpqscBuf_Size, &bytesNeeded);
				if(buffer.Space()<(lpqscBuf_Size+48)) buffer.Resize(buffer.size()+(lpqscBuf_Size+48));
				SERVICE_DESCRIPTION *p=(SERVICE_DESCRIPTION *)lpqscBuf;
				buffer.len()+=sprintf(buffer.str()+buffer.len(),"<sdesc><![CDATA[\r\n%s\r\n]]></sdesc>",p->lpDescription);
			}		
		}//?if(QueryServiceConfig
		if(lpqscBuf) ::free(lpqscBuf);
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"</service>");
		::CloseServiceHandle(hService); lpservice++;
	}//?while

	::CloseServiceHandle(schSCManager);
	if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"</xmlroot>");
	::free(lpservice); return dwret;
}
