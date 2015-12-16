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
   *	net4cpp 2.1
   *******************************************************************/
  
#ifndef __YY_CTIMES_H__
#define __YY_CTIMES_H__

#include <ctime>
namespace net4cpp21
{
	class cTime
	{
	private:
		tm m_time;
	protected:
		void SetDays( tm *ptm );
	public:
		// Constructors
		static cTime PASCAL GetCurrentTime();

		cTime();
		cTime(const cTime& timeSrc);
		cTime(tm time);
		cTime(int nYear, int nMonth, int nDay, int nHour, int nMin, int nSec,
			int nDST = -1);
		cTime(unsigned short wDosDate, unsigned short wDosTime, int nDST = -1);
		cTime(const SYSTEMTIME& sysTime, int nDST = -1);
		cTime(const FILETIME& fileTime, int nDST = -1);
		const cTime& operator=(const cTime& timeSrc);
		const cTime& operator=(tm t);

		struct tm* CvtGmtTm(); //本地时间转换为GMT时间
		struct tm* CvtLocTm(); //GMT时间转换为本地时间
		tm GetTime() const;
		int GetYear() const;
		int GetMonth() const;       // month of year (1 = Jan)
		int GetDay() const;         // day of month
		int GetHour() const;
		int GetMinute() const;
		int GetSecond() const;
		int GetDayOfWeek() const;   // 1=Sun, 2=Mon, ..., 7=Sat
		//yyc add 2006-02-10
		time_t Gettime() { return mktime(&m_time); }

		static bool IsLeapYear( int nYear );

		bool operator==(cTime time) const;
		bool operator!=(cTime time) const;
		bool operator<(cTime time) const;
		bool operator>(cTime time) const;
		bool operator<=(cTime time) const;
		bool operator>=(cTime time) const;

		// formatting using "C" strftime
		size_t Format(char *strDest, size_t maxsize,const char * pFormat) const;
		size_t FormatGmt(char *strDest, size_t maxsize,const char * pFormat) const;
		bool parseDate(const char *strDate);
	};

//#ifdef _DEBUG
inline cTime::cTime()
	{ }
inline cTime::cTime(const cTime& timeSrc)
	{ m_time = timeSrc.m_time; }
//#endif
inline cTime::cTime(tm time)
	{ m_time = time; }
inline const cTime& cTime::operator=(const cTime& timeSrc)
	{ m_time = timeSrc.m_time; return *this; }
inline const cTime& cTime::operator=(tm t)
	{ m_time = t; return *this; }
inline tm cTime::GetTime() const
	{ return m_time; }
inline int cTime::GetYear() const
   { return m_time.tm_year + 1900; }
inline int cTime::GetMonth() const
   { return m_time.tm_mon + 1; }
inline int cTime::GetDay() const
   { return m_time.tm_mday; }
inline int cTime::GetHour() const
   { return m_time.tm_hour; }
inline int cTime::GetMinute() const
   { return m_time.tm_min; }
inline int cTime::GetSecond() const
   { return m_time.tm_sec; }
inline int cTime::GetDayOfWeek() const
   { return m_time.tm_wday + 1; }
}//?namespace net4cpp21

#endif



