/*******************************************************************
   *	cLogger.cpp
   *    DESCRIPTION:日志记录实现，用于程序输出打印各种信息
   *			可以输出到控制台，文件，窗口或tcp socket通道
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2004-10-10
   *	Last modify: 2005-09-01
   *	net4cpp 2.1
   *******************************************************************/
 
#include "../include/sysconfig.h"  
#include "../include/cLogger.h"

#include <ctime>
#include <cstdarg>

using namespace net4cpp21;

cLogger *cLogger::plogger=new cLogger();//静态日志类实例指针
  
cLogger::cLogger()
{
	m_hout=(long)stdout;
	m_logtype=LOGTYPE_STDOUT;
	m_loglevel=LOGLEVEL_DEBUG;
	m_bOutputTime=false;
	m_bOutStdout=false;
	m_pcallback=NULL;
	m_lcbparam=0;
	m_fileopenType[0]='w';
	m_fileopenType[1]='b';
	m_fileopenType[2]=m_fileopenType[3]=0;
}

cLogger::~cLogger()
{
	if(m_logtype==LOGTYPE_FILE && m_hout) ::fclose((FILE *)m_hout);
}

LOGTYPE cLogger::setLogType(LOGTYPE lt,long lParam)
{
	if(m_logtype==LOGTYPE_FILE && m_hout) ::fclose((FILE *)m_hout);
	m_hout=0; m_logtype=LOGTYPE_NONE;
	switch(lt)
	{
		case LOGTYPE_STDOUT:
			m_hout=(long)stdout;
			m_logtype=LOGTYPE_STDOUT;
			break;
		case LOGTYPE_FILE:
			if( (m_hout=(long)fileopen((LPCTSTR)lParam,m_fileopenType)) )
				m_logtype=LOGTYPE_FILE;
			break;
		case LOGTYPE_HWND:
			m_hout=lParam;
			m_logtype=LOGTYPE_HWND;
			break;
		case LOGTYPE_SOCKET:
			m_hout=lParam;
			m_logtype=LOGTYPE_SOCKET;
			break;
	}//?switch
	return m_logtype;
}

void cLogger::printTime()
{
	if(m_logtype==LOGTYPE_NONE && m_pcallback==NULL) 
		return; //如果指定不输出日志且没有设置回调
	time_t tNow=time(NULL);
	struct tm * ltime=localtime(&tNow);
	TCHAR buf[64];
	size_t len=strprintf(buf,TEXT("[%04d-%02d-%02d %02d:%02d:%02d] - \r\n"),
		(1900+ltime->tm_year), ltime->tm_mon+1, ltime->tm_mday, 
		ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
	buf[len]=0;
	_dump(buf,len); return;
}

void cLogger::dump(LOGLEVEL ll,size_t len,LPCTSTR buf)
{
	if(m_logtype==LOGTYPE_NONE && m_pcallback==NULL) 
		return; //如果指定不输出日志且没有设置回调
	if(ll<m_loglevel) return;//此级别的日志不输出
	if(m_hout==0 || buf==NULL) return;
	if(len<=0) len=stringlen(buf);
	if(m_bOutputTime) printTime();//打印时间
	if(len>0) _dump(buf,len);
	return;
}

void cLogger::dump(LOGLEVEL ll,LPCTSTR fmt,...)
{
	if(m_logtype==LOGTYPE_NONE && m_pcallback==NULL) 
		return; //如果指定不输出日志且没有设置回调
	if(ll<m_loglevel) return;//此级别的日志不输出
	if(m_hout==0) return;
	TCHAR buffer[1024]; buffer[0]=0;
	va_list args;
	va_start(args,fmt);
	int len=vsnprintf(buffer,1024,fmt,args);
	va_end(args);
	if(len==-1){//写入数据超过了给定的缓冲区空间大小
		buffer[1018]=buffer[1019]=buffer[1020]='.';
		buffer[1021]='.'; buffer[1022]='\r';
		buffer[1023]='\n';
		len=1024;
	}
	if(m_bOutputTime) printTime();//打印时间
	if(len>0) _dump(buffer,len);
	return;
}

void cLogger::debug(size_t len,LPCTSTR buf)
{
	if(m_logtype==LOGTYPE_NONE && m_pcallback==NULL) 
		return; //如果指定不输出日志且没有设置回调
	if(LOGLEVEL_DEBUG<m_loglevel) return;//此级别的日志不输出
	if(m_hout==0 || buf==NULL) return;
	if(len<=0) len=stringlen(buf);
	if(m_bOutputTime) printTime();//打印时间
	if(len>0) _dump(buf,len);
	return;
}
void cLogger::debug(LPCTSTR fmt,...)
{
	if(m_logtype==LOGTYPE_NONE && m_pcallback==NULL) 
		return; //如果指定不输出日志且没有设置回调
	if(LOGLEVEL_DEBUG<m_loglevel) return;//此级别的日志不输出
	if(m_hout==0) return;
	TCHAR buffer[1024]; buffer[0]=0;
	va_list args;
	va_start(args,fmt);
	int len=vsnprintf(buffer,1024,fmt,args);
	va_end(args);
	if(len==-1){//写入数据超过了给定的缓冲区空间大小
		buffer[1018]=buffer[1019]=buffer[1020]='.';
		buffer[1021]='.'; buffer[1022]='\r';
		buffer[1023]='\n';
		len=1024;
	}
	if(m_bOutputTime) printTime();//打印时间
	if(len>0) _dump(buffer,len);
	return;
}


inline void cLogger::_dump(LPCTSTR buf,size_t len)
{
	if(m_pcallback){
		if(!(*m_pcallback)(buf,len,m_lcbparam)) return;
	}//if(m_pcallback)
	if(m_logtype==LOGTYPE_NONE) return;
	switch(m_logtype)
	{
		case LOGTYPE_STDOUT:
#ifdef _DEBUG
			printf("%s",buf);
#else
			fwrite((const void *)buf,sizeof(TCHAR),len,stdout);
#endif
			break;
		case LOGTYPE_FILE:
			fwrite((const void *)buf,sizeof(TCHAR),len,(FILE *)m_hout);
			break;
		case LOGTYPE_HWND:
#ifdef WIN32
		{
			int end = ::GetWindowTextLength((HWND)m_hout);
			::SendMessage((HWND)m_hout, EM_SETSEL, (WPARAM)end, (LPARAM)end);
			TCHAR c=*(buf+len); *(LPTSTR)(buf+len)=0;
			::SendMessage((HWND)m_hout, EM_REPLACESEL, 0, (LPARAM)buf);
			*(LPTSTR)(buf+len)=c;
		}
#endif
			break;
		case LOGTYPE_SOCKET:
			if(m_hout!=-1) //有效的socket句柄
				::send(m_hout,(const char *)buf,len*sizeof(TCHAR),MSG_NOSIGNAL);
			break;
	}//?switch
	if(m_bOutStdout && m_logtype!=LOGTYPE_STDOUT)
		fwrite((const void *)buf,sizeof(TCHAR),len,stdout);
}

void cLogger::dumpBinary(const char *buf,size_t len)
{
	if(m_logtype==LOGTYPE_NONE && m_pcallback==NULL) 
		return; //如果指定不输出日志且没有设置回调
	if(LOGLEVEL_DEBUG<m_loglevel) return;//此级别的日志不输出
	if(buf==NULL || len==0) return;

	if(m_bOutputTime) printTime();//打印时间
	
	int i,j,lines=(len+15)/16; //计算共多少行
	size_t count=0;//打印字符计数
	printf("Output Binary data,size=%d, lines=%d\r\n",len,lines);
	for(i=0;i<lines;i++)
	{
		count=i*16;
		for(j=0;j<16;j++)
		{
			if((count+j)<len)
				printf("%02X ",(unsigned char)buf[count+j]);
			else
				printf("   ");
		}
		printf("; ");
		for(j=0;j<16 && (count+j)<len;j++)
		{
			char c=buf[count+j];
			if(c<32 || c>=127) c='.';
			printf("%c",c);
		}
		printf("\r\n");
	}//?for(...
	return;
}

void cLogger::dumpMaps(std::map<std::string,std::string> &maps,const char *desc)
{
	if(m_logtype==LOGTYPE_NONE && m_pcallback==NULL) 
		return; //如果指定不输出日志且没有设置回调
	if(LOGLEVEL_DEBUG<m_loglevel) return;//此级别的日志不输出
	char buffer[256];
	int buflen=sprintf(buffer,"****** %s start ******",(desc)?desc:"dump std::map");
	_dump(buffer,buflen);

	std::map<std::string,std::string>::iterator it=maps.begin();
	int count=0;
	for(;it!=maps.end();it++)
	{
		buflen=sprintf(buffer,"\r\n\t%d : %s=",count++,(*it).first.c_str());
		_dump(buffer,buflen);
		_dump((*it).second.c_str(),(*it).second.length());
	}

	buflen=sprintf(buffer,"\r\n****** %s  end  ******\r\n",(desc)?desc:"dump std::map");
	_dump(buffer,buflen);
	return;
}

