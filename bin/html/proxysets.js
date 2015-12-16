//proxysets.js
function clkCasAuth(e)
{
	if(e.checked)
	{
		document.getElementById("casuser").disabled=false;
		document.getElementById("caspswd").disabled=false;
	}
	else
	{
		document.getElementById("casuser").disabled=true;
		document.getElementById("caspswd").disabled=true;
	}
}
function clkCasecade(e)
{
	if(e.checked)
	{
		document.getElementById("casSvrIP").disabled=false;
		document.getElementById("casSvrport").disabled=false;
		document.getElementById("castype0").disabled=false;
		document.getElementById("castype1").disabled=false;
		document.getElementById("castype2").disabled=false; 
		document.getElementById("casAuth").disabled=false;
		clkCasAuth(document.getElementById("casAuth"));
	}
	else
	{
		document.getElementById("casSvrIP").disabled=true;
		document.getElementById("casSvrport").disabled=true;
		document.getElementById("castype0").disabled=true;
		document.getElementById("castype1").disabled=true;
		document.getElementById("castype2").disabled=true; 
		document.getElementById("casAuth").disabled=true;
		document.getElementById("casuser").disabled=true;
		document.getElementById("caspswd").disabled=true;
	}
}

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
		if(node.length>0 && node.item(0).text=="1")
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
	var svrtype=0;
	node=xmlobj.getElementsByTagName("svrtype");
	if(node.length>0) svrtype=node.item(0).text;
	if(svrtype & 1)
		document.getElementById("svrtype0").checked=true;
	else	document.getElementById("svrtype0").checked=false;
	if(svrtype & 2)
		document.getElementById("svrtype1").checked=true;
	else	document.getElementById("svrtype1").checked=false;
	if(svrtype & 4)
		document.getElementById("svrtype2").checked=true;
	else	document.getElementById("svrtype2").checked=false;
	
	node=xmlobj.getElementsByTagName("bauth");
	if(node.length>0 && node.item(0).text=="1")
		document.getElementById("chkAuth").checked=true;
	else document.getElementById("chkAuth").checked=false;
	
	node=xmlobj.getElementsByTagName("autorun");
	if(node.length>0 && node.item(0).text=="1") 
		document.getElementById("chkAutorun").checked=true;
	else	document.getElementById("chkAutorun").checked=false;
	
	node=xmlobj.getElementsByTagName("cascade");
	if(node.length>0 && node.item(0).text=="1") 
		document.getElementById("chkCascade").checked=true;
	else	document.getElementById("chkCascade").checked=false;
	node=xmlobj.getElementsByTagName("cassvrip");
	if(node.length>0) document.getElementById("casSvrIP").value=node.item(0).text;
	node=xmlobj.getElementsByTagName("cassvrport");
	if(node.length>0) document.getElementById("cassvrport").value=node.item(0).text;
	node=xmlobj.getElementsByTagName("castype");
	if(node.length>0) 
		svrtype=node.item(0).text;
	else	svrtype=0;
	if(svrtype & 1)
		document.getElementById("castype0").checked=true;
	else	document.getElementById("castype0").checked=false;
	if(svrtype & 2)
		document.getElementById("castype1").checked=true;
	else	document.getElementById("castype1").checked=false;
	if(svrtype & 4)
		document.getElementById("castype2").checked=true;
	else	document.getElementById("castype2").checked=false;
	node=xmlobj.getElementsByTagName("casauth");
	if(node.length>0 && node.item(0).text=="1")
		document.getElementById("casAuth").checked=true;
	else document.getElementById("casAuth").checked=false;
	node=xmlobj.getElementsByTagName("casuser");
	if(node.length>0)
		document.getElementById("casuser").value=node.item(0).text;
	else	document.getElementById("casuser").value="";
	node=xmlobj.getElementsByTagName("caspswd");
	if(node.length>0)
		document.getElementById("caspswd").value=node.item(0).text;
	else	document.getElementById("caspswd").value="";
	clkCasAuth(document.getElementById("chkAuth"));
	clkCasecade(document.getElementById("chkCascade"));
	
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

function processRequest() 
{
	if (xmlHttp.readyState == 4) { // 判断对象状态
		if (xmlHttp.status == 200) { // 信息已经成功返回，开始处理信息
			
			var xmlobj = xmlHttp.responseXML;
			var stat_xml=xmlobj.selectSingleNode("//proxy_status")
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
	proxy_status();
}

function proxy_status()
{
	showPopup(250, 200, 150, 20);
	xmlHttp.open("GET", "/proxysets?cmd=stat", true);
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send( null );
}

function proxy_start()
{
	showPopup(250, 200, 150, 20);
	xmlHttp.open("GET", "/proxysets?cmd=run", true);
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send( null );
}

function proxy_stop()
{
	showPopup(250, 200, 150, 20);
	xmlHttp.open("GET", "/proxysets?cmd=stop", true);
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send( null );
}

function isvalidInteger(s)
{
	var re=/^\d+$/;
	if(re.test(s)) return true;
	alert("请输入合法的服务端口"); 
	return false;
}
//有效性检查
function chkvalid()
{
	var rets="";
	if( !isvalidInteger(document.getElementById("lblSvrport").value) )
		return "";
	else
		rets="svrport="+document.getElementById("lblSvrport").value;
	rets=rets+"&bindip="+document.getElementById("lblSvrip").value;
	var svrtype=0;
	if(document.getElementById("svrtype0").checked)
		svrtype=svrtype+1;
	if(document.getElementById("svrtype1").checked)
		svrtype=svrtype+2;
	if(document.getElementById("svrtype2").checked)
		svrtype=svrtype+4;
	rets=rets+"&svrtype="+svrtype;		
	
	if(document.getElementById("chkAuth").checked)
		rets=rets+"&bauth=1";
	else rets=rets+"&bauth=0";
	if(document.getElementById("chkAutorun").checked)
		rets=rets+"&autorun=1";
	else	rets=rets+"&autorun=0";
	
	if(document.getElementById("chkCascade").checked)
	{
		rets=rets+"&cascade=1";
		var sip=document.getElementById("casSvrIP").value;
		var sport=document.getElementById("casSvrport").value;
		if(sport!="") sip=sip+":"+sport;
		//可输入多个应用服务地址，格式aaa.bb.cc:ddd,aaa1.bb1.cc1:dddd
		var re_sip=/^((\w+.?)+:\d+,?)+$/;
		if( !re_sip.test(sip) )
		{
			alert("二级代理服务地址和端口输入不正确"); 
			document.getElementById("casSvrIP").focus();
			return "";
		}else rets=rets+"&cassvrip="+sip;
		var castype=0;
		if(document.getElementById("castype0").checked)
			castype=castype+1;
		if(document.getElementById("castype1").checked)
			castype=castype+2;
		if(document.getElementById("castype2").checked)
			castype=castype+4;
		rets=rets+"&castype="+castype;
		
		if(document.getElementById("casAuth").checked)
		{
			if(document.getElementById("casuser").value=="" &&  
		   	document.getElementById("caspswd").value=="")
			{
				alert("二级代理访问帐号和密码不许都为空"); 
				document.getElementById("casuser").focus();
				return "";
			}
			rets=rets+"&casauth=1&&casuser="+document.getElementById("casuser").value+"&caspswd="+document.getElementById("caspswd").value;
		}else rets=rets+"&casauth=0";
	}else rets=rets+"&cascade=0";
	
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

function proxy_sets()
{
	var strEncode=chkvalid();
	if(strEncode=="") return;
	showPopup(250, 200, 150, 20); 
	xmlHttp.open("POST", "/proxysets?cmd=setting", true);
//	xmlHttp.setRequestHeader("Content-Type","application/x-www-form-urlencoded; charset=utf-8");
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send( strEncode );
}
