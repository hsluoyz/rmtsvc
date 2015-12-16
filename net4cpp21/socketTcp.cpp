/*******************************************************************
   *	socketTcp.h
   *    DESCRIPTION:TCP socket 类的定义
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2004-10-10
   *	Last modify: 2005-09-02
   *	net4cpp 2.1
   *******************************************************************/

#include "include/sysconfig.h"
#include "include/socketTcp.h"
#include "include/cLogger.h"

using namespace std;
using namespace net4cpp21;

//TCP侦听，返回侦听服务端口
SOCKSRESULT socketTcp::ListenX(int port,BOOL bReuseAddr,const char *bindIP)
{
	//创建一个TCP socket句柄
	if( !create(SOCKS_TCP) ) return SOCKSERR_INVALID;
	SOCKSRESULT sr=Bind(port,bReuseAddr,bindIP);
	if(sr<=0){ Close(); return sr; }

	if( ::listen(m_sockfd, SOMAXCONN ) == SOCKET_ERROR)
	{ 
		m_errcode=SOCK_M_GETERROR; 
		Close(); 
		return SOCKSERR_LISTEN; 
	}
	m_sockstatus=SOCKS_LISTEN;
	return sr;
}
SOCKSRESULT socketTcp::ListenX(int startport,int endport,BOOL bReuseAddr,const char *bindIP)
{
	//创建一个TCP socket句柄
	if( !create(SOCKS_TCP) ) return SOCKSERR_INVALID;
	SOCKSRESULT sr=Bind(startport,endport,bReuseAddr,bindIP);
	if(sr<=0){ Close(); return sr; }

	if( ::listen(m_sockfd, SOMAXCONN ) == SOCKET_ERROR)
	{ 
		m_errcode=SOCK_M_GETERROR; 
		Close(); 
		return SOCKSERR_LISTEN; 
	}
	m_sockstatus=SOCKS_LISTEN;
	return sr;
}
//等待一个连接进来,返回当前连接的端口
//如果psock==NULL,则关闭当前侦听服务，用本socket接受此连接
//否则用指定的socketTcp接受此连接
SOCKSRESULT socketTcp::Accept(time_t lWaitout,socketTcp *psock)
{
	if( m_sockstatus!=SOCKS_LISTEN ) return SOCKSERR_INVALID;
	int fd=1;
	if(lWaitout>=0)
	{
		time_t t=time(NULL);
		while( (fd=checkSocket(SCHECKTIMEOUT,SOCKS_OP_READ))== 0 )
		{//检查句柄是否可读
			if( (time(NULL)-t)>lWaitout ) return SOCKSERR_TIMEOUT; //检查是否超时
		}//?while
	}//?if(lWaitout>=0)
	if(fd!=1)
	{
		if(fd==-1) m_errcode=SOCK_M_GETERROR;
		return fd; //发生错误
	}

	SOCKADDR_IN addr; int addrlen = sizeof(addr);
	fd=::accept(m_sockfd, (struct sockaddr *)&addr, (socklen_t *)&addrlen);
	if(fd==-1) {m_errcode=SOCK_M_GETERROR; return SOCKSERR_ERROR;} //发生系统错误
	if(psock)
		psock->m_parent=this;
	else psock=this;
	psock->Close(); 
	psock->m_sockfd=fd; 
	psock->m_socktype=SOCKS_TCP;
	psock->m_sockstatus=SOCKS_CONNECTED;
	psock->m_sockflag |=SOCKS_TCP_IN;
	memcpy((void *)&psock->m_remoteAddr,(const void *)&addr,sizeof(SOCKADDR_IN));
	return psock->getSocketInfo();//获取socket绑定的本地ip和端口
}

SOCKSRESULT socketTcp::Connect(time_t lWaitout,int bindport,const char *bindip)
{
	if(m_remoteAddr.sin_addr.s_addr==INADDR_NONE) 
		return SOCKSERR_HOST;//无效的主机IP
	//创建一个TCP socket句柄
	if( !create(SOCKS_TCP) ) return SOCKSERR_INVALID;	
	//绑定指定的ip 端口
	if(bindport>0 || (bindip!=NULL && bindip[0]!=0) ) 
		if( Bind(bindport,SO_REUSEADDR,bindip)<=0) return SOCKSERR_BIND;
	SOCKSRESULT sr=SOCKSERR_OK;
	//连接指定的主机
	if(lWaitout>=0) //设置了连接超时
	{
		setNonblocking(true); //设置socket为非阻塞方式
		sr=::connect(m_sockfd,(struct sockaddr *) &m_remoteAddr, sizeof(m_remoteAddr));
		//非阻塞模式一般不会返回0,而是返回有错误
		if(sr==SOCKSERR_OK || SOCK_M_GETERROR==WSAEWOULDBLOCK )
		{
			time_t t=time(NULL);
			while( (sr=checkSocket(SCHECKTIMEOUT,SOCKS_OP_WRITE))== 0 )
			{//检查句柄是否可写
				if( (time(NULL)-t)>(unsigned long)lWaitout ) break; //检查是否超时
			}//?while
			if(sr==0)
				sr=SOCKSERR_TIMEOUT;
			else if(sr>0)
				sr=SOCKSERR_OK;	
		}//?if(::connect(...
		else sr=SOCKSERR_CONN;
		setNonblocking(false);//恢复为阻塞方式
	}//?if(lWaitout>=0)
	else if(::connect(m_sockfd,(struct sockaddr *) &m_remoteAddr, sizeof(m_remoteAddr))!=0) //连接不成功
	{
		RW_LOG_DEBUG("Failed to connect(), error=%d\r\n",SOCK_M_GETERROR);
		sr=SOCKSERR_CONN;
	}

	if(sr==SOCKSERR_OK){
		m_sockstatus=SOCKS_CONNECTED;
		m_sockflag &=(~SOCKS_TCP_IN);
		sr=getSocketInfo();
	}//?if(sr==SOCKSERR_OK)
	else Close();
	if(sr==SOCKSERR_ERROR) m_errcode=SOCK_M_GETERROR;
	return sr;
}

//-----------------------------socketSSL--------------------------------
#ifdef _SURPPORT_OPENSSL_

static char default_cacert[]="-----BEGIN CERTIFICATE-----\n"
					"MIIDgzCCAuygAwIBAgIBADANBgkqhkiG9w0BAQQFADCBjjELMAkGA1UEBhMCQ04x\n"
					"EDAOBgNVBAgTB2JlaWppbmcxEDAOBgNVBAcTB3hpY2hlbmcxDTALBgNVBAoTBGhv\n"
					"bWUxDTALBgNVBAsTBGhvbWUxGDAWBgNVBAMTD3l5Y25ldC55ZWFoLm5ldDEjMCEG\n"
					"CSqGSIb3DQEJARYUeXljbWFpbEAyNjMuc2luYS5jb20wHhcNMDUwNDA3MDExMzI3\n"
					"WhcNMTUwNDA1MDExMzI3WjCBjjELMAkGA1UEBhMCQ04xEDAOBgNVBAgTB2JlaWpp\n"
					"bmcxEDAOBgNVBAcTB3hpY2hlbmcxDTALBgNVBAoTBGhvbWUxDTALBgNVBAsTBGhv\n"
					"bWUxGDAWBgNVBAMTD3l5Y25ldC55ZWFoLm5ldDEjMCEGCSqGSIb3DQEJARYUeXlj\n"
					"bWFpbEAyNjMuc2luYS5jb20wgZ8wDQYJKoZIhvcNAQEBBQADgY0AMIGJAoGBAOFb\n"
					"JLOA6+M4XVGS4L60ERmzzE6dIWZX3WSvsUnOsgGWEoaUBq24SKoWP5CbuX7+4awm\n"
					"N7DbBTW1TjdHV26yuo50kWnJBgkxKnwLcg+Ddhqdy3yLdlft6NsVmjd8BJ5i9GVt\n"
					"UatwiO4sTnSz2aA2vDb5esqUnJU99Y1dOiu7Uc/vAgMBAAGjge4wgeswHQYDVR0O\n"
					"BBYEFEtQsbWgZC5WSkGnybXMUVJV+jmrMIG7BgNVHSMEgbMwgbCAFEtQsbWgZC5W\n"
					"SkGnybXMUVJV+jmroYGUpIGRMIGOMQswCQYDVQQGEwJDTjEQMA4GA1UECBMHYmVp\n"
					"amluZzEQMA4GA1UEBxMHeGljaGVuZzENMAsGA1UEChMEaG9tZTENMAsGA1UECxME\n"
					"aG9tZTEYMBYGA1UEAxMPeXljbmV0LnllYWgubmV0MSMwIQYJKoZIhvcNAQkBFhR5\n"
					"eWNtYWlsQDI2My5zaW5hLmNvbYIBADAMBgNVHRMEBTADAQH/MA0GCSqGSIb3DQEB\n"
					"BAUAA4GBAGEEmmxsYUpvTIRJ3gLWp+IoDqqyldkfV9PkLhDpUePs3viTg0WkxQla\n"
					"JFlMslz/HAdZ/GPXcLsAJqeMKzWQq4EXOH3AJ8VEd089zmd8xf8n8dKID8WNovgR\n"
					"b/ko8Vo1D2Mrm2u2yTd0ZYR7NQhsUInQLIUrnznMN2ryEhoaA21A\n"
					"-----END CERTIFICATE-----\n";
static char default_cakey[] ="-----BEGIN RSA PRIVATE KEY-----\n"
					"Proc-Type: 4,ENCRYPTED\n"
					"DEK-Info: DES-EDE3-CBC,CB70670238B9A46F\n"
					"\n"
					"4VDw50tA8qVC3wHHhVirFUMIfvYsF5seRDZCi/YDhg7FykteEa/ksQxImcc83xD+\n"
					"M64Xg2H9PssLVh/cnOcB4IiZumV7vqmFD/L/DO2HdlGPmp/mCtpFeT4z0arqhiel\n"
					"fevG4xzgI1Ns8THTtHK+3oLejliFXruViaLw+Zg9mYHCPPlHD/4kORcPoptyI3hS\n"
					"CJrvsWlxTxua1VyOXpaZlYvxrYKV9Wsd0XT9BeRXbiMrW4qL1cZ2KienwukvBCKx\n"
					"E/uesEk1j4h5gc0Y2IQ69hss8cMcM1BVE9coMoWBRPWESgO1pd0EXKkqfl4wpIJJ\n"
					"C0kImYvnRHwStJ+zlDpmwWPZtGUZRkj+2pQGtWJlwkJKmSIksqqF91AOIIoMN4ql\n"
					"iWViV3ys4dH/stJGjU+Be8EnvcIyoPEZ1rrTK6QPjcjn7xiyg5PxT6zm3F2E04jj\n"
					"K+qiwj+KBbjMoUQom0IirwSPSfNVswQm3/BQ/2R/U/Ugps2Ze/AAUZ0ogVkpRZAM\n"
					"sIvPxWDayVjQ5xHuEzfe4AEYq7i+G51T+jJcDXJ+7mJPNTcuG3tMdYK2TWZeYsuO\n"
					"EfctWaw6AS7CtzsozaY3VGykOhtHewRYCQGcz0Sqn/33u+ALfaaaQ41pzs4JnBgv\n"
					"U5DI0zmatjKb5gNNG95FVF1l1hyCBx19j4npozsbvh97/uQjiI3G2+6rH7maNCil\n"
					"yBdzhUkMuVT21OtmwynHkGXzd5YhTTZ6sUaqfCCie1GfmJ5ImI8Vcqmlb6sn8Q29\n"
					"O0noKSLb9spUVIW9pqQ/kEPPodt4fpPeiFsamtwH9DEqfbNco/IVVg==\n"
					"-----END RSA PRIVATE KEY-----\n";
static char default_cakeypass[]="123456";

socketSSL :: socketSSL():
	m_ctx(NULL),m_ssl(NULL),m_ssltype(SSL_INIT_NONE)
{
	 m_bNotfile=true;
	 m_bSSLverify=false;
	 m_carootfile=""; //SSL 服务端验证客户端证书的根证书PEM文件
	 m_crlfile="";
}

socketSSL :: socketSSL(socketSSL &sockSSL) :socketTcp(sockSSL)
{
	m_ssltype=sockSSL.m_ssltype;
	m_ctx=sockSSL.m_ctx;
	m_ssl=sockSSL.m_ssl;
	m_cacert=sockSSL.m_cacert;
	m_cakey=sockSSL.m_cakey;
	m_cakeypass=sockSSL.m_cakeypass;
	m_bNotfile=sockSSL.m_bNotfile;
	m_bSSLverify=sockSSL.m_bSSLverify;
	m_carootfile=sockSSL.m_carootfile;
	m_crlfile=sockSSL.m_crlfile;

	sockSSL.m_ssltype=SSL_INIT_NONE;
	sockSSL.m_ctx=NULL;
	sockSSL.m_ssl=NULL;
}

socketSSL & socketSSL :: operator = (socketSSL &sockSSL)
{
	socketTcp::operator = (sockSSL);
	m_ssltype=sockSSL.m_ssltype;
	m_ctx=sockSSL.m_ctx;
	m_ssl=sockSSL.m_ssl;
	m_cacert=sockSSL.m_cacert;
	m_cakey=sockSSL.m_cakey;
	m_cakeypass=sockSSL.m_cakeypass;
	m_bNotfile=sockSSL.m_bNotfile;
	m_bSSLverify=sockSSL.m_bSSLverify;
	m_carootfile=sockSSL.m_carootfile;
	m_crlfile=sockSSL.m_crlfile;

	sockSSL.m_ssltype=SSL_INIT_NONE;
	sockSSL.m_ctx=NULL;
	sockSSL.m_ssl=NULL;
	return *this;
}

socketSSL :: ~socketSSL()
{
	Close();
	freeSSL();
}


//设置SSL的证书私钥密码
//bNotfile -- 指示strCaCert&strCaKey指向的是证书文件名还是证书内容
//如果bNotfile=true且strCaCert或strCaKey为空则用默认的证书和私钥
void socketSSL :: setCacert(const char *strCaCert,const char *strCaKey,const char *strCaKeypwd,
							bool bNotfile,const char *strCaRootFile,const char *strCRLfile)
{
	if( (m_bNotfile=bNotfile) && 
		(strCaCert==NULL || strCaCert[0]==0 || 
			strCaKey==NULL || strCaKey[0]==0) )
	{
		m_cacert.assign(default_cacert);
		m_cakey.assign(default_cakey);
		m_cakeypass.assign(default_cakeypass);
		m_carootfile="";
		m_crlfile="";
	}
	else
	{
		if(strCaCert) m_cacert.assign(strCaCert);
		if(strCaKey) m_cakey.assign(strCaKey);
		if(strCaKeypwd) m_cakeypass.assign(strCaKeypwd);
		if(strCaRootFile) m_carootfile.assign(strCaRootFile);
		else m_carootfile="";
		if(strCRLfile)	m_crlfile.assign(strCRLfile);
		else m_crlfile="";
	}
	//如果carootfile!=""则要求进行SSL客户端验证
	if(m_carootfile!="") m_bSSLverify=true; else m_bSSLverify=false;
	return;
}
void socketSSL :: setCacert(socketSSL *psock,bool bOnlyCopyCert)
{
	if(psock==NULL) return;
	m_cacert=psock->m_cacert;
	m_cakey=psock->m_cakey;
	m_cakeypass=psock->m_cakeypass;
	m_bNotfile=psock->m_bNotfile;
	if(!bOnlyCopyCert){
		m_bSSLverify=psock->m_bSSLverify;
		m_carootfile=psock->m_carootfile;
		m_crlfile=psock->m_crlfile;
	}else{
		m_bSSLverify=false;
		m_carootfile="";
		m_crlfile="";
	}
	return;
}

void socketSSL :: Close()
{
	if(m_ssl) SSL_shutdown (m_ssl);  // send SSL/TLS close_notify 
	socketTcp::Close();
	if(m_ssl) SSL_free(m_ssl);  
	m_ssl=NULL; return;
}

//进行SSL协商
bool socketSSL :: SSL_Associate()
{
	if(m_sockstatus!=SOCKS_CONNECTED) return false;
	if(m_ctx==NULL){
		RW_LOG_DEBUG(0,"[SSL] Must be init SSL\r\n");
		return false; 
	}
	if(m_ssl==NULL){
		if( (m_ssl = SSL_new (m_ctx))==NULL )
		{
			RW_LOG_DEBUG(0,"[SSL] failed to ssl_new()\r\n");
			return false;
		}
	}//?if(m_ssl==NULL)
	SSL_set_fd (m_ssl, m_sockfd);
	//SSL_CTX_set_timeout(m_ctx,1000); //默认为300ms
	if(m_ssltype==SSL_INIT_CLNT) //客户SSL
	{
		if(SSL_connect(m_ssl)!=-1)
		{
			/* --------------------------------------------------------------
			// Following two steps are optional and not required for
			// data exchange to be successful. 
			RW_LOG_DEBUG("SSL connection using %s\n", SSL_get_cipher (m_ssl));
			// Get server's certificate (note: beware of dynamic allocation) - opt 
			X509 *server_cert=SSL_get_peer_certificate (m_ssl);
			if(server_cert==NULL){	 
				RW_LOG_PRINT(LOGLEVEL_ERROR,0,"failed to Server certificate!\r\n");
				Close(); return SOCKSERR_SSL_ERROR; }
			char *str = X509_NAME_oneline (X509_get_subject_name (server_cert),0,0);
			if(str==NULL){ X509_free (server_cert); Close(); return SOCKSERR_SSL_ERROR; }
			OPENSSL_free (str);
			str = X509_NAME_oneline (X509_get_issuer_name  (server_cert),0,0);
			if(str==NULL){ X509_free (server_cert); Close(); return SOCKSERR_SSL_ERROR; }
			OPENSSL_free (str);
			// We could do all sorts of certificate verification stuff here before
			// deallocating the certificate.
			X509_free (m_server_cert); m_server_cert=NULL;
			// --------------------------------------------------- 
			// DATA EXCHANGE - Send a message and receive a reply. 
			// -------------------------------------------------------------*/
			return true;
		}//?if(SSL_connect(m_ssl)!=-1)
		else
			RW_LOG_DEBUG(0,"[SSL] failed to ssl_connect(),error=%d!\r\n",SSL_get_error(m_ssl,-1));
	}//?if(m_ssltype==SSL_INIT_CLNT)
	else
	{
		if(SSL_accept(m_ssl)!=-1)
		{
			RW_LOG_DEBUG("[SSL] SSL connection using %s\r\n",SSL_get_cipher (m_ssl));
			X509*    client_cert=SSL_get_peer_certificate (m_ssl);
			if (client_cert != NULL) 
			{
				RW_LOG_DEBUG(0,"[SSL] Client certificate:\r\n");
				char *str = X509_NAME_oneline (X509_get_subject_name (client_cert), 0, 0);
				if(str){
					RW_LOG_DEBUG("\t subject: %s\r\n",str);
					OPENSSL_free (str);
				}
				str = X509_NAME_oneline (X509_get_issuer_name  (client_cert), 0, 0);
				if(str){
					RW_LOG_DEBUG("\t issuer: %s\r\n",str);
					OPENSSL_free (str);
				}
				// We could do all sorts of certificate verification stuff here before
				//   deallocating the certificate. 
				X509_free (client_cert);
			} 
			else RW_LOG_DEBUG(0,"[SSL] Client does not have certificate!\r\n");
			return true;
		}//?if(SSL_accept(m_ssl)!=-1)
		else
		{
			RW_LOG_PRINT(LOGLEVEL_ERROR,"[SSL] failed to SSL_accept(),error=%d!\r\n",SSL_get_error(m_ssl,-1));
			long verify_error=SSL_get_verify_result(m_ssl);
			if (verify_error == X509_V_OK) return true;
			RW_LOG_PRINT(LOGLEVEL_ERROR,"[SSL] verify error:%s\n",X509_verify_cert_error_string(verify_error));
		}
	}//?if(...) else ...
	SSL_free(m_ssl); m_ssl=NULL;
	return false;
}

inline size_t socketSSL :: v_write(const char *buf,size_t buflen)
{
	size_t len=0;
	if(m_ssl)
		len = SSL_write (m_ssl, buf, buflen); //错误返回-1
	else
		len=::send(m_sockfd,buf,buflen,MSG_NOSIGNAL);
	return len;
}
inline size_t socketSSL :: v_read(char *buf,size_t buflen)
{
	size_t len=0;
	if(m_ssl)
		len=SSL_read (m_ssl, buf, buflen); //错误返回-1	
	else
		len=::recv(m_sockfd,buf,buflen,MSG_NOSIGNAL);
	return len;
}
//!!! SSL_peek查看SSL数据后会改变socket的可读标志，此时如果通过
//select 判断socket句柄，将永远返回不可读
inline size_t socketSSL :: v_peek(char *buf,size_t buflen)
{
	size_t len=0;
	if(m_ssl)
		len=SSL_peek (m_ssl, buf, buflen); //错误返回-1
	else
		len=::recv(m_sockfd,buf,buflen,MSG_NOSIGNAL|MSG_PEEK);
	return len;
}

//------------------------------------------------------------------
void socketSSL::freeSSL()
{
	if(m_ssltype!=SSL_INIT_NONE && m_ctx)
		SSL_CTX_free(m_ctx);
	m_ctx=NULL;
}
//初始化SSL socket。bInitServer=true则初始化服务端SSL，否则初始化客户端SSL
static int passwdcb( char * buf, int size, int rwflag, void * userdata )
{
	strcpy( buf , (const char *)userdata );
	return strlen( (const char *)userdata ); 
}
//证书验证回调，可在此回调中做其他处理...
static int verify_callback(int ok, X509_STORE_CTX *ctx)
{
	if(ok) //对于那些验证成功的证书进行CRL验证
	{
		X509 *ok_cert=X509_STORE_CTX_get_current_cert(ctx);
	}
	return ok;
}
//如果psock!=NULL则用psock的证书来初始化SSL服务端
bool socketSSL::initSSL(bool bInitServer,socketSSL *psock)
{
	if(m_ctx!=NULL) return true;
	m_ssltype=SSL_INIT_NONE;
	SSL_load_error_strings();//为打印调试信息作准备
						//如果调用了SSL_load_error_strings()后,便可以随时用ERR_print_errors_fp()来打印错误信息了
	SSLeay_add_ssl_algorithms();//初始化
	//采用什么协议(SSLv2/SSLv3/TLSv1)在此指定
	SSL_METHOD *meth=(bInitServer)?SSLv23_server_method(): //TLSv1_server_method();
								   SSLv23_client_method(); //SSLv2_client_method();
	if( (m_ctx = SSL_CTX_new (meth))==NULL ) return false;

	const char *strCacert=(psock!=NULL)?psock->m_cacert.c_str():this->m_cacert.c_str();
	const char *strCakey=(psock!=NULL)?psock->m_cakey.c_str():this->m_cakey.c_str();
	const char *strCakeypass=(psock!=NULL)?psock->m_cakeypass.c_str():this->m_cakeypass.c_str();
	bool bNotfile =(psock!=NULL)?psock->m_bNotfile:this->m_bNotfile;
	if(bInitServer && (strCacert==NULL || strCacert[0]==0) ){ //保护. 用默认的证书初始化服务端
		strCacert=default_cacert; strCakey=default_cakey;
		strCakeypass=default_cakeypass; bNotfile=true;
	}
	
	//初始化SSL加载证书(公钥)和私钥
	if(strCakeypass && strCakeypass[0]!=0){//strCakeypass!="" 如果没有指定私钥的密码，则会出现要求用户输入密码的提示
		SSL_CTX_set_default_passwd_cb(m_ctx,passwdcb);
		SSL_CTX_set_default_passwd_cb_userdata(m_ctx,(void *)strCakeypass); 
	}
	
	if(strCacert[0]!=0 && strCakey[0]!=0) //指定了证书和私钥
	{
		if(strCakeypass[0]!=0){//strCakeypass!="" 如果没有指定私钥的密码，则会出现要求用户输入密码的提示
			SSL_CTX_set_default_passwd_cb(m_ctx,passwdcb);
			SSL_CTX_set_default_passwd_cb_userdata(m_ctx,(void *)strCakeypass);
		}
		int ret=(bNotfile)? //加载证书
			SSL_CTX_use_certificate_buf(m_ctx, strCacert, SSL_FILETYPE_PEM):
			SSL_CTX_use_certificate_file(m_ctx, strCacert, SSL_FILETYPE_PEM);
		if(!(ret>0)){
			RW_LOG_PRINT(LOGLEVEL_ERROR,0,"[initSSL] 加载证书失败.\r\n");
			SSL_CTX_free (m_ctx);  m_ctx=NULL; return false;
		}
		ret=(bNotfile)? //加载私钥
			SSL_CTX_use_PrivateKey_buf(m_ctx, strCakey, SSL_FILETYPE_PEM):
			SSL_CTX_use_PrivateKey_file(m_ctx, strCakey, SSL_FILETYPE_PEM);
		if(!(ret>0)){
			RW_LOG_PRINT(LOGLEVEL_ERROR,0,"[initSSL] 加载私钥失败.\r\n");
			SSL_CTX_free (m_ctx);  m_ctx=NULL; return false;
		}
		if(!SSL_CTX_check_private_key(m_ctx)){
			RW_LOG_PRINT(LOGLEVEL_ERROR,0,"[initSSL] 密钥证书不匹配.\r\n");
			SSL_CTX_free (m_ctx);  m_ctx=NULL; return false;
		}
	}//否则如果初始化的是服务端则必须指定服务端证书和私钥，前面已经加了保护因此不必要判断
//	else if(bInitServer){ SSL_CTX_free (m_ctx); m_ctx=NULL; return false; }
	
	if(!bInitServer){ m_ssltype=SSL_INIT_CLNT; return true;}
	if(!m_bSSLverify){
		SSL_CTX_set_verify(m_ctx,SSL_VERIFY_NONE,NULL);
	}else{
		SSL_CTX_load_verify_locations(m_ctx, m_carootfile.c_str(), NULL);
		SSL_CTX_set_verify_depth(m_ctx,1);
		int mode=SSL_VERIFY_PEER|SSL_VERIFY_CLIENT_ONCE|SSL_VERIFY_FAIL_IF_NO_PEER_CERT;
		SSL_CTX_set_verify(m_ctx,mode,NULL); //verify_callback);
		//当需要客户端验证的时候，服务器把CAfile里面的可信任CA证书发往客户端.
		//如果不调用SSL_CTX_set_client_CA_list则客户端(IE)会列出所有安装的证书让用户选择
		//否则仅仅列出那些由此CA验证的证书让用户选择
		if(m_carootfile !="" )
			SSL_CTX_set_client_CA_list(m_ctx,SSL_load_client_CA_file(m_carootfile.c_str()));
		//为了从自己本身的程序中产生一个session_id，所以要给本程序设定一个session_id_context，
		//否则程序从外部获取session_id_context来得到session_id，那很容易产生错误
		//长度不能大于SSL_MAX_SSL_SESSION_ID_LENGTH
		//如果不调用SSL_CTX_set_session_id_context则默认是不启动session机制的这样就导致每次连接都会进行
		//证书验证以及握手，导致很慢(例如IE进行web访问)。此时可启用session机制，这样在第一次进行证书验证后
		//客户端会保持一个session，以后连接不必每次都进行验证协商等
		static const unsigned char s_server_session_id_context[]="yyc1234";
		SSL_CTX_set_session_id_context(m_ctx,s_server_session_id_context,sizeof(s_server_session_id_context));

		if(m_crlfile!=""){ //加载CRL列表
			X509_STORE *store=SSL_CTX_get_cert_store(m_ctx);
			X509_LOOKUP *lookup= X509_STORE_add_lookup(store, X509_LOOKUP_file());
			int iret=X509_load_crl_file(lookup, m_crlfile.c_str(), X509_FILETYPE_PEM);
			if(iret==1)
				X509_STORE_set_flags(store, X509_V_FLAG_CRL_CHECK |X509_V_FLAG_CRL_CHECK_ALL);
			else RW_LOG_DEBUG("[SSL] Failed to load CRL %s\r\n",m_crlfile.c_str());
		}
	}
	m_ssltype=SSL_INIT_SERV;
	return true;
}

#endif

/*
SOCKSRESULT socketSSL::Accept(time_t lWaitout,socketSSL *psock)
{
	SOCKSRESULT sr=socketTcp::Accept(lWaitout,psock);
	if(sr>0 && psock)
	{
		psock->m_ssltype=SSL_INIT_NONE;
		if((psock->m_ctx=this->m_ctx)==NULL)
		{//有可能psock需要初始化SSL服务端，因此将父socketSSL的证书信息复制一份给
		 //联入的psock,如果父socket已经初始化了SSL服务端则直接传递ctx对象
			psock->m_cacert=this->m_cacert;
			psock->m_cakey=this->m_cakey;
			psock->m_cakeypass=this->m_cakeypass;
			psock->m_bNotfile=this->m_bNotfile;
		}
	}//?if(sr>0 && psock)
	return sr;
}

  bool socketSSL::initSSL(bool bInitServer,socketSSL *psock)
{
	if(m_ctx!=NULL) return true;
	m_ssltype=SSL_INIT_NONE;
	SSL_load_error_strings();//为打印调试信息作准备
						//如果调用了SSL_load_error_strings()后,便可以随时用ERR_print_errors_fp()来打印错误信息了
	SSLeay_add_ssl_algorithms();//初始化
	//采用什么协议(SSLv2/SSLv3/TLSv1)在此指定
	SSL_METHOD *meth=(bInitServer)?SSLv23_server_method(): //TLSv1_server_method();
								   SSLv23_client_method(); //SSLv2_client_method();
	if( (m_ctx = SSL_CTX_new (meth))==NULL ) return false;
	if(!bInitServer){ m_ssltype=SSL_INIT_CLNT; return true;}

	const char *strCacert=(psock!=NULL)?psock->m_cacert.c_str():this->m_cacert.c_str();
	const char *strCakey=(psock!=NULL)?psock->m_cakey.c_str():this->m_cakey.c_str();
	const char *strCakeypass=(psock!=NULL)?psock->m_cakeypass.c_str():this->m_cakeypass.c_str();
	bool bNotfile =(psock!=NULL)?psock->m_bNotfile:this->m_bNotfile;

	if(strCacert==NULL || strCacert[0]==0){ //保护. 用默认的证书初始化
		strCacert=default_cacert;
		strCakey=default_cakey;
		strCakeypass=default_cakeypass;
		bNotfile=true;
	} //yyc add 2006-11-23

	//初始化SSL服务端，加载证书和私钥
	if(strCakeypass && strCakeypass[0]!=0){//strCakeypass!="" 如果没有指定私钥的密码，则会出现要求用户输入密码的提示
		SSL_CTX_set_default_passwd_cb(m_ctx,passwdcb);
		SSL_CTX_set_default_passwd_cb_userdata(m_ctx,(void *)strCakeypass); 
	}
	
	int ret=(bNotfile)?
			SSL_CTX_use_certificate_buf(m_ctx, strCacert, SSL_FILETYPE_PEM):
			SSL_CTX_use_certificate_file(m_ctx, strCacert, SSL_FILETYPE_PEM);

	if(ret>0)
	{//加载证书成功
		ret=(bNotfile)?
			SSL_CTX_use_PrivateKey_buf(m_ctx, strCakey, SSL_FILETYPE_PEM):
			SSL_CTX_use_PrivateKey_file(m_ctx, strCakey, SSL_FILETYPE_PEM);
		if(ret>0)
		{//加载私钥成功
			if(SSL_CTX_check_private_key(m_ctx))
			{
//				SSL_CTX_load_verify_locations(m_ctx, "cacert.pem", NULL);
//				SSL_CTX_set_verify_depth(m_ctx,1);
//				if(m_sslverifymode==SSL_VERIFY_PEER)
//					SSL_CTX_set_verify(m_ctx,SSL_VERIFY_PEER|SSL_VERIFY_CLIENT_ONCE);
//				else if(m_sslverifymode==SSL_VERIFY_FAIL_IF_NO_PEER_CERT)
//					SSL_CTX_set_verify(m_ctx,SSL_VERIFY_PEER|SSL_VERIFY_FAIL_IF_NO_PEER_CERT|SSL_VERIFY_CLIENT_ONCE,NULL); //
//				else 
					SSL_CTX_set_verify(m_ctx,SSL_VERIFY_NONE,NULL);

				m_ssltype=SSL_INIT_SERV;
				return true;
			}
			else
				RW_LOG_PRINT(LOGLEVEL_ERROR,0,"[initSSL] 密钥证书不匹配.\r\n");
		}//?//加载私钥成功
		else
			RW_LOG_PRINT(LOGLEVEL_ERROR,0,"[initSSL] 加载私钥失败.\r\n");
	}//加载证书成功
	else
		RW_LOG_PRINT(LOGLEVEL_ERROR,0,"[initSSL] 加载证书失败.\r\n");
	SSL_CTX_free (m_ctx); 
	m_ctx=NULL; return false;
}

*/
