/*******************************************************************
   *	cTelnet.cpp
   *    DESCRIPTION:Telnet for windows类的实现
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2004-10-10
   *	net4cpp 2.1
   *******************************************************************/


#include "../include/sysconfig.h"
#include "../include/cTelnet.h"
#include "../utils/cCmdShell.h"
#include "../include/cLogger.h"

using namespace std;
using namespace net4cpp21;


cTelnet :: cTelnet()
{
	m_telHello="";
	m_telTip="yyc>";
	m_bTelAuthentication=false;
	m_telClntnums=0;
	m_cmd_prefix=0;
}

//设置访问帐号,如果user==NULL则此服务无需授权访问
//否则需要授权访问
void cTelnet::setTelAccount(const char *user,const char *pwd)
{
	//访问此代理无需授权
	if(user==NULL){ m_bTelAuthentication=false; return; }
	m_telUser.assign(user);
	if(pwd) m_telPwd.assign(pwd);
	m_bTelAuthentication=true;
	return;
}

void send_LArrow(socketTCP *psock){
	char buf[4]; buf[3]=0;
	buf[0]=27; buf[1]='[';buf[2]='D'; 
	psock->Send(3,buf,-1);
}
void send_RArrow(socketTCP *psock){
	char buf[4]; buf[3]=0;
	buf[0]=27; buf[1]='[';buf[2]='C'; 
	psock->Send(3,buf,-1);
}
void send_Clear(socketTCP *psock){
	char buf[5]; buf[4]=0;
	buf[0]=27; buf[1]='[';buf[2]='2'; buf[3]='J'; 
	psock->Send(4,buf,-1);
}
//如果没有错误发生则返回真
bool cTelnet::getInput(socketTCP *psock,string &strRet,int bEcho,int timeout)
{
	bool bret=false; time_t t=time(NULL);
	int bESC=0; //=0没有接收到ESC =1接收到ESC =2接收到ESC且紧跟着接收了一个[
	int curpos=-1;//字符的当前位置
	while(psock->status()==SOCKS_CONNECTED)
	{
		int iret=psock->checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ);
		if(iret<0) break;
		if(iret==0){ if(timeout>=0 && (time(NULL)-t)>timeout)break; else continue; }
		
		char buf[2]; buf[1]=0;
		if( (iret=psock->Receive(buf,1,-1))<=0 ) break;
// yyc add 2005-06-113 //解析ESC序列
		if(bESC){//解析ESC序列 //27[ESC] 91[[] 67[D] 后退 27[ESC] 91[[] 67[C] 前进
			if(bESC==1) bESC=(buf[0]=='[')?2:0;
			else if(bESC==2){
				if(buf[0]=='D' && strRet.length()>0 ){ 
					if(curpos==-1) curpos=strRet.length();
					if(curpos>0){ curpos--; send_LArrow(psock); } 
				}//?if(buf[0]=='D'){
				else if(buf[0]=='C' && strRet.length()>0){
					if(curpos==-1) curpos=strRet.length();
					if(curpos<strRet.length()){ curpos++; send_RArrow(psock); } 
				}//?else if(buf[0]=='C') 
			bESC=0; }//?else if(bESC==2)
		}else{ // yyc add 2005-06-113 //解析ESC序列 end.................
			if(buf[0]=='\b'){ //收到一个退格建
				if(curpos==-1) curpos=strRet.length();
				if(curpos>0){ strRet.erase(--curpos); 
					if(bEcho==0) psock->Send(2," \b",-1);
					else psock->Send(3,"\b \b",-1);  } 	
				else send_RArrow(psock);
			}//?if(buf[0]=='\b'){ //收到一个退格建
			else if(buf[0]==0x03){ //Ctrl+C
				strRet.assign("\x03");
				bret=true; break;
			}//?else if(buf[i]=='\x03'){ //Ctrl+C
			else if(buf[0]=='\n'){ //收到一个换行符号
				bret=true; break;
			}//?else if(buf[i]=='\r'){ //收到一个换行符号
			else if(buf[0]!='\r') { //如果=='\r'则eat it，什么都不作
				if(buf[0]==27) bESC=1;
				else if(curpos>=0 && curpos<strRet.length())
					strRet[curpos++]=buf[0];
				else{ strRet.append(buf); curpos=strRet.length(); }
				if(bEcho){ if(bEcho>0) buf[0]=bEcho; psock->Send(1,buf,-1); }
			}//?else if(buf[0]!='\n')
		}//?if(bESC)...else
	}//?while(...
	return bret;
}

void cmdoutThread(std::pair<cCmdShell *,socketTCP *> *pp)
{
	if(pp==NULL) return;
	cCmdShell *pshell=pp->first;
	socketTCP *psock=pp->second;
	if(pshell==NULL || psock==NULL) return;
	char buf[1024];
	while(psock->status()==SOCKS_CONNECTED)
	{
		long iret=pshell->Read(buf,1024);
		if(iret<0) break;
		if( psock->Send(iret,buf,-1)<0) break;
	}//?while(...
	delete pp; return;
}

void cTelnet::onConnect(socketTCP *psock)
{
	if(m_bTelAuthentication && (psock->getFlag() & SOCKS_TCP_IN) ) 
	{   Sleep(300);//要求认证,但反向连出的无需认证
		for(int count=0;count<3;count++){
			int iret=psock->Send(10,"\r\nloguser:",-1);
			string strUser,strPwd;
			if(!getInput(psock,strUser,0,10)){
				psock->Send(15,"\r\n - Timeout!\r\n",-1);
				return;
			}
			psock->Send(11,"\r\npassword:",-1);
			if(!getInput(psock,strPwd,0 /* '*' */,10)){
				psock->Send(15,"\r\nTimeout!\r\n",-1);
				return;
			}
			if( (strUser==m_telUser && strPwd==m_telPwd) )break;

			if(count<2)//输入帐号或密码不正确
				psock->Send(48,"\r\nuser or password is wrong,please try again! \r\n",-1);
			else{
				psock->Send(34,"\r\nTry too much,to be disconnect.\r\n",-1); return; }
		}//for(... 最多登录三次
	}//?if(m_bAuthentication)
	
	RW_LOG_DEBUG(0,"[Telnet] one telnet-client is connnected.\r\n");
	m_telClntnums++;  Sleep(300);
	psock->Send(2,"\r\n",-1); //发送欢迎提示消息
	psock->Send(m_telHello.length(),m_telHello.c_str(),-1);
	psock->Send("/////Support specific commands ://///////"
				"\r\n cls       -- clear screen"
				"\r\n echo on   -- turn echo on"
				"\r\n echo off  -- turn echo off\r\n"
				"/////%d users have been connecting//////\r\n", m_telClntnums);
	
	cCmdShell cmdShell; cThread thread;
	if( onLogin() ){//创建cmd shell
		pair<cCmdShell *,socketTCP *> *pp=new pair<cCmdShell *,socketTCP *>(&cmdShell,psock);
		if(pp==NULL || !cmdShell.create() || 
			!thread.start((THREAD_CALLBACK *)&cmdoutThread,(void *)pp) )
		{
			cmdShell.destroy(); delete pp;
			m_telClntnums--; return;
		}else Sleep(200); //暂时休眠200ms，否则thread.status()返回状态可能不对
	}//?if( onLogin() )
	int bEcho=0; //默认关闭echo，设为-1默认打开echo 
	string strInput,strOutput;
	socketBase *psvr=psock->parent();
	while( psvr && psvr->status()!=SOCKS_CLOSED )
	{
		//如果没有重定向到cmd shell则输出提示符
		if(!thread.status()) psock->Send(m_telTip.length(),m_telTip.c_str(),-1);
		strInput="";//获取用户的输入
		if(!getInput(psock,strInput,bEcho,-1)) break;
		if(strInput=="echo off"){
			bEcho=0;strInput="";
		}
		else if(strInput=="echo on"){
			bEcho=-1; strInput="";
		}
		else if(strInput=="cls"){
			send_Clear(psock); strInput="";
		}
		else if(strInput[0]==0x3){//收到Ctrl+C命令
			cmdShell.sendCtrlC(); //模拟Ctrl+C;
			continue;
		}

		if(thread.status()) //重定向到cmd shell
		{
			if(bEcho!=0 && strInput.length()>0){//删除用户输入的回显
				string s; s.resize(strInput.length(),'\b');
				psock->Send(s.length(),s.c_str(),-1);
			}//?if(bEcho!=0 && (strInput.length()-2)>0){
			if(m_cmd_prefix && m_cmd_prefix==strInput[0]) //扩展命令
			{
				strOutput=strInput; strOutput.append("\r\n");
				psock->Send(strOutput.length(),strOutput.c_str(),-1);
				onCommand(strInput.c_str()+1,psock);//交给派生类处理用户输入
				strInput.assign("\r\n");
			}else strInput.append("\r\n");
			if( cmdShell.Write(strInput.c_str(),strInput.length())< 0) break;
		}else{
			strOutput=strInput; strOutput.append("\r\n");
			psock->Send(strOutput.length(),strOutput.c_str(),-1);
			onCommand(strInput.c_str(),psock);//交给派生类处理用户输入
		}
	}//?while(...
	RW_LOG_DEBUG(0,"[Telnet] one telnet-client is closing.\r\n");
	cmdShell.destroy();thread.join(); m_telClntnums--;
	RW_LOG_DEBUG(0,"[Telnet] one telnet-client closed.\r\n");
}

//------------------------------------------------------------
telServer :: telServer()
{
	m_strSvrname.assign("Telnet Server");
}
telServer :: ~telServer()
{
	 Close();
	 m_threadpool.join();
}


