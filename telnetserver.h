#include "net4cpp21/include/sysconfig.h"
#include "net4cpp21/include/cCoder.h"
#include "net4cpp21/include/cLogger.h"

#include "net4cpp21/include/cTelnet.h"


using namespace std;
using namespace net4cpp21;

class cTelnetEx : public cTelnet
{
public:
	cTelnetEx(){m_cmd_prefix='#';}
	virtual ~cTelnetEx(){}
	//当有一个新的客户连接此服务触发此函数
	void Attach(socketTCP *psock) { cTelnet::onConnect(psock); }
protected:
	void onCommand(const char *strCommand,socketTCP *psock);
	bool onLogin(){ return true; }
};


class telServerEx : public socketSvr,public cTelnetEx
{
	static void revConnectionThread(socketTCP *psock);
public:
		SOCKSRESULT revConnect(const char *host,int port,time_t lWaitout=-1);
public:
	telServerEx();
	virtual ~telServerEx();
	bool Start();
	void Stop(){ Close(); }
	void docmd_sets(const char *strParam);
	void docmd_iprules(const char *strParam);
protected:
	//当有一个新的客户连接此服务触发此函数
	void onAccept(socketTCP *psock){ cTelnet::onConnect(psock); }
public:
	int m_svrport;
	std::string m_bindip;
};


