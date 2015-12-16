/*******************************************************************
   *	httpsvr.cpp
   *    DESCRIPTION:HTTP协议服务端实现
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:
   *
   *	net4cpp 2.1
   *	HTTP/1.1传输协议
   *******************************************************************/

#include "../include/sysconfig.h"
#include "../include/httpsvr.h"
#include "../utils/cTime.h"
#include "../utils/utils.h"
#include "../include/cLogger.h"

using namespace std;
using namespace net4cpp21;

const long httpSession::SESSION_VALIDTIME=20*60;//20分钟
const char httpSession::SESSION_IDNAME[]="aspsessionidgggggled";//"ASPSESSIONIDGGGGGLEC";

httpSession::httpSession()
{
	m_startTime=time(NULL);
	m_lastTime=m_startTime;
	//产生唯一sessionID
	srand( (unsigned)m_startTime );
	/* Display 23 大写字母. */
	for(int i = 0;   i < 23;i++ )
	{
		 m_sessionID[i]=(rand()*25)/RAND_MAX+65;
	} m_sessionID[23]=0;
}
//用户可以自己设置新的sessionID，而不用自动生成的
bool httpSession::SetSessionID(const char *strID)
{
	if(strID==NULL || strID[0]==0) return false;
	for(int i=0;i<23,*strID;i++) m_sessionID[i]=*strID++;
	m_sessionID[i]=0; return true;
}

httpServer :: httpServer()
{
	m_strSvrname="HTTP Server";
}

httpServer :: ~httpServer()
{
	Close();
	//HTTP服务析构前要保证线程都结束，因为线程中访问了httpServer类的对象
	m_threadpool.join(); 

	std::map<std::string,httpSession *>::iterator it=m_sessions.begin();
	for(;it!=m_sessions.end();it++) delete (*it).second;
}

//设置web服务的主目录以及虚目录
bool httpServer :: setvpath(const char *vpath,const char *rpath,long lAccess)
{
	if(vpath==NULL || rpath==NULL) return false;
	if(vpath[0]!='/') return false;
	std::string str_vpath(vpath);
	::_strlwr((char *)str_vpath.c_str());
	if(str_vpath[str_vpath.length()-1]!='/') str_vpath.append("/");
	std::pair<std::string,long> p(rpath,lAccess);
#ifdef WIN32
	if(p.first!="") //末尾添加'\'
		if(p.first[p.first.length()-1]!='\\') p.first.append("\\");
#endif
	m_dirAccess[str_vpath]=p;
	return true;
}

//如果当前连接数大于当前设定的最大连接数则触发此事件
void httpServer :: onTooMany(socketTCP *psock)
{
	psock->Send(0,"HTTP/1.1 200 OK\r\n\r\n"
				"<html><head><title>Too many connections</title></head>"
				"<body>Too many connections,try later!</body></html>",-1);
	return;
}

//在此事件中检查Session超时
void httpServer :: onIdle(void)
{
	static time_t checkedTime=time(NULL);
	if(checkedTime-time(NULL)>20){//20秒钟检查一次session是否超时
		checkedTime=time(NULL);
		m_mutex.lock();
		std::map<std::string,httpSession *>::iterator it=m_sessions.begin();
		for(;it!=m_sessions.end();it++)
		{
			httpSession *psession=(*it).second;
			if(!psession->isValid(checkedTime)) { delete psession; m_sessions.erase(it); }
		}
		m_mutex.unlock(); 
	}
	return;
}
//当有一个新的客户连接此服务触发此函数
void httpServer :: onAccept(socketTCP *psock)
{
	httpRequest httpreq;
	while(true){ //循环处理接收多个http请求
		SOCKSRESULT sr=httpreq.recv_reqH(psock,HTTP_MAX_RESPTIMEOUT);
		if(sr<=HTTP_REQ_UNKNOWN) break; //不是http请求或者超时
		httpResponse httprsp; httpSession *psession;//处理并获取session对象
		const char *strSessionID=httpreq.Cookies(httpSession::SESSION_IDNAME);
		m_mutex.lock();
		if(strSessionID){
			std::map<std::string,httpSession *>::iterator it=m_sessions.find(strSessionID);
			if(it!=m_sessions.end()) psession=(*it).second; else psession=NULL;
		}else psession=NULL;
		if(psession==NULL){ //设置新的sessionID
			if( (psession=new httpSession)==NULL ) return;
			m_sessions[psession->sessionID()]=psession;	
			httprsp.SetCookie(httpSession::SESSION_IDNAME,psession->sessionID(),"/");
		}//防止用户处理阻塞或者非常耗时超过了设定的session的有效时间，导致此session被删掉
		psession->m_lastTime=0x7fffffff; 
		m_mutex.unlock();
		if(!httpreq.ifReceivedAll() && httpreq.get_contentType(NULL)==HTTP_CONTENT_APPLICATION)
			if(!httpreq.recv_remainder(psock,-1)) return; //接收完剩余的form提交数据,未接收完则返回关闭
	//	RW_LOG_PRINTMAPS(httpreq.Cookies(),"Cookies");
	//	RW_LOG_PRINTMAPS(httpreq.QueryString(),"Get param");
	//	RW_LOG_PRINTMAPS(httpreq.Form(),"Post Param");

		//开始准备处理http请求
		if(!onHttpReq(psock,httpreq,*psession,m_application,httprsp)){
			string vpath=httpreq.url(); //转交web服务默认处理
			long lAccess=cvtVPath2RPath(vpath);
			if( lAccess!=HTTP_ACCESS_NONE) //将虚目录转化为实目录并获取目录的访问权限
			{
				if(vpath.length()>0 && vpath[vpath.length()-1]=='\\') 
					vpath.erase(vpath.length()-1);
				long iret=FILEIO::fileio_exist(vpath.c_str());
				if(iret==-1) //指定的文件或目录不存在
					httprsp_fileNoFind(psock,httprsp);
				else if(iret==-2) //指定的是目录,list目录中的内容
				{	
					if((lAccess & HTTP_ACCESS_LIST)==0)
						httprsp_listDenied(psock,httprsp);
					else{
						httprsp.NoCache(); //CacheControl("No-cache");
						vpath.append("\\*"); 
						httprsp_listDir(psock,vpath,httpreq,httprsp);
					}
				}else{ //指定的是文件
					//判断文件是否被修改-------------- start---------------------
					cTime ct0; time_t t0=0,t1=1;
					const char *p=httpreq.Header("If-Modified-Since");
					if(p && ct0.parseDate(p) ){
						WIN32_FIND_DATA finddata;
						HANDLE hd=::FindFirstFile(vpath.c_str(), &finddata);
						if(hd!=INVALID_HANDLE_VALUE)
						{
							FILETIME ft=finddata.ftLastWriteTime;
							::FindClose(hd); t0=ct0.Gettime();
							cTime ct1(ft);
							t1=ct1.Gettime()+_timezone;//进行时区调整						 
						}
					}//?if(p)
					if(t1<=t0) //文件没有被修改过
						httprsp_NotModify(psock,httprsp);
					else
					//判断文件是否被修改--------------  end ---------------------
					{
						long lstartpos,lendpos;//获取文件的范围
						int iRangeNums=httpreq.get_requestRange(&lstartpos,&lendpos,0);
						if(iRangeNums>1){
							long *lppos=new long[iRangeNums*2];
							if(lppos){
								for(int i=0;i<iRangeNums;i++) httpreq.get_requestRange(&lppos[i],&lppos[iRangeNums+i],i);
								httprsp.sendfile(psock,vpath.c_str(),MIMETYPE_UNKNOWED,&lppos[0],&lppos[iRangeNums],iRangeNums);
							} delete[] lppos;
						}else{
							if(iRangeNums==0) { lstartpos=0; lendpos=-1; }
							httprsp.sendfile(psock,vpath.c_str(),MIMETYPE_UNKNOWED,lstartpos,lendpos);
						}//?if(iRangeNums>1)
					}//?文件被修改过
				} //指定的是文件
			} else httprsp_accessDenied(psock,httprsp);
		}//?if(!onHttpReq(psock,httpreq,*psession,m_application,httprsp))

		psession->m_lastTime=time(NULL); //设置session的最后访问时间
		if(!httpreq.bKeepAlive()) break;
	}//?while(true)
}

//将绝对虚路径转换为绝对实路径(!!!虚路径不区分大小写)
//返回对当前实路径的可操作权限
//vpath -- [in|out] 输入绝对虚路径，输出绝对实路径
long httpServer :: cvtVPath2RPath(std::string &vpath)
{
	if(vpath=="" || vpath[0]!='/') return HTTP_ACCESS_NONE; //异常保护
	::_strlwr((char *)vpath.c_str());
	std::map<std::string,std::pair<std::string,long> >::iterator it=m_dirAccess.end();
	const char *ptr=vpath.c_str();
	const char *ptrBegin=ptr;
	do
	{	
		ptr++; char c=*ptr;
		*(char *)ptr=0;
		std::map<std::string,std::pair<std::string,long> >::iterator itTmp=m_dirAccess.find(ptrBegin);
		*(char *)ptr=c;
		if(itTmp==m_dirAccess.end()) break;
		it=itTmp;
	}while( (ptr=strchr(ptr,'/'))!=NULL );

	//实际上不可能发生这种情况,m_dirAccess总要设置根'/'
	if(it==m_dirAccess.end()) { return HTTP_ACCESS_NONE;}
 
	vpath.erase(0,(*it).first.length());
	long lAccess=(*it).second.second;
	if((lAccess & HTTP_ACCESS_SUBDIR_INHERIT)!=0)
	{
		if(strchr(vpath.c_str(),'/')!=NULL) lAccess=0; //目录权限禁止继承，则下级目录的权限为0
	}
	for(int i=0;i<vpath.length();i++) if(vpath[i]=='/') vpath[i]='\\';
	vpath.insert(0,(*it).second.first.c_str());
	return lAccess;
}
