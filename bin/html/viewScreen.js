var autoRefresh=0; //自动刷新时间ms
var imgLoaded=true;//屏幕捕获是否完成
function loadImg()
{
	if(imgLoaded)
	{
		document.screenimage.src="/capDesktop";
		imgLoaded=false;
	}
}

function Imgloaded()
{
	imgLoaded=true;
	if(autoRefresh>0) window.setTimeout("loadImg()",autoRefresh);
}

function msmove_IE()
{
 	var o=window.document.all("divScreen");
 	var x1=window.event.x+o.scrollLeft-o.parentElement.offsetLeft; 
  	var y1=window.event.y+o.scrollTop-o.parentElement.offsetTop; 
  	var w=window.parent.frmLeft;
  	w.document.all("lblXY").innerText="X:"+x1+" , Y:"+y1;
}

function processRequest() 
{
	if (xmlHttp.readyState == 4) { // 判断对象状态
		if (xmlHttp.status == 200) 
		{ // 信息已经成功返回，开始处理信息
			var xmlobj = xmlHttp.responseXML;
			var wtext=xmlobj.getElementsByTagName("wtext");
			var hwnd=xmlobj.getElementsByTagName("hwnd");
			if(hwnd!=null && hwnd.length>0)
			{
				loadImg();
				var sHelp="\r\n按下Alt然后左键单击密码框获取密码";
				if(hwnd.item(0).text==0)
					alert("当前捕获的是整个桌面，如果要捕获指定窗口\r\n按下Ctrl+Shift然后左键单击指定的窗口"+sHelp);
				else  alert("当前捕获 "+"\""+wtext.item(0).text+"\" 窗口\r\n按下Ctrl+Shift然后右键单击取消对此窗口的捕获"+sHelp);
			}
			var iret=xmlobj.getElementsByTagName("result");
			if(iret!=null && iret.length>0)
			{
				if(iret.item(0).text==0)
					alert("密码框中密码为: "+wtext.item(0).text);
				else if(iret.item(0).text==1)
					alert("非密码框,'"+wtext.item(0).text+"'")
				else alert("获取失败, err="+iret.item(0).text);
			}
            	}
        }
}

//获取单击动作
function msup()
{
	var o=window.document.all("divScreen");
 	var x1=window.event.x+o.scrollLeft-o.parentElement.offsetLeft; 
  	var y1=window.event.y+o.scrollTop-o.parentElement.offsetTop;
	var altk=0;
	if(window.event.ctrlKey) altk=altk | 1;
	if(window.event.shiftKey) altk=altk | 2;
	if(window.event.altKey) altk=altk | 4;
	
  	if(altk==4) //获取指定密码框中密码
  	{	
  		var qx=parent.frmLeft.userQX;
  		if((qx & ACCESS_SCREEN_ALL)==ACCESS_SCREEN_ALL)
  			xmlHttp.open("GET", "/getPassword?x="+x1+"&y="+y1, true);
  		else{ alert("你没有权限不允许查看密码框中密码"); return; }
  	}else{
  		var act=0;
  		if(altk==3 && window.event.button==1) act=1; //捕获指定的窗口
  		else if(altk==3 && window.event.button==2) act=2; //捕获整个桌面
  		xmlHttp.open("GET", "/capWindow?act="+act+"&x="+x1+"&y="+y1, true);
  	}
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send(null);
}

function window_onload()
{
	var o=window.parent.frmLeft.document.all("chkAuto");
	if( o.checked ) autoRefresh=o.value;
	if(!xmlHttp) createXMLHttpRequest();
	loadImg()
	return;	
}

