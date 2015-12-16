/*******************************************************************
   *	sniffer.h
   *    DESCRIPTION:sniffer类声明(混杂模式)
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-12-10
   *	
   *	net4cpp 2.1
   *******************************************************************/
#ifndef __YY_SNIFFER_H__
#define __YY_SNIFFER_H__

#include "socketRaw.h"
#include "IPRules.h"
#include "cThread.h"

namespace net4cpp21
{
	class sniffer : public socketRaw
	{
	public:
		sniffer(){}
		virtual ~sniffer();
		virtual void Close();
		iprules &rules() { return m_rules;}
		void setLogfile(const char *filename){
			if(filename!=NULL && filename[0]!=0)
				m_logfile.assign(filename);
			else m_logfile="";
			return;
		}
		//是否记录ip包到文件
		bool ifLog()  const { return m_logfile!=""; }
		//打开ip记录文件并模拟sniff
		bool openLogfile(const char *logfile);

		//bindip==NULL或==""默认绑定本机第一个ip
		//否则绑定指定的ip,创建Raw socket启动sniff
		//成功返回SOCKSERR_OK，(启动sniff必须绑定IP，以便指定设置混杂模式的网卡,否则报10022错误)
		SOCKSRESULT sniff(const char *bindip);
	protected:
		//有数据到达
		virtual void onData(char *dataptr);
	private:
		std::string m_logfile; //ip包记录文件
		iprules m_rules;//过滤规则
		cThread m_thread;
		static void sniffThread(sniffer *psniffer);
		static void sniffThread_fromfile(sniffer *psniffer);
	}; 

}//?namespace net4cpp21

#endif

