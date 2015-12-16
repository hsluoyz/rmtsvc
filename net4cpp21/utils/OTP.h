/*******************************************************************
   *	OTP.h
   *    DESCRIPTION:一次口令系统 RFC2289.txt
   *				A One-Time Password System
   *    AUTHOR:yyc
   *
   *
   *    DATE:2006-01-17
   *	net4cpp 2.1
   *******************************************************************/
  
#ifndef __YY_COTPS_H__
#define __YY_COTPS_H__

namespace net4cpp21
{
	class OTP
	{
	public:
		OTP(){}
		~OTP(){}

		const char *md4(const char *seed,const char *passphrase,int count);
		const char *md5(const char *seed,const char *passphrase,int count);
	private:
		char m_buffer[128];
	};
}//?namespace net4cpp21

#endif

