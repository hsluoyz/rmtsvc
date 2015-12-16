/*******************************************************************
   *	ftpsvr.h
   *    DESCRIPTION:FTP协议服务端声明
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-12-16
   *
   *	net4cpp 2.1
   *	文件传输协议
   *******************************************************************/

#ifndef __YY_FTP_SERVER_H__
#define __YY_FTP_SERVER_H__

#include "ftpdef.h"
#include "socketSvr.h"

namespace net4cpp21
{

	class cFtpsvr
	{
		class cFtpSession //一个ftp客户端会话信息对象
		{
			cFtpsvr *m_psvr; //指向cFtpsvr的指针
		public:
			FTPACCOUNT *m_paccount;//此session关联的帐号信息，如果登录成功则m_paccount关联指定的帐号信息
			socketTCP * m_pcmdsock;//命令传输socket
			socketTCP m_datasock;//数据传输socket
			time_t m_tmLogin;//登录时间
			char m_dataMode;//数据传输模式 S-stream C-compressed B-Block
			char m_dataconnMode; //数据传输模式 FTP_DATACONN_PORT/FTP_DATACONN_PASV
			char m_dataType;//后续数据传输使用的数据类型
					// A--ascII，E-EBCDIC文本 I-IMAGE 一系列8位字节表示的原始二进制数据
					// L-LOCAL 使用可变字节大小的原始二进制数据
			char m_sslMode;//SSL数据传输加密模式 'C' -- 不加密数据传输通道 'P'加密数据传输通道
			char m_opMode;//当前数据操作动作 
						//L--LIST m_filename指向要list的目录/文件
						//S--STOR文件上载 m_filename指向上载的文件名
						//R--RETR文件下载 m_filename指向文件下载的文件名
			std::string m_filename;//临时存储当前操作文件/目录名
			long m_startPoint;//开始下载或上载文件的起始点
							//对于LIST操作此参数指明要list目录的权限,如果为0说明m_filename为虚目录
			
			explicit cFtpSession(socketTCP *psock,cFtpsvr *psvr);
			~cFtpSession(){}
			cFtpsvr *pserver() const { return m_psvr; }
			const char *getvpath() const {return m_relativePath.c_str();}

			SOCKSRESULT setvpath(const char *vpath);//设置当前虚目录
			SOCKSRESULT getRealPath(std::string &vpath);
			SOCKSRESULT ifvpath(std::string &vpath);//是否为设置的虚目录
			void list();//List子虚目录
		private:
			std::string m_relativePath;//当前虚目录路径,!!!最后一个字符为/
			std::string m_realPath;//当前虚目录路径对应的真实路径
			long m_iAccess;//当前路径对应的操作权限

			const char *cvtRelative2Absolute(std::string &vpath);
			long cvtVPath2RPath(std::string &vpath);
		};

	public:
		cFtpsvr();
		virtual ~cFtpsvr(){}
		void setHelloTip(const char *strTip){
			if(strTip) m_helloTip.assign(strTip);
			return;
		}
		//指定FTP服务的数据传输端口的范围[startport,endport]
		//如果设为[0,0]则由系统自动随机分配端口
		//如果设为[startport,0],则分配的端口>=startport
		//如果设为[0,endport],则分配的端口<=endport
		void setDataPort(int startport,int endport)
		{
			if( (m_dataport_start=startport)< 0) 
				m_dataport_start=0;
			if( (m_dataport_end=endport)< 0) 
				m_dataport_end=0;
			return;
		}
		
	protected:
		//获取帐号信息，返回指定的帐号对象
		FTPACCOUNT *getAccount(const char *struser);
		//添加新帐号信息
		FTPACCOUNT *newAccount(const char *struser);
		SOCKSRESULT delAccount(const char *struser);
		//有一个新的客户连接此服务
		void onConnect(socketTCP *psock,time_t tmOpened,unsigned long curConnection,unsigned long maxConnection);
		//当前连接数大于当前设定的最大连接数
		void onManyClient(socketTCP *psock);
		
		virtual bool onNewTask(THREAD_CALLBACK *pfunc,void *pargs)
		{
			return false;
		}
		//扩展命令处理,如果返回真则是可识别的扩展命令，否则为不可识别的扩展命令
		virtual bool onCommandEx(socketTCP *psock,const char *strCommand
			,cFtpSession &clientSession)
		{
			return false;
		}
		virtual void onLogEvent(long eventID,cFtpSession &session)
		{
			return;
		}

	private:
		void parseCommand(cFtpSession &clientSession,socketTCP *psock
									  ,const char *ptrCommand);
		bool docmd_user(socketTCP *psock,const char *strUser,cFtpSession &clientSession);
		void docmd_type(socketTCP *psock,const char *strType,cFtpSession &clientSession);
		void docmd_cwd(socketTCP *psock,const char *strDir,cFtpSession &clientSession);
		void docmd_mkd(socketTCP *psock,const char *strDir,cFtpSession &clientSession);
		unsigned long docmd_rmd(socketTCP *psock,const char *strDir,cFtpSession &clientSession);
		unsigned long  docmd_dele(socketTCP *psock,const char *strfile,cFtpSession &clientSession);
		void docmd_rnfr(socketTCP *psock,const char *strfile,cFtpSession &clientSession);
		void docmd_rnto(socketTCP *psock,const char *strfile,cFtpSession &clientSession);
		void docmd_size(socketTCP *psock,const char *strfile,cFtpSession &clientSession);
		void docmd_rest(socketTCP *psock,const char *strRest,cFtpSession &clientSession);
		void docmd_prot(socketTCP *psock,const char *strParam,cFtpSession &clientSession);
		void docmd_list(socketTCP *psock,const char *strfile,cFtpSession &clientSession);
		void docmd_retr(socketTCP *psock,const char *strfile,cFtpSession &clientSession);
		void docmd_stor(socketTCP *psock,const char *strfile,cFtpSession &clientSession);
		void docmd_pswd(socketTCP *psock,const char *strpwd,cFtpSession &clientSession);
		void docmd_port(socketTCP *psock,char *strParam,cFtpSession &clientSession);
		
		void docmd_pasv(socketTCP *psock,cFtpSession &clientSession);
		void docmd_pwd(socketTCP *psock,cFtpSession &clientSession);
		void docmd_abor(socketTCP *psock,cFtpSession &clientSession);
		void docmd_cdup(socketTCP *psock,cFtpSession &clientSession);
		void docmd_rein(socketTCP *psock,cFtpSession &clientSession);
		void docmd_sitelist(socketTCP *psock,cFtpSession &clientSession);
		void docmd_authssl(socketTCP *psock);
		void docmd_feat(socketTCP *psock);
		void docmd_quit(socketTCP *psock);

		void resp_noLogin(socketTCP *psock);
		void resp_OK(socketTCP *psock);
		void resp_noImplement(socketTCP *psock);
		void resp_unknowed(socketTCP *psock);

	private:
		std::string m_helloTip;
		int m_dataport_start; //指定FTP服务的数据传输端口的范围
		int m_dataport_end;	//如果[0,0]则随机分配，否则按照指定的区间分配端口
		//此FTP服务的帐号信息
		std::map<std::string,FTPACCOUNT> m_accounts;

		static void dataTask(cFtpSession *psession);
	};

	class ftpServer : public socketSvr,public cFtpsvr
	{
	public:
		ftpServer();
		virtual ~ftpServer();
	private:
		//当有一个新的客户连接此服务触发此函数
		virtual void onAccept(socketTCP *psock)
		{
			cFtpsvr::onConnect(psock,m_tmOpened,curConnection(),maxConnection());
			return;
		}
		//如果当前连接数大于当前设定的最大连接数则触发此事件
		virtual void onTooMany(socketTCP *psock)
		{
			cFtpsvr::onManyClient(psock);
			return;
		}
		virtual bool onNewTask(THREAD_CALLBACK *pfunc,void *pargs)
		{
			return (m_threadpool.addTask(pfunc,pargs,THREADLIVETIME)!=0);
		}
	};
}//?namespace net4cpp21

#endif
