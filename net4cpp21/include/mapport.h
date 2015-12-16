/*******************************************************************
   *	mapport.h
   *    DESCRIPTION:端口映射服务
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:
   *
   *	net4cpp 2.1
   *	
   *******************************************************************/

#ifndef __YY_MAPPORT_H__
#define __YY_MAPPORT_H__

#include "socketSvr.h"

namespace net4cpp21
{
	
	class mapServer : public socketSvr
	{
	public:
		mapServer();
		virtual ~mapServer();
		//设置映射的应用服务
		bool mapped(const char *appsvr,int appport,int apptype);

	protected:
		//连接映射应用服务
		socketTCP * connect_mapped(std::pair<std::string,int>* &p);

		//当有一个新的客户连接此服务触发此函数
		virtual void onAccept(socketTCP *psock);
		//收到转发数据，用于数据分析处理
		virtual void onData(char *buf,long len,socketTCP *from,socketTCP *to)
		{ return; }
	private:
		static void transThread(socketTCP *psock);
		//被映射的应用服务
		std::vector<std::pair<std::string,int> > m_mappedApp;
	};
}//?namespace net4cpp21

#endif
