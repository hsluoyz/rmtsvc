/*******************************************************************
   *	vidcdef.h
   *    DESCRIPTION:声明定义头文件
   *
   *    AUTHOR:yyc
   *
   *    http://hi.baidu.com/yycblog/home
   *
   *    DATE:2005-06-03
   *	
   *******************************************************************/
#ifndef __YY_VIDCDEF_H__
#define __YY_VIDCDEF_H__

#define VIDCS_VERSION 26 //vIDC 服务端版本
#define VIDCC_VERSION 26 //vIDC 客户端版本
#define VIDCC_MIN_VERSION 25 //支持的vidcc最小版本

#define VIDC_SERVER_PORT 8080 //默认vIDC服务端口
#define VIDC_MAX_RESPTIMEOUT 10 //s 最大等待超时时间
#define VIDC_PIPE_ALIVETIME 20 //s 管道最长存活时间，如果在指定的时间还没有被绑定则释放
#define VIDC_MAX_COMMAND_SIZE 256 //vIDC命令最大字节长度
#define VIDC_MAX_CLIENT_TIMEOUT 180 //如果多久没有收到vIDC客户端的命令或消息则认为断开了
#define VIDC_NOP_INTERVAL 60  //vIDCc发送心跳的间隔s
//定义VIDC的错误返回信息
#define SOCKSERR_VIDC_VER -301 //版本不匹配
#define SOCKSERR_VIDC_PSWD -302 //密码不正确
#define SOCKSERR_VIDC_RESP -303 //超时或响应错误
#define SOCKSERR_VIDC_NAME -304 //无效的名称
#define SOCKSERR_VIDC_MEMO -305 //内存分配失败
#define SOCKSERR_VIDC_MAP  -306 //映射失败
#define SOCKSERR_VIDC_SURPPORT -307 //暂时不支持此功能
#endif

/***********************vIDC处理流程******************************************************

vidcc                              vidcs
           <connect>
	------------------------------->
	       HELO 连接认证
	------------------------------->
	       200 ... OK
	<-------------------------------
	       ADDR ...
	------------------------------->
		   200 ... OK
	<-------------------------------
	       <开始其它命令交互>
	       .................
		   <close>
	<------------------------------->


------------------vIDC Server 处理流程-----------------------------------------
1、启动侦听
2、当有一个连接进来，建立连接
3、等待接收命令数据，如果VIDC_MAX_RESPTIMEOUT内没有收到任何命令则关闭此连接
4、判断命令是否为HELO或PIPE，如果不是则关闭连接
5、如果是HELO命令，则进行客户端连接认证，如果不通过则返回错误，关闭连接
6、否则建立一个session回话，循环等待处理vIDC命令，并返回成功消息
7、如果是PIPE命令，
------------------vIDC Client 处理流程-----------------------------------------
*****************************************************************************************/