/*******************************************************************
   *	ftpdef.h
   *    DESCRIPTION:定义ftp协议所用到的常量、结构以及enum的定义
   *				
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-12-16
   *	
   *	net4cpp 2.1
   *******************************************************************/

#ifndef __YY_FTPDEF_H__
#define __YY_FTPDEF_H__

#include <map>
#include <string>
#include "IPRules.h"

//FTP常量定义
#define FTP_SERVER_PORT	21 //默认FTP服务的端口
#define FTP_MAX_COMMAND_SIZE 256 //FTP命令最大字节长度
#define FTP_MAX_LOGINTIMEOUT 10 //s FTP客户端最长登录延时，超过此时间则关闭连接

//ftp返回结果常量定义
#define SOCKSERR_FTP_RESP -301 //命令响应错误
#define SOCKSERR_FTP_AUTH SOCKSERR_FTP_RESP-1 //ftp认证失败
#define SOCKSERR_FTP_SURPPORT SOCKSERR_FTP_RESP-2 //不支持的ftp密码加密传输方式
#define SOCKSERR_FTP_REST SOCKSERR_FTP_RESP-3 //该站点不支持断点续传
#define SOCKSERR_FTP_DATACONN SOCKSERR_FTP_RESP-4 //不能连接数据端口
#define SOCKSERR_FTP_LIST SOCKSERR_FTP_RESP-5 //List应答错误
#define SOCKSERR_FTP_RETR SOCKSERR_FTP_RESP-6 //RETR应答错误
#define SOCKSERR_FTP_STOR SOCKSERR_FTP_RESP-7 //Stor应答错误
#define SOCKSERR_FTP_FILE SOCKSERR_FTP_RESP-8 //文件操作错误
#define SOCKSERR_FTP_FAILED SOCKSERR_FTP_RESP-9 //一般性错误
#define SOCKSERR_FTP_DENY SOCKSERR_FTP_RESP-10 //访问拒绝
#define SOCKSERR_FTP_NOEXIST SOCKSERR_FTP_RESP-11 //目录或文件不存在
#define SOCKSERR_FTP_EXIST SOCKSERR_FTP_RESP-12 //目录或文件存在
#define SOCKSERR_FTP_USER SOCKSERR_FTP_RESP-13 //不存在的帐号
#define SOCKSERR_FTP_UNKNOWED SOCKSERR_FTP_RESP-14 //未知错误

#define FTP_DATACONN_PORT 0 //FTP数据传输模式
#define FTP_DATACONN_PASV 1

//ftp日志事件定义
#define FTP_LOGEVENT_LOGIN 1
#define FTP_LOGEVENT_LOGOUT 2
#define FTP_LOGEVENT_UPLOAD 4
#define FTP_LOGEVENT_DWLOAD 8
#define FTP_LOGEVENT_DELETE 16
#define FTP_LOGEVENT_RMD 32
#define FTP_LOGEVENT_SITE 64

//密码验证加密方式
#define OTP_NONE 0
#define OTP_MD4 1
#define OTP_MD5 2

//定义帐号类型
#define ACCOUNT_NORMAL 0
#define ACCOUNT_ADMIN 1
#define ACCOUNT_ROOT 2

typedef struct _ftpaccount //ftp帐号信息
{
	std::string m_username;//帐号,帐号不区分大小写(帐号转换为小写)
	std::string m_userpwd;//密码,如果密码==""则，无需密码验证
	std::string m_username_root;//此帐号隶属的ROOT权限用户，如果此值不为空说明此帐号是ROOT用户动态创建的
	unsigned long m_maxupratio;//最大上载速率 K/s,如果=0则不限
	unsigned long m_maxdwratio;//最大下载速率 K/s,如果=0则不限
	unsigned long m_maxupfilesize;//最大上载文件大小 KBytes ,0--不限
	unsigned long m_maxdisksize;//限制最大磁盘使用空间 KBytes,0--不限
	unsigned long m_curdisksize;//当前已使用磁盘空间 KBytes.
	std::map<std::string,std::pair<std::string,long> > m_dirAccess;//目录访问权限,目录区分大小写
			//first --- string : ftp的虚目录路径，最后以/结束。例如/ 或 /aa/，
			//second --- pair : 此ftp虚目录对应的实际目录和目录的访问权限，实际目录必须为\结尾(win平台)
	net4cpp21::iprules m_ipRules;//ip访问规则
	long m_loginusers;//当前以此帐号登录ftp服务的用户个数,只有没用用户连接时才能删除此帐号
	long m_maxLoginusers;//限制此帐号的最大同时登录用户数,<=0则不限制 
	time_t m_limitedTime;//限制此帐号只在某个日期之前有效，==0不限制
	long m_bitQX; //0~1bit 密码验证方式
				  //2~3bit 定义帐号类型
				  //4bit 是否显示隐藏文件

	bool bDsphidefiles() { return ((m_bitQX & 0x10)!=0); }
	void bDsphidefiles(bool b)
	{
		if(b)
			m_bitQX |=0x10;
		else
			m_bitQX &=0xffffffef;
		return;
	}
	long lRemoteAdmin() { return (m_bitQX & 0x0c)>>2; }
	void lRemoteAdmin(long l)
	{
		l =(l & 0x3)<<2; 
		m_bitQX &=0xfffffff3;
		m_bitQX |=l; return;
	}
	long lPswdMode() { return (m_bitQX & 0x03); }
	void lPswdMode(long l)
	{
		l &=0x3; 
		m_bitQX &=0xfffffffc;
		m_bitQX |=l; return;
	}
}FTPACCOUNT;

//FTP读写权限常量定义
#define FTP_ACCESS_FILE_READ 1 //文件读允许
#define FTP_ACCESS_FILE_WRITE 2 //文件写允许
#define FTP_ACCESS_FILE_DELETE 4 //文件删除允许
#define FTP_ACCESS_FILE_EXEC 8 //文件执行允许
#define FTP_ACCESS_DIR_LIST 16 //允许目录浏览
#define FTP_ACCESS_DIR_CREATE 32 //允许创建目录
#define FTP_ACCESS_DIR_DELETE 64 //允许删除目录
#define FTP_ACCESS_SUBDIR_INHERIT 128 //子目录继承禁止

#define FTP_ACCESS_ALL 0x7f
#define FTP_ACCESS_NONE 0x0
#endif

/*
The following are the FTP commands:

            USER <SP> <username> <CRLF>
            PASS <SP> <password> <CRLF>
            ACCT <SP> <account-information> <CRLF>
            CWD  <SP> <pathname> <CRLF>
            CDUP <CRLF>
            SMNT <SP> <pathname> <CRLF>
            QUIT <CRLF>
            REIN <CRLF>
            PORT <SP> <host-port> <CRLF>
            PASV <CRLF>
            TYPE <SP> <type-code> <CRLF>
            STRU <SP> <structure-code> <CRLF>
            MODE <SP> <mode-code> <CRLF>
            RETR <SP> <pathname> <CRLF>
            STOR <SP> <pathname> <CRLF>
            STOU <CRLF>
            APPE <SP> <pathname> <CRLF>
            ALLO <SP> <decimal-integer>
                [<SP> R <SP> <decimal-integer>] <CRLF>
            REST <SP> <marker> <CRLF>
            RNFR <SP> <pathname> <CRLF>
            RNTO <SP> <pathname> <CRLF>
            ABOR <CRLF>
            DELE <SP> <pathname> <CRLF>
            RMD  <SP> <pathname> <CRLF>
            MKD  <SP> <pathname> <CRLF>
            PWD  <CRLF>
            LIST [<SP> <pathname>] <CRLF>
            NLST [<SP> <pathname>] <CRLF>
            SITE <SP> <string> <CRLF>
            SYST <CRLF>
            STAT [<SP> <pathname>] <CRLF>
            HELP [<SP> <string>] <CRLF>
            NOOP <CRLF>

  The syntax of the above argument fields (using BNF notation
         where applicable) is:

            <username> ::= <string>
            <password> ::= <string>
            <account-information> ::= <string>
            <string> ::= <char> | <char><string>
            <char> ::= any of the 128 ASCII characters except <CR> and
            <LF>
            <marker> ::= <pr-string>
            <pr-string> ::= <pr-char> | <pr-char><pr-string>
            <pr-char> ::= printable characters, any
                          ASCII code 33 through 126
            <byte-size> ::= <number>
            <host-port> ::= <host-number>,<port-number>
            <host-number> ::= <number>,<number>,<number>,<number>
            <port-number> ::= <number>,<number>
            <number> ::= any decimal integer 1 through 255
            <form-code> ::= N | T | C
            <type-code> ::= A [<sp> <form-code>]
                          | E [<sp> <form-code>]
                          | I
                          | L <sp> <byte-size>
            <structure-code> ::= F | R | P
            <mode-code> ::= S | B | C
            <pathname> ::= <string>
            <decimal-integer> ::= any decimal integer

*/

/*
>>3.3 ssl FTP扩展
在RFC 2228中，ftp协议扩展了如下指令:
AUTH (Authentication/Security Mechanism),
ADAT (Authentication/Security Data),
PROT (Data Channel Protection Level),
PBSZ (Protection Buffer Size),
CCC (Clear Command Channel),
MIC (Integrity Protected Command),
CONF (Confidentiality Protected Command), and
ENC (Privacy Protected Command).

其中和SSL扩展相关的主要指令有以下几条:
AUTH (协商扩展验证): 指定扩展认证方法,SSL或TLS；
PBSZ (协商保护缓冲区): 制定保护缓冲区,SSL/TLS模式中必须为0；
PROT (切换保护级别): 切换保护级别，可以为"C"无保护，或"P"保护级别；
*/

/*
*****Reply Codes by Function Groups******
200 Command okay.
         500 Syntax error, command unrecognized.
             This may include errors such as command line too long.
         501 Syntax error in parameters or arguments.
         202 Command not implemented, superfluous at this site.
         502 Command not implemented.
         503 Bad sequence of commands.
         504 Command not implemented for that parameter.

110 Restart marker reply.
             In this case, the text is exact and not left to the
             particular implementation; it must read:
                  MARK yyyy = mmmm
             Where yyyy is User-process data stream marker, and mmmm
             server's equivalent marker (note the spaces between markers
             and "=").
         211 System status, or system help reply.
         212 Directory status.
         213 File status.
         214 Help message.
             On how to use the server or the meaning of a particular
             non-standard command.  This reply is useful only to the
             human user.
         215 NAME system type.
             Where NAME is an official system name from the list in the
             Assigned Numbers document.
          
         120 Service ready in nnn minutes.
         220 Service ready for new user.
         221 Service closing control connection.
             Logged out if appropriate.
         421 Service not available, closing control connection.
             This may be a reply to any command if the service knows it
             must shut down.
         125 Data connection already open; transfer starting.
         225 Data connection open; no transfer in progress.
         425 Can't open data connection.
         226 Closing data connection.
             Requested file action successful (for example, file
             transfer or file abort).
         426 Connection closed; transfer aborted.
         227 Entering Passive Mode (h1,h2,h3,h4,p1,p2).
          
         230 User logged in, proceed.
         530 Not logged in.
         331 User name okay, need password.
         332 Need account for login.
         532 Need account for storing files.

150 File status okay; about to open data connection.
         250 Requested file action okay, completed.
         257 "PATHNAME" created.
         350 Requested file action pending further information.
         450 Requested file action not taken.
             File unavailable (e.g., file busy).
         550 Requested action not taken.
             File unavailable (e.g., file not found, no access).
         451 Requested action aborted. Local error in processing.
         551 Requested action aborted. Page type unknown.
         452 Requested action not taken.
             Insufficient storage space in system.
         552 Requested file action aborted.
             Exceeded storage allocation (for current directory or
             dataset).
         553 Requested action not taken.
             File name not allowed.

*****Numeric  Order List of Reply Codes***
110 Restart marker reply.
 In this case, the text is exact and not left to the
 particular implementation; it must read:
      MARK yyyy = mmmm
 Where yyyy is User-process data stream marker, and mmmm
 server's equivalent marker (note the spaces between markers
 and "=").
120 Service ready in nnn minutes.
125 Data connection already open; transfer starting.
150 File status okay; about to open data connection.

200 Command okay.
202 Command not implemented, superfluous at this site.
211 System status, or system help reply.
212 Directory status.
213 File status.
214 Help message.
 On how to use the server or the meaning of a particular
 non-standard command.  This reply is useful only to the
 human user.
215 NAME system type.
 Where NAME is an official system name from the list in the
 Assigned Numbers document.
220 Service ready for new user.
221 Service closing control connection.
 Logged out if appropriate.
225 Data connection open; no transfer in progress.
226 Closing data connection.
 Requested file action successful (for example, file
 transfer or file abort).
227 Entering Passive Mode (h1,h2,h3,h4,p1,p2).
230 User logged in, proceed.
250 Requested file action okay, completed.
257 "PATHNAME" created.

331 User name okay, need password.
332 Need account for login.
350 Requested file action pending further information.

421 Service not available, closing control connection.
 This may be a reply to any command if the service knows it
 must shut down.
425 Can't open data connection.
426 Connection closed; transfer aborted.
450 Requested file action not taken.
 File unavailable (e.g., file busy).
451 Requested action aborted: local error in processing.
452 Requested action not taken.
 Insufficient storage space in system.

500 Syntax error, command unrecognized.
 This may include errors such as command line too long.
501 Syntax error in parameters or arguments.
502 Command not implemented.
503 Bad sequence of commands.
504 Command not implemented for that parameter.
530 Not logged in.
532 Need account for storing files.
550 Requested action not taken.
 File unavailable (e.g., file not found, no access).
551 Requested action aborted: page type unknown.
552 Requested file action aborted.
 Exceeded storage allocation (for current directory or
 dataset).
553 Requested action not taken.
 File name not allowed.

*/
