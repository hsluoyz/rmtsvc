var qx=0;
var spath="";

function processRequest() 
{
	if (xmlHttp.readyState == 4) { // 判断对象状态
		if (xmlHttp.status == 200) 
		{ // 信息已经成功返回，开始处理信息
			var xmlobj = xmlHttp.responseXML;
			var node = xmlobj.getElementsByTagName("fname");
			if(node.length>0)
				document.all("fname").innerText=node.item(0).text;
			node = xmlobj.getElementsByTagName("ftype");
			if(node.length>0)
				document.all("ftype").innerText=node.item(0).text;
			node = xmlobj.getElementsByTagName("opmode");
			if(node.length>0)
				document.all("opmode").innerText=node.item(0).text;
			node = xmlobj.getElementsByTagName("fpath");
			if(node.length>0)
				document.all("fpath").innerText=node.item(0).text;
			node = xmlobj.getElementsByTagName("fsize");
			if(node.length>0)
				document.all("fsize").innerText=node.item(0).text;
			node = xmlobj.getElementsByTagName("fctime");
			if(node.length>0)
				document.all("fctime").innerText=node.item(0).text;
			node = xmlobj.getElementsByTagName("fmtime");
			if(node.length>0)
				document.all("fmtime").innerText=node.item(0).text;
			node = xmlobj.getElementsByTagName("fatime");
			if(node.length>0)
				document.all("fatime").innerText=node.item(0).text;
			node = xmlobj.getElementsByTagName("protype");
			if(node.length>0)
			{
				var pro=node.item(0).text;
				var pos=pro.indexOf('R');
				if(pos!=-1) document.all("chkRead").checked=true;
				pos=pro.indexOf('H');
				if(pos!=-1) document.all("chkHide").checked=true;
				pos=pro.indexOf('A');
				if(pos!=-1) document.all("chkAchi").checked=true;
			}
            	}
            hidePopup();
        }
}

function processRequest_ver() 
{
	if (xmlHttp.readyState == 4) { // 判断对象状态
		if (xmlHttp.status == 200) 
		{ // 信息已经成功返回，开始处理信息
			var xmlobj = xmlHttp.responseXML;
			var node = xmlobj.getElementsByTagName("FileVer");
			if(node.length>0)
				document.all("ver_fver").innerText=node.item(0).text;
			node = xmlobj.getElementsByTagName("FileDesc");
			if(node.length>0)
				document.all("ver_fdesc").innerText=node.item(0).text;
			node = xmlobj.getElementsByTagName("Copyright");
			if(node.length>0)
				document.all("ver_cpyr").innerText=node.item(0).text;
			node = xmlobj.getElementsByTagName("Company");
			if(node.length>0)
				document.all("ver_company").innerText=node.item(0).text;
			node = xmlobj.getElementsByTagName("Comments");
			if(node.length>0)
				document.all("ver_comments").innerText=node.item(0).text;
			node = xmlobj.getElementsByTagName("ProductName");
			if(node.length>0)
				document.all("ver_pname").innerText=node.item(0).text;
			node = xmlobj.getElementsByTagName("InterName");
			if(node.length>0)
				document.all("ver_intername").innerText=node.item(0).text;
				
            	}
            hidePopup();
        }
}


function window_onload()
{
	spath=window.dialogArguments;
	var p=spath.indexOf(',');
	if(p!=-1)
	{
		qx=spath.substr(0,p);
		spath=spath.substr(p+1);
	}
	
	GetFileInfo();
}

function mdProperty()
{
	showPopup(100, 150, 150, 20);
	
	var pf="";
	if(document.all("chkRead").checked) pf=pf+"R";
	if(document.all("chkHide").checked) pf=pf+"H";
	if(document.all("chkAchi").checked) pf=pf+"A";
	xmlHttp.open("GET", "/profile?path="+spath+"&prof="+pf, true);
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send( null );
}

function GetFileInfo()
{
	document.all("divVerInfo").style.visibility="hidden";
	document.all("divVerInfo").style.display="none";
	document.all("divFileInfo").style.visibility="visible";
	document.all("divFileInfo").style.display="";
	
	if(!oPopup) createpopup();
	if(!xmlHttp) createXMLHttpRequest();
	xmlHttp.open("GET", "/profile?path="+spath, true);
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send( null );
	showPopup(100, 150, 150, 20);
	
	if((qx & ACCESS_FILE_ALL)==ACCESS_FILE_ALL)
	{
		document.all("chkRead").disabled=false;
		document.all("chkHide").disabled=false;
		document.all("chkAchi").disabled=false;
		document.all("btnApply").disabled=false;
	}
}

function GetVerInfo()
{
	document.all("divFileInfo").style.visibility="hidden";
	document.all("divFileInfo").style.display="none";
	document.all("divVerInfo").style.visibility="visible";
	document.all("divVerInfo").style.display="";
	
	if(!oPopup) createpopup();
	if(!xmlHttp) createXMLHttpRequest();
	xmlHttp.open("GET", "/profile_ver?path="+spath, true);
	xmlHttp.onreadystatechange = processRequest_ver;
	xmlHttp.send( null );
	showPopup(100, 150, 150, 20);
}
