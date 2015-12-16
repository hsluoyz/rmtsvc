/*******************************************************************
   *	upnpdef.h
   *    DESCRIPTION:定义UPnP协议所用到的常量、结构以及enum的定义
   *	UPnP - 即插即用，用于寻找支持UPnP的NAT/路由设备，以便在网关上进行端口映射
   *	UPnP 设备会侦听多播端口。一旦收到搜索请求，该设备就检查该搜索条件以确定它们是否匹配。
   *	如果匹配，一个单播 SSDP（通过 HTTPU）响应将被发送到该控制点	
   *	同样，当将设备插入网络时，它会发出多播 SSDP 展示通知消息，通知它所支持的服务。 
   *	
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *	
   *	net4cpp 2.1
   *******************************************************************/

#ifndef __YY_UPNPDEF_H__
#define __YY_UPNPDEF_H__

#define UPnP_MULTI_ADDR	"239.255.255.250" //UPnP组播地址和端口
#define UPnP_MULTI_PORT	1900 
#define UPnP_MAX_MESAGE_SIZE 4096

#define HTTP_STATUS_OK 200

typedef struct _TUPnPInfo
{
	bool budp; //是否为udp类型
	int mapport;
	int appport;
	bool bsuccess; //映射是否成功
	std::string appsvr;
	std::string appdesc;
	std::string retmsg;  //如果不成功返回的错误消息

}UPnPInfo,*PUPnPInfo;

#endif
/* 组播地址的范围是多少？
组播的地址是保留的D类地址从224.0.0.0—239.255.255.255，而且一些地址有特定的用处如，
224.0.0.0—244.0.0.255只能用于局域网中路由器是不会转发的，并且224.0.0.1是所有主机的地址，
224.0.0.2所有路由器的地址，224.0.0.5所有ospf路由器的地址，224.0.13事PIMv2路由器的地址；
239.0.0.0—239.255.255.255是私有地址（如192.168.x..x）；224.0.1.0—238.255.255.255可以用与Internet上的。
*/
/*
[UPnP] Sended Search Packet(len=132), return 132
M-SEARCH * HTTP/1.1
HOST: 239.255.255.250:1900
MAN: "ssdp:discover"
MX: 6
ST: urn:schemas-upnp-org:service:WANIPConnection:1

[UPnP] Received response ,len=322
HTTP/1.1 200 OK
CACHE-CONTROL: max-age=100
DATE: Wed, 25 Jul 2007 00:42:39 GMT
EXT:
LOCATION: http://192.168.0.3:1900/igd.xml
SERVER: TP-LINK Router, UPnP/1.0
ST: urn:schemas-upnp-org:service:WANIPConnection:1
USN: uuid:upnp-WANConnectionDevice-192168035678900001::urn:schemas-upnp-org:service:WANIPConnection:1

[UPnP] Found Loaction: http://192.168.0.3:1900/igd.xml
[httpreq] Sending HTTP Request Header,len=207
GET /igd.xml HTTP/1.1
Accept: * /*
Accept-Language: zh-cn
User-Agent: Mozilla/4.0 (compatible; MSIE 6.0; Windows NT 5.0)
Cache-Control: no-cache
Connection: close
Host: 192.168.0.3
Pragma: no-cache

[httprsp] Received HTTP Response Header
HTTP/1.1 200 OK
CONTENT-LENGTH: 2884
CONTENT-TYPE: text/xml
DATE: Wed, 25 Jul 2007 00:42:39 GMT
LAST-MODIFIED: Tue, 28 Oct 2003 08:46:08 GMT
SERVER: TP-LINK Router, UPnP/1.0
CONNECTION: close
[UPnP] Receive XML: 2884 / 2884


*/
