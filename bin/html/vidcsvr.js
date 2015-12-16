//vidcsvr.js
function disp_status(xmlobj)
{
	var node=xmlobj.getElementsByTagName("status");
	var port=0;
	if(node.length>0) port=node.item(0).text;
	if(port>0)
	{
		document.getElementById("btnStart").disabled=true;
		document.getElementById("btnStop").disabled=false;
		document.getElementById("btnSet").disabled=true;
		node=xmlobj.getElementsByTagName("ifssl");
		if(node.length>0 && node.item(0).text!="0")
			document.getElementById("lblStatus").innerHTML="<font color=green>●</font>SSL加密服务正在运行... &nbsp;端口:"+port;
		else	document.getElementById("lblStatus").innerHTML="<font color=green>●</font>服务正在运行... &nbsp;端口:"+port;
	}
	else
	{
		document.getElementById("btnStart").disabled=false;
		document.getElementById("btnStop").disabled=true;
		document.getElementById("btnSet").disabled=false;
		document.getElementById("lblStatus").innerHTML="<font color=red>●</font>服务停止运行";
	}
	node=xmlobj.getElementsByTagName("curtime");
	if(node.length>0) document.getElementById("lblCurtime").innerText=node.item(0).text;
	node=xmlobj.getElementsByTagName("starttime");
	if(node.length>0) document.getElementById("lblRuntime").innerText=node.item(0).text;
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
	node=xmlobj.getElementsByTagName("pswd");
	if(node.length>0)
		document.getElementById("authpswd").value=node.item(0).text;
	else	document.getElementById("authpswd").value="";
	node=xmlobj.getElementsByTagName("bauth");
	if(node.length>0 && node.item(0).text==1)
	{
		document.getElementById("chkAuth").checked=true;
		document.getElementById("authpswd").disabled=false;
	}else{
	 document.getElementById("chkAuth").checked=false;
	 document.getElementById("authpswd").disabled=true;
	}
	node=xmlobj.getElementsByTagName("autorun");
	if(node.length>0 && node.item(0).text==1) 
		document.getElementById("chkAutorun").checked=true;
	else	document.getElementById("chkAutorun").checked=false;
	
	var ipfilter_xml=xmlobj.selectSingleNode("//ipfilter")
	if(ipfilter_xml!=null && ipfilter_xml.childNodes.length>0)
	{
		node=ipfilter_xml.getElementsByTagName("ipaddr");
		document.getElementById("lblIPAddr").value=node.item(0).text;
		node=ipfilter_xml.getElementsByTagName("access");
		if(node.length>0 && node.item(0).text=="1")
		{
			document.getElementById("chkIPAccess1").checked=true;
			document.getElementById("chkIPAccess0").checked=false;
		}else{
			document.getElementById("chkIPAccess1").checked=false;
			document.getElementById("chkIPAccess0").checked=true;
		}
	}
}

function disp_vidcc_status(xmlobj)
{
	var node=xmlobj.getElementsByTagName("vname");
	if(node.length>0)
		document.getElementById("vidccname").innerText=node.item(0).text;
	node=xmlobj.getElementsByTagName("vip");
	if(node.length>0)
		document.getElementById("vidccip").innerText=node.item(0).text;
	node=xmlobj.getElementsByTagName("ver");
	if(node.length>0)
		document.getElementById("lblVer").innerText=node.item(0).text;
	node=xmlobj.getElementsByTagName("starttime");
	if(node.length>0)
		document.getElementById("lblConntime").innerText=node.item(0).text;
	node=xmlobj.getElementsByTagName("vdesc");
	if(node.length>0)
		document.getElementById("lblDesc").innerText=node.item(0).text;
	
	var maplist=xmlobj.selectSingleNode("//maplist")
	if(maplist!=null && maplist.childNodes.length>0)
		infoXML.appendChild(maplist);
}

function processRequest() 
{
	if (xmlHttp.readyState == 4) { // 判断对象状态
		if (xmlHttp.status == 200) { // 信息已经成功返回，开始处理信息
			
			var xmlobj = xmlHttp.responseXML;
			var vidccs=xmlobj.selectSingleNode("//vidccs")
			if(vidccs!=null)
			{
				if(vidccXML.documentElement!=null)
					vidccXML.removeChild(vidccXML.documentElement);
				if(vidccs.childNodes.length>0) vidccXML.appendChild(vidccs);
			}
			var stat_xml=xmlobj.selectSingleNode("//vidcs_status")
			if(stat_xml!=null && stat_xml.childNodes.length>0)
				disp_status(stat_xml);
			
			var vidcc_stat_xml=xmlobj.selectSingleNode("//vidcc_status")
			if(vidcc_stat_xml!=null && vidcc_stat_xml.childNodes.length>0)
				disp_vidcc_status(vidcc_stat_xml);
            	} //else alert("请求的页面有异常,status="+xmlHttp.status);
            	hidePopup();
        }
}

function window_onload()
{
	if(!oPopup) createpopup();   	
	if(!xmlHttp) createXMLHttpRequest();
	vidcs_status();
}

function vidcs_status()
{
	init_vidccinfo();
	showPopup(250, 200, 150, 20);
	xmlHttp.open("GET", "/vidcsvr?cmd=stat", true);
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send( null );
}

function vidcs_start()
{
	showPopup(250, 200, 150, 20);
	xmlHttp.open("GET", "/vidcsvr?cmd=run", true);
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send( null );
}

function vidcs_stop()
{
	showPopup(250, 200, 150, 20);
	xmlHttp.open("GET", "/vidcsvr?cmd=stop", true);
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send( null );
}

//有效性检查
function chkvalid()
{
	var rets="";
	if(document.getElementById("lblSvrport").value=="")
	{
		alert("请输入vIDCs服务的端口号,0由系统自动分配"); 
		document.getElementById("lblSvrport").focus();
		return "";
	}else
		rets="svrport="+document.getElementById("lblSvrport").value;
	rets=rets+"&bindip="+document.getElementById("lblSvrip").value;
	if(document.getElementById("chkAutorun").checked)
		rets=rets+"&autorun=1";
	else	rets=rets+"&autorun=0";
	if(document.getElementById("chkAuth").checked)
		rets=rets+"&bauth=1";
	else	rets=rets+"&bauth=0";
	rets=rets+"&pswd="+document.getElementById("authpswd").value;
	
	if(document.getElementById("lblIPAddr").value!="")
	{
		var s=document.getElementById("lblIPAddr").value;
		var re=/^(((\d{1,3}|\*{1}).){3}(\d{1,3}|\*{1}),)*((\d{1,3}|\*{1}).){3}(\d{1,3}|\*{1}),?$/;
		if(!re.test(s))
		{
			alert("请输入正确的IP过滤地址");
			return "";
		}
		if(document.getElementById("chkIPAccess1").checked==false && 
		   document.getElementById("chkIPAccess0").checked==false )
		{
		   	alert("请设置IP过滤的访问许可/禁止");
		   	return "";
		}
		rets=rets+"&ipaddr="+document.getElementById("lblIPAddr").value;
		if(document.getElementById("chkIPAccess1").checked)
			rets=rets+"&ipaccess=1";
		else rets=rets+"&ipaccess=0";
	}else rets=rets+"&ipaccess=1&ipaddr=";
	return rets;
}

function vidcs_sets()
{
	var strEncode=chkvalid();
	if(strEncode=="") return;
	showPopup(250, 200, 150, 20); 
	xmlHttp.open("POST", "/vidcsvr?cmd=setting", true);
	xmlHttp.setRequestHeader("Content-Type","application/x-www-form-urlencoded; charset=utf-8");
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send( strEncode );
}

function vidcc_status()
{
	var vidccID=vidccXML.recordset("vID");
	if(vidccID==0) return;
	init_vidccinfo();
	showPopup(250, 200, 150, 20);
	xmlHttp.open("GET", "/vidccs?cmd=info&vid="+vidccID, true);
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send( null );
}

function vidcc_disconn()
{
	var vidccID=vidccXML.recordset("vID");
	if(vidccID==0) return;
	init_vidccinfo();
	showPopup(250, 200, 150, 20);
	xmlHttp.open("GET", "/vidccs?cmd=dele&vid="+vidccID, true);
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send( null );
}
function vidcc_setlog()
{
	if(infoXML.documentElement==null) return;
	var blogd=infoXML.recordset("blogd");
	var vidccID=vidccXML.recordset("vID");
	if(vidccID==0) return;
	init_vidccinfo();
	showPopup(250, 200, 150, 20);
	if(blogd=="*")
		xmlHttp.open("GET", "/vidccs?cmd=sets&blogd=0&vid="+vidccID, true);
	else xmlHttp.open("GET", "/vidccs?cmd=sets&blogd=1&vid="+vidccID, true);
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send( null );
}

function vidccClick(tblElement)
{
	var row=tblElement.rowIndex;
	vidccXML.recordset.absoluteposition=row;
	vidcc_status();
}

function init_vidccinfo()
{
	document.getElementById("vidccname").innerText="vidcc";
	document.getElementById("vidccip").innerText="";
	document.getElementById("lblVer").innerText="";
	document.getElementById("lblConntime").innerText="YYYY年MM月DD日 hh:mm:ss";
	document.getElementById("lblDesc").innerText="";
	if(infoXML.documentElement!=null)
		infoXML.removeChild(infoXML.documentElement);
}

function clkAuth(el)
{
	if(el.checked)
		document.getElementById("authpswd").disabled=false;
	else document.getElementById("authpswd").disabled=true;
}