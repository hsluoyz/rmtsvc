//vidcMapR.js
function disp_vidcsinfo(xmlobj)
{
	document.getElementById("btnMod_vidcs").disabled=false;
	document.getElementById("btnDel_vidcs").disabled=false;
	
	document.getElementById("btnConn_vidcs").disabled=false;
	var node=xmlobj.getElementsByTagName("connected");
	if(node.length>0 && node.item(0).text=="1")
	{
		document.getElementById("btnConn_vidcs").value="断开";
		document.getElementById("lblStatus").innerHTML="<font color=green>●</font>已连接此vIDCs服务";
		node=xmlobj.getElementsByTagName("starttime");
		if(node.length>0) document.getElementById("lblRuntime").innerText=node.item(0).text;
	}else	document.getElementById("btnConn_vidcs").value="连接";
	
	
	node=xmlobj.getElementsByTagName("vname");
	if(node.length>0) document.getElementById("lblName").innerText=node.item(0).text;
	node=xmlobj.getElementsByTagName("vhost");
	if(node.length>0) document.getElementById("vidcsip").innerText=node.item(0).text;
	node=xmlobj.getElementsByTagName("vport");
	if(node.length>0) document.getElementById("vidcsport").innerText=node.item(0).text;
	
	var oSelect=document.getElementById("selMapip");
	for(i=oSelect.options.length;i>1;i--) oSelect.options.remove(i-1);
	node=xmlobj.getElementsByTagName("localip");
	if(node.length>0){
		var localip=node.item(0).text;
		var ss=localip.split(" ");
		for(i=0;i<ss.length;i++)
		{
			var oOption = document.createElement("OPTION");
			oOption.text=ss[i];
			oOption.value=ss[i];
			oSelect.add(oOption);
		}
	}
	oSelect.selectedIndex=0;
	
}

function disp_maplist(xmlobj)
{
	var oSelect=document.getElementById("selMapped");
	for(i=oSelect.options.length;i>1;i--) oSelect.options.remove(i-1);
	for(i=0;i<xmlobj.childNodes.length;i++)
	{
		var o=xmlobj.childNodes.item(i).attributes.item(0);
		var s=xmlobj.childNodes.item(i).text;
		var oOption = document.createElement("OPTION");
		oOption.text=o.nodeValue+" - " +s;
		oOption.value=o.nodeValue;
		oSelect.add(oOption);
	}
	
	document.getElementById("tcpudp0").disabled=false;
	document.getElementById("tcpudp1").disabled=false;
	document.getElementById("tcpudp2").disabled=false;
	document.getElementById("btnMap").disabled=true;
	document.getElementById("btnDel").disabled=true;
	document.getElementById("btnAdd").disabled=false;
}

function processRequest() 
{
	if (xmlHttp.readyState == 4) { // 判断对象状态
		if (xmlHttp.status == 200) { // 信息已经成功返回，开始处理信息
			
			var xmlobj = xmlHttp.responseXML;
			var vidcslist=xmlobj.selectSingleNode("//vidcslist")
			if(vidcslist!=null)
			{
				if(vidcsXML.documentElement!=null)
					vidcsXML.removeChild(vidcsXML.documentElement);
				if(vidcslist.childNodes.length>0) vidcsXML.appendChild(vidcslist);
			}
			
			var vidcsinfo=xmlobj.selectSingleNode("//vidcsinfo")
			if(vidcsinfo!=null && vidcsinfo.childNodes.length>0)
				disp_vidcsinfo(vidcsinfo);
				
			var maplist=xmlobj.selectSingleNode("//maplist")
			if(maplist!=null) disp_maplist(maplist);
				
			var mapinfo=xmlobj.selectSingleNode("//mapinfo")
			if(mapinfo!=null && mapinfo.childNodes.length>0)
				disp_mapinfo(mapinfo);
			
			var retmsg=xmlobj.getElementsByTagName("retmsg");
    			if(retmsg.length>0)
				alert(retmsg.item(0).text);
            	} //else alert("请求的页面有异常,status="+xmlHttp.status);
            	hidePopup();
        }
}

function window_onload()
{
	if(!oPopup) createpopup();   	
	if(!xmlHttp) createXMLHttpRequest();
	
	showPopup(250, 200, 150, 20);
	xmlHttp.open("GET", "/mportR?cmd=vidcclist", true);
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send( null );
}

function vidcs_add()
{
	var qx=parent.frmLeft.userQX;
	var v=window.showModalDialog("vidcMapR_pro.htm",qx+"," , "dialogHeight: 200px;dialogWidth: 250px;center: yes;resizable: no;scroll: no;status: no");
	if(v!=null && v!="")
	{
		initinfo();
		showPopup(250, 200, 150, 20);
		xmlHttp.open("POST", "/mportR?cmd=vidccmodi", true);
		xmlHttp.onreadystatechange = processRequest;
		xmlHttp.send( v );
	}
}

function vidcs_mod()
{
	var qx=parent.frmLeft.userQX;
	var vname=vidcsXML.recordset("vhost")+","+vidcsXML.recordset("vport")+","+vidcsXML.recordset("vpswd")+","+vidcsXML.recordset("vconn");
	vname=vname+","+vidcsXML.recordset("vname");
	var v=window.showModalDialog("vidcMapR_pro.htm",qx+","+vname , "dialogHeight: 200px;dialogWidth: 250px;center: yes;resizable: no;scroll: no;status: no");
	if(v!=null && v!="")
	{
		initinfo();
		showPopup(250, 200, 150, 20);
		xmlHttp.open("POST", "/mportR?cmd=vidccmodi", true);
		xmlHttp.onreadystatechange = processRequest;
		xmlHttp.send( v );
	}
}

function vidcs_del()
{
	var vname=vidcsXML.recordset("vname");
	initinfo();
	showPopup(250, 200, 150, 20);
	xmlHttp.open("GET", "/mportR?cmd=vidccdele&vname="+vname, true);
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send( null );
}

function vidcs_conn()
{
	var vname=vidcsXML.recordset("vname");
	var stype="tcp";
	if(document.getElementById("tcpudp1").checked) stype="udp";
	else if(document.getElementById("tcpudp2").checked) stype="proxy";
	var vcmd="disconn";
	if(document.getElementById("btnConn_vidcs").value=="连接") vcmd="connect";
	initinfo();
	showPopup(250, 200, 150, 20);
	xmlHttp.open("GET", "/mportR?cmd="+vcmd+"&vname="+vname+"&type="+stype, true);
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send( null );
}


function vidcsClick(tblElement)
{
	var row=tblElement.rowIndex;
	vidcsXML.recordset.absoluteposition=row;
	var vname=vidcsXML.recordset("vname");
	tcpudp_sel();
}

function tcpudp_sel()
{
	var vname=vidcsXML.recordset("vname");
	var stype="tcp";
	if(document.getElementById("tcpudp1").checked) stype="udp";
	else if(document.getElementById("tcpudp2").checked) stype="proxy";
	initinfo();
	showPopup(250, 200, 150, 20);
	xmlHttp.open("GET", "/mportR?cmd=vidccinfo&vname="+vname+"&type="+stype, true);
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send( null );
}

function ssltype_sel()
{
	if(document.getElementById("ssltype1").checked)
	{
	document.getElementById("sslverify0").disabled=false;
	document.getElementById("sslverify1").disabled=false;
	}
	else if(document.getElementById("ssltype2").checked)
	{
	document.getElementById("sslverify0").disabled=true;
	document.getElementById("sslverify1").disabled=true;
	var scert=document.getElementById("clicert").value;
	var new_clicert=prompt("如果应用服务需要进行客户证书验证,请在此输入客户端证书(PEM格式)\r\n格式:<证书>,<私钥>,<密码> 例如: client.pem,client.key,1234",scert);
	if(new_clicert!=null) document.getElementById("clicert").value=new_clicert;
	}else{
	document.getElementById("sslverify0").disabled=true;
	document.getElementById("sslverify1").disabled=true;
	}
}

function mapped_change(el)
{
	var vname=vidcsXML.recordset("vname");
	var stype="tcp";
	if(document.getElementById("tcpudp1").checked) stype="udp";
	else if(document.getElementById("tcpudp2").checked) stype="proxy";
	initmapinfo();
	if(el.selectedIndex==0){
		document.getElementById("btnAdd").value="添加";
		document.getElementById("lblMapname").disabled=false;
		document.getElementById("btnMap").disabled=true;
		document.getElementById("btnDel").disabled=true;
		 return;
	}else{
		 document.getElementById("btnAdd").value="修改"; 
		 document.getElementById("lblMapname").disabled=true;
	}
	showPopup(250, 200, 150, 20);
	xmlHttp.open("GET", "/mportR?cmd=mapinfo&type="+stype+"&mapname="+el[el.selectedIndex].value+"&vname="+vname, true);
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send( null );
	return;
}

function mapped_map()
{
	var oSelect=document.getElementById("selMapped");
	if(oSelect.selectedIndex==0)
		alert("请选择要映射的服务!");
	else
	{
		var vname=vidcsXML.recordset("vname");
		var mapname=oSelect[oSelect.selectedIndex].value;
		var stype="tcp";
		if(document.getElementById("tcpudp1").checked) stype="udp";
		else if(document.getElementById("tcpudp2").checked) stype="proxy";
		var scmd="unmap";
		if(document.getElementById("btnMap").value=="映射") scmd="maped";
		initmapinfo();
		showPopup(250, 200, 150, 20);
		xmlHttp.open("GET", "/mportR?cmd="+scmd+"&type="+stype+"&mapname="+mapname+"&vname="+vname, true);
		xmlHttp.onreadystatechange = processRequest;
		xmlHttp.send( null );
	}
}

function mapped_add()
{
	var strEncode=chkvalid();
	if(strEncode=="") return;
	var vname=vidcsXML.recordset("vname");
	var mapname=document.getElementById("lblMapname").value;
	if(document.getElementById("btnAdd").value=="修改"){
		 if( !confirm("确信修改映射信息 "+mapname+"?") ) return;
	}else if( !confirm("确信添加映射信息 "+mapname+"?") ) return;
	
	var stype="tcp";
	if(document.getElementById("tcpudp1").checked) stype="udp";
	else if(document.getElementById("tcpudp2").checked) stype="proxy";
	initmapinfo();
	showPopup(250, 200, 150, 20);
	xmlHttp.open("POST", "/mportR?cmd=mapmodi&type="+stype+"&vname="+vname, true);
	xmlHttp.setRequestHeader("Content-Type","application/x-www-form-urlencoded; charset=utf-8");
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send( strEncode );
}

function mapped_del()
{
	var oSelect=document.getElementById("selMapped");
	if(oSelect.selectedIndex==0)
		alert("请选择要删除的映射信息!");
	else if(confirm("确信删除映射 "+oSelect[oSelect.selectedIndex].value+"?"))
	{
		var stype="tcp";
		if(document.getElementById("tcpudp1").checked) stype="udp";
		else if(document.getElementById("tcpudp2").checked) stype="proxy";
		var vname=vidcsXML.recordset("vname");
		var mapname=oSelect[oSelect.selectedIndex].value;
		initmapinfo();
		showPopup(250, 200, 150, 20);
		xmlHttp.open("GET", "/mportR?cmd=mapdel&type="+stype+"&mapname="+mapname+"&vname="+vname, true);
		xmlHttp.onreadystatechange = processRequest;
		xmlHttp.send( null );
	}
}

function initinfo()
{
	document.getElementById("btnConn_vidcs").disabled=true;
	document.getElementById("btnMod_vidcs").disabled=true;
	document.getElementById("btnDel_vidcs").disabled=true;
	document.getElementById("btnConn_vidcs").value="连接";
	
	document.getElementById("lblName").innerText="vIDCs";
	document.getElementById("vidcsip").innerText="";
	document.getElementById("vidcsport").innerText="";
	
	document.getElementById("lblStatus").innerHTML="<font color=red>●</font>未连接此vIDCs服务";
	document.getElementById("lblRuntime").innerText="YYYY年MM月DD日 hh:mm:ss";
	
	document.getElementById("tcpudp0").disabled=true;
	document.getElementById("tcpudp1").disabled=true;
	document.getElementById("tcpudp2").disabled=true;
	document.getElementById("btnDel").disabled=true;
	document.getElementById("btnAdd").disabled=true;
	document.getElementById("btnMap").disabled=true;
	document.getElementById("btnMap").value="映射";
	document.getElementById("btnAdd").value="添加";
	
	var oSelect=document.getElementById("selMapped");
	for(i=oSelect.options.length;i>1;i--) oSelect.options.remove(i-1);
	document.getElementById("selMapped").selectedIndex=0;
	initmapinfo();
}

function initmapinfo()
{	
	document.getElementById("lblmapped").innerHTML="<font color=red>●</font>未映射到远程vIDCs";
	document.getElementById("lblAppIP").value="";
	document.getElementById("lblAppPort").value="";
	document.getElementById("lblAppDesc").value="";
	document.getElementById("svrtype0").checked=true;
	document.getElementById("svrtype1").checked=true;
	document.getElementById("svrtype2").checked=true;
	document.getElementById("chkAuth").checked=false;
	document.getElementById("authuser").value="";
	document.getElementById("authpswd").value="";
	document.getElementById("authuser").disabled=true;
	document.getElementById("authpswd").disabled=true;
	
	document.getElementById("lblMapport").value="";
	document.getElementById("lblMapname").value="";
	document.getElementById("lblMapname").disabled=false;
	document.getElementById("selMapip").selectedIndex=0;
	document.getElementById("lblMaxconn").value="0";
	document.getElementById("lblMaxratio").value="0";
	
	document.getElementById("ssltype0").checked=true;
	document.getElementById("apptype0").checked=true;
	if(document.getElementById("tcpudp0").checked)
	{
		document.getElementById("ssltype0").disabled=false;
		document.getElementById("ssltype1").disabled=false;
		document.getElementById("ssltype2").disabled=false;
		document.getElementById("apptype0").disabled=false;
		document.getElementById("apptype1").disabled=false;
		document.getElementById("apptype2").disabled=false;
		document.getElementById("apptype3").disabled=false;
	}
	else
	{
		document.getElementById("ssltype0").disabled=true;
		document.getElementById("ssltype1").disabled=true;
		document.getElementById("ssltype2").disabled=true;
		document.getElementById("apptype0").disabled=true;
		document.getElementById("apptype1").disabled=true;
		document.getElementById("apptype2").disabled=true;
		document.getElementById("apptype3").disabled=true;
	}
	
	if(document.getElementById("tcpudp2").checked)
	{
		document.getElementById("divProxyinfo").style.display="";
		document.getElementById("divAppinfo").style.display="none";
		
		document.getElementById("lblMaxconn").disabled=true;
		document.getElementById("lblMaxratio").disabled=true;
	}else{
		document.getElementById("divProxyinfo").style.display="none";
		document.getElementById("divAppinfo").style.display="";
		
		document.getElementById("lblMaxconn").disabled=false;
		document.getElementById("lblMaxratio").disabled=false;
	}
	
	document.getElementById("sslverify0").checked=true;
	document.getElementById("sslverify0").disabled=true;
	document.getElementById("sslverify1").disabled=true;
	document.getElementById("clicert").value="";
	
	document.getElementById("chkAutorun").checked=false;
	document.getElementById("chkIPAccess1").checked=true;
	document.getElementById("lblIPAddr").value="";
}

function clkAuth(e)
{
	if(e.checked)
	{
		document.getElementById("authuser").disabled=false;
		document.getElementById("authpswd").disabled=false;
	}
	else
	{
		document.getElementById("authuser").disabled=true;
		document.getElementById("authpswd").disabled=true;
	}
}

//有效性检查
function chkvalid()
{
	var rets="";
	var mapname=document.getElementById("lblMapname").value;
	var re_mapname=/^\w{1,20}$/;
	if( !re_mapname.test(mapname) ){
		alert("映射服务名称不能为空且最多20个字符\r\n只能包含下列字母 A-Za-z0-9_");
		document.getElementById("lblMapname").focus();
		return "";
	}else rets="mapname="+mapname;
	
	if(document.getElementById("tcpudp2").checked)
	{//内网代理
		var svrtype=0;
		if(document.getElementById("svrtype0").checked)
			svrtype=svrtype+1;
		if(document.getElementById("svrtype1").checked)
			svrtype=svrtype+2;
		if(document.getElementById("svrtype2").checked)
			svrtype=svrtype+4;
		rets=rets+"&svrtype="+svrtype;
		if(document.getElementById("chkAuth").checked)
		{
			if(document.getElementById("authuser").value=="" &&  document.getElementById("authpswd").value=="")
			{
				alert("代理访问帐号和密码不许都为空"); 
				document.getElementById("authuser").focus();
				return "";
			}
			rets=rets+"&bauth=1&&authuser="+document.getElementById("authuser").value+"&authpswd="+document.getElementById("authpswd").value;
		}else rets=rets+"&bauth=0";
		rets=rets+"&appdesc=内网代理";
	}
	else
	{//普通tcp/udp应用服务映射
		var sip=document.getElementById("lblAppIP").value;
		var sport=document.getElementById("lblAppPort").value;
		if(sport!="") sip=sip+":"+sport;
		//可输入多个应用服务地址，格式aaa.bb.cc:ddd,aaa1.bb1.cc1:dddd
		var re_sip=/^((\w+.?)+:\d+,?)+$/;
		if( !re_sip.test(sip) )
		{
			alert("要映射得应用服务地址和端口输入不正确"); 
			document.getElementById("lblAppIP").focus();
			return "";
		}else rets=rets+"&appsvr="+sip;
		var appdesc=document.getElementById("lblAppDesc").value;
		var re_desc=/^[^&]*$/;
		if(!re_desc.test(appdesc))
		{
			alert("描述中不允许包含&字符");
			document.getElementById("lblAppDesc").focus();
			return "";
		}else rets=rets+"&appdesc="+appdesc;
	}
	
	var mport=document.getElementById("lblMapport").value;
	var re_mport=/^\d*-?\d*$/;
	if(!re_mport.test(mport))
	{
		alert("请输入合法的映射端口");
		document.getElementById("lblMapport").focus();
		return "";
	}else rets=rets+"&mport="+mport;
	if(document.getElementById("tcpudp0").checked)
	{
		if(document.getElementById("ssltype2").checked)
			rets=rets+"-ssl";
		else if(document.getElementById("ssltype1").checked)
			rets=rets+"+ssl";
	}
	if(document.getElementById("ssltype1").checked && document.getElementById("sslverify1").checked)
		rets=rets+"&sslverify=1";
	rets=rets+"&bindip="+document.getElementById("selMapip").value;
	
	if(document.getElementById("apptype3").checked)
		rets=rets+"&apptype=FTP";
	else if(document.getElementById("apptype2").checked)
		rets=rets+"&apptype=WWW";
	else if(document.getElementById("apptype1").checked)
		rets=rets+"&apptype=TCP";
	else rets=rets+"&apptype=UNK";
	
	if(document.getElementById("ssltype2").checked)
	{
		rets=rets+"&clicert="+document.getElementById("clicert").value;
	}else rets=rets+"&clicert=";
	
	if(document.getElementById("chkAutorun").checked)
		rets=rets+"&autorun=1";
	else	rets=rets+"&autorun=0";
	
	var re_maxd=/^\d*$/;
	var maxconn=document.getElementById("lblMaxconn").value;
	if(!re_maxd.test(maxconn))
		rets=rets+"&maxconn=0";
	else rets=rets+"&maxconn="+maxconn;
	var maxratio=document.getElementById("lblMaxratio").value;
	if(!re_maxd.test(maxratio))
		rets=rets+"&maxratio=0";
	else rets=rets+"&maxratio="+maxratio;
	
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
			rets=rets+"&access=1";
		else rets=rets+"&access=0";
	}else rets=rets+"&ipaddr=&access=1";
	return rets;
}

//--------------------------------------------------

function disp_mapinfo(xmlobj)
{
	var mapped_port=0;
	var node=xmlobj.getElementsByTagName("svrport");
	if(node.length>0) mapped_port=node.item(0).text;
	document.getElementById("btnMap").disabled=false;
	if(mapped_port>0){
		node=xmlobj.getElementsByTagName("ifssl");
		if(node.length>0 && node.item(0).text=="1")
		{
			var s="&nbsp;(SSL - 无需客户证书验证)";
			node=xmlobj.getElementsByTagName("ifsslv");
			if(node.length>0 && node.item(0).text=="1") s="&nbsp;(SSL - 需客户证书验证)";
			document.getElementById("lblmapped").innerHTML="<font color=green>●</font>映射到vIDCs...&nbsp;端口:"+mapped_port+s;
		}
		else	document.getElementById("lblmapped").innerHTML="<font color=green>●</font>映射到vIDCs...&nbsp;端口:"+mapped_port;
		document.getElementById("btnMap").value="取消";
		document.getElementById("btnAdd").disabled=true;
		document.getElementById("btnDel").disabled=true;
	}else{
		document.getElementById("lblmapped").innerHTML="<font color=red>●</font>未映射到远程vIDCs";
		document.getElementById("btnMap").value="映射";
		document.getElementById("btnAdd").disabled=false;
		document.getElementById("btnDel").disabled=false;
	}
	if( document.getElementById("btnConn_vidcs").value=="连接" )
		document.getElementById("btnMap").disabled=true;
	
	if(document.getElementById("tcpudp2").checked)
	{
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
		node=xmlobj.getElementsByTagName("authuser");
		if(node.length>0)
			document.getElementById("authuser").value=node.item(0).text;
		else	document.getElementById("authuser").value="";
		node=xmlobj.getElementsByTagName("authpswd");
		if(node.length>0)
			document.getElementById("authpswd").value=node.item(0).text;
		else	document.getElementById("authpswd").value="";
		clkAuth(document.getElementById("chkAuth"));
	}
	else
	{
		node=xmlobj.getElementsByTagName("appip");
		if(node.length>0) document.getElementById("lblAppIP").value=node.item(0).text;
		node=xmlobj.getElementsByTagName("appport");
		if(node.length>0) document.getElementById("lblAppPort").value=node.item(0).text;
		node=xmlobj.getElementsByTagName("appdesc");
		if(node.length>0) document.getElementById("lblAppDesc").value=node.item(0).text;
	}
	node=xmlobj.getElementsByTagName("mapport");
	if(node.length>0) document.getElementById("lblMapport").value=node.item(0).text;
	node=xmlobj.getElementsByTagName("mapname");
	if(node.length>0) document.getElementById("lblMapname").value=node.item(0).text;
	node=xmlobj.getElementsByTagName("maxconn");
	if(node.length>0) document.getElementById("lblMaxconn").value=node.item(0).text;
	node=xmlobj.getElementsByTagName("maxratio");
	if(node.length>0) document.getElementById("lblMaxratio").value=node.item(0).text;
	node=xmlobj.getElementsByTagName("bindip");
	if(node.length>0){
		var oSelect=document.getElementById("selMapip");
		for(i=0;i<oSelect.options.length;i++){
			if(oSelect.options[i].value==node.item(0).text){oSelect.selectedIndex=i;break; }
		}
	}
	node=xmlobj.getElementsByTagName("autorun");
	if(node.length>0 && node.item(0).text=="1")
		document.getElementById("chkAutorun").checked=true;
	else document.getElementById("chkAutorun").checked=false;
	node=xmlobj.getElementsByTagName("ssltype");
	if(node.length>0)
	{
		if(node.item(0).text=="-ssl")
			document.getElementById("ssltype2").checked=true;
		else if(node.item(0).text=="+ssl")
			document.getElementById("ssltype1").checked=true;
		else document.getElementById("ssltype0").checked=true;
	}else document.getElementById("ssltype0").checked=true;
	
	if(document.getElementById("ssltype1").checked)
	{
		document.getElementById("sslverify0").disabled=false;
		document.getElementById("sslverify1").disabled=false;
	}
	
	node=xmlobj.getElementsByTagName("sslverify");
	if(node.length>0 && node.item(0).text=="1")
		document.getElementById("sslverify1").checked=true;
	else document.getElementById("sslverify0").checked=true;
	node=xmlobj.getElementsByTagName("clicert");
	if(node.length>0)
		document.getElementById("clicert").value=node.item(0).text;
	else document.getElementById("clicert").value="";
	
	node=xmlobj.getElementsByTagName("apptype");
	var apptype="UNK";
	if(node.length>0) apptype=node.item(0).text;
	if(apptype=="FTP")
		document.getElementById("apptype3").checked=true;
	else if(apptype=="WWW")
		document.getElementById("apptype2").checked=true;
	else if(apptype=="TCP")
		document.getElementById("apptype1").checked=true;
	else document.getElementById("apptype0").checked=true;
		
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

