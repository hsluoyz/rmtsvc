/*******************************************************************
   *	dnsdef.h
   *    DESCRIPTION:定义dns协议所用到的常量、结构以及enum的定义
   *				
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-11-02
   *	
   *	net4cpp 2.1
   *******************************************************************/

#ifndef __YY_DNSPDEF_H__
#define __YY_DNSPDEF_H__

#define SOCKSERR_DNS_SERVER -101 //无效的IP地址
#define SOCKSERR_DNS_IP SOCKSERR_DNS_SERVER-1 //无效的dns服务地址
#define SOCKSERR_DNS_NAMES SOCKSERR_DNS_SERVER-2 //无效的要解析的域名

#define DNS_SERVER_PORT	53 //默认DNS服务的端口
#define DNS_MAX_PACKAGE_SIZE 512 //一个DNS查询包大小

//----------------此处常量定义了LITTER_BYTE计算机系统DNS_HEADER.flags位掩码的含义-----
#define DNS_FLAGS_QR 0x80 //网络第一字节的第7位
		//0代表查询，1代表DNS回复
#define DNS_FLAGS_OPCODE 0x78 //网络第一字节的第6,5,4,3位
		//指示查询种类：0:标准查询；1:反向查询；2:服务器状态查询；3-15:未使用。
#define DNS_FLAGS_AA 0x04 //网络第一字节的第2位
		//是否权威回复
#define DNS_FLAGS_TC 0x02 //网络第一字节的第1位
		//因为一个UDP报文为512字节，所以该位指示是否截掉超过的部分
#define DNS_FLAGS_RD 0x01 //网络第一字节的第0位
		//此位在查询中指定，回复时相同。设置为1指示服务器进行递归查询
#define DNS_FLAGS_RA 0x8000 //网络第二字节的第7位
		//由DNS回复返回指定，说明DNS服务器是否支持递归查询
#define DNS_FLAGS_Z 0x7000 //网络第二字节的第6,5,4位
		//保留字段，必须设置为0
#define DNS_FLAGS_RCODE 0x0f00 //网络第二字节的第3,2,1,0位
		//由回复时指定的返回码：0:无差错；1:格式错；2:DNS出错；3:域名不存在；4:DNS不支持这类查询；5:DNS拒绝查询；6-15:保留字段

#define DNS_OPCODE_QUERY 0x0 //标准查询
#define DNS_OPCODE_IQUERY 0x08 //反向查询
#define DNS_OPCODE_STATUS 0x10 //状态查询

#define DNS_RCODE_ERR_OK 0x0 //无差错
#define DNS_RCODE_ERR_FORMAT 0x0100 //格式错
#define DNS_RCODE_ERR_DNS 0x0200 //DNS出错
#define DNS_RCODE_ERR_EXIST 0x0300 //域名不存在
#define DNS_RCODE_ERR_SURPPORT 0x0400 //DNS不支持这类查询
#define DNS_RCODE_ERR_REJECT 0x0500 //DNS拒绝查询

//--------------此处常量定义了BIG_BYTE计算机系统DNS_HEADER.flags位掩码的含义--------
//#define DNS_FLAGS_QR 0x8000 //网络第一字节的第7位
//#define DNS_FLAGS_OPCODE 0x7800 //网络第一字节的第6,5,4,3位
//#define DNS_FLAGS_AA 0x0400 //网络第一字节的第2位
//#define DNS_FLAGS_TC 0x0200 //网络第一字节的第1位
//#define DNS_FLAGS_RD 0x0100 //网络第一字节的第0位
//#define DNS_FLAGS_RA 0x0080 //网络第二字节的第7位
//#define DNS_FLAGS_Z 0x0070 //网络第二字节的第6,5,4位
//#define DNS_FLAGS_RCODE 0x000f //网络第二字节的第3,2,1,0位

//#define DNS_OPCODE_QUERY 0x0 //标准查询
//#define DNS_OPCODE_IQUERY 0x0800 //反向查询
//#define DNS_OPCODE_STATUS 0x1000 //状态查询

//#define DNS_RCODE_ERR_OK 0x0 //无差错
//#define DNS_RCODE_ERR_FORMAT 0x01 //格式错
//#define DNS_RCODE_ERR_DNS 0x02 //DNS出错
//#define DNS_RCODE_ERR_EXIST 0x03 //域名不存在
//#define DNS_RCODE_ERR_SURPPORT 0x04 //DNS不支持这类查询
//#define DNS_RCODE_ERR_REJECT 0x05 //DNS拒绝查询
//-----------------------------------------------------------------------
/* 查询类型 Q_type
 * Type values for resources and queries
 */
#define T_A			1		/* host address 指定计算机 IP 地址*/
#define T_NS		2		/* authoritative server 指定用于命名区域的 DNS 名称服务器*/
#define T_MD		3		/* mail destination 指定邮件接收站（此类型已经过时了，使用MX代替）*/
#define T_MF		4		/* mail forwarder 指定邮件中转站（此类型已经过时了，使用MX代替）*/
#define T_CNAME		5		/* canonical name 指定用于别名的规范名称*/
#define T_SOA		6		/* start of authority zone 指定用于 DNS 区域的“起始授权机构”*/
#define T_MB		7		/* mailbox domain name 指定邮箱域名*/
#define T_MG		8		/* mail group member 指定邮件组成员*/
#define T_MR		9		/* mail rename name 指定邮件重命名域名*/
#define T_NULL		10		/* null resource record 指定空的资源记录*/
#define T_WKS		11		/* well known service 描述已知服务*/
#define T_PTR		12		/* domain name pointer 如果查询是 IP 地址，则指定计算机名；否则指定指向其它信息的指针*/
#define T_HINFO		13		/* host information 指定计算机 CPU 以及操作系统类型*/
#define T_MINFO		14		/* mailbox information 指定邮箱或邮件列表信息*/
#define T_MX		15		/* mail routing information 指定邮件交换器*/
#define T_TXT		16		/* text strings 指定文本信息*/
#define T_RP		17		/* responsible person */
#define T_AFSDB		18		/* AFS cell database */
#define T_X25		19		/* X_25 calling address */
#define T_ISDN		20		/* ISDN calling address */
#define T_RT		21		/* router */
#define T_NSAP		22		/* NSAP address */
#define T_NSAP_PTR	23		/* reverse NSAP lookup (deprecated) */
#define T_SIG		24		/* security signature */
#define T_KEY		25		/* security key */
#define T_PX		26		/* X.400 mail mapping */
#define T_GPOS		27		/* geographical position (withdrawn) */
#define T_AAAA		28		/* IP6 Address */
#define T_LOC		29		/* Location Information */
#define T_NXT		30		/* Next Valid Name in Zone */
#define T_EID		31		/* Endpoint identifier */
#define T_NIMLOC	32		/* Nimrod locator */
#define T_SRV		33		/* Server selection */
#define T_ATMA		34		/* ATM Address */
#define T_NAPTR		35		/* Naming Authority PoinTeR */
	/* non standard */
#define T_UINFO		100		/* user (finger) information 指定用户信息*/
#define T_UID		101		/* user ID 指定用户标识符*/
#define T_GID		102		/* group ID 指定组名的组标识符*/
#define T_UNSPEC	103		/* Unspecified format (binary data) */
	/* Query type values which do not appear in resource records */
#define	T_IXFR		251		/* incremental zone transfer */
#define T_AXFR		252		/* transfer zone of authority */
#define T_MAILB		253		/* transfer mailbox records */
#define T_MAILA		254		/* transfer mail agent records */
#define T_ANY		255		/* wildcard match 指定所有数据类型*/
/* 查询class Q_class
 * Values for class field
 */
#define C_IN		1		/* the arpa internet 指定 Internet 类别*/
#define C_CSNET		2		/* 指定 CSNET 类别。（已过时）*/
#define C_CHAOS		3		/* for chaos net (MIT) 指定 Chaos 类别*/
#define C_HS		4		/* for Hesiod name server (MIT) (XXX) 指定 MIT Athena Hesiod 类别*/
	/* Query class values which do not appear in resource records 指定任何以前列出的通配符*/
#define C_ANY		255		/* wildcard match */

//Q_type中的T_A,T_MX,T_CNAME为常用，Q_class中的C_IN为常用

typedef struct dns_protocol_header //DNS数据报头
{
	unsigned short id;
	//标识，通过它客户端可以将DNS的请求与应答相匹配
	unsigned short flags;
	//标志：[QR | opcode | AA| TC| RD| RA | zero | rcode ]
	unsigned short quests;
	//问题数目
	unsigned short answers;
	//资源记录数目
	unsigned short author;
	//授权资源记录数目
	unsigned short addition;
	//额外资源记录数目
}DNS_HEADER,*PDNS_HEADER;
typedef struct dns_protocol_query //DNS查询数据报
{
	const char *name;
	//查询的域名,这是一个大小在0到63之间的字符串
	unsigned short type;
	//查询类型，见Q_type_array定义
	unsigned short classes;
	//查询类,见Q_class_array 通常是A类既查询IP地址

}DNS_QUERY,*PDNS_QUERY;
typedef struct dns_protocol_response //DNS响应数据报
{
	const char *name; //回复查询的域名，不定长
	//查询的域名
	unsigned short type; //回复的类型。2字节，与查询同义。指示RDATA中的资源记录类型
	//查询类型
	unsigned short classes; //回复的类。2字节，与查询同义。指示RDATA中的资源记录类
	//类型码
	unsigned long	ttl; //生存时间。4字节，指示RDATA中的资源记录在缓存的生存时间
	//生存时间
	unsigned short length; //长度。2字节，指示RDATA块的长度(不包括本字段的两字节)
	//资源数据长度
	unsigned char *	rdata; //资源记录。不定义，依TYPE的不同，此记录的格示不同，
						  //通常一个MX记录是由一个2字节的指示该邮件交换器的优先级值及不定长的邮件交换器名组成的
	//资源数据
}DNS_RESPONSE,*PDNS_RESPONSE;

#endif

/*************************
名称的组合形式。名称由多个标识序列组成，每一个标识序列的首字节说明该标识符的长度，
接着用是ASCII码表示字符，多个序列之后由字节0表示名字结束。
其中某一个标识序列的首字符的长度若是0xC0的话，表示下一字节指示不是标识符序列，
而是指示接下部分在本接收包内的偏移位置此时不已字节0表示名字结束。 
　　比如　bbs.zzsy.com　以.分开bbs、zzsy、com三个部分。每个部分的长度为3、4、3 
　　在DNS报文中的形式就如 3 b b s 4 z z s y 3 c o m 0 
　　假如在包内的第12个字节位置存在有 4 z z s y 3 c o m 0 这样的名称了。 
　　那此时有可能为：3 b b s 4 z z s y 0xC0 0x0C 这样的形式。
**************************/
