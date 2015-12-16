/*******************************************************************
   *	Wutils.cpp
   *    DESCRIPTION:windows系统工具函数集
   *    获取系统的cpu占用率和内存使用情况
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-09-19
   *
   *******************************************************************/

#include "Wutils.h"
//**********************************************
//***********only for NT/win2000****************
#define SystemBasicInformation 0
#define SystemPerformanceInformation 2
#define SystemTimeInformation 3

#define Li2Double(x) ((double)((x).HighPart) * 4.294967296E9 + (double)((x).LowPart))

typedef struct
{
DWORD dwUnknown1;
ULONG uKeMaximumIncrement;
ULONG uPageSize;
ULONG uMmNumberOfPhysicalPages;
ULONG uMmLowestPhysicalPage;
ULONG uMmHighestPhysicalPage;
ULONG uAllocationGranularity;
PVOID pLowestUserAddress;
PVOID pMmHighestUserAddress;
ULONG uKeActiveProcessors;
BYTE bKeNumberProcessors;
BYTE bUnknown2;
WORD wUnknown3;
} SYSTEM_BASIC_INFORMATION;

typedef struct
{
LARGE_INTEGER liIdleTime;
DWORD dwSpare[76];
} SYSTEM_PERFORMANCE_INFORMATION;

typedef struct
{
LARGE_INTEGER liKeBootTime;
LARGE_INTEGER liKeSystemTime;
LARGE_INTEGER liExpTimeZoneBias;
ULONG uCurrentTimeZoneId;
DWORD dwReserved;
} SYSTEM_TIME_INFORMATION;

typedef LONG (WINAPI *PROCNTQSI)(UINT,PVOID,ULONG,PULONG);

PROCNTQSI NtQuerySystemInformation=NULL;
//***************only for NT/win2000 end*********************

//返回当时cpu的占用率(0-100)
int Wutils :: getCPUusage()
{
	int retv=0;
	OSVERSIONINFO vi;
	vi.dwOSVersionInfoSize=sizeof(vi);
	::GetVersionEx(&vi); 
	if(vi.dwPlatformId == VER_PLATFORM_WIN32_NT)
	{//NT/win2000
		SYSTEM_PERFORMANCE_INFORMATION SysPerfInfo;
		SYSTEM_TIME_INFORMATION SysTimeInfo;
		SYSTEM_BASIC_INFORMATION SysBaseInfo;
		double dbIdleTime;
		double dbSystemTime;
		static LARGE_INTEGER liOldIdleTime = {0,0};
		static LARGE_INTEGER liOldSystemTime = {0,0};

		NtQuerySystemInformation = (PROCNTQSI)GetProcAddress(
		GetModuleHandle("ntdll"),
		"NtQuerySystemInformation"
		);

		if (!NtQuerySystemInformation) return 0;
		// get number of processors in the system
		if(NtQuerySystemInformation(SystemBasicInformation,&SysBaseInfo,sizeof(SysBaseInfo),NULL)!=NO_ERROR)
			return 0;
		// get new system time
		if(NtQuerySystemInformation(SystemTimeInformation,&SysTimeInfo,sizeof(SysTimeInfo),0)!=NO_ERROR)
			return 0;
		// get new CPU's idle time
		if(NtQuerySystemInformation(SystemPerformanceInformation,&SysPerfInfo,sizeof(SysPerfInfo),NULL)!=NO_ERROR)
			return 0;

		// if it's a first call - skip it
		if (liOldIdleTime.QuadPart != 0)
		{
			// CurrentValue = NewValue - OldValue
			dbIdleTime = Li2Double(SysPerfInfo.liIdleTime) - Li2Double(liOldIdleTime);
			dbSystemTime = Li2Double(SysTimeInfo.liKeSystemTime) - Li2Double(liOldSystemTime);

			// CurrentCpuIdle = IdleTime / SystemTime
			dbIdleTime = dbIdleTime / dbSystemTime;

			// CurrentCpuUsage% = 100 - (CurrentCpuIdle * 100) / NumberOfProcessors
			dbIdleTime = 100.0 - dbIdleTime * 100.0 / (double)SysBaseInfo.bKeNumberProcessors + 0.5;

			retv=(int)dbIdleTime;
		}
		// store new CPU's idle and system time
		liOldIdleTime = SysPerfInfo.liIdleTime;
		liOldSystemTime = SysTimeInfo.liKeSystemTime;
	}
	else
	{//win9x
		HKEY hkey;
		DWORD dwDataSize;
		DWORD dwType;
		DWORD dwCpuUsage;

		// geting current counter's value
		if ( RegOpenKeyEx( HKEY_DYN_DATA,
		"PerfStats\\StatData",
		0,KEY_READ,
		&hkey ) != ERROR_SUCCESS)
		return 0;
		dwDataSize=sizeof(DWORD);
		RegQueryValueEx( hkey,"KERNEL\\CPUUsage",NULL,&dwType,(LPBYTE)&dwCpuUsage,&dwDataSize );
		retv=(int)(dwCpuUsage>100?100:dwCpuUsage);
		RegCloseKey(hkey);
	}
	return retv;
}

//********************************************
//返回当时mem的使用率(0-100)
int Wutils :: getMEMusage()
{
	int retv=0;
	MEMORYSTATUS memoryStatus;

	memset (&memoryStatus, sizeof (MEMORYSTATUS), 0);
	memoryStatus.dwLength = sizeof (MEMORYSTATUS);
	GlobalMemoryStatus (&memoryStatus);
	retv=int((memoryStatus.dwTotalPhys-memoryStatus.dwAvailPhys)/1024*100/(memoryStatus.dwTotalPageFile/1024));
	return retv;
}




