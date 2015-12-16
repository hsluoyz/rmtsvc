/*******************************************************************
   *	httpdef.h
   *    DESCRIPTION:定义http协议所用到的常量、结构以及enum的定义
   *				
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2006-02-08
   *	
   *	net4cpp 2.1
   *******************************************************************/

#ifndef __YY_HTTPDEF_H__
#define __YY_HTTPDEF_H__


//HTTP常量定义
#define HTTP_SERVER_PORT	80 //默认HTTP服务的端口
#define HTTPS_SERVER_PORT	443 //默认HTTP SSL服务的端口
#define HTTP_MAX_RESPTIMEOUT 10 //http数据接收响应最大延迟时间

//HTTP返回结果常量定义
#define SOCKSERR_HTTP_RESP -301 //命令响应错误
#define SOCKSERR_HTTP_SENDREQ SOCKSERR_HTTP_RESP-1 //编码发送http请求失败


//定义HTTP请求类型
typedef enum
{
	HTTP_REQ_UNKNOWN   = 0,
	HTTP_REQ_POST      = 1,
	HTTP_REQ_GET       = 2,
	HTTP_REQ_HEAD      = 3,
	HTTP_REQ_PUT       = 4,
	HTTP_REQ_LINK      = 5,
	HTTP_REQ_DELETE    = 6,
	HTTP_REQ_UNLINK    = 7,	
	HTTP_REQ_CONNECT   = 8,
}HTTPREQ_TYPE;

//定义HTTP请求的content的类型
typedef enum
{
	HTTP_CONTENT_APPLICATION=0, //form提交数据
	HTTP_CONTENT_TEXTXML=1, //xml内容
	HTTP_CONTENT_MULTIPART=2, //
	HTTP_CONTENT_UNKNOWED=3
}HTTPREQ_CONTENT_TYPE;

//http认证类型
typedef enum HttpAuthorization
{
	HTTP_AUTHORIZATION_ANONYMOUS = 0, //匿名访问
	HTTP_AUTHORIZATION_PLAINTEXT = 1,
}HTTPAUTH_TYPE;  

//http响应的MIME类型
typedef enum 
{
MIMETYPE_HTML=0,
MIMETYPE_XML,
MIMETYPE_TEXT,
MIMETYPE_CSS,
MIMETYPE_ZIP,
MIMETYPE_WORD,
MIMETYPE_OCTET,
MIMETYPE_ICON,
MIMETYPE_BMP,
MIMETYPE_GIF,
MIMETYPE_PNG,
MIMETYPE_JPG,
MIMETYPE_AVI,
MIMETYPE_ASF,
MIMETYPE_MPEG,
MIMETYPE_PDF,
MIMETYPE_MHT,
MIMETYPE_UNKNOWED,
MIMETYPE_NONE		//sendfile时不再设定Content-Type,由用户自己设定
}MIMETYPE_ENUM;

//HTTP服务虚目录权限常量定义
#define HTTP_ACCESS_READ 1 //读取
#define HTTP_ACCESS_WRITE 2 //写入
#define HTTP_ACCESS_EXEC 4 //可执行
#define HTTP_ACCESS_LIST 8 //目录浏览
#define HTTP_ACCESS_VHIDE 64 //显示隐藏文件或目录
#define HTTP_ACCESS_SUBDIR_INHERIT 128 //子目录继承禁止

#define HTTP_ACCESS_ALL 0x7f
#define HTTP_ACCESS_NONE 0x0
#endif

/*
100 Continue：初始的请求已经接受，客户应当继续发送请求的其余部分。 
　　101tching Protocols：服务器将遵从客户的请求转换到另外一种协议 
　　200 OK：一切正常，对GET和POST请求的应答文档跟在后面。如果不用SetStatus设置状态代码，Servlet默认使用202状态代码。 
　　201 Created 服务器已经创建了文档，Location头给出了它的URL。 
　　202 Accepted：已经接受请求，但处理尚未完成。 
　　203 Non-Authoritative Information：文档已经正常地返回，但一些应答头可能不正确，因为使用的是文档的拷贝。 
　　204 No Content：没有新文档，浏览器应该继续显示原来的文档。如果用户定期地刷新页面，而Servlet可以确定用户文档足够新，这个状态代码是很有用的。 
　　205 Reset Content：没有新的内容，但浏览器应该重置它所显示的内容。用来强制浏览器清除表单输入内容。 
　　206 Partial Content：客户发送了一个带有Range头的GET请求，服务器完成了它。 
　　300 Multiple Choices：客户请求的文档可以在多个位置找到，这些位置已经在返回的文档内列出。如果服务器要提出优先选择，则应该在Location应答头指明。 
　　301 Moved Permanently：客户请求的文档在其他地方，新的URL在Location头中给出，浏览器应该自动地访问新的URL。 
　　302 Found：类似于301，但新的URL应该被视为临时性的替代，而不是永久性的。注意，在HTTP1.0中对应的状态信息是“Moved Temporatily”，而HttpServletResponse中相应的常量是SC_MOVED_TEMPORARILY，而不是SC_FOUND。出现该状态代码时，浏览器能够自动访问新的URL，因此它是一个很有用的状态代码。为此，Servlet提供了一个专用的方法，即sendRedirect。使用response.sendRedirect(url)比使用response.setStatus(response.SC_MOVED_TEMPORARILY)和response.setHeader("Location",url)更好。注意这个状态代码有时候可以和301替换使用。　　 
　　303 See Other：类似于301/302，不同之处在于，如果原来的请求是POST，Location头指定的重定向目标文档应该通过GET提取。 
　　304 Not Modified：客户端有缓冲的文档并发出了一个条件性的请求（一般是提供If-Modified-Since头表示客户只想比指定日期更新的文档）。服务器告诉客户，原来缓冲的文档还可以继续使用。 
　　305 Use Proxy：客户请求的文档应该通过Location头所指明的代理服务器提取。 
　　307 Temporary Redirect：和302（Found）相同。许多浏览器会错误地响应302应答进行重定向，即使原来的请求是POST，即使它实际上只能在POST请求的应答是303时才能重定向。由于这个原因，HTTP 1.1新增了307，以便更加清除地区分几个状态代码：当出现303应答时，浏览器可以跟随重定向的GET和POST请求；如果是307应答，则浏览器只能跟随对GET请求的重定向。注意：HttpServletResponse中没有为该状态代码提供相应的常量。 
　　400 Bad Request：请求出现语法错误。 
　　401 Unauthorized：客户试图未经授权访问受密码保护的页面。应答中会包含一个WWW-Authenticate头，浏览器据此显示用户名字/密码对话框，然后在填写合适的Authorization头后再次发出请求。 
　　403 Forbidden：资源不可用。服务器理解客户的请求，但拒绝处理它。通常由于服务器上文件或目录的权限设置导致。 
　　404 Not Found：无法找到指定位置的资源。这也是一个常用的应答，HttpServletResponse专门提供了相应的方法：sendError(message)。 
　　405 Method Not Allowed：请求方法（GET、POST、HEAD、DELETE、PUT、TRACE等）对指定的资源不适用。 
　　406 Not Acceptable：指定的资源已经找到，但它的MIME类型和客户在Accpet头中所指定的不兼容。 
　　407 Proxy Authentication Required：类似于401，表示客户必须先经过代理服务器的授权。 
　　408 Request Timeout：在服务器许可的等待时间内，客户一直没有发出任何请求。客户可以在以后重复同一请求。 
　　409 Conflict：通常和PUT请求有关。由于请求和资源的当前状态相冲突，因此请求不能成功。 
　　410 Gone：所请求的文档已经不再可用，而且服务器不知道应该重定向到哪一个地址。它和404的不同在于，返回407表示文档永久地离开了指定的位置，而404表示由于未知的原因文档不可用。 
　　411 Length Required：服务器不能处理请求，除非客户发送一个Content-Length头。 
　　412 Precondition Failed：请求头中指定的一些前提条件失败。 
　　413 Request Entity Too Large：目标文档的大小超过服务器当前愿意处理的大小。如果服务器认为自己能够稍后再处理该请求，则应该提供一个Retry-After头。 
　　414 Request URI Too Long：URI太长。 
　　416 Requested Range Not Satisfiable：服务器不能满足客户在请求中指定的Range头。 
　　500 Internal Server Error：服务器遇到了意料不到的情况，不能完成客户的请求。 
　　501 Not Implemented：服务器不支持实现请求所需要的功能。例如，客户发出了一个服务器不支持的PUT请求。 
　　502 Bad Gateway：服务器作为网关或者代理时，为了完成请求访问下一个服务器，但该服务器返回了非法的应答。 
　　503 Service Unavailable：服务器由于维护或者负载过重未能应答。例如，Servlet可能在数据库连接池已满的情况下返回503。服务器返回503时可以提供一个Retry-After头。 
　　504 Gateway Timeout：由作为代理或网关的服务器使用，表示不能及时地从远程服务器获得应答。 
　　505 HTTP Version Not Supported：服务器不支持请求中所指明的HTTP版本。
*/

/*
http协议通常包括的方法有很多，以下只列出在http/1.1协议定义中看到的方法：get、post、head、options、put、delete、trace、connect。

get方法用于获取URI资源，是最为常用的一种方法。

post方法用于向指定URI提交内容，服务器端响应其行为，该方法也极为常用。

head方法向URI发送请求，仅仅只需要获得响应的协议头。

put方法用于向URI发送请求，若URI不存在，则要求服务器端根据请求创建资源。当URI存在时，服务器端必须接受请求内容，将其作为URI资源的修改后版本。

delete方法用于删除URI标识的指定资源。

trace方法用于激活服务器端对请求的循环反馈，反馈作为http响应的正文内容被传输回客户端。

connect方法通常被用于使用代理连接。
*/
/* hhtp 响应头范例
HTTP/1.1 200 OK
Server: Microsoft-IIS/5.0
Date: Thu, 12 Jul 2007 07:10:25 GMT
X-Powered-By: ASP.NET
X-AspNet-Version: 1.1.4322
Cache-Control: no-cache
Pragma: no-cache
Expires: -1
Content-Type: text/html; charset=gb2312
Content-Length: 3381
*/