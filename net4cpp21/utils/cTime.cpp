/*******************************************************************
   *	cTime.h
   *    DESCRIPTION:和MFC中CTime类似的日期时间类
   *
   *    AUTHOR:yyc
   *	http://hi.baidu.com/yycblog/home
   *
   *    HISTORY: 改自 CGoodTime类
   *
   *    DATE:2005-08-22
   *	net4cpp 2.0
   *******************************************************************/

#include "../include/sysconfig.h"  
#include "cTime.h"
#include <cstdio>

//using namespace std;
using namespace net4cpp21;


cTime::cTime(int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec,
	int nDST)
{
	struct tm atm;

#ifdef _DEBUG
   memset( &atm, 0xFF, sizeof(atm) );

   switch ( nMonth )
   {
      case 1:  // January
      case 3:  // March
      case 5:  // May
      case 7:  // July
      case 8:  // August
      case 10: // October
      case 12: // December
//         ASSERT( nDay <= 31 );
         break;

      case 4:  // April
      case 6:  // June
      case 9:  // September
      case 11: // November
//         ASSERT( nDay <= 30 );
         break;

      case 2:
//         ASSERT( (nDay <= 28) || (IsLeapYear(nYear) && (nDay == 29)) );
         break;

//      default: // Bad month
//         ASSERT(FALSE);
   }
#endif

	atm.tm_sec = nSec;
	atm.tm_min = nMin;
	atm.tm_hour = nHour;
//	ASSERT(nDay >= 1 && nDay <= 31);
	atm.tm_mday = nDay;
//	ASSERT(nMonth >= 1 && nMonth <= 12);
	atm.tm_mon = nMonth - 1;        // tm_mon is 0 based
//	ASSERT(nYear >= 1900);
	atm.tm_year = nYear - 1900;     // tm_year is 1900 based
   // Note: nDST < 0 we should adjust
 	atm.tm_isdst = nDST;
	atm.tm_isdst = nDST;
   // Set tm_yday and tm_wday
   SetDays( &atm );
	m_time = atm;
}

void cTime::SetDays( tm *ptm )
{
   int   nDaysToMonth[12] = {
                            0,
                            31,
                            59, // Adjust for leap years later
                            90,
                            120,
                            151,
                            181,
                            212,
                            243,
                            273,
                            304,
                            334
                            };

   // The month should have been OK when we were called
//   ASSERT( (ptm->tm_mon >= 0) && (ptm->tm_mon <= 11) );

   // Set the number of days since Jan 1
   ptm->tm_yday = nDaysToMonth[ptm->tm_mon] + ptm->tm_mday - 1;

   // Adjust for leap years
   if ( ( IsLeapYear( ptm->tm_year+1900 ) ) &&
        ( ptm->tm_mon > 1 ) )
   {
      ptm->tm_yday++;
   }

   // Calculate the number of days since AD 1
   long  nDayNum;
   int   nYear = ptm->tm_year + 1900;

   // Approximate the number of days since 1/1/1
   nDayNum = (nYear - 1) * 365;

   // Add days for the leap years
   nDayNum += (nYear - 1) / 4;

   // Subtract years which are divisible by 100
   nDayNum -= (nYear - 1) / 100;

   // But replace years which are divisible by 400
   nDayNum += (nYear - 1) / 400;

   // This class is 1900-based, but 1582 is algorithmically significant.
//   ASSERT( nYear > 1582 );

   // But before 1582 all centuries were leap years. 
   // In 1582, Pope Gregory XIII mandated that October 4 would
   // be followed by October 15.  This accounts for those facts.
   nDayNum += 2;

   // Now add the days to this point this year
   nDayNum += ptm->tm_yday;

   // Finally, calculate the day of the week based on what we now know.
   ptm->tm_wday = (((nDayNum % 7) + 6) % 7);
}

cTime::cTime(unsigned short wDosDate, unsigned short wDosTime, int nDST)
{
	struct tm atm;
	atm.tm_sec = (wDosTime & ~0xFFE0) << 1;
	atm.tm_min = (wDosTime & ~0xF800) >> 5;
	atm.tm_hour = wDosTime >> 11;

	atm.tm_mday = wDosDate & ~0xFFE0;
	atm.tm_mon = ((wDosDate & ~0xFE00) >> 5) - 1;
	atm.tm_year = (wDosDate >> 9) + 80;
   // Note: nDST < 0 we should adjust
 	atm.tm_isdst = nDST;
   // Set tm_yday and tm_wday
   SetDays( &atm );
	m_time = atm;
}

cTime::cTime(const SYSTEMTIME& sysTime, int nDST)
{
//	ASSERT(sysTime.wYear >= 1900);
	cTime timeT(
		(int)sysTime.wYear, (int)sysTime.wMonth, (int)sysTime.wDay,
		(int)sysTime.wHour, (int)sysTime.wMinute, (int)sysTime.wSecond,
		nDST);
	*this = timeT;
}

cTime::cTime(const FILETIME& fileTime, int nDST)
{
	// first convert file time (UTC time) to local time
	FILETIME localTime;
	if (!FileTimeToLocalFileTime(&fileTime, &localTime))
	{
		memset( &m_time, 0, sizeof(m_time) );
		return;
	}

	// then convert that time to system time
	SYSTEMTIME sysTime;
	if (!FileTimeToSystemTime(&localTime, &sysTime))
	{
		memset( &m_time, 0, sizeof(m_time) );
		return;
	}

	// then convert the system time to a time_t (C-runtime local time)
	cTime timeT(sysTime, nDST);
	*this = timeT;
}

cTime PASCAL cTime::GetCurrentTime()
// return the current system time
{
   SYSTEMTIME              curTime;

   // Find the current system time
   GetLocalTime( &curTime );

	return cTime(curTime, _daylight);
}


struct tm* cTime::CvtGmtTm()
{
	time_t t=mktime(&m_time);
	t+=_timezone; //进行时区调整
	struct tm * ltime=localtime(&t);
	if(ltime) m_time=*ltime;
	return &m_time;
}
struct tm* cTime::CvtLocTm()
{
	time_t t=mktime(&m_time);
	t-=_timezone; //进行时区调整
	struct tm * ltime=localtime(&t);
	if(ltime) m_time=*ltime;
	return &m_time;
}

bool cTime::IsLeapYear( int nYear )
{
   // This will work for years before 1900, but it won't
   //   work for tm_years which are 1900-based so that
   //   2000 is represented as 100. 
//   ASSERT( nYear > 1900 );

   if ( (nYear % 4) != 0 )
   {
      // Years not divisible by four are not leap years
      return false;
   }
   else
   {
      // Years divisible by 4 are leap years
      // Unless they are also divisible by 100
      // and not divisible by 400.
      // Pope Gregory XIII didn't like programmers.

      if ( (nYear % 100) != 0 )
      {
         return true;
      }
      else
      {
         if ( (nYear % 400) != 0 )
         {
            return false;
         }
         else
         {
            return true;
         }
      }
   }
}

size_t cTime::Format(char *strDest, size_t maxsize,const char * pFormat) const
{
	return ::strftime(strDest,maxsize,pFormat,&m_time);
}


size_t cTime::FormatGmt(char *strDest, size_t maxsize,const char * pFormat) const
{
	cTime gmtTime(*this); gmtTime.CvtGmtTm();
	return gmtTime.Format(strDest,maxsize,pFormat);
}


/////////////////////////////////////////////////////////////////////////////

bool cTime::operator==(cTime time) const
{
   if ( ( m_time.tm_sec   == time.m_time.tm_sec   ) && 
        ( m_time.tm_min   == time.m_time.tm_min   ) && 
        ( m_time.tm_hour  == time.m_time.tm_hour  ) && 
        ( m_time.tm_mday  == time.m_time.tm_mday  ) && 
        ( m_time.tm_mon   == time.m_time.tm_mon   ) && 
        ( m_time.tm_year  == time.m_time.tm_year  ) && 
        ( m_time.tm_wday  == time.m_time.tm_wday  ) && 
        ( m_time.tm_yday  == time.m_time.tm_yday  ) && 
        ( m_time.tm_isdst == time.m_time.tm_isdst ) )
   {
      return true;
   }
   else
   {
      return false;
   }
}

bool cTime::operator!=(cTime time) const
{ 
   return ((*this == time) ? false : true); 
}

bool cTime::operator<(cTime time) const
{ 
   if ( m_time.tm_year < time.m_time.tm_year  )
   {
      return true;
   }
   else
   {
      double   dThisSeconds = (m_time.tm_yday * (24 * 60 * 60)) 
                            + (m_time.tm_hour * (60 * 60))
                            + (m_time.tm_min  * 60)
                            + m_time.tm_sec;
      double   dCmpSeconds  = (time.m_time.tm_yday * (24 * 60 * 60)) 
                            + (time.m_time.tm_hour * (60 * 60))
                            + (time.m_time.tm_min  * 60)
                            + time.m_time.tm_sec;
      return ( dThisSeconds < dCmpSeconds );
   }
}

bool cTime::operator>(cTime time) const
{ 
   if ( m_time.tm_year > time.m_time.tm_year  )
   {
      return true;
   }
   else
   {
      double   dThisSeconds = (m_time.tm_yday * (24 * 60 * 60)) 
                            + (m_time.tm_hour * (60 * 60))
                            + (m_time.tm_min  * 60)
                            + m_time.tm_sec;
      double   dCmpSeconds  = (time.m_time.tm_yday * (24 * 60 * 60)) 
                            + (time.m_time.tm_hour * (60 * 60))
                            + (time.m_time.tm_min  * 60)
                            + time.m_time.tm_sec;
      return ( dThisSeconds > dCmpSeconds );
   }
}

bool cTime::operator<=(cTime time) const
{ 
   return ((*this < time) || (*this == time));
}

bool cTime::operator>=(cTime time) const
{ 
   return ((*this > time) || (*this == time));
}

//----------------------------------------------------------------------
//yyc add 2006-02-10
bool ParseWeekDay(char* pszToken, int& nWeekDay)
{
  bool bSuccess = true;
  if (strcmp(pszToken, "Sun") == 0 || strcmp(pszToken, "Sunday") == 0)
    nWeekDay = 0;
  else if (strcmp(pszToken, "Mon") == 0 || strcmp(pszToken, "Monday") == 0)
    nWeekDay = 1;
  else if (strcmp(pszToken, "Tue") == 0 || strcmp(pszToken, "Tuesday") == 0)
    nWeekDay = 2;
  else if (strcmp(pszToken, "Wed") == 0 || strcmp(pszToken, "Wednesday") == 0)
    nWeekDay = 3;
  else if (strcmp(pszToken, "Thu") == 0 || strcmp(pszToken, "Thursday") == 0)
    nWeekDay = 4;
  else if (strcmp(pszToken, "Fri") == 0 || strcmp(pszToken, "Friday") == 0)
    nWeekDay = 5;
  else if (strcmp(pszToken, "Sat") == 0 || strcmp(pszToken, "Saturday") == 0)
    nWeekDay = 6;
  else
    bSuccess = false;
  return bSuccess;
}

bool ParseMonth(char* pszToken, int& nMonth)
{
  bool bSuccess = true;
  if (strcmp(pszToken, "Jan") == 0)
    nMonth = 1;
  else if (strcmp(pszToken, "Feb") == 0)
    nMonth = 2;
  else if (strcmp(pszToken, "Mar") == 0)
    nMonth = 3;
  else if (strcmp(pszToken, "Apr") == 0)
    nMonth = 4;
  else if (strcmp(pszToken, "May") == 0)
    nMonth = 5;
  else if (strcmp(pszToken, "Jun") == 0)
    nMonth = 6;
  else if (strcmp(pszToken, "Jul") == 0)
    nMonth = 7;
  else if (strcmp(pszToken, "Aug") == 0)
    nMonth = 8;
  else if (strcmp(pszToken, "Sep") == 0)
    nMonth = 9;
  else if (strcmp(pszToken, "Oct") == 0)
    nMonth = 10;
  else if (strcmp(pszToken, "Nov") == 0)
    nMonth = 11;
  else if (strcmp(pszToken, "Dec") == 0)
    nMonth = 12;
  else
    bSuccess = false;
  return bSuccess;
}

//解析如期时间格式，支持下列日期时间格式 例如RFC 1123, RFC 850 等
//[Week,] Mmm DD YY/YYYY HH:mm:ss		例如: Sat, Sep 11 2006 12:58:20
//[Week,] DD Mmm/MM YY/YYYY HH:mm:ss	例如: Sat, 14 Mar 98 23:13:43 +0800
//YYYY MM DD HH:mm:ss					例如: 2009 09 18 12:01:01
bool cTime::parseDate(const char *strDate)
{
	int lDateLen=(strDate)?strlen(strDate):0;
	if( lDateLen<8 ) return false; //最少8位例如 YY-MM-DD
	char *szDatePtr,*szDate=new char[lDateLen+1];
	if(szDate==NULL) return false;
	strcpy(szDate,strDate); szDatePtr=szDate; //用于释放
	SYSTEMTIME time; ::memset((void *)&time,0,sizeof(SYSTEMTIME));

	//我们总是假定week是,分割的
	char* pszToken=strchr(szDate,',');
	if(pszToken){
		*pszToken='\0'; int nWeekDay=0;
		if(ParseWeekDay(szDate, nWeekDay)) time.wDayOfWeek = (WORD) nWeekDay;
		szDate=pszToken+1;
		while(*szDate==' ') szDate++; //删除前导空格
	}//?跳过week域
	char seps[] = " :-"; bool bSuccess=false;
	pszToken = strtok(szDate, seps);
	while( pszToken ){
	//处理年月日，只有可能为下面三种形式 1、YYYY MM DD 2、DD Mmm YY/YYYY 3、Mmm DD YY/YYYY
	int iNum=atoi(pszToken);
	if(iNum<=0){ //examples: Mmm DD YY/YYYY
		bSuccess=ParseMonth(pszToken, iNum);
		if(!bSuccess) break;
		time.wMonth = (WORD) iNum; 
		bSuccess= ((pszToken = strtok(NULL, seps))!=NULL);
		if(!bSuccess) break;
		time.wDay=(WORD)atoi(pszToken);
		bSuccess= ((pszToken = strtok(NULL, seps))!=NULL);
		if(!bSuccess) break;
		time.wYear = (WORD)atoi(pszToken); //可能是YY或YYYY
		if(time.wYear<50) time.wYear+=2000;
		else if(time.wYear<100) time.wYear+=1900;
	}else if(iNum>=1900){ //examples: YYYY MM DD
		time.wYear = (WORD)iNum;
		bSuccess= ((pszToken = strtok(NULL, seps))!=NULL);
		if(!bSuccess) break;
		time.wMonth =(WORD)atoi(pszToken);
		bSuccess= ((pszToken = strtok(NULL, seps))!=NULL);
		if(!bSuccess) break;
		time.wDay = (WORD)atoi(pszToken);
	}else{ //examples: DD Mmm/MM YY/YYYY
		time.wDay=(WORD)iNum;
		bSuccess= ((pszToken = strtok(NULL, seps))!=NULL);
		if(!bSuccess) break;
		if(!ParseMonth(pszToken, iNum)) iNum=atoi(pszToken);
		if(iNum<1||iNum>12) { bSuccess=false; break; }
		time.wMonth =(WORD)iNum;
		bSuccess= ((pszToken = strtok(NULL, seps))!=NULL);
		if(!bSuccess) break;
		time.wYear = (WORD)atoi(pszToken); //可能是YY或YYYY
		if(time.wYear<50) time.wYear+=2000;
		else if(time.wYear<100) time.wYear+=1900;
	}
	//处理时分秒 HH:mm:ss ,时分秒可以没有，默认为 00:00:00
	pszToken = strtok(NULL, seps);
	if(pszToken==NULL) break;
	time.wHour = (WORD) atoi(pszToken);
	pszToken = strtok(NULL, seps);
	if(pszToken==NULL) break;
	time.wMinute = (WORD) atoi(pszToken);
	pszToken = strtok(NULL, seps);
	if(pszToken==NULL) break;
	time.wSecond = (WORD) atoi(pszToken);
	//处理时区信息  GMT UTC 或+0800或-0700等 ...
	break; //处理结束，退出
	}//?while( pszToken )
	
	if(bSuccess) { cTime timeT(time, -1); *this = timeT; }
	delete[] szDatePtr; return bSuccess;
}
