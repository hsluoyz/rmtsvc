/*******************************************************************
   *	cCoder.cpp
   *    DESCRIPTION:字符编解码工具集
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2004-10-10
   *	net4cpp 2.0
   *******************************************************************/
   
#include "../include/sysconfig.h"  
#include "../include/cCoder.h"
   
using namespace net4cpp21;
   
//编码时候的每行的长度
unsigned int cCoder::m_LineWidth = 76; //80; yyc modify. smtp等默认行长度为76

//Base64解码表
const char cCoder::BASE64_ENCODE_TABLE[64] = {
	 65,  66,  67,  68,  69,  70,  71,  72,  // 00 - 07 A - H
	 73,  74,  75,  76,  77,  78,  79,  80,  // 08 - 15 I - P
	 81,  82,  83,  84,  85,  86,  87,  88,  // 16 - 23 Q - X
	 89,  90,  97,  98,  99, 100, 101, 102,  // 24 - 31 Y - f
	103, 104, 105, 106, 107, 108, 109, 110,  // 32 - 39 j - n
	111, 112, 113, 114, 115, 116, 117, 118,  // 40 - 47 o - v
	119, 120, 121, 122,  48,  49,  50,  51,  // 48 - 55 w - 3
	 52,  53,  54,  55,  56,  57,  43,  47 };// 56 - 63 4 - /
	
	//Base64编码表
const unsigned int cCoder::BASE64_DECODE_TABLE[256] = {
	255, 255, 255, 255, 255, 255, 255, 255, //  00 -  07
	255, 255, 255, 255, 255, 255, 255, 255, //  08 -  15
	255, 255, 255, 255, 255, 255, 255, 255, //  16 -  23
	255, 255, 255, 255, 255, 255, 255, 255, //  24 -  31
	255, 255, 255, 255, 255, 255, 255, 255, //  32 -  39
	255, 255, 255,  62, 255, 255, 255,  63, //  40 -  47
	 52,  53,  54,  55,  56,  57,  58,  59, //  48 -  55
	 60,  61, 255, 255, 255, 255, 255, 255, //  56 -  63
	255,   0,   1,   2,   3,   4,   5,   6, //  64 -  71
	  7,   8,   9,  10,  11,  12,  13,  14, //  72 -  79
	 15,  16,  17,  18,  19,  20,  21,  22, //  80 -  87
	 23,  24,  25, 255, 255, 255, 255, 255, //  88 -  95
	255,  26,  27,  28,  29,  30,  31,  32, //  96 - 103
	 33,  34,  35,  36,  37,  38,  39,  40, // 104 - 111
	 41,  42,  43,  44,  45,  46,  47,  48, // 112 - 119
	 49,  50,  51, 255, 255, 255, 255, 255, // 120 - 127
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255 };
//------------------------------------------------------------------------------
		
//Quoted编码表
const unsigned char cCoder::QUOTED_ENCODE_TABLE[256] = {
	255, 255, 255, 255, 255, 255, 255, 255, // 00 -  07
	255, 255,  10, 255, 255,  13, 255, 255, // 08 - 15
	255, 255, 255, 255, 255, 255, 255, 255, // 16 - 23
	255, 255, 255, 255, 255, 255, 255, 255, // 24 - 31
	255,  33,  34,  35,  36,  37,  38,  39,
	 40,  41,  42,  43,  44,  45,  46,  47,
	 48,  49,  50,  51,  52,  53,  54,  55,
	 56,  57,  58,  59,  60, 255,  62,  63,
	 64,  65,  66,  67,  68,  69,  70,  71,
	 72,  73,  74,  75,  76,  77,  78,  79,
	 80,  81,  82,  83,  84,  85,  86,  87,
	 88,  89,  90,  91,  92,  93,  94,  95,
	 96,  97,  98,  99, 100, 101, 102, 103,
	104, 105, 106, 107, 108, 109, 110, 111,
	112, 113, 114, 115, 116, 117, 118, 119,
	120, 121, 122, 123, 124, 125, 126, 127,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255,
	255, 255, 255, 255, 255, 255, 255, 255 };
//------------------------------------------------------------------------------
	
//获取Base64编码长度
//nLineWidth: ==-1编码后每行长度不限，==0编码后每行长度为m_LineWidth
int cCoder::Base64EncodeSize(int iSize,unsigned int nLineWidth)
{
	if(nLineWidth==0) nLineWidth=m_LineWidth;
	int nSize, nCR;
	nSize = (iSize + 2) / 3 * 4 ;
	nCR = nSize / nLineWidth; //计算回车数量
	nSize+= nCR * 2;
	return nSize;
}
	
//------------------------------------------------------------------------------
	
//获取Base64解码长度
int cCoder::Base64DecodeSize(int iSize)
{
	return (iSize + 3) / 4 * 3;
}

//------------------------------------------------------------------------------

//获取UUCode编码长度
int cCoder::UUEncodeSize(int iSize)
{
	int nSize, nCR;
	nSize = (iSize + 2) / 3 * 4 ;
	nCR = nSize / m_LineWidth + 2; //计算回车数量
	nSize+= nCR * 3 + 2;
	return nSize;
}
			
//------------------------------------------------------------------------------

//获取UUCode解码长度
int cCoder::UUDecodeSize(int iSize)
{
	return (iSize + 3) / 4 * 3;
}
			
//------------------------------------------------------------------------------

//获取Quoted编码长度
int cCoder::QuotedEncodeSize(int iSize)
{
	int nSize = iSize * 3 + 1;
	int nCR = nSize / m_LineWidth;
	nSize+= nCR * 3;
	return nSize;
}

//------------------------------------------------------------------------------

//Base64编码
//nLineWidth: ==-1编码后每行长度不限，==0编码后每行长度为m_LineWidth
int cCoder::base64_encode(char *pSrc, unsigned int nSize, char *pDest,
						  unsigned int nLineWidth)
{
	if ((pSrc == NULL) || (nSize <= 0)) return 0;
	if(nLineWidth==0) nLineWidth=m_LineWidth;	
	unsigned int iB, iInMax3, Len;
	char *pInPtr, *pInLimitPtr;
	char *OutPtr ;
			
	pInPtr = pSrc;
	iInMax3 = nSize / 3 * 3;
	OutPtr = pDest;
	pInLimitPtr = pInPtr + iInMax3;
				
	while (pInPtr != pInLimitPtr)
	{
		Len = 0;
		while ((Len < nLineWidth) && (pInPtr != pInLimitPtr))
		{
			iB = (unsigned char) *pInPtr++;
			iB = iB << 8;
				
			iB = iB | (unsigned char) *pInPtr++;
			iB = iB << 8;
						
			iB = iB | (unsigned char) *pInPtr++;
						
			//以4 byte倒序写入输出缓冲
			OutPtr[3] = BASE64_ENCODE_TABLE[iB & 0x3F];
			iB = iB >> 6;
			OutPtr[2] = BASE64_ENCODE_TABLE[iB & 0x3F];
			iB = iB >> 6;
			OutPtr[1] = BASE64_ENCODE_TABLE[iB & 0x3F];
			iB = iB >> 6;
			OutPtr[0] = BASE64_ENCODE_TABLE[iB];
			OutPtr+=4;
			Len+=4;
		}
		if (Len >= nLineWidth)
		{
			*OutPtr++ = '\r'; //加上回车换行符
			*OutPtr++ = '\n';
		}
	}
	//设置尾部
	switch (nSize - iInMax3)
	{
		case 1:
			iB = (unsigned char) *pInPtr;
			iB = iB << 4;
			OutPtr[1] = BASE64_ENCODE_TABLE[iB & 0x3F];
			iB = iB >> 6;
			OutPtr[0] = BASE64_ENCODE_TABLE[iB];
			OutPtr[2] = '='; //用'='也就是64码填充剩余部分
			OutPtr[3] = '=';
			OutPtr+=4;
			break;
		case 2:
			iB = (unsigned char) *pInPtr++;
			iB = iB << 8;
			iB = iB | (unsigned char) *pInPtr;
			iB = iB << 2;
			OutPtr[2] = BASE64_ENCODE_TABLE[iB & 0x3F];
			iB = iB >> 6;
			OutPtr[1] = BASE64_ENCODE_TABLE[iB & 0x3F];
			iB = iB >> 6;
			OutPtr[0] = BASE64_ENCODE_TABLE[iB];
			OutPtr[3] = '='; // Fill remaining byte.
			OutPtr+=4;
			break;
	}
	return (unsigned int) (OutPtr - pDest);
}
			
//------------------------------------------------------------------------------
//Base64解码

int cCoder::base64_decode(char *pSrc, unsigned int nSize, char *pDest)
{
	if ((pSrc == NULL) || (pDest == NULL) || (nSize <= 0)) return 0;
			
	unsigned int lByteBuffer, lByteBufferSpace;
	unsigned int C; //临时阅读变量
	int reallen;
	char *InPtr, *InLimitPtr;
	char *OutPtr;
				
	lByteBuffer = 0; lByteBufferSpace = 4;
				
	InPtr = pSrc;
	InLimitPtr= InPtr + nSize;
	OutPtr = pDest;
				
	while (InPtr != InLimitPtr)
	{
		C = BASE64_DECODE_TABLE[*InPtr]; // Read from InputBuffer.
		InPtr++;
		if (C == 0xFF) continue; //读到255非法字符
		lByteBuffer = lByteBuffer << 6 ;
		lByteBuffer = lByteBuffer | C ;
		lByteBufferSpace--;
		if (lByteBufferSpace != 0) continue; //一次读入4个字节
		//到序写入3个字节到缓冲
		OutPtr[2] = lByteBuffer;
		lByteBuffer = lByteBuffer >> 8;
		OutPtr[1] = lByteBuffer;
		lByteBuffer = lByteBuffer >> 8;
		OutPtr[0] = lByteBuffer;
		//准备写入后3位
		OutPtr+= 3; lByteBuffer = 0; lByteBufferSpace = 4;
	}
	reallen = (unsigned int)OutPtr - (unsigned int)pDest;
	//处理尾部 返回实际长度
	switch (lByteBufferSpace)
	{
		case 1:
			lByteBuffer = lByteBuffer >> 2;
			OutPtr[1] = lByteBuffer;
			lByteBuffer = lByteBuffer >> 8;
			OutPtr[0] = lByteBuffer;
			return reallen + 2;
		case 2:
			lByteBuffer = lByteBuffer >> 4;
			OutPtr[0] = lByteBuffer;
			return reallen + 1;
		default:
			return reallen;
	}
}

//------------------------------------------------------------------------------

//UUCode编码
int cCoder::UU_encode(char *pSrc, unsigned int nSize, char *pDest)
{
	if ((pSrc == NULL) || (pDest == NULL) || (nSize <= 0)) return 0;
				
	unsigned int lByteBufferSpace, Len;
	unsigned char B[3]; //临时阅读变量
	char *InPtr, *InLimitPtr;
	char *OutPtr;
			
	InPtr = pSrc;
	InLimitPtr= InPtr + nSize;
	OutPtr = pDest;
				
	while (InPtr < InLimitPtr)
	{
		Len = 0;
					
		while ((InPtr < InLimitPtr) && (Len < m_LineWidth))
		{
			lByteBufferSpace = (unsigned int) InLimitPtr - (unsigned int) InPtr;
			if (lByteBufferSpace > 3) lByteBufferSpace = 3; //设置步长
			//取值
			for (unsigned int i = 0; i < lByteBufferSpace; i++ )
			{
				B[i] = *InPtr++;
			}
			if (Len == 0)
			{
				*OutPtr++ = 'M';
				Len++;
			}
			//编码运算
			OutPtr[0] = B[0] >> 2;
			OutPtr[1] = (unsigned int) (B[0] << 4 & 0x30) + (unsigned int) (B[1] >> 4 & 0x0F);
			OutPtr[2] = (unsigned int) (B[1] << 2 & 0x3C) + (unsigned int) (B[2] >> 6 & 0x03);
			OutPtr[3] = B[2] & 0x3F;
			for (i = 0; i < 4; i++)
			{
				if (OutPtr[i] == NULL)
					OutPtr[i] = '`';//(unsigned char) (OutPtr[i] + 0x60);
				else 
					OutPtr[i] = ' ' + OutPtr[i];//(unsigned char) (OutPtr[i] + 0x20);
			}	
			OutPtr+=4;
			Len+=4;
		}
					
		*OutPtr++ = '\r'; //设置回车
		*OutPtr++ = '\n';
	}
	return (unsigned int) (OutPtr - pDest);
}

//------------------------------------------------------------------------------

//UUCode解码
int cCoder::UU_decode(char *pSrc, unsigned int nSize, char *pDest)
{
	char C[4]; //临时阅读变量
	char Tmp;
	int CurrIndex, Index;
	char *InPtr, *InLimitPtr;
	char *OutPtr;
		
	if ((pSrc == NULL) || (pDest == NULL) || (nSize <= 0)) return 0;
		
	CurrIndex = 0;
				
	InPtr = pSrc;
	InLimitPtr= InPtr + nSize;
	OutPtr = pDest;
				
	while (InPtr != InLimitPtr) //读取4个字符
	{
		memset(C, 0, sizeof(C));
		Index = 0;
		do
		{
			Tmp = *InPtr++;
			if (Tmp == 0x60) 
			{
				Tmp = 0x20;       //为了兼容OutLook Express
			}
			else if (Tmp =='\r')  //首个字母不处理
			{
				InPtr++;
				CurrIndex = 0;
			}
			
			if ((Tmp > 0x20) || ( CurrIndex > 0 ))
			{
				Tmp = Tmp - 0x20;
				if (CurrIndex == 0) 
				{
					CurrIndex = Tmp;
				}
				else
				{
					C[Index] = Tmp;  // 向数组中追加字符
					Index++;
				}
			}
		}
		while ((InPtr < InLimitPtr) && (Index < sizeof(C)));
					
		OutPtr[0] = (char) ((C[0] << 2) + (C[1] >> 4));
		OutPtr[1] = (char) ((C[1] << 4) + (C[2] >> 2));
		OutPtr[2] = (char) ((C[2] << 6) + C[3]);
		OutPtr+=3; //设置起始位置
	}
	return (unsigned int)OutPtr - (unsigned int)pDest;
}

//------------------------------------------------------------------------------

//Quoted编码
int cCoder::quoted_encode(char *pSrc, unsigned int nSize, char *pDest)
{
	unsigned int Len;
	unsigned char B; //临时阅读变量
	char *InPtr, *InLimitPtr;
	char *OutPtr;
			
	if ((pSrc == NULL) || (pDest == NULL) || (nSize <= 0)) return 0;
			
	InPtr = pSrc;
	InLimitPtr= InPtr + nSize;
	OutPtr = pDest;
				
	while (InPtr < InLimitPtr)
	{
		Len = 0;
		while ((InPtr < InLimitPtr) && (Len < m_LineWidth))
		{
			B = *InPtr++;
			if (QUOTED_ENCODE_TABLE[ B ]  == 255)
			{
				*OutPtr++ = '=';
				*OutPtr++ = DecToHex(B >> 4);
				*OutPtr++ = DecToHex(B & 0x0f);
				Len+=3;
			}
			else
			{
				*OutPtr++ = B;
				Len++;
			}
		}
		if (Len >= m_LineWidth)
		{
			*OutPtr++ = '=';
			*OutPtr++ = '\r'; //设置新行
			*OutPtr++ = '\n';
		}
	}
	*OutPtr = '\0';
	return (unsigned int) (OutPtr - pDest);
}

//------------------------------------------------------------------------------

//Quoted解码
int cCoder::quoted_decode(char *pSrc, unsigned int nSize, char *pDest)
{
	if ((pSrc == NULL) || (pDest == NULL) || (nSize <= 0)) return 0;
				
	unsigned char nA, nB;
	char C[2]; //临时阅读变量
				
	char *InLimitPtr= pSrc + nSize;
	char *pDestOrg = pDest;
				
	while (pSrc < InLimitPtr)
	{
		C[0] = *pSrc++; //先取第一字符
		if (C[0] == '=') //如果后面是经过编码的
		{
			C[0] = *pSrc++;  //取出两个编码字符
			C[1] = *pSrc++;

			if (C[0] != '\r')
			{
				nA = HexToDec(C[0]); //解码变换
				nB = HexToDec(C[1]);
				*pDest++ = (nA << 4) + nB;
			}
		}
		else //否则直接输出
		{
			*pDest++ = C[0];
		}
	}
	return (unsigned int)(pDest - pDestOrg);
}
//url编解码 将&amp; &lt; &gt; &quot; &mdash; 编解码为 & < > " -
int cCoder::url_decode(const char *pSrc,int nSize,char *pDest)
{
	if(nSize<=0) nSize=(pSrc)?strlen(pSrc):0;
	if(pSrc==NULL || nSize<=0) return 0;
	char *pOld=pDest;
	while(nSize>0){
		if(*pSrc=='&'){
			if(nSize>=5 && strncmp(pSrc,"&amp;",5)==0)
			{	*pDest++='&'; pSrc+=5; nSize-=5; }
			else if(nSize>=4 && strncmp(pSrc,"&lt;",4)==0)
			{	*pDest++='<'; pSrc+=4; nSize-=4; }
			else if(nSize>=4 && strncmp(pSrc,"&gt;",4)==0)
			{	*pDest++='>'; pSrc+=4; nSize-=4; }
			else if(nSize>=6 && strncmp(pSrc,"&quot;",6)==0)
			{	*pDest++='"'; pSrc+=6; nSize-=6; }
			else if(nSize>=7 && strncmp(pSrc,"&mdash;",7)==0)
			{	*pDest++='-'; pSrc+=7; nSize-=7; }
			else{ *pDest++=*pSrc++; nSize--; }
		}else{ *pDest++=*pSrc++; nSize--; }
	}//?while
	*pDest='\0'; return pDest-pOld;
}
int cCoder::url_encode(const char *pSrc,int nSize,char *pDest)
{
	if(nSize<=0) nSize=(pSrc)?strlen(pSrc):0;
	if(pSrc==NULL || nSize<=0) return 0;
	int nPos=0;
	while(nSize>0){
		if(*pSrc=='&'){
			pDest[nPos++]='&';
			pDest[nPos++]='a';
			pDest[nPos++]='m';
			pDest[nPos++]='p';
			pDest[nPos++]=';';
		}else if(*pSrc=='<'){
			pDest[nPos++]='&';
			pDest[nPos++]='l';
			pDest[nPos++]='t';
			pDest[nPos++]=';';
		}else if(*pSrc=='>'){
			pDest[nPos++]='&';
			pDest[nPos++]='g';
			pDest[nPos++]='t';
			pDest[nPos++]=';';
		}else pDest[nPos++]=*pSrc;
		pSrc++; nSize--;
	}//?while
	pDest[nPos]=0; return nPos;
}
//MIME编码
int cCoder::mime_encode(const char *pSrc,unsigned int nSize,char *pDest)
{
	int pos=0;
	for(unsigned int i=0;i<nSize;i++)
	{
		if(pSrc[i]=='@' || pSrc[i]==':' || pSrc[i]=='/' || 
		   pSrc[i]=='<' || pSrc[i]=='=' || pSrc[i]=='\"' || 
		   pSrc[i]=='>' || pSrc[i]=='+' || pSrc[i]==' ')
		{
			//sprintf(pDest+pos,"%%%02X",pSrc[i]); pos+=3;
			pDest[pos++]='%';
			pDest[pos++]=cCoder::DecToHex( ((unsigned char)pSrc[i])/16 );
			pDest[pos++]=cCoder::DecToHex( ((unsigned char)pSrc[i])%16 );
		}
		else
			pDest[pos++]=pSrc[i];
	}//?for(int i=0;...
	pDest[pos]=0;
	return pos;
}
//MIME编码
int cCoder::mime_encodeEx(const char *pSrc,unsigned int nSize,char *pDest)
{
	int pos=0;
	for(unsigned int i=0;i<nSize;i++)
	{
		if(pSrc[i]<0 || pSrc[i]=='@' || pSrc[i]==':' || pSrc[i]=='/' || 
		   pSrc[i]=='<' || pSrc[i]=='=' || pSrc[i]=='\"' || 
		   pSrc[i]=='>' || pSrc[i]=='+' || pSrc[i]==' ' )
		{
			//sprintf(pDest+pos,"%%%02X",pSrc[i]); pos+=3;
			pDest[pos++]='%';
			pDest[pos++]=cCoder::DecToHex( ((unsigned char)pSrc[i])/16 );
			pDest[pos++]=cCoder::DecToHex( ((unsigned char)pSrc[i])%16 );
		}
		else
			pDest[pos++]=pSrc[i];
	}//?for(int i=0;...
	pDest[pos]=0;
	return pos;
}
//MIME编码URL 和mime_encodeEx相比仅仅不编码/
int cCoder::mime_encodeURL(const char *pSrc,unsigned int nSize,char *pDest)
{
	int pos=0;
	for(unsigned int i=0;i<nSize;i++)
	{
		if(pSrc[i]<0 || pSrc[i]=='@' || pSrc[i]==':' ||  
		   pSrc[i]=='<' || pSrc[i]=='=' || pSrc[i]=='\"' || 
		   pSrc[i]=='>' || pSrc[i]=='+' || pSrc[i]==' ' )
		{
			//sprintf(pDest+pos,"%%%02X",pSrc[i]); pos+=3;
			pDest[pos++]='%';
			pDest[pos++]=cCoder::DecToHex( ((unsigned char)pSrc[i])/16 );
			pDest[pos++]=cCoder::DecToHex( ((unsigned char)pSrc[i])%16 );
		}
		else
			pDest[pos++]=pSrc[i];
	}//?for(int i=0;...
	pDest[pos]=0;
	return pos;
}
//MIME解码
int cCoder::mime_decode(const char *pSrc,unsigned int nSize,char *pDest)
{
	unsigned int i=0;
	int pos=0;
	while(nSize>1 && i<(nSize-2))
	{
		if(pSrc[i]=='%')
		{
			unsigned char c=HexToDec(pSrc[i+1])*16+HexToDec(pSrc[i+2]);
			pDest[pos++]=c;
			i+=3;
		}
		else
			pDest[pos++]=pSrc[i++];
	}//?while(i<nSize)
	while(i<nSize) pDest[pos++]=pSrc[i++];
	pDest[pos]=0;
	return pos;
}

//utf8编码
int cCoder::utf8_encodeW(const unsigned short *buf,unsigned int nSize,char *pDest)
{
	unsigned int i=0;
	int pos=0;
	if(nSize==0) nSize=stringlenW(buf);
	for(i=0;i<nSize;i++)
	{
		if(buf[i]<=0x7f)
		{
			pDest[pos++]=(unsigned char)buf[i];
		}
		else if(buf[i]>=0x80 && buf[i]<=0x7ff)
		{
			pDest[pos++]=(((unsigned char)(buf[i]>>6)) | 0xe0) & 0xdf;
			pDest[pos++]=(((unsigned char)buf[i]) | 0xc0) & 0xbf;
		}
		else if(buf[i]>=0x800 && buf[i]<=0xffff)
		{
			pDest[pos++]=(((unsigned char)(buf[i]>>12)) | 0xf0) & 0xef;
			pDest[pos++]=(((unsigned char)(buf[i]>>6)) | 0xc0) & 0xbf;
			pDest[pos++]=(((unsigned char)buf[i]) | 0xc0) & 0xbf;
		}
	}
	pDest[pos]=0; return pos;
}

int cCoder::utf8_encode(const char *pSrc,unsigned int nSize,char *pDest)
{
	unsigned int i=0;
	//判断是否需要编码
	for(i=0;i<nSize;i++)
	{
		if(((unsigned char)pSrc[i])>=0x80) break;
	}
	if(i==nSize){ strncpy(pDest,pSrc,nSize); pDest[nSize]=0; return nSize;} //无须编码
	//将单字节编码转换为unicode双字节编码
	unsigned short *buf=(unsigned short *)::malloc(nSize*sizeof(unsigned short));
	if(buf==NULL) return 0;
#ifdef WIN32
	nSize=MultiByteToWideChar(CP_ACP,0,pSrc,nSize,buf,nSize);
#else
	return 0;
#endif

	int pos=utf8_encodeW(buf,nSize,pDest);
	::free(buf); return pos;
}

//utf8解码
int cCoder::utf8_decodeW(const char *pSrc,unsigned int nSize,unsigned short *pDest)
{
	unsigned short *buf=pDest;
	int pos=0; unsigned long lc=0;
	if(nSize==0) nSize=strlen(pSrc);
	for(unsigned int i=0;i<nSize;i++)
	{
		unsigned char c=pSrc[i];
		if((c & 0x80)==0)
			lc=c;
		else if( (c & 0xe0)==0xc0 )
		{
			if((nSize-i)<=1) 
				return 0; //错误数据
			lc=(((unsigned long)c & 0x1f)<<6) + ((unsigned long)pSrc[i+1] & 0x3f);
			i+=1;
		}
		else if( (c & 0xf0)==0xe0 )
		{
			if((nSize-i)<=2) 
				return 0; //错误数据

			lc=(((unsigned long)c & 0x0f)<<12) + (((unsigned long)pSrc[i+1] & 0x3f)<<6) +  
				(((unsigned long)pSrc[i+2] & 0x3f));
			i+=2;
		}
		else if( (c & 0xf8)==0xf0 )
		{
			if((nSize-i)<=3) 
				return 0; //错误数据
			lc=(((unsigned long)c & 0x07)<<18) + (((unsigned long)pSrc[i+1] & 0x3f)<<12) + 
				(((unsigned long)pSrc[i+2] & 0x3f)<<6) + ((unsigned long)pSrc[i+3] & 0x3f);
			i+=3;
		}
		else if( (c & 0xfc)==0xf8 )
		{
			if((nSize-i)<=4) 
				return 0; //错误数据
			lc=(((unsigned long)c & 0x03)<<24) + (((unsigned long)pSrc[i+1] & 0x3f)<<18) + 
				(((unsigned long)pSrc[i+2] & 0x3f)<<12) + (((unsigned long)pSrc[i+3] & 0x3f)<<6) + 
				(((unsigned long)pSrc[i+4] & 0x3f));
			i+=4;
		}
		else if( (c & 0xfe)==0xfc )
		{
			if((nSize-i)<=5) 
				return 0; //错误数据
			lc=(((unsigned long)c & 0x01)<<30) + (((unsigned long)pSrc[i+1] & 0x3f)<<24) + 
				(((unsigned long)pSrc[i+2] & 0x3f)<<18) + (((unsigned long)pSrc[i+3] & 0x3f)<<12) + 
				(((unsigned long)pSrc[i+4] & 0x3f)<<6) +((unsigned long)pSrc[i+5] & 0x3f);
			i+=5;
		}
		else lc=c; //yyc modify return 0; //错误数据
		
		buf[pos++]=(unsigned short)lc;
	}//?for(int i=0;...
	buf[pos]=0;
	return pos;
}

int cCoder::utf8_decode(const char *pSrc,unsigned int nSize,char *pDest)
{
	unsigned int i=0;
	//判断是否需要编码
	for(i=0;i<nSize;i++)
	{
		if(((unsigned char)pSrc[i])>=0x80) break;
	}
	if(i==nSize){ if(pSrc!=pDest) strncpy(pDest,pSrc,nSize); pDest[nSize]=0; return nSize;} //无须编码
	unsigned short *buf=(unsigned short *)::malloc((nSize+1)*sizeof(unsigned short));
	if(buf==NULL) return 0;
	int len=utf8_decodeW(pSrc,nSize,buf);
	if(len>0)
	{//将unicode编码转化为多字节编码字符集
#ifdef WIN32
		len=WideCharToMultiByte(CP_ACP,WC_COMPOSITECHECK|WC_DISCARDNS|WC_SEPCHARS|WC_DEFAULTCHAR,buf,-1,(char *)pDest,nSize,NULL,NULL);
		if(len>0) len--;//WideCharToMultiByte返回得长度包含结尾得null
#else
		len=0;
#endif
	}
	::free(buf); return len;
}
//=?charset?encoding-type?data?= 数据格式解码
//charset指明data数据的字符集，例如utf-8
//encoding-type: B或Q， base64和Quoted-Printble
int cCoder::eml_decode(const char *pSrc,unsigned int nSize,char *pDest)
{
	if(pSrc[0]=='=' && pSrc[1]=='?' && pSrc[nSize-1]=='=' && pSrc[nSize-2]=='?')
	{//判断是否需要解码
		const char *ptr=strchr(pSrc+2,'?');
		char cType=(ptr)?(*(ptr+1)):0;
		if(cType=='B' || cType=='Q'){//可识别的编码
			char *buf=(char *)::malloc(nSize*sizeof(char));
			if(buf==NULL) return 0;
			::memcpy(buf,pSrc,nSize); buf[nSize-2]=0; //去掉最后的?
			int npos=(ptr-pSrc); buf[npos]=0;
			char *lpCharset=buf+2;
			char *lpData=buf+npos+3;
			int iDataLen=strlen(lpData);
			//开始根据cType进行解码
			if(cType=='B') //base64编码
				 iDataLen=cCoder::base64_decode(lpData,iDataLen,lpData);
			else iDataLen=cCoder::quoted_decode(lpData,iDataLen,lpData);
			//开始字符编码
			if(strcasecmp(lpCharset,"utf-8")==0)
				iDataLen=cCoder::utf8_decode(lpData,iDataLen,lpData);
			strncpy(pDest,lpData,iDataLen); pDest[iDataLen]=0; 
			::free(buf); return iDataLen;
		}//?if(cType=='B' || cType=='Q')
	}//?if(pSrc[0]=='=' && pSrc[1]=='?'
	//否则不是有效或支持的编码格式，无须解码
	if(pSrc!=pDest) strncpy(pDest,pSrc,nSize); pDest[nSize]=0; return nSize;
}
//总是进行utf-8和base64编码
int cCoder::eml_encode(const char *pSrc,unsigned int nSize,char *pDest)
{
	strcpy(pDest,"=?utf-8?B?"); int npos=10;
	nSize=cCoder::utf8_encode(pSrc,nSize,(pDest+npos));
	char *buf=(char *)::malloc(nSize+1);
	if(buf==NULL) return 0;
	::memcpy(buf,pDest+npos,nSize); buf[nSize]=0;
	npos+=cCoder::base64_encode(buf,nSize,(pDest+npos));
	pDest[npos++]='?'; pDest[npos++]='='; pDest[npos]=0;
	::free(buf); return npos;
}
int cCoder::EmlEncodeSize(int iSize)
{
	iSize=cCoder::Utf8EncodeSize(iSize);
	iSize=cCoder::Base64EncodeSize(iSize);
	return iSize+12;
}

//十进制-->十六进制
inline unsigned char cCoder::DecToHex( unsigned char B)
{
	return B < 10 ? '0' + B:'A' + B - 10;
}

//------------------------------------------------------------------------------

//十六进制-->十进制
inline unsigned char cCoder::HexToDec( unsigned char C )
{
	if(C<='9') return C-'0';
	return C>='a' ? (C-'a'+10) : (C-'A'+10);
//	return C <= '9' ? C - '0' : C - 'A' + 10; //yyc remove 2007-12-20
}

inline long power(long x,long z)
{
	long lret=1;
	while(--z>=0) lret *=x;
	return lret;
}
//将16进制字符串(大写)转为数值
unsigned long cCoder::hex_atol(const char *str)
{
	//首先去掉前导空格
	while(*str==' ') str++;
	//判断有效字符长度
	const char *ptr=str; int len=0;
	while( (*ptr>='0' && *ptr<='9') || (*ptr>='A' && *ptr<='F') ){ ptr++; if(++len>=8) break; }
	unsigned long ul=0; unsigned char c;
	while(len-->0){ 
		if( (c=cCoder::HexToDec(*str++))!=0 )
			ul+=c * power(16,len);
	}
	return ul;
}

/*UTF8编码的原理： 
因为一个字母还有一些键盘上的符号加起来只用二进制七位就可以表示出来，而一个字节就是八位，所以UTF8就用一个字节来表式字母和一些键盘上的符号。然而当我们拿到被编码后的一个字节后怎么知道它的组成？它有可能是英文字母的一个字节，也有可能是汉字的三个字节中的一个字节！所以，UTF8是有标志位的！ 

当要表示的内容是7位的时候就用一个字节：0******* 第一个0为标志位，剩下的空间正好可以表示ASCII 0－127 的内容。 

当要表示的内容在8到11位的时候就用两个字节：110***** 10****** 第一个字节的110和第二个字节的10为标志位。 

当要表示的内容在12到16位的时候就用三个字节：1110***** 10****** 10****** 和上面一样，第一个字节的1110和第二、三个字节的10都是标志位，剩下的空间正好可以表示汉字。 

以此类推： 
四个字节：11110**** 10****** 10****** 10****** 
五个字节：111110*** 10****** 10****** 10****** 10****** 
六个字节：1111110** 10****** 10****** 10****** 10****** 10****** 
*/