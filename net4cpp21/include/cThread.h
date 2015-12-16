/*******************************************************************
   *	cTthread.h
   *    DESCRIPTION:线程类
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2004-10-10
   *	net4cpp 2.1
   *******************************************************************/
  
#ifndef __YY_CTHREAD_H__
#define __YY_CTHREAD_H__

#ifdef WIN32 //windows系统平台
	// 当使用此多线程类时，This program requires the multithreaded library. 
	// 用vc编译时要指定 /MT或/MD参数
	// /MT表示同C运行库的多线程版静态链接。如果要动态链接，用/MD。
	//		如果VC++是v5.0的话并且有高版本的msvcrt.dll，应该用动态链接。
	// If you are using the Visual C++ development environment, select the 
   	// Multi-Threaded runtime library in the compiler Project Settings dialog box.	
   	// The Multithread Libraries Compile Option
   	// To build a multithread application that uses the C run-time libraries, 
   	// you must tell the compiler to use a special version of the libraries (LIBCMT.LIB).
   	// To select these libraries, first open the Project Settings dialog box (Build menu) 
   	// and click the C/C++ tab. Select Code Generation from the Category drop-down list box. 
   	// From the Use Run-Time Library drop-down box, select Multithreaded. Click OK to return to editing.
	extern "C"
	{//多线程
		#include <process.h>
	}
	#define pthread_t unsigned long
	#define pthread_self GetCurrentThreadId
	#define pthread_exit(retcode) \
	{ \
		_endthreadex(retcode); \
	}
#elif defined MAC //暂时不支持
	//....
#else  //unix/linux平台
	//为了保证可移植，此处的多线程函数用的是POSIX的多线程库，因此编译时应指定连接 -lpthread库。如果用sun系统本身的多线程函数则应制定连接-lthread库。
	//关于STL的多线程安全
	//在sun的系统下编写多线程程序时，如果用了c++的STL库，则要小心，因为你会发现在sun下有可能会发生core dump，而同样的程序在linux下缺始终没有问题。
	//用gdb定位core dump位置，发现可能会在你程序的任何地方，但总在STL库的stl_alloc.h的内存分配处发生问题，某个地址值被修改为一个非法的地址导致出
	//问题，当然这也有可能是由于你的内存非法操作的问题，如释放了某个内存空间却还在引用此地址。查查stl_alloc.h文件你会发现原来STL的内存分配默认为
	//single_pthread模式，你必须定义某个宏才会支持多线程模式，包括POSIX多线程、win32多线程，Solaris多线程等。
	//因此sun系统下编写POSIX标准的多线程程序，如果用到了STL库，则在编译时一定要指定-D__STL_PTHREADS宏参数，
	//linux下可能默认是以多线程安全方式编译的，因此不会出问题。其他的多线程宏定义你可看看stl_alloc.h头文件。
	//多线程
	extern "C"
	{ //多线程函数,互斥锁
		#include <pthread.h>
	}
#endif

#include "cMutex.h"
#include <map>
#include <deque>

#define THREADLIVETIME 5 //s ,临时线程在多长时间内没有任务则自动结束
namespace net4cpp21
{
	typedef void (THREAD_CALLBACK)(void *);
	class cThread
	{
		pthread_t m_thrid;//线程ID或线程句柄(windows)
		THREAD_CALLBACK *m_threadfunc;
		void *m_pArgs;//传递给线程函数的参数
		bool m_bStarted;//线程是否正在运行
	private:
#ifdef WIN32 //windows系统平台
    	static unsigned int __stdcall threadfunc(void* param);
#else //unix/linux平台
        static void* threadfunc(void* param);
#endif
	public:
		cThread();
		~cThread();
		bool start(THREAD_CALLBACK *pfunc,void *pargs);
		void join(time_t timeout=-1);//停止线程并等待线程结束才返回,timeout :s等待结束的秒
		bool status(){return m_bStarted;} //返回当前线程是否运行
	};
	
	//*******************************************************************************
	//线程池类************************************************************************
	//可动态创建多个线程进入线程池，线程池的线程结束一个任务后自动取任务列表中的任务进行执行
	#define TASKID unsigned long
	typedef struct _TASKPARAM //任务参数结构
	{
		TASKID m_taskid;//唯一标识本任务的任务ID
		THREAD_CALLBACK *m_pFunc;//任务函数入口指针
		void *m_pArgs;//传递给任务的参数
	}TASKPARAM;
	typedef struct _THREADPARAM //线程参数结构
	{
		pthread_t m_thrid;//线程ID或线程句柄(windows)
		time_t m_waittime;//线程休眠等待时间
		cCond *m_pcond;
	}THREADPARAM;
	class cThreadPool
	{
		TASKID m_taskid;//任务的唯一标识
		cMutex m_mutex;//互斥锁
		std::deque<TASKPARAM> m_tasklist;//任务列表
		std::map<pthread_t,THREADPARAM> m_thrmaps;//线程队列
	public:
		cThreadPool();
		~cThreadPool();
		void join(time_t timeout=-1);
		//初始化工作线程个数
		//threadnum --- 要新创建的线程个数
		//waittime --- 新创建的线程如果休眠指定的时间后仍然没有任务处理自动结束
		//		如果==-1则一直休眠知道有指定的任务要处理
		//返回当前总的工作线程个数
		long initWorkThreads(long threadnum,time_t waittime=-1);
		//添加一个任务进入任务队列
		//pfunc --- 任务函数指针
		//pargs --- 传递给任务函数的参数
		//waittime --- 如果当前线程池中的线程都被占用是否临时创建一个新的线程进入线程池
		//		如果<0则不创建，等待其他线程空闲后处理；否则创建，此时waittime为当此线程完成此任务在线程池中的最大休眠等待时间
		//如果成功则返回任务TASKID，否则返回0
		TASKID addTask(THREAD_CALLBACK *pfunc,void *pargs,time_t waittime);
		//检测某个任务是否在任务列表中待执行
		//如果bRemove==true则从任务列表中删除
		//如果在任务列表中则返回真否则返回假
		bool delTask(TASKID taskid,bool bRemove=true);
		//清除所有待执行的任务
		void clearTasks();
		//返回当前待执行的任务数
		long numTasks(){ long lret=0; m_mutex.lock(); lret=m_tasklist.size();m_mutex.unlock();return lret;}
		//返回当前的工作线程个数
		long numThreads(){ long lret=0; m_mutex.lock(); lret=m_thrmaps.size();m_mutex.unlock();return lret;}
	private:
		//创建一个工作线程，成功返回线程ID，否则返回0
		pthread_t createWorkThread(time_t waittime);
#ifdef WIN32 //windows系统平台
    	static unsigned int __stdcall workThread(void* param);
#else //unix/linux平台
        static void* workThread(void* param);
#endif
	};
}//?namespace net4cpp21


#endif





