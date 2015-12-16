/*******************************************************************
   *	webAction_fport.cpp web请求处理 - 枚举进程和端口的关联
   *    DESCRIPTION:
   *
   *    AUTHOR:yyc
   *
   *    原理通过调用Iphlpapi.dll中的未公开函数枚举关联
   *    Iphlpapi.dll公开的函数中有GetTcpTable/GetUdpTable,但无法关联进程
   *	
   *******************************************************************/

#include "rmtsvc.h"
#include "other\wutils.h"
#include <Iprtrmib.h>

BOOL portList(cBuffer &buffer);
//buffer - 返回的xml文档,格式:
//<?xml version="1.0" encoding="gb2312" ?>
//<xmlroot>
//<fport>
//<id>序号</id>
//<pid>进程ID</pid>
//<pname>进程名</pname>
//<ptype>类型</ptype>
//<laddr>本地地址</laddr>
//<raddr>远程地址</raddr>
//<status>状态</status>
//</fport>
//...
//</xmlroot>
bool webServer::httprsp_fport(socketTCP *psock,httpResponse &httprsp)
{
	cBuffer buffer(2048);
	portList(buffer);
	httprsp.NoCache();//CacheControl("No-cache");
	//设置MIME类型，默认为HTML
	httprsp.set_mimetype(MIMETYPE_XML);
	//设置响应内容长度
	httprsp.lContentLength(buffer.len()); 
	httprsp.send_rspH(psock,200,"OK");
	psock->Send(buffer.len(),buffer.str(),-1);
	return true;
}


//----------------------private function--------------------------------
typedef struct _MIB_TCPROW_EX
{
DWORD dwState; // MIB_TCP_STATE_*
DWORD dwLocalAddr;
DWORD dwLocalPort;
DWORD dwRemoteAddr;
DWORD dwRemotePort;
DWORD dwProcessId;
} MIB_TCPROW_EX, *PMIB_TCPROW_EX;

typedef struct _MIB_TCPTABLE_EX
{
DWORD dwNumEntries;
MIB_TCPROW_EX table[ANY_SIZE];
} MIB_TCPTABLE_EX, *PMIB_TCPTABLE_EX;

typedef struct _MIB_UDPROW_EX
{
DWORD dwLocalAddr;
DWORD dwLocalPort;
DWORD dwProcessId;
} MIB_UDPROW_EX, *PMIB_UDPROW_EX;

typedef struct _MIB_UDPTABLE_EX
{
DWORD dwNumEntries;
MIB_UDPROW_EX table[ANY_SIZE];
} MIB_UDPTABLE_EX, *PMIB_UDPTABLE_EX;


/*
DWORD
WINAPI
AllocateAndGetTcpExTableFromStack(
OUT PMIB_TCPTABLE_EX *pTcpTableEx,
IN BOOL bOrder,
IN HANDLE hAllocHeap,
IN DWORD dwAllocFlags,
IN DWORD dwProtocolVersion; // 2 - TCP, 23 - TCPv6 (size of *pTcpTableEx must be 56!)
);
*/
typedef DWORD (WINAPI *PROCALLOCATEANDGETTCPEXTABLEFROMSTACK)(PMIB_TCPTABLE_EX*,BOOL,HANDLE,DWORD,DWORD);

/*
DWORD
WINAPI
AllocateAndGetUdpExTableFromStack(
OUT PMIB_UDPTABLE_EX *pUdpTable,
IN BOOL bOrder,
IN HANDLE hAllocHeap,
IN DWORD dwAllocFlags,
IN DWORD dwProtocolVersion; // 2 - UDP, 23 - UDPv6 (size of *pUdpTable must be 28!)
);
*/
typedef DWORD (WINAPI *PROCALLOCATEANDGETUDPEXTABLEFROMSTACK)(PMIB_UDPTABLE_EX*,BOOL,HANDLE,DWORD,DWORD);

static char *TCP_STATE[13]={"","CLOSED","LISTEN","SYN_SENT","SYN_RCVD","ESTAB","FIN_WAIT1",
			"FIN_WAIT2","CLOSE_WAIT","CLOSING","LAST_ACK","TIME_WAIT","DELETE_TCB"};

BOOL portList(cBuffer &buffer)
{

	HMODULE hModule=NULL;
	PROCALLOCATEANDGETTCPEXTABLEFROMSTACK lpfnAllocateAndGetTcpExTableFromStack = NULL;
	PROCALLOCATEANDGETUDPEXTABLEFROMSTACK lpfnAllocateAndGetUdpExTableFromStack = NULL;

	hModule=::LoadLibrary("iphlpapi.dll");
	if(hModule==NULL) return FALSE; //加载dll失败
	//获取函数指针 // XP and later - 实际测试2k也能用
	lpfnAllocateAndGetTcpExTableFromStack = (PROCALLOCATEANDGETTCPEXTABLEFROMSTACK)GetProcAddress(hModule,"AllocateAndGetTcpExTableFromStack");
	lpfnAllocateAndGetUdpExTableFromStack = (PROCALLOCATEANDGETUDPEXTABLEFROMSTACK)GetProcAddress(hModule,"AllocateAndGetUdpExTableFromStack");
	if (lpfnAllocateAndGetTcpExTableFromStack == NULL || lpfnAllocateAndGetUdpExTableFromStack==NULL) return FALSE;
	
	Wutils::EnablePrivilege(SE_DEBUG_NAME,true);
	if(buffer.Space()<256) buffer.Resize(buffer.size()+256);
	 buffer.len()+=sprintf(buffer.str()+buffer.len(),"<?xml version=\"1.0\" encoding=\"gb2312\" ?><xmlroot>");

	DWORD dwLastError,dwSize,dwState,dwCount=0;
	PMIB_TCPTABLE_EX lpBuffer = NULL;
	PMIB_UDPTABLE_EX lpBuffer1 = NULL;
	//枚举所有TCP
	dwLastError = lpfnAllocateAndGetTcpExTableFromStack(&lpBuffer,TRUE,GetProcessHeap(),0,2);
	if (dwLastError == NO_ERROR)
	{
		char strRemoteAddr[24];
//		printf("Local IP\tLocal Port\tRemote Ip\tRemote Port\tPID\n\n");
		for (dwSize = 0; dwSize < lpBuffer->dwNumEntries; dwSize++)
		{
			if(lpBuffer->table[dwSize].dwProcessId==0) continue;
			if(buffer.Space()<512) buffer.Resize(buffer.size()+512);

			dwState=lpBuffer->table[dwSize].dwState;
			if(dwState>MIB_TCP_STATE_DELETE_TCB) dwState=0;
			sprintf(strRemoteAddr,"%s",socketBase::IP2A(lpBuffer->table[dwSize].dwRemoteAddr));
			const char *szProcessName=Wutils::GetNameFromPID(lpBuffer->table[dwSize].dwProcessId);
			buffer.len()+=sprintf(buffer.str()+buffer.len(),
					"<fport><id>%d</id>"
					"<ptype>TCP</ptype>"
					"<pid>%04d</pid>"
					"<pname>%s</pname>"
					"<laddr>%s:%d</laddr>"
					"<raddr>%s:%d</raddr>"
					"<status>%s</status>"
					"</fport>",++dwCount,
					lpBuffer->table[dwSize].dwProcessId,
					((szProcessName)?szProcessName:"OS kernel"),
					socketBase::IP2A(lpBuffer->table[dwSize].dwLocalAddr),
					ntohs(lpBuffer->table[dwSize].dwLocalPort),
					strRemoteAddr,
					ntohs(lpBuffer->table[dwSize].dwRemotePort),
					TCP_STATE[dwState] );
		}//?for
	}//?if (dwLastError == NO_ERROR)
	
	//枚举所有UDP
	dwLastError = lpfnAllocateAndGetUdpExTableFromStack(&lpBuffer1,TRUE,GetProcessHeap(),0,2);
	if (dwLastError == NO_ERROR)
	{
//		printf("Local IP\tLocal Port\tPID\n\n");
		for (dwSize = 0; dwSize < lpBuffer1->dwNumEntries; dwSize++)
		{
			if(lpBuffer1->table[dwSize].dwProcessId==0) continue; 
			if(buffer.Space()<512) buffer.Resize(buffer.size()+512);

			const char *szProcessName=Wutils::GetNameFromPID(lpBuffer1->table[dwSize].dwProcessId);
			buffer.len()+=sprintf(buffer.str()+buffer.len(),
					"<fport><id>%d</id>"
					"<ptype>UDP</ptype>"
					"<pid>%04d</pid>"
					"<pname>%s</pname>"
					"<laddr>%s:%d</laddr>"
					"<raddr>*.*</raddr>"
					"<status></status>"
					"</fport>",++dwCount,
					lpBuffer1->table[dwSize].dwProcessId,
					((szProcessName)?szProcessName:"OS kernel"),
					socketBase::IP2A(lpBuffer1->table[dwSize].dwLocalAddr),
					ntohs(lpBuffer1->table[dwSize].dwLocalPort) );
		}//?for
	}//?if (dwLastError == NO_ERROR)

	if (lpBuffer) HeapFree(GetProcessHeap(),0,lpBuffer);
	if (lpBuffer1) HeapFree(GetProcessHeap(),0,lpBuffer1);
	::FreeLibrary(hModule);

	if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"</xmlroot>");
	return TRUE;
}

BOOL portList(string &strret)
{

	HMODULE hModule=NULL;
	PROCALLOCATEANDGETTCPEXTABLEFROMSTACK lpfnAllocateAndGetTcpExTableFromStack = NULL;
	PROCALLOCATEANDGETUDPEXTABLEFROMSTACK lpfnAllocateAndGetUdpExTableFromStack = NULL;

	hModule=::LoadLibrary("iphlpapi.dll");
	if(hModule==NULL) return FALSE; //加载dll失败
	//获取函数指针 // XP and later - 实际测试2k也能用
	lpfnAllocateAndGetTcpExTableFromStack = (PROCALLOCATEANDGETTCPEXTABLEFROMSTACK)GetProcAddress(hModule,"AllocateAndGetTcpExTableFromStack");
	lpfnAllocateAndGetUdpExTableFromStack = (PROCALLOCATEANDGETUDPEXTABLEFROMSTACK)GetProcAddress(hModule,"AllocateAndGetUdpExTableFromStack");
	if (lpfnAllocateAndGetTcpExTableFromStack == NULL || lpfnAllocateAndGetUdpExTableFromStack==NULL) return FALSE;
	
	Wutils::EnablePrivilege(SE_DEBUG_NAME,true);
	char stmp[512]; long len=0;
	len=sprintf(stmp,"id\ttype\tLocal\tRemote\tpname\r\n");
	strret.append(stmp,len);
	DWORD dwLastError,dwSize,dwState,dwCount=0;
	PMIB_TCPTABLE_EX lpBuffer = NULL;
	PMIB_UDPTABLE_EX lpBuffer1 = NULL;
	//枚举所有TCP
	dwLastError = lpfnAllocateAndGetTcpExTableFromStack(&lpBuffer,TRUE,GetProcessHeap(),0,2);
	if (dwLastError == NO_ERROR)
	{
		char strRemoteAddr[24];
//		printf("Local IP\tLocal Port\tRemote Ip\tRemote Port\tPID\n\n");
		for (dwSize = 0; dwSize < lpBuffer->dwNumEntries; dwSize++)
		{
			if(lpBuffer->table[dwSize].dwProcessId==0) continue;

			dwState=lpBuffer->table[dwSize].dwState;
			if(dwState>MIB_TCP_STATE_DELETE_TCB) dwState=0;
			sprintf(strRemoteAddr,"%s",socketBase::IP2A(lpBuffer->table[dwSize].dwRemoteAddr));
			const char *szProcessName=Wutils::GetNameFromPID(lpBuffer->table[dwSize].dwProcessId);
			len=sprintf(stmp,"%d\tTCP\t%s:%d\t%s:%d\t%s\t%s\r\n",++dwCount,
						socketBase::IP2A(lpBuffer->table[dwSize].dwLocalAddr),
					ntohs(lpBuffer->table[dwSize].dwLocalPort),
					strRemoteAddr,
					ntohs(lpBuffer->table[dwSize].dwRemotePort),
					TCP_STATE[dwState],
						((szProcessName)?szProcessName:"OS kernel") );
			strret.append(stmp,len);
		}//?for
	}//?if (dwLastError == NO_ERROR)
	
	//枚举所有UDP
	dwLastError = lpfnAllocateAndGetUdpExTableFromStack(&lpBuffer1,TRUE,GetProcessHeap(),0,2);
	if (dwLastError == NO_ERROR)
	{
//		printf("Local IP\tLocal Port\tPID\n\n");
		for (dwSize = 0; dwSize < lpBuffer1->dwNumEntries; dwSize++)
		{
			if(lpBuffer1->table[dwSize].dwProcessId==0) continue; 

			const char *szProcessName=Wutils::GetNameFromPID(lpBuffer1->table[dwSize].dwProcessId);
			len=sprintf(stmp,"%d\tUDP\t%s:%d\t*.*\t \t%s\r\n",++dwCount,
						socketBase::IP2A(lpBuffer1->table[dwSize].dwLocalAddr),
					ntohs(lpBuffer1->table[dwSize].dwLocalPort),
					((szProcessName)?szProcessName:"OS kernel") );
			strret.append(stmp,len);
		}//?for
	}//?if (dwLastError == NO_ERROR)

	if (lpBuffer) HeapFree(GetProcessHeap(),0,lpBuffer);
	if (lpBuffer1) HeapFree(GetProcessHeap(),0,lpBuffer1);
	::FreeLibrary(hModule);
	return TRUE;
}
