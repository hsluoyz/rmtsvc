/*******************************************************************
   *	webAction_rview.cpp web请求处理 - 注册表浏览
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

//列出本机所有服务
//buffer - 返回的xml文档,格式:
//<?xml version="1.0" encoding="gb2312" ?>
//<xmlroot>
//<rekeys>
//<kitem>
//<id></id>
//<subkeys>+</subkeys>
//<regkey></regkey>
//</kitem>
//....
//</regkeys>
//
//<regitems>
//<vitem>
//<id></id>
//<rtype></rtype>
//<rname></rname>
//<rdlen></rdlen>
//<rdata></rdata>
//</vitem>
//....
//</regitems>
//</xmlroot>


bool regkeyList(cBuffer &buffer,const char *skey);
bool regitemList(cBuffer &buffer,const char *skey);
//listWhat -- 指明列举什么项目 1 : list subKey 2 : list dataItem
bool webServer :: httprsp_reglist(socketTCP *psock,httpResponse &httprsp,const char *skey,int listWhat)
{
	bool bret=false;
	cBuffer buffer(1024);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<?xml version=\"1.0\" encoding=\"gb2312\" ?><xmlroot>");
	
	if(listWhat & 1) bret|=regkeyList(buffer,skey);
	if(listWhat & 2) bret|=regitemList(buffer,skey);

	if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
	if(buffer.str())
		buffer.len()+=sprintf(buffer.str()+buffer.len(),"</xmlroot>");
	
	httprsp.NoCache();//CacheControl("No-cache");
	//设置MIME类型，默认为HTML
	httprsp.set_mimetype(MIMETYPE_XML);
	//设置响应内容长度
	httprsp.lContentLength(buffer.len());
	if(!bret)
		httprsp.send_rspH(psock,400,"Bad Request");
	else httprsp.send_rspH(psock,200,"OK");
	
	if(buffer.str()) psock->Send(buffer.len(),buffer.str(),-1);
	return true;
}

bool webServer::httprsp_regkey_del(socketTCP *psock,httpResponse &httprsp,const char *spath,const char *skey)
{
	if(spath && spath[0]=='\\')
	{
		const char *sroot=spath+1;
		const char *lpregpath=NULL;
		//分离出HKEY_ROOT和RegPath
		const char *ptr=strchr(sroot,'\\');
		if(ptr) { *(char *)ptr=0; lpregpath=ptr+1; }
		HKEY hKEY_ROOT,hKEY=NULL;
		if( strcmp(sroot,"HKEY_CLASSES_ROOT")==0 )
			hKEY_ROOT=HKEY_CLASSES_ROOT;
		else if(strcmp(sroot,"HKEY_CURRENT_USER")==0 )
			hKEY_ROOT=HKEY_CURRENT_USER;
		else if(strcmp(sroot,"HKEY_LOCAL_MACHINE")==0 )
			hKEY_ROOT=HKEY_LOCAL_MACHINE;
		else if(strcmp(sroot,"HKEY_USERS")==0 )
			hKEY_ROOT=HKEY_USERS;
		else if(strcmp(sroot,"HKEY_CURRENT_CONFIG")==0 )
			hKEY_ROOT=HKEY_CURRENT_CONFIG;
		else hKEY_ROOT=NULL;
		if(ptr) *(char *)ptr='\\';
		if(lpregpath==NULL || lpregpath[0]==0)
			hKEY=hKEY_ROOT;
		else if(::RegOpenKeyEx(hKEY_ROOT, lpregpath, 0, KEY_WRITE|KEY_ENUMERATE_SUB_KEYS, &hKEY)!=ERROR_SUCCESS)
			hKEY=NULL;
		if(hKEY) RegDeleteKey(hKEY,skey);
		::RegCloseKey(hKEY);
	}//?if(spath && spath[0]=='\\')
	return httprsp_reglist(psock,httprsp,spath,1);
}

bool webServer::httprsp_regkey_add(socketTCP *psock,httpResponse &httprsp,const char *spath,const char *skey)
{
	if(spath && spath[0]=='\\')
	{
		const char *sroot=spath+1;
		const char *lpregpath=NULL;
		//分离出HKEY_ROOT和RegPath
		const char *ptr=strchr(sroot,'\\');
		if(ptr) { *(char *)ptr=0; lpregpath=ptr+1; }
		HKEY hKEY_ROOT,hKEY=NULL,hkSub=NULL;
		DWORD dwDisposition=0;
		if( strcmp(sroot,"HKEY_CLASSES_ROOT")==0 )
			hKEY_ROOT=HKEY_CLASSES_ROOT;
		else if(strcmp(sroot,"HKEY_CURRENT_USER")==0 )
			hKEY_ROOT=HKEY_CURRENT_USER;
		else if(strcmp(sroot,"HKEY_LOCAL_MACHINE")==0 )
			hKEY_ROOT=HKEY_LOCAL_MACHINE;
		else if(strcmp(sroot,"HKEY_USERS")==0 )
			hKEY_ROOT=HKEY_USERS;
		else if(strcmp(sroot,"HKEY_CURRENT_CONFIG")==0 )
			hKEY_ROOT=HKEY_CURRENT_CONFIG;
		else hKEY_ROOT=NULL;
		if(ptr) *(char *)ptr='\\';
		if(lpregpath==NULL || lpregpath[0]==0)
			hKEY=hKEY_ROOT;
		else if(::RegOpenKeyEx(hKEY_ROOT, lpregpath, 0, KEY_CREATE_SUB_KEY, &hKEY)!=ERROR_SUCCESS)
			hKEY=NULL;
		if(hKEY) RegCreateKeyEx(hKEY,skey,0,"",REG_OPTION_NON_VOLATILE,KEY_READ | KEY_WRITE,NULL,&hkSub,&dwDisposition);
		::RegCloseKey(hKEY); ::RegCloseKey(hkSub);
	}//?if(spath && spath[0]=='\\')
	return httprsp_reglist(psock,httprsp,spath,1);
}

bool webServer::httprsp_regitem_del(socketTCP *psock,httpResponse &httprsp,const char *spath,const char *sname)
{
	if(spath && spath[0]=='\\')
	{
		const char *sroot=spath+1;
		const char *lpregpath=NULL;
		//分离出HKEY_ROOT和RegPath
		const char *ptr=strchr(sroot,'\\');
		if(ptr) { *(char *)ptr=0; lpregpath=ptr+1; }
		HKEY hKEY_ROOT,hKEY=NULL;
		if( strcmp(sroot,"HKEY_CLASSES_ROOT")==0 )
			hKEY_ROOT=HKEY_CLASSES_ROOT;
		else if(strcmp(sroot,"HKEY_CURRENT_USER")==0 )
			hKEY_ROOT=HKEY_CURRENT_USER;
		else if(strcmp(sroot,"HKEY_LOCAL_MACHINE")==0 )
			hKEY_ROOT=HKEY_LOCAL_MACHINE;
		else if(strcmp(sroot,"HKEY_USERS")==0 )
			hKEY_ROOT=HKEY_USERS;
		else if(strcmp(sroot,"HKEY_CURRENT_CONFIG")==0 )
			hKEY_ROOT=HKEY_CURRENT_CONFIG;
		else hKEY_ROOT=NULL;
		if(ptr) *(char *)ptr='\\';
		if(lpregpath==NULL || lpregpath[0]==0)
			hKEY=hKEY_ROOT;
		else if(::RegOpenKeyEx(hKEY_ROOT, lpregpath, 0, KEY_WRITE|KEY_ENUMERATE_SUB_KEYS, &hKEY)!=ERROR_SUCCESS)
			hKEY=NULL;
		if(hKEY) RegDeleteValue(hKEY,sname);
		::RegCloseKey(hKEY);
	}//?if(spath && spath[0]=='\\')
	return httprsp_reglist(psock,httprsp,spath,2);
}
//将二进制字符串转换为二进制数组，返回数组大小
DWORD cvtBinaryString2Binary(char *strBinary)
{
	LPBYTE pbyte=(LPBYTE)strBinary;
	char *p,*ptr=strBinary;
	while(*ptr==' ') ptr++; //去掉前导空格
	::strupr(ptr);//转换为大写
	while(true)
	{
		p=strchr(ptr,' ');
		if(p) *p='\0';
		*pbyte++=(BYTE)cCoder::hex_atol(ptr);
		if(p==NULL) break;
		*p=' '; ptr=p+1;
		while(*ptr==' ') ptr++; //去掉前导空格
	}
	return pbyte-(LPBYTE)strBinary;
}

bool webServer::httprsp_regitem_add(socketTCP *psock,httpResponse &httprsp,const char *spath,
									const char *stype,const char *sname,const char *svalue)
{
	if(spath && spath[0]=='\\')
	{
		const char *sroot=spath+1;
		const char *lpregpath=NULL;
		//分离出HKEY_ROOT和RegPath
		const char *ptr=strchr(sroot,'\\');
		if(ptr) { *(char *)ptr=0; lpregpath=ptr+1; }
		HKEY hKEY_ROOT,hKEY=NULL;
		if( strcmp(sroot,"HKEY_CLASSES_ROOT")==0 )
			hKEY_ROOT=HKEY_CLASSES_ROOT;
		else if(strcmp(sroot,"HKEY_CURRENT_USER")==0 )
			hKEY_ROOT=HKEY_CURRENT_USER;
		else if(strcmp(sroot,"HKEY_LOCAL_MACHINE")==0 )
			hKEY_ROOT=HKEY_LOCAL_MACHINE;
		else if(strcmp(sroot,"HKEY_USERS")==0 )
			hKEY_ROOT=HKEY_USERS;
		else if(strcmp(sroot,"HKEY_CURRENT_CONFIG")==0 )
			hKEY_ROOT=HKEY_CURRENT_CONFIG;
		else hKEY_ROOT=NULL;
		if(ptr) *(char *)ptr='\\';
		if(lpregpath==NULL || lpregpath[0]==0)
			hKEY=hKEY_ROOT;
		else if(::RegOpenKeyEx(hKEY_ROOT, lpregpath, 0, KEY_WRITE|KEY_ENUMERATE_SUB_KEYS, &hKEY)!=ERROR_SUCCESS)
			hKEY=NULL;
		if(hKEY)
		{
			if(strcmp(stype,"REG_BINARY")==0)
			{
				DWORD dwlen=cvtBinaryString2Binary((char *)svalue);
				::RegSetValueEx(hKEY, sname, NULL,REG_BINARY, (LPBYTE)svalue,dwlen);
			}//?if(strcmp(stype,"REG_BINARY")==0)
			else if(strcmp(stype,"REG_DWORD")==0)
			{
				::strupr((char *)svalue);//转换为大写
				DWORD dw=(svalue[1]=='X')?cCoder::hex_atol(svalue+2):(DWORD)atol(svalue);
				::RegSetValueEx(hKEY, sname, NULL,REG_DWORD, (LPBYTE)&dw,sizeof(DWORD));
			} 
			else 
				::RegSetValueEx(hKEY, sname, NULL,REG_SZ, (LPBYTE)svalue,strlen(svalue)+1);
		}
		::RegCloseKey(hKEY);
	}//?if(spath && spath[0]=='\\')
	return httprsp_reglist(psock,httprsp,spath,2);
}

bool webServer::httprsp_regitem_md(socketTCP *psock,httpResponse &httprsp,const char *spath,
									const char *stype,const char *sname,const char *svalue)
{
	if(spath && spath[0]=='\\')
	{
		const char *sroot=spath+1;
		const char *lpregpath=NULL;
		//分离出HKEY_ROOT和RegPath
		const char *ptr=strchr(sroot,'\\');
		if(ptr) { *(char *)ptr=0; lpregpath=ptr+1; }
		HKEY hKEY_ROOT,hKEY=NULL;
		if( strcmp(sroot,"HKEY_CLASSES_ROOT")==0 )
			hKEY_ROOT=HKEY_CLASSES_ROOT;
		else if(strcmp(sroot,"HKEY_CURRENT_USER")==0 )
			hKEY_ROOT=HKEY_CURRENT_USER;
		else if(strcmp(sroot,"HKEY_LOCAL_MACHINE")==0 )
			hKEY_ROOT=HKEY_LOCAL_MACHINE;
		else if(strcmp(sroot,"HKEY_USERS")==0 )
			hKEY_ROOT=HKEY_USERS;
		else if(strcmp(sroot,"HKEY_CURRENT_CONFIG")==0 )
			hKEY_ROOT=HKEY_CURRENT_CONFIG;
		else hKEY_ROOT=NULL;
		if(ptr) *(char *)ptr='\\';
		if(lpregpath==NULL || lpregpath[0]==0)
			hKEY=hKEY_ROOT;
		else if(::RegOpenKeyEx(hKEY_ROOT, lpregpath, 0, KEY_WRITE|KEY_ENUMERATE_SUB_KEYS, &hKEY)!=ERROR_SUCCESS)
			hKEY=NULL;
		if(hKEY)
		{
			if(strcmp(stype,"REG_BINARY")==0)
			{
				DWORD dwlen=cvtBinaryString2Binary((char *)svalue);
				::RegSetValueEx(hKEY, sname, NULL,REG_BINARY, (LPBYTE)svalue,dwlen);
			}
			else if(strcmp(stype,"REG_DWORD")==0)
			{
				::strupr((char *)svalue);//转换为大写
				DWORD dw=(svalue[1]=='X')?cCoder::hex_atol(svalue+2):(DWORD)atol(svalue);
				::RegSetValueEx(hKEY, sname, NULL,REG_DWORD, (LPBYTE)&dw,sizeof(DWORD));
			}
			else ::RegSetValueEx(hKEY, sname, NULL,REG_SZ, (LPBYTE)svalue,strlen(svalue)+1);
		}
		::RegCloseKey(hKEY);
	}//?if(spath && spath[0]=='\\')
	return httprsp_reglist(psock,httprsp,spath,2);
}

//-------------------------------------------------------------------------------

bool regkeyList(cBuffer &buffer,const char *skey)
{
	if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
	buffer.len()+=sprintf(buffer.str()+buffer.len(),"<regkeys>");
	
	long lret=0;
	if(skey==NULL || skey[0]==0)
	{
		buffer.len()+=sprintf(buffer.str()+buffer.len(),
			"<kitem><subkeys>+</subkeys><regkey>HKEY_CLASSES_ROOT</regkey></kitem>"
			"<kitem><subkeys>+</subkeys><regkey>HKEY_CURRENT_USER</regkey></kitem>"
			"<kitem><subkeys>+</subkeys><regkey>HKEY_LOCAL_MACHINE</regkey></kitem>"
			"<kitem><subkeys>+</subkeys><regkey>HKEY_USERS</regkey></kitem>"
			"<kitem><subkeys>+</subkeys><regkey>HKEY_CURRENT_CONFIG</regkey></kitem>");
		lret=5;
	}else if(skey[0]=='\\'){
		skey++; const char *lpregpath=NULL;
		//分离出HKEY_ROOT和RegPath
		const char *ptr=strchr(skey,'\\');
		if(ptr) { *(char *)ptr=0; lpregpath=ptr+1; }
		HKEY hKEY_ROOT,hKEY=NULL;
		if( strcmp(skey,"HKEY_CLASSES_ROOT")==0 )
			hKEY_ROOT=HKEY_CLASSES_ROOT;
		else if(strcmp(skey,"HKEY_CURRENT_USER")==0 )
			hKEY_ROOT=HKEY_CURRENT_USER;
		else if(strcmp(skey,"HKEY_LOCAL_MACHINE")==0 )
			hKEY_ROOT=HKEY_LOCAL_MACHINE;
		else if(strcmp(skey,"HKEY_USERS")==0 )
			hKEY_ROOT=HKEY_USERS;
		else if(strcmp(skey,"HKEY_CURRENT_CONFIG")==0 )
			hKEY_ROOT=HKEY_CURRENT_CONFIG;
		else hKEY_ROOT=NULL;
		if(ptr) *(char *)ptr='\\';
		DWORD dwRegpathLen=(lpregpath)?strlen(lpregpath):0;
		if(lpregpath==NULL || lpregpath[0]==0)
			hKEY=hKEY_ROOT;
		else if(::RegOpenKeyEx(hKEY_ROOT, lpregpath, 0, KEY_READ|KEY_ENUMERATE_SUB_KEYS, &hKEY)!=ERROR_SUCCESS)
			hKEY=NULL;

		if(hKEY)
		{
			DWORD dwMaxSubKeyLen=0,dwIndex,dwBufferSize,dwSubKeys=0; 
			::RegQueryInfoKey(hKEY,NULL,NULL,NULL,&dwSubKeys,&dwMaxSubKeyLen,NULL,NULL,NULL,NULL,NULL,NULL);
			char *subkey_buffer,*ptr_tmpbuf=(dwSubKeys>0)?(new char[dwRegpathLen+1+dwMaxSubKeyLen+1]):NULL;
			if(ptr_tmpbuf)
			{   
				subkey_buffer=ptr_tmpbuf+dwRegpathLen+1;
				if(lpregpath) sprintf(ptr_tmpbuf,"%s",lpregpath);
				ptr_tmpbuf[dwRegpathLen]='\\';
				FILETIME ft; dwBufferSize=dwMaxSubKeyLen+1; dwIndex=0;
				while( ::RegEnumKeyEx(hKEY,dwIndex,subkey_buffer,&dwBufferSize,NULL,NULL,NULL,&ft)==  ERROR_SUCCESS)
				{
					subkey_buffer[dwBufferSize]=0;
					if(buffer.Space()<(dwBufferSize+80)){
						if( (dwBufferSize+=80)<256 ) dwBufferSize=256;
						if(buffer.Resize(buffer.size()+dwBufferSize)==NULL) break;
					}
					//获取此键，子键的个数 start----------------------------------
					dwSubKeys=0; HKEY hsubKey=NULL;
					if(::RegOpenKeyEx(hKEY_ROOT, ((ptr_tmpbuf[0]=='\\')?(ptr_tmpbuf+1):ptr_tmpbuf), 0, KEY_READ|KEY_ENUMERATE_SUB_KEYS, &hsubKey)==ERROR_SUCCESS)
					{
						::RegQueryInfoKey(hsubKey,NULL,NULL,NULL,&dwSubKeys,NULL,NULL,NULL,NULL,NULL,NULL,NULL);
						::RegCloseKey(hsubKey);
					}
					////获取此键，子键的个数 end-----------------------------
					buffer.len()+=sprintf(buffer.str()+buffer.len(),
						"<kitem><id>%d</id><subkeys>%c</subkeys><regkey>%s</regkey></kitem>",
						++lret, ((dwSubKeys>0)?'+':' '), subkey_buffer);
					dwBufferSize=dwMaxSubKeyLen+1; dwIndex++;
				}//?while
				delete[] ptr_tmpbuf;
			}else lret=-1;
			::RegCloseKey(hKEY);
		}else lret=-1;
	}else lret=-1;
	
	if(lret==0 && buffer.str())
		buffer.len()+=sprintf(buffer.str()+buffer.len(),
					"<kitem><id></id><regkey>(none - 没有数据项)</regkey></kitem>");
	if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
	if(buffer.str()) buffer.len()+=sprintf(buffer.str()+buffer.len(),"</regkeys>");
	return (lret<0)?false:true;
}

bool regitemList(cBuffer &buffer,const char *skey)
{
	static char STR_REG_TYPE[][20]={"REG_NONE","REG_SZ","REG_EXPAND_SZ","REG_BINARY","REG_DWORD",
				"REG_DWORD_LE","REG_DWORD_BE","REG_LINK","REG_MULTI_SZ",
				"REG_RES_LIST","REG_RES_DESC","REG_RES_REQL",""};
// REG_DWORD_LITTLE_ENDIAN - REG_DWORD_LE
// REG_DWORD_BIG_ENDIAN    - REG_DWORD_BE
// REG_RESOURCE_LIST       - REG_RES_LIST
// REG_FULL_RESOURCE_DESCRIPTOR - REG_RES_DESC
// REG_RESOURCE_REQUIREMENTS_LIST - REG_RES_REQL
	if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
	if(buffer.str()) buffer.len()+=sprintf(buffer.str()+buffer.len(),"<regitems>");

	long lret=0;
	if(skey && skey[0]=='\\')
	{
		skey++; const char *lpregpath=NULL;
		//分离出HKEY_ROOT和RegPath
		const char *ptr=strchr(skey,'\\');
		if(ptr) { *(char *)ptr=0; lpregpath=ptr+1; }
		HKEY hKEY_ROOT,hKEY=NULL;
		if( strcmp(skey,"HKEY_CLASSES_ROOT")==0 )
			hKEY_ROOT=HKEY_CLASSES_ROOT;
		else if(strcmp(skey,"HKEY_CURRENT_USER")==0 )
			hKEY_ROOT=HKEY_CURRENT_USER;
		else if(strcmp(skey,"HKEY_LOCAL_MACHINE")==0 )
			hKEY_ROOT=HKEY_LOCAL_MACHINE;
		else if(strcmp(skey,"HKEY_USERS")==0 )
			hKEY_ROOT=HKEY_USERS;
		else if(strcmp(skey,"HKEY_CURRENT_CONFIG")==0 )
			hKEY_ROOT=HKEY_CURRENT_CONFIG;
		else hKEY_ROOT=NULL;
		if(ptr) *(char *)ptr='\\';
		if(lpregpath==NULL || lpregpath[0]==0)
			hKEY=hKEY_ROOT;
		else if(::RegOpenKeyEx(hKEY_ROOT, lpregpath, 0, KEY_READ|KEY_ENUMERATE_SUB_KEYS, &hKEY)!=ERROR_SUCCESS)
			hKEY=NULL;
		if(hKEY)
		{
			DWORD dwMaxValueNameLen=0,dwMaxValueLen=0,dwIndex,dwNameBufferSize,dwValueBufferSize; 
			::RegQueryInfoKey(hKEY,NULL,NULL,NULL,NULL,NULL,NULL,NULL,&dwMaxValueNameLen,&dwMaxValueLen,NULL,NULL);
			char *subname_buffer=new char[dwMaxValueNameLen+dwMaxValueLen+2];
			if(subname_buffer)
			{   
				LPBYTE subvalue_buffer=(LPBYTE)subname_buffer+dwMaxValueNameLen+1;
				dwNameBufferSize=dwMaxValueNameLen+1;
				dwValueBufferSize=dwMaxValueLen+1;
				DWORD dwType; dwIndex=0;
				while( ::RegEnumValue(hKEY,dwIndex,subname_buffer,&dwNameBufferSize,NULL,&dwType,subvalue_buffer,&dwValueBufferSize)==  ERROR_SUCCESS)
				{
					if(buffer.Space()<(dwNameBufferSize+100)){
						if( (dwNameBufferSize+=100)<256 ) dwNameBufferSize=256;
						if( buffer.Resize(buffer.size()+dwNameBufferSize)==NULL ) break;
					}
					buffer.len()+=sprintf(buffer.str()+buffer.len(),
						"<vitem><id>%d</id><rtype>%s</rtype>"
						"<rname><![CDATA[%s]]></rname>"
						"<rdlen>%d</rdlen>",
						++lret,STR_REG_TYPE[dwType],
						((subname_buffer[0]==0)?"(默认)":subname_buffer),
						dwValueBufferSize);
					
					if(dwType==REG_BINARY)
					{
						int i,j,lines=(dwValueBufferSize+15)/16; //计算共多少行
						size_t count=0;//打印字符计数
						if(lines>10) lines=10; //只显示10行的数据

						if(buffer.Space()<(lines*50+20)){
							if( buffer.Resize(buffer.size()+lines*50+30)==NULL ) break;
						}
						buffer.len()+=sprintf(buffer.str()+buffer.len(),"<rdata>");
						for(i=0;i<lines;i++)
						{
							count=i*16;
							for(j=0;j<16;j++)
							{
								if((count+j)<dwValueBufferSize)
									buffer.len()+=sprintf(buffer.str()+buffer.len(),"%02X ",subvalue_buffer[count+j]);
								else
									buffer.len()+=sprintf(buffer.str()+buffer.len(),"   ");
							}
							buffer.len()+=sprintf(buffer.str()+buffer.len(),"\r\n");
						}//?for(...
						buffer.len()+=sprintf(buffer.str()+buffer.len(),"</rdata>");	
					}else{
						if(buffer.Space()<(dwValueBufferSize+30)){
							if( (dwValueBufferSize+=30)<256 ) dwValueBufferSize=256;
							if(buffer.Resize(buffer.size()+dwValueBufferSize)==NULL) break;
						}
						if(dwType==REG_DWORD)
							buffer.len()+=sprintf(buffer.str()+buffer.len(),"<rdata>0x%X</rdata>",*((DWORD *)subvalue_buffer));
						else if(dwType==REG_DWORD_LITTLE_ENDIAN)
							buffer.len()+=sprintf(buffer.str()+buffer.len(),"<rdata>0x%X</rdata>",*((DWORD *)subvalue_buffer));
						else if(dwType==REG_DWORD_BIG_ENDIAN)
						{
							DWORD dw=*((DWORD *)subvalue_buffer);
							dw=ntohl(dw);
							buffer.len()+=sprintf(buffer.str()+buffer.len(),"<rdata>0x%X</rdata>",dw);
						}
						else if(dwType==REG_MULTI_SZ)
						{
							buffer.len()+=sprintf(buffer.str()+buffer.len(),"<rdata><![CDATA[%S]]></rdata>",subvalue_buffer);
						}
						else
						{
							if(subvalue_buffer[0]==0)
								buffer.len()+=sprintf(buffer.str()+buffer.len(),"<rdata></rdata>");
							else
								buffer.len()+=sprintf(buffer.str()+buffer.len(),"<rdata><![CDATA[%s]]></rdata>",subvalue_buffer);
						}
					} //非二进制数据
					if(buffer.Space()<12) buffer.Resize(buffer.size()+12);
					if(buffer.str()) buffer.len()+=sprintf(buffer.str()+buffer.len(),"</vitem>");
					dwNameBufferSize=dwMaxValueNameLen+1;
					dwValueBufferSize=dwMaxValueLen+1; 
					dwIndex++;
				}//?while
				delete[] subname_buffer;
			}else lret=-1;
			::RegCloseKey(hKEY);
		}else lret=-1;
	}else lret=-1;
	
	if(lret==0)
	{
		if(buffer.Space()<128) buffer.Resize(buffer.size()+128);
		if(buffer.str()) 
			buffer.len()+=sprintf(buffer.str()+buffer.len(),"<vitem><id></id><rtype>REG_SZ</rtype><rname>(默认)</rname>"
						"<rdlen>0</rdlen><rdata>(数据未设置)</rdata></vitem>");
	}
	if(buffer.Space()<16) buffer.Resize(buffer.size()+16);
	if(buffer.str()) buffer.len()+=sprintf(buffer.str()+buffer.len(),"</regitems>");
	return (lret<0)?false:true;
}



