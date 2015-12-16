/*
**	FILENAME			IPF.h
**
**	PURPOSE				图像文件的打开，存储,格式转换
**						目前只支持BMP,JPEG两种图像格式
**
**	CREATION DATE		2003-12-24
**	LAST MODIFICATION	2005-09-22 去掉了tiff和Pix图像格式部分
**
**	AUTHOR				yyc
**
**	http://hi.baidu.com/yycblog/home
*/

#ifndef __IPF_20031224_H__
#define __IPF_20031224_H__

//----------------const define------------------
#define BMPINFOSIZE 2048
// DIBSCANLINE_WIDTHBYTES 执行DIB扫描行的DWORD对齐操作。宏参数“bits”是
// DIB信息结构中成员biWidth和biBitCount的乘积。宏的结果是经过对齐后一个
// 扫描行所占的字节数。
#define DIBSCANLINE_WIDTHBYTES(bits)    (((bits)+31)/32*4)
// DDBSCANLINE_WIDTHBYTES 执行DDB扫描行的WORD对齐操作。宏参数“bits”是
// DDB信息结构中成员bmWidth和bmBitCount的乘积。宏的结果是经过对齐后一个
// 扫描行所占的字节数。
#define DDBSCANLINE_WIDTHBYTES(bits)    (((bits)+15)/16*2)

#ifndef BI_JPEG
	#define BI_JPEG        4L
#endif

#define SWAP_INT(x,y) \
{ \
	int l=(x); \
	(x)=(y); \
	(y)=l; \
}
//少量字节复制
//#define BITSCOPY(dst,src,c) \
//	memcpy((dst),(src),(c))

#define BITSCOPY(dst,src,c) \
	{ \
		for(int n=0;n<(c);n++) \
			*((dst)+n)=*((src)+n); \
	}
//----------------------------------------------

#define IPFRESULT DWORD

class cImageF
{
public:
	//打开位图文件 -- 
	//[in] filename ---- 位图文件名
	//[out] lpbi ---- 返回位图信息,包括可能的调色板信息，用户必须保证足够大。
	//					一般来说定义BMPINFOSIZE字节足够了
	//[out] lppBits ---- 返回位图数据指针(DWORD 对齐)，用户必须保证空间足够大
	//				如果lpBits==NULL则仅仅返回位图信息
	//返回：如果失败返回0，否则返回非0(图像数据大小)
	static IPFRESULT IPF_LoadBMPFile(const char *filename,LPBITMAPINFO lpbi,LPBYTE lpBits);
	//打开JPEG文件 -- 
	//[in] filename ---- JPEG文件名
	//[out] lpbi ---- 返回位图信息
	//[out] lpBits ---- 返回位图数据指针(DWORD 对齐)，用户必须保证空间足够大
	//				如果lpBits==NULL则仅仅返回位图信息
	//返回：如果失败返回0，否则返回非0(图像数据大小)
//	static IPFRESULT IPF_LoadJPEGFile(const char *filename,LPBITMAPINFO lpbi,LPBYTE lpBits);
	//存储BMP文件 -- 
	//[in] filename ---- 目的位图文件名
	//[in] lpbi ---- 位图信息
	//[in] lpBits ---- 位图数据指针
	//返回：如果失败返回0，否则返回文件大小
//	static IPFRESULT IPF_SaveBMPFile(const char *filename,LPBITMAPINFO lpbi,LPBYTE lpBits);

	//存储JPEG文件 -- 
	//目前只支持将8位灰度图或24位真彩色
	//[in] filename ---- 目的位图文件名
	//[in] lpbi ---- 位图信息头
	//[in] lpBits ---- 位图数据指针
	//[in] quality --- jpeg压缩质量 (0~100)
	//返回：如果失败返回0，否则返回文件大小
	static IPFRESULT IPF_SaveJPEGFile(const char *filename,LPBITMAPINFOHEADER lpbih,LPBYTE lpBits,int quality);
	//lpBuf --- jpeg数据流 
	//dwSize --- jpeg数据流大小
	//返回：如果失败返回0，否则返回文件大小
	static IPFRESULT IPF_SaveJPEGFile(const char *filename,LPBYTE lpBuf,DWORD dwSize);
	//将位图数据压缩为JPEG数据流 -- 
	//目前只支持将8位灰度图或24位真彩色
	//[in] lpbih ---- 位图信息头
	//[in] lpBits ---- 位图数据指针
	//[out] dstBuf ---- 存储转换后的JPEG数据的空间,用户必须保证空间足够大
	//					一般来说分配和原位图一样大的空间即可
	//[in] quality --- jpeg压缩质量 (0~100)
	//返回：如果失败返回0，否则返回压缩成jpeg后数据的大小
	static IPFRESULT IPF_EncodeJPEG(LPBITMAPINFOHEADER lpbih,LPBYTE lpBits,LPBYTE dstBuf,int quality);
	//将jpeg数据解压缩为位图数据流 -- 
	//[in] srcBuf ---- jpeg数据指针
	//[in] dwSize ---- srcBuf指向的空间的大小
	//[out] lpbi ---- 返回位图信息
	//[out] lpBits ---- 返回位图数据指针(DWORD 对齐)，用户必须保证空间足够大
	//				如果lpBits==NULL则仅仅返回位图信息
	//返回：如果失败返回0，否则返回非0(图像数据大小)
//	static IPFRESULT IPF_DecodeJPEG(LPBYTE srcBuf,DWORD dwSize,LPBITMAPINFO lpbi,LPBYTE lpBits);

	//捕捉窗口图像 --- 24位真彩色图像
	//如果hWnd==NULL则捕捉整个屏幕
	//lpbih --- biWidth ，biHeight指定捕捉窗口的宽高,==0则捕捉窗口的宽或高
	//			biCompression指定图像数据的压缩方式，目前只支持BI_RGB(不压缩)，BI_JPEG(jpeg压缩)
	//			返回图像的信息
	//lpBits --- 保存图像数据或压缩后的图像数据
	//			如果==NULL,则仅仅返回图像数据需要的空间大小
	//quality --- 如果指定了BI_JPEG压缩则此参数指明jpeg压缩质量
	//lprc --- 指定捕捉窗口的区域==NULL则为整个窗口区域
	//失败返回0，否则返回图像数据大小
	//ifCapCursor是否捕获鼠标光标
	static IPFRESULT capWindow(HWND hWnd,LPBITMAPINFOHEADER lpbih,LPBYTE lpBits,int quality,bool ifCapCursor);

	//获取指定DIB的调色板尺寸（以字节为单位）
//	static WORD PaletteSize(LPBITMAPINFOHEADER lpbih);

};

#endif

