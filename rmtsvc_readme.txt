Rmtsvc 2.5.x
基于web的集FTP服务、代理服务、端口映射等一体远程控制工具。


【RmtSvc前生今世】

2000年左右用过一款远控软件remoteAnywhere，那是第一次使用也是我第一次见到基于web方式的远控，非常让我吃惊，原来IE还可以做这些事。 

01年的时候在一家网络公司，主要写ASP方面的应用，当时用ASP写了一个基于IE的仿windows资源管理器操作风格的文件远程操作的程序WebE即web资源浏览器。
02年到了一家做电信的公司，做linux下的网络程序的设计开发，积累封装了一套比较简单可靠的linux/windows通用的网络库net4cpp1.0（因为当时linux下有个log4cpp的用于记录输出程序日志的类库，所以就起名net4cpp:) ）.
02年开始基于net4cpp11.0 写了rmtsvc的第一个原始版本remoteCtrol 1.0，仅仅可以通过IE进行远程桌面控制.
03年增加了远程文件，telnet，ftp等等功能并正式更名为rmtsvc2.3.x，此后两年不断的进行优化更新增加了MSN，集成了vIDC等功能一直持续到2.4.7这个版本。
06年rmtsvc进行了完全重构，发布了2.5.x版本，
2007年底发布2.5.2 ，07年后rmtsvc一直没有再进行更新改进，一方面工作的琐事多了，另一方面人也懒了，不愿意写东西了，没有了激情，动力。
rmtsvc应该是国内第一款完全意义的BS架构的远控软件，虽然它有很多的不足但它的优势也很明显，它可以很方便的支持移动设备，譬如我们的智能手机，ipad等，只要能上网的设备就能远程控制，而我们要做的仅仅是更改一下前端的web页面一边支持更多的浏览器而已。
与其放在手里烂掉不如开源让更多的人参与，把这个软件做的更好。




【目录说明】
net4cpp21  基础网络lib库，rmtsvc的网络部分是基于此类库开发，包含了基础网络的封装以及常用协议的封装以及服务，譬如http，ftp，dns，smtp，proxy等
libs       rmtsvc编译所需要的lib库文件，以及其它lib库的源代码，譬如msn(版本比较老，只支持到msnp13即msn7.x版本）
           bin为所有编译所需要的lib库文件存放地（除了openssl的lib库）
libs\vidc  vidc库源代码
bin	  rmtsvc可执行文件目录
bin\html  rmtsvc web前端展现控制 静态web页面，web页面通过JS访问rmtsvc服务解析处理xml数据进行展现。

【编译说明】
本工程用vc6 编译测试过没有任何问题。其它版本没有测试过。
用vc6编译时请在编译环境中增加net4cpp21下的OPENSSL的头文件和lib文件路径
vc6菜单-->tools--->options--->Directories 增加inlude包含路径
<rmtsvc路径>\net4cpp21\OPENSSL
library包含路径
<rmtsvc路径>\net4cpp21\OPENSSL\LIB




net4cpp21 网络类关系图
***********************socket*****************************

                     socketBase
                         |
     |-----------------------------------------|
     |                   |                     |
 socketRaw           socketUdp            socketTcp-----|
     |                   |                     |        |
     |                   |                     |        |
 |--------|          dnsClient            socketSSL     |
 |        |                                    |________|
socketIcmp                                     |        
                          |------------------------------|
                          |                              |
                      socketSvr                     socketProxy
                          |                              |
                          |                              |
          |--------------------------|        |-----------------------------|   
                |         |          |        |          |          |
                |         |          |        |          |          |
            httpServer ftpServer smtpServer smtpClient ftpClient httpClient