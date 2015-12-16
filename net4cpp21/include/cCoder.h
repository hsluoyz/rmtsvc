/*******************************************************************
   *	cCoder.h
   *    DESCRIPTION:字符编解码工具集
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2004-10-10
   *	net4cpp 2.0
   *******************************************************************/
   
#ifndef __YY_CCODER_H__
#define __YY_CCODER_H__

namespace net4cpp21
{
	class cCoder
	{
		static unsigned char DecToHex(unsigned char B);		//为Quoted编码操作进行字符转换
		static unsigned char HexToDec(unsigned char C);		//为Quoted解码操作进行字符转换
		
		static const char BASE64_ENCODE_TABLE[64];		//Base64编码表
		static const unsigned int BASE64_DECODE_TABLE[256];	//Base64解码表
		static const unsigned char QUOTED_ENCODE_TABLE[256];	//Quoted编码表
		
	public:
		static unsigned int m_LineWidth;			//指定编码后每行的长度，缺省是76
		//根据文件实际长度获取编码Base64后的长度,-1编码后每行长度不限，0编码后每行长度为m_LineWidth
		static int Base64EncodeSize(int iSize,unsigned int nLineWidth=0);  
		static int Base64DecodeSize(int iSize);		//根据已编码文件长度获取Base64的解码长度
		static int UUEncodeSize(int iSize);			//根据文件实际长度获取UUCode编码后的长度
		static int UUDecodeSize(int iSize);			//根据已编码文件长度获取UUCode解码后的长度
		static int QuotedEncodeSize(int iSize);		//根据实际文件的长度获取Quoted编码
		
		
		/*
		*  对一段Buffer进行Base64编码
		*
		*	 	pSrc	输入Buffer
		*		nSize	Buffer长度
		*		pDest	输出缓冲
		*
		*	 注: 输出Buffer的长度可以使用 Base64EncodeSize(int) 方法取得
		*/
		static int base64_encode(char *pSrc, unsigned int nSize, char *pDest,
			unsigned int nLineWidth=0); //隔指定的LineWidth添加CRLF
		//==-1编码后每行长度不限，==0编码后每行长度为m_LineWidth
		/*
		*  对一段Buffer进行Base64解码
		*	
		*	 	pSrc	输入Buffer
		*		nSize	Buffer长度
		*		pDest	输出缓冲
		*		return	解码后的实际长度
		*
		*	 注: 输出Buffer的长度可以使用 Base64DecodeSize(int) 方法取得
		*/
		static int  base64_decode(char *pSrc, unsigned int nSize, char *pDest);
		/*
		*  对一段Buffer进行UUCODE编码
		*
		*	 	pSrc	输入Buffer
		*		nSize	Buffer长度
		*		pDest	输出缓冲
		*
		*	 注: 输出Buffer的长度可以使用 UUEncodeSize(int) 方法取得
		*/
		static int UU_encode(char *pSrc, unsigned int nSize, char *pDest);
		
		/*
		*  对一段Buffer进行UUCODE解码
		*
		*	 	pSrc	输入Buffer
		*		nSize	Buffer长度
		*		pDest	输出缓冲
		*
		*	 注: 输出Buffer的长度可以使用 UUDecodeSize(int) 方法取得
		*/
		static int UU_decode(char *pSrc, unsigned int nSize, char *pDest);
		/*
		*  对一段Buffer进行Quoted-Printble编码
		*
		*	 	pSrc	输入Buffer
		*		nSize	Buffer长度
		*		pDest	输出缓冲
		*		return	编码后的实际长度
		*
		*	 注: 输出Buffer的长度可以使用 QuotedDecodeSize(int) 方法取得
		*/
		static int quoted_encode(char *pSrc, unsigned int nSize, char *pDest);
		
		/*
		*  对一段Buffer进行Quoted-Printble解码
		*
		*	 	pSrc	输入Buffer
		*		nSize	Buffer长度
		*		pDest	输出缓冲
		*		return	解码后的实际长度
		*
		*	 注：这里没有提供计算解码长度的方法 直接使用输入Buffer作输出Buffer就可以了
		*/
		static int quoted_decode(char *pSrc, unsigned int nSize, char *pDest);
		
		//url编解码 将&amp; &lt; &gt; 编解码为 & < >
		static int url_decode(const char *pSrc,int nSize,char *pDest);
		static int url_encode(const char *pSrc,int nSize,char *pDest);
		static int MimeEncodeSize(int iSize){ return iSize *3;}
		/*
		*  对指定的字符串进行Mime编码
		*
		*	 	pSrc	输入字符串
		*		nSize	字符串长度
		*		pDest	输出缓冲
		*		return	编码后的实际长度
		*
		*	 注：此函数不对汉字进行编码
		*/
		static int mime_encode(const char *pSrc,unsigned int nSize,char *pDest);
		//汉字也进行编码
		static int mime_encodeEx(const char *pSrc,unsigned int nSize,char *pDest);
		static int mime_encodeURL(const char *pSrc,unsigned int nSize,char *pDest);
		/*
		*  对指定的字符串进行Mime解码
		*
		*	 	pSrc	输入字符串
		*		nSize	字符串长度
		*		pDest	输出缓冲
		*		return	解码码后的实际长度
		*
		*	 注：这里没有提供计算解码长度的方法 直接使用输入Buffer作输出Buffer就可以了
		*/
		static int mime_decode(const char *pSrc,unsigned int nSize,char *pDest);
		
		//UTF-8 - ASCII 兼容的多字节(1~3)字节 Unicode 编码
		//实际的utf8编码的字节为1~6字节，但我们一般也就用双字节的字符集，这样最多用到3字节
		//因为0x00000800 - 0x0000FFFF字符转化为utf8才为3字节
		static int Utf8EncodeSize(int iSize){ return iSize *3;}
		/*
		*  对指定的Buffer进行utf8编码
		*
		*	 	pSrc	输入Buffer
		*		nSize	Buffer长度
		*		pDest	输出缓冲
		*		return	编码后的实际长度
		*
		*/
		static int utf8_encode(const char *pSrc,unsigned int nSize,char *pDest);
		static int utf8_encodeW(const unsigned short *pSrc,unsigned int nSize,char *pDest);
		/*
		*  对指定的Buffer进行Utf8解码
		*
		*	 	pSrc	输入字符串
		*		nSize	字符串长度
		*		pDest	输出缓冲
		*		return	解码码后的实际长度
		*
		*	 注：这里没有提供计算解码长度的方法 直接使用输入Buffer作输出Buffer就可以了
		*/
		static int utf8_decode(const char *pSrc,unsigned int nSize,char *pDest);
		static int utf8_decodeW(const char *pSrc,unsigned int nSize,unsigned short *pDest);

		//将16进制字符串转为数值
		static unsigned long hex_atol(const char *str);

		//=?charset?encoding-type?data?= 数据格式解码
		static int eml_decode(const char *pSrc,unsigned int nSize,char *pDest);
		static int eml_encode(const char *pSrc,unsigned int nSize,char *pDest);
		static int EmlEncodeSize(int iSize);
	};
}//?namespace net4cpp21

#endif

