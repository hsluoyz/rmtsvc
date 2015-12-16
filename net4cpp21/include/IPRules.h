/*******************************************************************
   *	IPRules.h
   *    DESCRIPTION: IP应用规则过滤类
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2004-10-10
   *	net4cpp 2.1
   *******************************************************************/
  
#ifndef __YY_IPRULES_H__
#define __YY_IPRULES_H__

#include <vector>
namespace net4cpp21
{
	typedef enum //socket句柄的类型
	{
		RULETYPE_TCP=1,
		RULETYPE_UDP=2,
		RULETYPE_ICMP=4,
		RULETYPE_ALL=7
	}RULETYPE;
	typedef struct _IPRule
	{
		unsigned long IPAddr_src;
		unsigned long IPMask_src;
		unsigned short port_src;
		
		unsigned long IPAddr_to;
		unsigned long IPMask_to;
		unsigned short port_to;

		RULETYPE ruleType;
		bool bEnabled;
		bool bBidirection; //是否双向验证

		struct _IPRule():IPAddr_src(INADDR_NONE),IPMask_src(INADDR_NONE),port_src(0),
			IPAddr_to(INADDR_ANY),IPMask_to(INADDR_ANY),port_to(0),
			ruleType(RULETYPE_ALL),bEnabled(true),bBidirection(false){}
	}IPRule;
	class iprules
	{
		bool m_defaultEnabled;
		std::vector<IPRule> m_rules;
	public:
		iprules():m_defaultEnabled(true){}
		~iprules(){};
		bool getDefaultEnable() const { return m_defaultEnabled; }
		void setDefaultEnabled(bool b) { m_defaultEnabled=b; }
		//返回规则条数
		int rules() const { return m_rules.size(); }
		//返回某条IP过滤规则
		IPRule &rules(int i) { return m_rules[i]; }
		bool check(unsigned long ip,unsigned short port,RULETYPE rt);
		bool check(unsigned long ip_fr,unsigned short port_fr,
					unsigned long ip_to,unsigned short port_to,RULETYPE rt);
		bool check(const char *hostip,unsigned short port,RULETYPE rt)
		{
			if(m_rules.empty()) return m_defaultEnabled;
			unsigned long ip=inet_addr(hostip);
			return check(ip,port,rt);
		}
		bool check(const char *hostip_fr,unsigned short port_fr,
				   const char *hostip_to,unsigned short port_to,
					RULETYPE rt)
		{
			if(m_rules.empty()) return m_defaultEnabled;
			unsigned long ip_fr=inet_addr(hostip_fr);
			unsigned long ip_to=inet_addr(hostip_to);
			return check(ip_fr,port_fr,ip_to,port_to,rt);
		}
		bool addRule(const char *strRule);
		long addRules(const char *strRules);
		bool addRule_new(const char *strRule);
		long addRules_new(const char *strRules);
		long addRules_new(RULETYPE rt,int ipaccess,const char *strIP);
	};
}//?namespace net4cpp21

#endif

