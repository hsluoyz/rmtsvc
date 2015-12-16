/*******************************************************************
   *	vIDCc.h
   *    DESCRIPTION:vIDC客户类的定义
   *
   *    AUTHOR:yyc
   *	http://hi.baidu.com/yycblog/home
   *
   *******************************************************************/
   
#ifndef __YY_CVIDCC_H__
#define __YY_CVIDCC_H__

#include "../../net4cpp21/include/proxyclnt.h"
#include "../../net4cpp21/include/buffer.h"
#include "vidcdef.h"
#include "vidccdef.h"

namespace net4cpp21
{
	//vidcc客户端类
	class vidcClient : public socketProxy
	{
	public:
		explicit vidcClient(const char *strname,const char *strdesc);
		virtual ~vidcClient();
		VIDCSINFO &vidcsinfo() { return m_vidcsinfo; }
		void Destroy(); //销毁并释放资源
		//连接指定的vIDCs服务
		SOCKSRESULT ConnectSvr();
		void DisConnSvr(); //断开和vIDCs的连接
		bool mapinfoDel(const char *mapname);
		mapInfo * mapinfoGet(const char *mapname,bool bCreate);
		
		//成功返回SOCKSERR_OK
		int Mapped(const char *mapname,mapInfo *pinfo); //映射指定的服务
		int Unmap(const char *mapname,mapInfo *pinfo); //取消映射指定的服务

		void xml_list_mapped(cBuffer &buffer,VIDC_MAPTYPE maptype);
		void str_list_mapped(const char *vname,std::string &strini);
	private:
		bool sendCommand(int response_expected,const char *buf,int buflen);
		void parseCommand(const char *ptrCommand);
		static void onPipeThread(vidcClient *pvidcc);
		static void onCommandThread(vidcClient *pvidcc);
	private:
		std::map<std::string,mapInfo *> m_mapsets; //映射集合
		time_t m_lTimeout;//最大等待超时返回s
		std::string m_strName; //本vidcc的名称
		std::string m_strDesc;
		VIDCSINFO m_vidcsinfo;
		cThreadPool m_threadpool;//服务线程池

		char m_szLastResponse[VIDC_MAX_COMMAND_SIZE]; //保存最近一次从vIDCs的命令返回
	};
	
	class vidccSets
	{
	public:
		vidccSets();
		~vidccSets();
		void Destroy();
		void autoConnect();
		vidcClient *GetVidcClient(const char *vname,bool bCreate);
		bool DelVidcClient(const char *vname);
		
		bool xml_info_vidcc(cBuffer &buffer,const char *vname,VIDC_MAPTYPE maptype);
		void xml_list_vidcc(cBuffer &buffer);
		void str_list_vidcc(std::string &strini);
	private:
		cMutex m_mutex;
		//每个vidcClient对应连接一个vIDCs
		std::map<std::string,vidcClient *> m_vidccs;
		std::string m_strName; //vidcc的名称
		std::string m_strDesc;
	};

}//?namespace net4cpp21

#endif

