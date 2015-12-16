//ftpsets.js
function disp_status(xmlobj)
{
	var node=xmlobj.getElementsByTagName("status");
	var port=0;
	if(node.length>0) port=node.item(0).text;
	if(port>0)
	{
		document.getElementById("btnStart").disabled=true;
		document.getElementById("btnStop").disabled=false;
		node=xmlobj.getElementsByTagName("ifssl");
		if(node.length>0 && node.item(0).text!="0")
			document.getElementById("lblStatus").innerHTML="<font color=green>●</font>SSL加密服务正在运行... &nbsp;端口:"+port;
		else	document.getElementById("lblStatus").innerHTML="<font color=green>●</font>服务正在运行... &nbsp;端口:"+port;
	}
	else
	{
		document.getElementById("btnStart").disabled=false;
		document.getElementById("btnStop").disabled=true;
		document.getElementById("lblStatus").innerHTML="<font color=red>●</font>服务停止运行";
	}
	node=xmlobj.getElementsByTagName("curtime");
	if(node.length>0) document.getElementById("lblCurtime").innerText=node.item(0).text;
	node=xmlobj.getElementsByTagName("starttime");
	if(node.length>0) document.getElementById("lblRuntime").innerText=node.item(0).text;
	node=xmlobj.getElementsByTagName("connected");
	if(node.length>0) document.getElementById("lblConnected").innerText=node.item(0).text;
	node=xmlobj.getElementsByTagName("svrport");
	if(node.length>0) document.getElementById("lblSvrport").value=node.item(0).text;
	node=xmlobj.getElementsByTagName("bindip");
	var bindip="";
	if(node.length>0) bindip=node.item(0).text;
	var oSelect=document.getElementById("lblSvrip");
	for(i=oSelect.options.length;i>1;i--) oSelect.options.remove(i-1);
	node=xmlobj.getElementsByTagName("localip");
	if(node.length>0)
	{
		var oSelect=document.getElementById("lblSvrip");
		var localip=node.item(0).text;
		var ss=localip.split(" ");
		for(i=0;i<ss.length;i++)
		{
			var oOption = document.createElement("OPTION");
			oOption.text=ss[i];
			oOption.value=ss[i];
			if(bindip!="" && bindip==ss[i]) oOption.selected=true;
			oSelect.add(oOption);
		}
	}
	node=xmlobj.getElementsByTagName("svrtype");
	if(node.length>0 && node.item(0).text!=0) 
		document.getElementById("ftptype1").checked=true;
	else	document.getElementById("ftptype0").checked=true;
	node=xmlobj.getElementsByTagName("autorun");
	if(node.length>0 && node.item(0).text!=0) 
		document.getElementById("chkAutorun").checked=true;
	else document.getElementById("chkAutorun").checked=false;
	node=xmlobj.getElementsByTagName("maxconn");
	if(node.length>0 && node.item(0).text!=0) 
		document.getElementById("lblMaxconn").value=node.item(0).text;
	else	document.getElementById("lblMaxconn").value="不限";
	node=xmlobj.getElementsByTagName("dataport");
	if(node.length>0 && node.item(0).text!="0-0") 
		document.getElementById("lblDataport").value=node.item(0).text;
	else	document.getElementById("lblDataport").value="不限";
	var logevent=0;
	node=xmlobj.getElementsByTagName("logging");
	if(node.length>0) logevent=node.item(0).text;
	for(i=0;i<6;i++)
	{
		var idx=1<<i;
		if(logevent & idx) 
			document.getElementById("chkLog"+idx).checked=true;
		else	document.getElementById("chkLog"+idx).checked=false;
	}
	node=xmlobj.getElementsByTagName("tips");
	if(node.length>0) document.getElementById("lblTips").value=node.item(0).text;
}

function processRequest() 
{
	if (xmlHttp.readyState == 4) { // 判断对象状态
		if (xmlHttp.status == 200) { // 信息已经成功返回，开始处理信息
			
			var xmlobj = xmlHttp.responseXML;
			var stat_xml=xmlobj.selectSingleNode("//ftp_status")
			if(stat_xml!=null && stat_xml.childNodes.length>0)
				disp_status(stat_xml);
		
            	} //else alert("请求的页面有异常,status="+xmlHttp.status);
            	hidePopup();
        }
}

function window_onload()
{
	if(!oPopup) createpopup();   	
	if(!xmlHttp) createXMLHttpRequest();
	ftp_status();
}

function ftp_status()
{
	showPopup(250, 200, 150, 20);
	xmlHttp.open("GET", "/ftpsets?cmd=stat", true);
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send( null );
}

function ftp_start()
{
	showPopup(250, 200, 150, 20);
	xmlHttp.open("GET", "/ftpsets?cmd=run", true);
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send( null );
}

function ftp_stop()
{
	showPopup(250, 200, 150, 20);
	xmlHttp.open("GET", "/ftpsets?cmd=stop", true);
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send( null );
}

//有效性检查
function chkvalid()
{
	var rets="";
	if(document.getElementById("lblSvrport").value=="")
	{
		alert("请输入FTP服务的端口号,0由系统自动分配"); 
		document.getElementById("lblSvrport").focus();
		return "";
	}else
		rets="svrport="+document.getElementById("lblSvrport").value;
	rets=rets+"&bindip="+document.getElementById("lblSvrip").value;
	if(document.getElementById("ftptype1").checked)
		rets=rets+"&svrtype=1";
	else	rets=rets+"&svrtype=0";
	if(document.getElementById("chkAutorun").checked)
		rets=rets+"&autorun=1";
	else	rets=rets+"&autorun=0";
	if(document.getElementById("lblMaxconn").value=="不限")
		rets=rets+"&maxconn=0";
	else	rets=rets+"&maxconn="+document.getElementById("lblMaxconn").value;
	if(document.getElementById("lblDataport").value=="不限")
		rets=rets+"&dataport=0-0";
	else rets=rets+"&dataport="+document.getElementById("lblDataport").value;
	var logging=0;
	for(i=0;i<6;i++)
	{
		var idx=1<<i;
		if(document.getElementById("chkLog"+idx).checked) logging=logging+idx;
	}
	rets=rets+"&logging="+logging;
	var s=document.getElementById("lblTips").value
	rets=rets+"&tips="+s.replace(/&/g,"%26");
	return rets;
}

function ftp_sets()
{
	var strEncode=chkvalid();
	if(strEncode=="") return;
	showPopup(250, 200, 150, 20); 
	xmlHttp.open("POST", "/ftpsets?cmd=setting", true);
	xmlHttp.setRequestHeader("Content-Type","application/x-www-form-urlencoded; charset=utf-8");
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send( strEncode );
}
