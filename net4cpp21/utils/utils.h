/*******************************************************************
   *	utils.h
   *    DESCRIPTION:工具函数集
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-11-30
   *	net4cpp 2.1
   *******************************************************************/
  
#ifndef __YY_CUTILS_H__
#define __YY_CUTILS_H__

namespace net4cpp21
{
	class cUtils
	{
	public:
#ifdef WIN32
		static void usleep(unsigned int us)
		{
			Sleep(us/1000);
		}
		//创建一个进程 //返回0成功
		static int execp(const char *pathfile,const char *args);
#else
		static void usleep(unsigned int us)
		{
			struct timeval to;
			//to.tv_sec = 0;
			//引该加保护，us必须>=0且小于1000000
			//为了提高效率，此保护由调用者提供
			//to.tv_usec = us;
			to.tv_sec =us/1000000L;
			to.tv_usec =us%1000000L;
			select(0, NULL, NULL, NULL, &to);
		}
#endif
		//剔除字符串的前导空格，以及尾部空格
		static const char *strTrimLeft(const char *str)
		{
			if(str) for(;*str==' ';str++) NULL;
			return str;
		}
		static char *strTrimRight(char *str)
		{
			if(str)
			{
				char *ptr=str+strlen(str)-1;
				while(ptr>=str && *ptr==' ') *ptr--='\0';
			}
			return str;
		}
		static char *strTrim(char *str)
		{
			if(str){
				for(;*str==' ';str++) NULL;
				char *ptr=str+strlen(str)-1;
				while(ptr>=str && *ptr==' ') *ptr--='\0';
			}
			return str;
		}

		//返回替换字符的个数
		static int Replace(char *str,char findC,char replaceC)
		{
			int count=0;
			for(;*str!='\0' && *str==findC;*str++=replaceC,count++) NULL;
			return count;
		}
	};

	class FILEIO
	{
	public:
#ifdef WIN32
		static bool fileio_rename(const char *filename,const char *rename)
		{
		//	char *p=(char *)rename; while(*p){ if(*p=='/') *p='\\'; p++;}
			return (::MoveFile(filename,rename))?true:false;
		}

		static bool fileio_deleteFile(const char *filename)
		{
			return (::DeleteFile(filename))?true:false;
		}
		static bool fileio_createDir(const char *spath)
		{
			return (::CreateDirectory(spath,NULL))?true:false;
		}
		static unsigned long fileio_deleteDir(const char *spath);
		
		//如果文件不存在则返回-1
		//如果指定的路径是一个目录则返回-2
		//否则返回文件大小Bytes
		static long fileio_exist(const char *spath);
		static bool fileio_exec(char *filename,bool ifHide);
#endif
	};
}//?namespace net4cpp21

#endif


/*
//二分法从一个有序数组中查找指定的值
	//返回数组下标,返回-1未找到
	//uiSize -- 数组大小, ifUp -- 指明是否为升序排列
	template<class X> 
	int findit(const X iArray[],size_t uiSize,const X& xfind,bool ifUp)
	{
		if(uiSize==0) return -1;
		int idx,lbound=0,ubound=uiSize-1;
		if(ifUp)
		{
			while((idx=(ubound-lbound))>1)
			{
				idx=lbound+idx/2;
				if(iArray[idx]>xfind){ ubound=idx; continue; }
				if(iArray[idx]<xfind){ lbound=idx; continue; }
				return idx;
			}
		}
		else
		{
			while((idx=(ubound-lbound))>1)
			{
				idx=lbound+idx/2;
				if(iArray[idx]>xfind){ lbound=idx; continue; }
				if(iArray[idx]<xfind){ ubound=idx; continue; }
				return idx;
			}
		}
		if(iArray[lbound]==xfind) return lbound;
		if(iArray[ubound]==xfind) return ubound;
		return -1;
	}

	//如何判断一个数是2的指数倍
	bool if2(long num)
	{
		return ((num-1) & num)==0);
	}

  //冒泡法排序
//最简单的排序方法是冒泡排序方法。这种方法的基本思想是，将待排序的元素看作是竖着排列的“气泡”，
//较小的元素比较轻，从而要往上浮。在冒泡排序算法中我们要对这个“气泡”序列处理若干遍。
//所谓一遍处理，就是自底向上检查一遍这个序列，并时刻注意两个相邻的元素的顺序是否正确。
//如果发现两个相邻元素的顺序不对，即“轻”的元素在下面，就交换它们的位置。显然，处理
//一遍之后，“最轻”的元素就浮到了最高位置；处理二遍之后，“次轻”的元素就浮到了次高位置。
//在作第二遍处理时，由于最高位置上的元素已是“最轻”元素，所以不必检查。一般地，第i遍处理时，
//不必检查第i高位置以上的元素，因为经过前面i-1遍的处理，它们已正确地排好序	
template <class X>
void Sort(X iArray[],size_t lbound,size_t ubound)
{
	X tmp; 
	bool bSwap;
	for(size_t j=ubound;j>lbound;j--)
	{
		bSwap=false;
		for(size_t i=lbound;i<j;i++)
		{
			if(iArray[i]>iArray[i+1])
			{
				tmp=iArray[i];
				iArray[i]=iArray[i+1];
				iArray[i+1]=tmp;
				bSwap=true;
			}
		}
		if(!bSwap) break;
	}
	return;
}

//快速排序算法的基本思想：
//快速排序的基本思想是基于分治策略的。对于输入的子序列ap..ar，如果规模足够小则直接进行排序，否则分三步处理：
//
//分解(Divide)：将输入的序列ap..ar划分成两个非空子序列ap..aq和aq+1..ar，使ap..aq中任一元素的值不大于aq+1..ar中任一元素的值。
//递归求解(Conquer)：通过递归调用快速排序算法分别对ap..aq和aq+1..ar进行排序。
//合并(Merge)：由于对分解出的两个子序列的排序是就地进行的，所以在ap..aq和aq+1..ar都排好序后不需要执行任何计算ap..ar就已排好序。
//这个解决流程是符合分治法的基本步骤的。因此，快速排序法是分治法的经典应用实例之一。
template <class X>
void quickSort(X v[],int n)
{
	if(n<=1)  return;
	int i,last = 0;
	X temp;
    for(i=1; i<n; i++)
    {
         if(v[i] < v[0] && (++last!=i) )
         {
				temp = v[last];
				v[last] = v[i];
				v[i] = temp;
         }
    }
	temp = v[last];
	v[last] = v[0];
	v[0] = temp;

    quickSort(v, last);    //recursively srot
    quickSort(v+last+1, n-last-1);    //each part
}
*/


