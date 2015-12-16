var userQX=0;
var ssid="";
function ArrangeTaskList(){
	var TotalHeight=document.body.clientHeight;
	var TabHeight=0;
	var i;
	var tabtool = document.all.tags("TABLE");
	for(i=0;i<tabtool.length;i++)
	{
		if(tabtool(i).item=="1")
		{
			TabHeight+=27;
		}
	}
	var show=true;
	for(i=0;i<tabtool.length;i++)
	{
		if(tabtool(i).item=="1")
		{
			tabtool(i).style.height=(TotalHeight-TabHeight-4)+"px";
			if(show)
			{
				tabtool(i).style.visibility="visible";
				tabtool(i).style.display="";
				show=false;
			}
			else
			{
				tabtool(i).style.visibility="hidden";
				tabtool(i).style.display="none";
			}
		}
	}		
}

function Show_ChangeTool(name){
	var Arr=new Array("Control","Manage","VIDC"); 
	var i;
	for(i=0;i<Arr.length;i++)
	{
		if(document.all("Tab"+Arr[i])!=null)
		{
			if(Arr[i]==name)
			{
				document.all("Tab"+Arr[i]).style.visibility="visible";
				document.all("Tab"+Arr[i]).style.display="block";
				document.all("td"+Arr[i]).bgColor='#e5e5e5';
				document.all("td"+Arr[i]).onmouseout="";
				document.all("td"+Arr[i]).onmouseover="";
			}
			else
			{
				document.all("Tab"+Arr[i]).style.visibility="hidden";
				document.all("Tab"+Arr[i]).style.display="none";
				document.all("td"+Arr[i]).bgColor='Silver';
				document.all("td"+Arr[i]).onmouseout=new Function("this.bgColor='Silver'");
				document.all("td"+Arr[i]).onmouseover=new Function("this.bgColor='#e5e5e5'");
			}
		}
	}
}

function setButtonStatus()
{
	if((userQX & ACCESS_SCREEN_ALL)!=0)
    	{
    		o=document.all("fView")
    		o.disabled=false;
    		o.href="viewScreen.htm";
    		if((userQX & ACCESS_SCREEN_ALL)==ACCESS_SCREEN_ALL)
    		{
    		var o=document.all("fKeys");
    		o.disabled=false;
    		o.href="javascript:sendKey();";
    		o=document.all("fControl")
    		o.disabled=false;
    		o.href="viewCtrl.htm";
    		o=document.all("fCtAlDe")
    		o.disabled=false;
    		o.href="/command?cmd=CtAlDe";
    		o=document.all("fSetClip")
    		o.disabled=false;
    		o.href="javascript:SetClipBoard();";
    		o=document.all("fGetClip")
    		o.disabled=false;
    		o.href="javascript:GetClipBoard();";
    		o=document.all("fRun")
    		o.disabled=false;
    		o.href="javascript:RunProcess();";
    		o=document.all("fShDw")
    		o.disabled=false;
    		o.href="/command?cmd=ShDw";
    		o=document.all("fRest")
    		o.disabled=false;
    		o.href="/command?cmd=ReSt";
    		o=document.all("fLgOf")
    		o.disabled=false;
    		o.href="/command?cmd=LgOf";
    		o=document.all("fLock")
    		o.disabled=false;
    		o.href="/command?cmd=Lock";
    		o=document.all("fProc")
    		o.disabled=false;
    		o.href="viewProcess.htm";
    		o=document.all("fPort")
    		o.disabled=false;
    		o.href="viewPort.htm";
    		}
    	}
    	
    	if((userQX & ACCESS_REGIST_ALL)!=0)
    	{
    		var o=document.all("fReg");
    		o.disabled=false;
    		o.href="viewReg.htm";
	}
    	if((userQX & ACCESS_SERVICE_ALL)!=0)
    	{
    		var o=document.all("fServ");
    		o.disabled=false;
    		o.href="viewService.htm";
	}
    	if((userQX & ACCESS_TELNET_ALL)==ACCESS_TELNET_ALL)
    	{
    		var o=document.all("fTele");
    		o.disabled=false;
    		o.href="javascript:start_telnet()";
	}
	if((userQX & ACCESS_FILE_ALL)!=0)
    	{
    		var o=document.all("fFile");
    		o.disabled=false;
    		o.href="viewFile.htm";
    	}
    	if((userQX & ACCESS_FTP_ADMIN)!=0)
    	{
    		var o=document.all("fFTP");
    		o.disabled=false;
    		o.href="ftpsets.htm";
    	}
    	if((userQX & ACCESS_VIDC_ADMIN)!=0)
    	{
    		var o=document.all("fUPnP");
    		o.disabled=false;
    		o.href="upnp.htm";
    		o=document.all("fMapL");
    		o.disabled=false;
    		o.href="vidcMapL.htm";
    		o=document.all("fMapR");
    		o.href="vidcMapR.htm";
    		o.disabled=false;
    		o=document.all("fProxy");
    		o.disabled=false;
    		o.href="proxysets.htm";
    		o=document.all("fvIDCs");
    		o.disabled=false;
    		o.href="vidcsvr.htm";
    		o=document.all("fvIDCi");
    		o.disabled=false;
    		o.href="vidcini.htm";
    	}
}

function processRequest() 
{
	if (xmlHttp.readyState == 4) { // 判断对象状态
		if (xmlHttp.status == 200) 
		{ // 信息已经成功返回，开始处理信息
		//	alert(xmlHttp.responseText);
			var xmlobj = xmlHttp.responseXML;
			var quality = xmlobj.getElementsByTagName("quality");
			document.all("quality"+quality[0].firstChild.data).checked=true;
			var qxnode=xmlobj.getElementsByTagName("ssid");
			if(qxnode.length>0) ssid=qxnode[0].firstChild.data;
			qxnode=xmlobj.getElementsByTagName("qx");
			if(qxnode.length>0) userQX=qxnode[0].firstChild.data;
			var userAgent=xmlobj.getElementsByTagName("userAgent");
			strUserAgent=userAgent[0].firstChild.data;
			setButtonStatus(); 
            	}
        }
}

function processRequestX() 
{
	if (xmlHttp.readyState == 4) { // 判断对象状态
		if (xmlHttp.status == 200) { // 信息已经成功返回，开始处理信息
			
			var xmlobj = xmlHttp.responseXML;
    			var retmsg=xmlobj.getElementsByTagName("retmsg");
    			var dwurl=xmlobj.getElementsByTagName("dwurl");
    			if(retmsg.length>0)
				alert(retmsg.item(0).text);
			if(dwurl.length>0)
    				window.open(dwurl.item(0).text);
			//refreshScreen();
            	} //else alert("请求的页面有异常,status="+xmlHttp.status);
        }
}

function window_onload()
{
	ArrangeTaskList();
	Show_ChangeTool('Control');
	if(!xmlHttp) createXMLHttpRequest();
	xmlHttp.open("GET", "/capSetting", true);
	xmlHttp.onreadystatechange = processRequest;
    	xmlHttp.send(null);
}

function capsetting()
{
	var lockmskb=0; var qualityVal=0;
//	if(document.all("chkMsKb").checked) lockmskb=1;
	if(document.all("quality10").checked) qualityVal=10;
	else if(document.all("quality30").checked) qualityVal=30;
	else if(document.all("quality60").checked) qualityVal=60;
	else if(document.all("quality90").checked) qualityVal=90;
	else qualityVal=60;
	xmlHttp.open("GET", "/capSetting?quality="+qualityVal+"&lockmskb="+lockmskb, true);
	xmlHttp.onreadystatechange = processRequest;
    	xmlHttp.send(null);
}

function chkAuto_click(e)
{
	var w=window.parent.frmView; 
	if(e.checked)
	{ 
		var interval=prompt("请设定自动刷新间隔,范围[300~8000]毫秒",e.value);
		if(interval<300 || interval>8000) interval=e.value;
		w.autoRefresh=interval;
		document.all("lblInterval").innerText=interval+"ms";
		w.loadImg();
	} else{
		document.all("lblInterval").innerText="";
	 	w.autoRefresh=0;
	}
}

function refreshScreen()
{
	var w=window.parent.frmView;
	w.imgLoaded=true;
	w.loadImg(); 
}
//发送特殊按键
function sendKey()
{
	var altk=document.all("selCSA").value;
	var fx=document.all("selFX").value;
	altk=altk*256+fx*1;
	xmlHttp.open("GET","/keyevent?vkey="+altk+",",false);
	xmlHttp.send(null);
	refreshScreen();
	
}

//设置剪贴板内容
function SetClipBoard()
{
	var v=window.showModalDialog("setClipBoard.htm","","dialogHeight:300px;dialogWidth=300px;center:yes;status:no;scroll:no;");
	if(v!=null && v!="")
	{
		var v1=v.replace(/&/g,"%26"); //对特殊字符&进行mime编码
		var strEncode="val="+v1;
		xmlHttp.open("POST", "/SetClipBoard", true);
		xmlHttp.setRequestHeader("Content-Type","application/x-www-form-urlencoded; charset=utf-8");
//		xmlHttp.setRequestHeader("Content-Length",strEncode.length); //此句可不要，对象会自动加上
		xmlHttp.onreadystatechange = processRequest;
		xmlHttp.send(strEncode);
	}
}

//获得剪贴板内容
function GetClipBoard()
{
	window.open("/GetClipBoard","_blank","height=300,width=300,directories=no,location=no,menubar=no,status=no,toolbar=no");
}

function RunProcess()
{
	var scmd=prompt("请键入程序、Iternet资源名称或扩展命令,服务将为你远程执行或打开它","");
	if(scmd==null) return;
	var strEncode="path="+scmd.replace(/&/g,"%26");
	xmlHttp.open("POST","/file_run",true);
	xmlHttp.setRequestHeader("Content-Type","application/x-www-form-urlencoded; charset=utf-8");
	xmlHttp.onreadystatechange = processRequestX;
	xmlHttp.send(strEncode);
}

function start_telnet()
{
	xmlHttp.open("GET", "/telnet", false);
	xmlHttp.send(null);
	var rspUrl=xmlHttp.responseText;
	var s=rspUrl.substr(0,7);
	if(s=="telnet:")
		window.location.href=rspUrl;
	else alert(s);
	return;
}
function fileNanage()
{
	xmlHttp.open("GET", "/ftp", false);
	xmlHttp.send(null);
	var rspUrl=xmlHttp.responseText;
	var s=rspUrl.substr(0,4);
	if(s=="ftp:")
	{
		window.open(rspUrl);
	} else alert(s);
	return;
}


