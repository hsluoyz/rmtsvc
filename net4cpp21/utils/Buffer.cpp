/*******************************************************************
   *	Buffer.h
   *    DESCRIPTION:循环buffer缓冲 和 buffer缓冲
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-08－19
   *	net4cpp 2.1
   *******************************************************************/

#include "../include/Buffer.h"  
#include <cstring>

using namespace net4cpp21;

cLoopBuffer::cLoopBuffer(size_t size)
:buf(new char[size])
,m_max(size)
,m_q(0)
,m_b(0)
,m_t(0)
,m_count(0)
{
}


cLoopBuffer::~cLoopBuffer()
{
	delete[] buf;
}

bool cLoopBuffer::Write(const char *s,size_t l)
{
	if (m_q + l > m_max)
	{//write buffer overflow
		return false; // overflow
	}
	m_count += (unsigned long)l;
	if (m_t + l > m_max) // block crosses circular border
	{
		size_t l1 = m_max - m_t; // size left until circular border crossing
		memcpy(buf + m_t, s, l1);
		memcpy(buf, s + l1, l - l1);
		m_t = l - l1;
		m_q += l;
	}
	else
	{
		memcpy(buf + m_t, s, l);
		m_t += l;
		if (m_t >= m_max)
			m_t -= m_max;
		m_q += l;
	}
	return true;
}

//从缓冲区中读取指定的字节
bool cLoopBuffer::Read(char *s,size_t l)
{
	if (l > m_q)
	{//attempt to read beyond buffer
		return false; // not enough chars
	}
	if (m_b + l > m_max) // block crosses circular border
	{
		size_t l1 = m_max - m_b;
		if (s)
		{
			memcpy(s, buf + m_b, l1);
			memcpy(s + l1, buf, l - l1);
		}
		m_b = l - l1;
		m_q -= l;
	}
	else
	{
		if (s)
		{
			memcpy(s, buf + m_b, l);
		}
		m_b += l;
		if (m_b >= m_max)
			m_b -= m_max;
		m_q -= l;
	}
	if (!m_q)
	{
		m_b = m_t = 0;
	}
	return true;
}


bool cLoopBuffer::Remove(size_t l)
{
	return Read(NULL, l);
}

//-----------------------------------------
cBuffer::cBuffer(size_t size)
:m_len(0)
{
	if(size!=0)
		m_buf=new char[size];
	else m_buf=NULL;
	m_max=(m_buf)?size:0;
}


cBuffer::~cBuffer()
{
	delete[] m_buf;
}

/*//拷贝构造
cBuffer::cBuffer(const cBuffer &buf1){
cBuffer &buf=const_cast<cBuffer &>(buf1);
	delete[] m_buf;
	m_buf=buf.m_buf;
	m_max=buf.m_max;
	m_len=buf.m_len ;
	buf.m_buf=NULL;
	buf.m_max=0;
	buf.m_len=0;
} */
//拷贝构造
cBuffer::cBuffer(cBuffer &buf){
	delete[] m_buf;
	m_buf=buf.m_buf;
	m_max=buf.m_max;
	m_len=buf.m_len ;
	buf.m_buf=NULL;
	buf.m_max=0;
	buf.m_len=0;
}
//赋值
cBuffer & cBuffer::operator = (cBuffer &buf)
{
	delete[] m_buf;
	m_buf=buf.m_buf;
	m_max=buf.m_max;
	m_len=buf.m_len ;
	buf.m_buf=NULL;
	buf.m_max=0;
	buf.m_len=0;
	return *this;
}
char & cBuffer::operator [] (size_t pos)
{
	static char c=0; //越界保护字符
	if(pos<0 || pos>=m_max) return c;
	return m_buf[pos];
}

char * cBuffer::Resize(size_t size)
{
	if(m_buf)
	{
		char *buf=(size!=0)?(new char[size]):NULL;
		if(buf==NULL)
		{
			delete[] m_buf;
			m_buf=NULL; m_max=m_len=0;
		}
		else
		{
			if(m_len>size) m_len=size;
			memcpy(buf,m_buf,m_len);
			delete[] m_buf;
			m_max=size; m_buf=buf;
		}
	}//?if(m_buf)
	else if(size!=0)
	{
		m_buf=new char[size];
		if(m_buf) m_max=size;
	}
	return m_buf;
}
