/*******************************************************************
   *	sntpdef.h
   *    DESCRIPTION:定义SNTP协议所用到的常量、结构以及enum的定义
   *				
   *	http://hi.baidu.com/yycblog/home
   *	net4cpp 2.1
   *******************************************************************/

#ifndef __YY_SNTPDEF_H__
#define __YY_SNTPDEF_H__


#define SNTP_MAX_RESPTIMEOUT 10  //s 响应最大延时
#define SNTP_SERVER_PORT	 123 //默认服务端口

//Representation of an NTP timestamp
struct CNtpTimePacket
{
	DWORD m_dwInteger;
	DWORD m_dwFractional;
};

//Helper class to encapulate NTP time stamps
class CNtpTime
{
public:
//Constructors / Destructors
	CNtpTime();
	CNtpTime(const CNtpTime& time);
	CNtpTime(CNtpTimePacket& packet);
	CNtpTime(const SYSTEMTIME& st);

//General functions
	CNtpTime& operator=(const CNtpTime& time);
	double operator-(const CNtpTime& time) const;
	CNtpTime operator+(const double& timespan) const;
	operator SYSTEMTIME() const;
	operator CNtpTimePacket() const;
	operator unsigned __int64() const { return m_Time; };
	DWORD Seconds() const;
	DWORD Fraction() const;

//Static functions
	static CNtpTime GetCurrentTime();
	static DWORD MsToNtpFraction(WORD wMilliSeconds);
	static WORD NtpFractionToMs(DWORD dwFraction);
	static double NtpFractionToSecond(DWORD dwFraction);

protected:
//Internal static functions and data
	static long GetJulianDay(WORD Year, WORD Month, WORD Day);
	static void GetGregorianDate(long JD, WORD& Year, WORD& Month, WORD& Day);
	static DWORD m_MsToNTP[1000];

//The actual data
	unsigned __int64 m_Time;
};

struct NtpServerResponse
{
  int m_nLeapIndicator; //0: no warning
                        //1: last minute in day has 61 seconds
                        //2: last minute has 59 seconds
                        //3: clock not synchronized

  int m_nStratum; //0: unspecified or unavailable
                  //1: primary reference (e.g., radio clock)
                  //2-15: secondary reference (via NTP or SNTP)
                  //16-255: reserved

  CNtpTime     m_OriginateTime;    //Time when the request was sent from the client to the SNTP server
  CNtpTime     m_ReceiveTime;      //Time when the request was received by the server
  CNtpTime     m_TransmitTime;     //Time when the server sent the request back to the client
  CNtpTime     m_DestinationTime;  //Time when the reply was received by the client
  double       m_RoundTripDelay;   //Round trip time in seconds
  double       m_LocalClockOffset; //Local clock offset relative to the server
};

#endif
