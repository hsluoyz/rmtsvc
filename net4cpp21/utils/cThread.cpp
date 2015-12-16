   /*******************************************************************
   *	cThread.cpp
   *    DESCRIPTION:线程类的实现
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2004-10-10
   *	net4cpp 2.1
   *******************************************************************/

#include "../include/sysconfig.h"  
#include "../include/cThread.h"
#include "../include/cLogger.h"

#include <vector>
using namespace std;
using namespace net4cpp21;

cThread::cThread()
{
	m_thrid=0;
	m_bStarted=false;
	m_threadfunc=NULL;
	m_pArgs=NULL;
}
cThread::~cThread()
{
	join(0); //yyc add 2009-12-15
	//线程对象释放时并不管线程是否结束，如果用户要求必须结束，自己调用join函数
#ifdef WIN32 //windows系统平台
	if(m_thrid) CloseHandle((HANDLE)m_thrid);//关闭打开的线程句柄
#else //unix/linux平台
	if(m_thrid) pthread_detach(m_thrid);//将线程分离,线程结束后自己释放相关资源
#endif
}

bool cThread::start(THREAD_CALLBACK *pfunc,void *pargs)
{
	if((m_threadfunc=pfunc)==NULL) return false;
	m_pArgs=pargs;
#ifdef WIN32 //windows系统平台
	unsigned int id;
	m_thrid=_beginthreadex(0,0,&threadfunc,(void *)this,0,&id);
	if(m_thrid==0)  return false;
#else //unix/linux平台
	int res = 0;
    	res = pthread_create(&m_thrid, 0, &threadfunc, (void *)this);
    	if (res != 0){ m_thrid=0; return false;}
#endif
	return true;
}

void cThread::join(time_t timeout)//停止线程并等待线程结束才返回
{
	if(m_thrid==0) return;
#ifdef WIN32 //windows系统平台
	DWORD dwMilliseconds=INFINITE;
	if(timeout>=0) dwMilliseconds=timeout*1000;
	int res=WaitForSingleObject((HANDLE)m_thrid, dwMilliseconds);
	if(res==WAIT_TIMEOUT) ::TerminateThread((HANDLE)m_thrid,0);
//	if(res==WAIT_TIMEOUT){printf("aaaaaaqqqq\r\n"); ::TerminateThread((HANDLE)m_thrid,0);}
	CloseHandle((HANDLE)m_thrid);
#else //unix/linux平台
	//结束所有正阻塞等待的函数；如usleep(),select()等...
	pthread_kill(m_thrid,SIGALRM);
	pthread_join(m_thrid, 0);
#endif
	m_thrid=0;
	return;
}

#ifdef WIN32 //windows系统平台
unsigned int __stdcall cThread::threadfunc(void* param)
#else //unix/linux平台
void* cThread::threadfunc(void* param)
#endif
{
	cThread *pthread=(cThread *)param;
	pthread->m_bStarted=true;
	(*(pthread->m_threadfunc))(pthread->m_pArgs);
	//yyc 2009-12-15恢复 ，使用cTread对象时必须保证cThread对象释放时线程已经结束
	pthread->m_bStarted=false; //yyc remove cThread *对象可能已经无效了
	pthread_exit(0);
	return 0;	
}

//--------------------------------------------------------------------------------------------
//---------------------cThreadPool类的实现-----------------------------------------------------
cThreadPool :: cThreadPool()
{
	m_taskid=0;
}
cThreadPool :: ~cThreadPool()
{
	join();
}
//停止线程池中的所有线程并等待线程结束才返回
void cThreadPool :: join(time_t timeout)
{
	vector<pthread_t> vec;//临时保存所有工作线程的句柄或id
	m_mutex.lock();
	m_tasklist.clear(); //清除任务列表
	if(!m_thrmaps.empty()){
		map<pthread_t,THREADPARAM>::iterator it=m_thrmaps.begin();
		for(;it!=m_thrmaps.end();it++)
		{
			THREADPARAM &thrparam=(*it).second;
			vec.push_back(thrparam.m_thrid);
			thrparam.m_thrid=0;//告诉线程可以不必从m_thrmaps中erase
			thrparam.m_waittime=0;
			thrparam.m_pcond->active();
		}
	}//?if(!m_thrmaps.empty()){
	m_mutex.unlock();
	//等待所有正执行的工作线程结束
	if(!vec.empty()){
		DWORD dwMilliseconds=INFINITE;
		if(timeout>=0) dwMilliseconds=timeout*1000;
		vector<pthread_t>::iterator itVec=vec.begin();
		for(;itVec!=vec.end();itVec++)
		{
#ifdef WIN32 //windows系统平台
			WaitForSingleObject((HANDLE)(*itVec), dwMilliseconds);
#else //unix/linux平台
			//结束所有正阻塞等待的函数；如usleep(),select()等...
			pthread_kill(*itVec,SIGALRM);
			pthread_join(*itVec, 0);
#endif		
		}//?for(;itVec...
	}//?if(!vec.empty()){
	m_thrmaps.clear();
}

//初始化工作线程个数
//threadnum --- 要新创建的线程个数
//waittime --- 新创建的线程如果休眠指定的时间后仍然没有任务处理自动结束
//		如果==-1则一直休眠知道有指定的任务要处理
//返回当前总的工作线程个数
long cThreadPool :: initWorkThreads(long threadnum,time_t waittime)
{
	for(long i=0;i<threadnum;i++)
		createWorkThread(waittime);
	long threads=0;
	m_mutex.lock();
	threads=m_thrmaps.size();
	m_mutex.unlock();
	return threads;
}
//添加一个任务进入任务队列
//pfunc --- 任务函数指针
//pargs --- 传递给任务函数的参数
//waittime --- 如果当前线程池中的线程都被占用是否临时创建一个新的线程进入线程池
//		如果<0则不创建，等待其他线程空闲后处理；否则创建，此时waittime为当此线程完成此任务在线程池中的最大休眠等待时间
//如果成功则返回任务TASKID，否则返回0
TASKID cThreadPool :: addTask(THREAD_CALLBACK *pfunc,void *pargs,time_t waittime)
{
	if(pfunc==NULL) return 0;
	TASKPARAM taskparam;
	taskparam.m_taskid=++m_taskid;
	taskparam.m_pFunc=pfunc;
	taskparam.m_pArgs=pargs;
	m_mutex.lock();
	m_tasklist.push_back(taskparam); //添加到任务列表中
	m_mutex.unlock();
	bool bStarted=false; long threads; //当前线程个数
	m_mutex.lock();
	threads=m_thrmaps.size();
	map<pthread_t,THREADPARAM>::iterator it=m_thrmaps.begin();
	for(;it!=m_thrmaps.end();it++)
	{
		THREADPARAM &thrparam=(*it).second;
		if(thrparam.m_pcond && thrparam.m_pcond->status()) //此线程正处于休眠状态
		{
			thrparam.m_pcond->active();
			bStarted=true;
			break;
		}
	}//?for(;it!=...
	m_mutex.unlock();
	if(!bStarted && waittime>=0 ){ //创建一个临时线程执行处理任务
		if(createWorkThread(waittime)==0){ //创建线程失败
			if(delTask(taskparam.m_taskid)){
				RW_LOG_PRINT(LOGLEVEL_WARN,0,"有一个任务未被执行!\r\n");
				return 0; //删除任务，如果删除不成功说明任务已被其他线程运行
			}//?if(delTask(taskparam.m_taskid))
		}
		else
			threads++;
	}//?if(!bStarted &&...
//	RW_LOG_DEBUG("taskid=%d,threads=%d\r\n",taskparam.m_taskid,threads);
	return taskparam.m_taskid;
}
//检测某个任务是否在任务列表中待执行
//如果bRemove==true则从任务列表中删除
//如果在任务列表中则返回真否则返回假
bool cThreadPool :: delTask(TASKID taskid,bool bRemove)
{
	bool bRet=false;
	m_mutex.lock();
	deque<TASKPARAM>::iterator it=m_tasklist.begin();
	for(;it!=m_tasklist.end();it++)
	{
		TASKPARAM &taskparam=*it;
		if(taskparam.m_taskid==taskid)
		{
			if(bRemove) m_tasklist.erase(it);
			bRet=true;
			break;
		}
	}//?for(;it!=...
	m_mutex.unlock();
	return bRet;
}
//清除所有待执行的任务
void cThreadPool :: clearTasks()
{
	m_mutex.lock();
	m_tasklist.clear();
	m_mutex.unlock();
	return;
}

//创建一个工作线程，成功返回线程ID，否则返回0
pthread_t cThreadPool :: createWorkThread(time_t waittime)
{
	THREADPARAM thrparam;
	pthread_t thrid=0;
	//初始化THREADPARAM
	thrparam.m_waittime=waittime;
	if( (thrparam.m_pcond=new cCond())==NULL) return 0;
	thrparam.m_thrid=0;
	//创建工作线程
	m_mutex.lock();//先锁定，这样线程运行后会先阻塞，直到解锁m_thrmaps[thrid]=thrparam;
#ifdef WIN32 //windows系统平台
	if( (thrparam.m_thrid=_beginthreadex(0,0,&workThread,(void *)this,0,(unsigned int *)&thrid))==0 )
		thrid=0;
#else //unix/linux平台
	int res = 0;
    	res = pthread_create(&m_thrid, 0, &threadfunc, (void *)this);
    	if (res != 0)
    		thrparam.m_thrid=0;
    	else
    		thrid=thrparam.m_thrid;
#endif
	//线程创建完毕后添加到线程队列中
	if(thrid!=0)
	{
		m_thrmaps[thrid]=thrparam;
		thrparam.m_pcond=NULL;
	}
	m_mutex.unlock();
	if(thrparam.m_pcond) delete thrparam.m_pcond;	
	return thrid;
}
	
//线程池中的工作线程
#ifdef WIN32 //windows系统平台
unsigned int __stdcall cThreadPool::workThread(void* param)
#else //unix/linux平台
void* cThreadPool::workThread(void* param)
#endif
{
	cThreadPool *pthreadpool=(cThreadPool *)param;
	if(pthreadpool==NULL) { pthread_exit(0); return 0; }
	pthread_t thrid=pthread_self();//得到唯一标识此线程的identifier
	THREADPARAM *pthrparam=NULL;
	pthreadpool->m_mutex.lock();
	if (pthreadpool->m_thrmaps.count(thrid)>0 )
		pthrparam=&pthreadpool->m_thrmaps[thrid];
	pthreadpool->m_mutex.unlock();
	if(pthrparam)
	{
#ifdef WIN32
		HANDLE Hthread=(HANDLE)pthrparam->m_thrid;
#endif
		cCond *pcond=pthrparam->m_pcond;
		TASKPARAM taskparam;
		do
		{//获取工作任务
NEXTTASK:		 
			taskparam.m_taskid=0;
			pthreadpool->m_mutex.lock();
			if(pthreadpool->m_tasklist.size()>0)
			{
				taskparam=pthreadpool->m_tasklist.front();
				pthreadpool->m_tasklist.pop_front();
			}
			pthreadpool->m_mutex.unlock();
			if(taskparam.m_taskid!=0)
			{
				(*(taskparam.m_pFunc))(taskparam.m_pArgs);
				goto NEXTTASK;
			}
		}while(pcond->wait(pthrparam->m_waittime));
		pthrparam->m_pcond=NULL; delete pcond;
		if(pthrparam->m_thrid!=0){
			pthreadpool->m_mutex.lock();
			pthreadpool->m_thrmaps.erase(thrid);
			pthreadpool->m_mutex.unlock();
		}
#ifdef WIN32
		CloseHandle(Hthread);//关闭打开的线程句柄
#endif
	}//?if(pthrparam->)
	//工作线程结束
	pthread_exit(0);
	return 0;	
}

/*关于 _beginthreadex和CreateThread的区别
_beginthreadex是微软的C/C++运行时库函数，CreateThread是操作系统的函数。
_beginthreadex通过调用CreateThread来实现的，但比CreateThread多做了许多工作.
CreateThread、_beginthread和_beginthreadex都是用来启动线程的
，_beginthread是_beginthreadex的功能子集，虽然_beginthread内部是调用
_beginthreadex但他屏蔽了象安全特性这样的功能，所以_beginthread与CreateThread不是同等级别，
_beginthreadex和CreateThread在功能上完全可替代，我们就来比较一下_beginthreadex与CreateThread!

CRT的函数库在线程出现之前就已经存在，所以原有的CRT不能真正支持线程，这导致我们在编程的时候
有了CRT库的选择，在MSDN中查阅CRT的函数时都有：
Libraries
LIBC.LIB Single thread static library, retail version 
LIBCMT.LIB Multithread static library, retail version 
MSVCRT.LIB Import library for MSVCRT.DLL, retail version 
这样的提示！
对于线程的支持是后来的事！
这也导致了许多CRT的函数在多线程的情况下必须有特殊的支持，不能简单的使用CreateThread就OK。
大多的CRT函数都可以在CreateThread线程中使用，看资料说只有signal()函数不可以，会导致进程终止！
但可以用并不是说没有问题！

有些CRT的函数象malloc(), fopen(), _open(), strtok(), ctime(), 或localtime()等函数需要专门
的线程局部存储的数据块，这个数据块通常需要在创建线程的时候就建立，如果使用CreateThread，
这个数据块就没有建立，然后会怎样呢？在这样的线程中还是可以使用这些函数而且没有出错，
实际上函数发现这个数据块的指针为空时，会自己建立一个，然后将其与线程联系在一起，这意味着
如果你用CreateThread来创建线程，然后使用这样的函数，会有一块内存在不知不觉中创建，遗憾的
是，这些函数并不将其删除，而CreateThread和ExitThread也无法知道这件事，于是就会有
Memory Leak，在线程频繁启动的软件中(比如某些服务器软件)，迟早会让系统的内存资源耗尽！

_beginthreadex(内部也调用CreateThread)和_endthreadex就对这个内存块做了处理，所以没有问题！
(不会有人故意用CreateThread创建然后用_endthreadex终止吧，而且线程的终止最好不要显式的调用
终止函数，自然退出最好！)

谈到Handle的问题，_beginthread的对应函数_endthread自动的调用了CloseHandle，而
_beginthreadex的对应函数_endthreadex则没有，所以CloseHandle无论如何都是要调用的不过
_endthread可以帮你执行自己不必写，其他两种就需要自己写！(Jeffrey Richter强烈推荐尽量
不用显式的终止函数，用自然退出的方式，自然退出当然就一定要自己写CloseHandle)
*/
