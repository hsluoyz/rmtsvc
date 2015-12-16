var strUserAgent="";
var xmlHttp=false;
var oPopup=false;
var timerID_popup=0;
var ACCESS_SCREEN_ALL=0x0003;
var ACCESS_REGIST_ALL=0x000c
var ACCESS_SERVICE_ALL=0x0030
var ACCESS_TELNET_ALL=0x00c0
var ACCESS_FILE_ALL=0x0300
var ACCESS_FTP_ADMIN=0x0c00
var ACCESS_VIDC_ADMIN=0x3000

function Str2Bytes(str,charset)
{
	var ms=false;
	try
	{
		ms = new ActiveXObject("ADODB.Stream"); //建立流对象
		if(!ms) throw "error";
	}
	catch(e)
	{
		return str;
	}
	
	ms.Type = 2;			//Text
        ms.Charset = charset;		//设置流对象的编码方式为 charset
        ms.Open();
        ms.WriteText(str);		//把str写入流对象中
        
        ms.Position = 0;		//设置流对象的起始位置是0 以设置Charset属性
        ms.Type = 1;			//Binary
        vout = ms.Read(ms.Size);		//取字符流
        ms.close();			//关闭流对象
        return vout;
}

function createXMLHttpRequest() {
    if (window.ActiveXObject) {
        xmlHttp = new ActiveXObject("Microsoft.XMLHTTP");
    } 
    else if (window.XMLHttpRequest) {
        xmlHttp = new XMLHttpRequest();
    }
    if(!xmlHttp) alert("创建XMLHTTP对象失败");
}

function createpopup()
{
	if(strUserAgent.indexOf("MSIE")!=-1)
	{//仅仅IE支持
	oPopup = window.createPopup();
	var oPopBody = oPopup.document.body;
    	oPopBody.style.backgroundColor = "lightyellow";
    	oPopBody.style.border = "solid black 1px";
    	oPopBody.innerHTML = "&nbsp;&nbsp;正在处理中... ";
	}
}
var ista=0;
function moviePopup()
{
	var oPopBody = oPopup.document.body;
	if(ista==0)
	{
		oPopBody.innerHTML = "&nbsp;&nbsp;正在处理中../ ";
		ista=1;
	}
	else if(ista==1)
	{
		oPopBody.innerHTML = "&nbsp;&nbsp;正在处理中..- ";
		ista=2;
	}
	else if(ista==2)
	{
		oPopBody.innerHTML = "&nbsp;&nbsp;正在处理中..\\ ";
		ista=0;
	}
}
function showPopup(lx,ty,w,h)
{
	if(!oPopup) return;
	oPopup.show(lx, ty, w, h, document.body);
	timerID_popup=window.setInterval(moviePopup,500);
}

function hidePopup()
{
	if(!oPopup) return;
	if(timerID_popup!=0) window.clearInterval(timerID_popup);
	if(oPopup.isOpen) oPopup.hide();
	var oPopBody = oPopup.document.body;
	oPopBody.innerHTML = "&nbsp;&nbsp;正在处理中... ";
	ista=0;
}
