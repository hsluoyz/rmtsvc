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
   
#ifndef __YY_CBUFFER_H__
#define __YY_CBUFFER_H__

namespace net4cpp21
{
	class cLoopBuffer
	{
	public:
		explicit cLoopBuffer(size_t size);
		~cLoopBuffer();
		//追加写入l字节到缓冲区中
		bool Write(const char *p,size_t l);
		//读取l字节到dest缓冲区中
		bool Read(char *dest,size_t l);
		/** skip l bytes from buffer */
		bool Remove(size_t l);

		/** total buffer length */
		size_t GetLength() { return m_q; }
		/** pointer to circular buffer beginning */
		char *GetStart() { return buf + m_b; }
		/** return number of bytes from circular buffer beginning to buffer physical end */
		size_t GetL() { return (m_b + m_q > m_max) ? m_max - m_b : m_q; }
		/** return free space in buffer, number of bytes until buffer overrun */
		size_t Space() { return m_max - m_q; }

		/** return total number of bytes written to this buffer, ever */
		unsigned long ByteCounter() { return m_count; }

private:
	char *buf; //缓冲区
	size_t m_max;//缓冲区大小
	size_t m_q; //缓冲区数据大小
	size_t m_b; //缓冲区数据读的起始位置
	size_t m_t; //缓冲区数据写的起始位置
	unsigned long m_count; //缓冲区中总的写入数据大小
	};

	class cBuffer
	{
	public:
		explicit cBuffer(size_t size=0);
		~cBuffer();
		
		cBuffer(cBuffer &buf);
		cBuffer & operator = (cBuffer &buf);
		char *str() { return m_buf;}
		size_t &len() { return m_len;}
		char & operator [] (size_t pos);
		size_t size() { return m_max; }
		size_t Space() { return m_max - m_len; } //剩余空间
		char * Resize(size_t size);
		
	private:
		char *m_buf; //缓冲区
		size_t m_max;//缓冲区大小
		size_t m_len; //缓冲区数据大小
	};
}//?namespace net4cpp21

#endif

