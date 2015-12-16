//ftpaccount.js
function disp_userlist(xmlobj)
{
	var oSelect=document.getElementById("selAccount");
	for(i=oSelect.options.length;i>1;i--) oSelect.options.remove(i-1);
	for(i=0;i<xmlobj.childNodes.length;i++)
	{
		var s=xmlobj.childNodes.item(i).text;
		var oOption = document.createElement("OPTION");
		oOption.text=s;
		oOption.value=s;
		oSelect.add(oOption);
	}
}

function disp_userinfo(xmlobj)
{
	var node=xmlobj.getElementsByTagName("account");
	if(node.length>0) document.getElementById("lblUser").value=node.item(0).text;
	node=xmlobj.getElementsByTagName("password");
	if(node.length>0) document.getElementById("lblPswd").value=node.item(0).text;
	node=xmlobj.getElementsByTagName("description");
	if(node.length>0) document.getElementById("lblDesc").value=node.item(0).text;
	node=xmlobj.getElementsByTagName("pswdmode");
	var pwdmode=0;
	if(node.length>0) pwdmode=node.item(0).text;
	if(pwdmode==1)
		document.getElementById("chkPswdmode1").checked=true;
	else if(pwdmode==2)
		document.getElementById("chkPswdmode2").checked=true;
	else	document.getElementById("chkPswdmode0").checked=true;
	node=xmlobj.getElementsByTagName("expired");
	if(node.length>0 && node.item(0).text!="")
	{
		document.getElementById("chkExpired").checked=false;
		document.getElementById("lblExpired").readOnly=false;
		document.getElementById("lblExpired").value=node.item(0).text;
	}
	
	node=xmlobj.getElementsByTagName("maxsignin");
	if(node.length>0) document.getElementById("lblMaxSignin").value=node.item(0).text;
	node=xmlobj.getElementsByTagName("maxup");
	if(node.length>0) document.getElementById("lblMaxUp").value=node.item(0).text;
	node=xmlobj.getElementsByTagName("maxupfile");
	if(node.length>0) document.getElementById("lblMaxUpfile").value=node.item(0).text;
	node=xmlobj.getElementsByTagName("maxdw");
	if(node.length>0) document.getElementById("lblMaxDw").value=node.item(0).text;
	node=xmlobj.getElementsByTagName("maxdisk");
	if(node.length>0) document.getElementById("lblMaxDisk").value=node.item(0).text;
	node=xmlobj.getElementsByTagName("usedisk");
	if(node.length>0) document.getElementById("lblUseDisk").value=node.item(0).text;
	node=xmlobj.getElementsByTagName("disphidden");
	if(node.length>0 && node.item(0).text=="1") 
		document.getElementById("chkDispHidden").checked=true;
	node=xmlobj.getElementsByTagName("forbid");
	if(node.length>0 && node.item(0).text=="1") 
		document.getElementById("chkForbid").checked=true;
	
	node=xmlobj.getElementsByTagName("user_root");
	if(node.length>0)
	{
		document.getElementById("lblRoot").value=node.item(0).text;
		var o=node.item(0).attributes.item(0);//item("access")
		var qx=o.nodeValue;
		for(i=0;i<7;i++)
		{
			var idx=1<<i;
			if(qx & idx) document.getElementById("lAccess"+idx).checked=true;
		}
	}
	
	var vapths_xml=xmlobj.selectSingleNode("//vpaths")
	if(vapths_xml!=null && vapths_xml.childNodes.length>0)
	{
		var vname="";
		var vpath="";
		var qx="";
		var o=null;
		var oSelect=document.getElementById("selVPath");
		for(i=0;i<vapths_xml.childNodes.length;i++)
		{
			vpath=vapths_xml.childNodes.item(i).text;
			o=vapths_xml.childNodes.item(i).attributes.item(0);//item("vname");
			vname=o.nodeValue; 
			vname=vname.toLowerCase();
			o=vapths_xml.childNodes.item(i).attributes.item(1);//item("access");
			qx=o.nodeValue;  
			var oOption = document.createElement("OPTION");
			oOption.value=vname;
			oOption.text=vname+","+qx+","+vpath;
			oSelect.add(oOption);
		}
	}
	
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
			var userlist=xmlobj.selectSingleNode("//userlist")
			if(userlist!=null && userlist.childNodes.length>0)
				disp_userlist(userlist);
				
			var userinfo=xmlobj.selectSingleNode("//userinfo")
			if(userinfo!=null && userinfo.childNodes.length>0)
				disp_userinfo(userinfo);
				
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
	xmlHttp.open("GET", "/ftpusers?cmd=list", true);
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send( null );
}

function clkExpired(el)
{
	document.getElementById("lblExpired").readOnly=el.checked;
}

function initAccount()
{
	document.getElementById("lblUser").value="";
	document.getElementById("lblPswd").value="";
	document.getElementById("chkPswdmode0").checked=true;
	document.getElementById("chkPswdmode1").checked=false;
	document.getElementById("chkPswdmode2").checked=false;
	document.getElementById("lblDesc").value="";
	document.getElementById("lblExpired").value="";
	document.getElementById("lblExpired").readOnly=true;
	document.getElementById("chkExpired").checked=true;
	document.getElementById("lblMaxSignin").value="0";
	document.getElementById("lblMaxUp").value="0";
	document.getElementById("lblMaxUpfile").value="0";
	document.getElementById("lblMaxDw").value="0";
	document.getElementById("lblMaxDisk").value="0";
	document.getElementById("lblUseDisk").value="0";
	document.getElementById("chkDispHidden").checked=false;
	document.getElementById("chkForbid").checked=false;
	
	document.getElementById("lblRoot").value="";
	for(i=0;i<7;i++)
		document.getElementById("lAccess"+(1<<i)).checked=true;
	document.getElementById("lAccess8").checked=false;
	document.getElementById("lAccess128").checked=false;
	var oSelect=document.getElementById("selVPath");
	for(i=oSelect.options.length;i>1;i--) oSelect.options.remove(i-1);
	document.getElementById("lblVName").value="";
	document.getElementById("lblVPath").value="";
	for(i=0;i<7;i++)
		document.getElementById("lVAccess"+(1<<i)).checked=true;
		document.getElementById("lVAccess8").checked=false;
	document.getElementById("lVAccess128").checked=false;	
	document.getElementById("chkIPAccess1").checked=false;
	document.getElementById("chkIPAccess0").checked=false;
	document.getElementById("lblIPAddr").value="";
}
function isvalidInteger(s)
{
	var re=/^\d*$/;
	if(re.test(s)) return true;
	alert("请输入合法的整数"); 
	return false;
}
function isvalidNumber(s)
{
	var re=/^\d*.\d*$/;
	if(re.test(s)) return true;
	alert("请输入合法的数值"); 
	return false;
}
//有效性检查
function chkvalid()
{
	var rets="";
	var qx=0;
	if(document.getElementById("lblUser").value=="")
	{
		alert("无效得帐号名"); 
		document.getElementById("lblUser").focus();
		return "";
	}else{
		var s=document.getElementById("lblUser").value;
		s=s.toLowerCase();
		rets="account="+s;
	}
	rets=rets+"&pswd="+document.getElementById("lblPswd").value;
	if(document.getElementById("chkPswdmode1").checked)
		rets=rets+"&pswdmode=1";
	else if(document.getElementById("chkPswdmode2").checked)
		rets=rets+"&pswdmode=2";
	else	rets=rets+"&pswdmode=0";
	rets=rets+"&desc="+document.getElementById("lblDesc").value;
	if(document.getElementById("chkExpired").checked!=true)
	{
		var s=document.getElementById("lblExpired").value;
		var re=/^((((1[6-9]|[2-9]\d)\d{2})-(0?[13578]|1[02])-(0?[1-9]|[12]\d|3[01]))|(((1[6-9]|[2-9]\d)\d{2})-(0?[13456789]|1[012])-(0?[1-9]|[12]\d|30))|(((1[6-9]|[2-9]\d)\d{2})-0?2-(0?[1-9]|1\d|2[0-8]))|(((1[6-9]|[2-9]\d)(0[48]|[2468][048]|[13579][26])|((16|[2468][048]|[3579][26])00))-0?2-29))$/;
//		var re_time=/^^((((1[6-9]|[2-9]\d)\d{2})-(0?[13578]|1[02])-(0?[1-9]|[12]\d|3[01]))|(((1[6-9]|[2-9]\d)\d{2})-(0?[13456789]|1[012])-(0?[1-9]|[12]\d|30))|(((1[6-9]|[2-9]\d)\d{2})-0?2-(0?[1-9]|1\d|2[0-8]))|(((1[6-9]|[2-9]\d)(0[48]|[2468][048]|[13579][26])|((16|[2468][048]|[3579][26])00))-0?2-29-)) (20|21|22|23|[0-1]?\d):[0-5]?\d:[0-5]?\d$/;
		if(!re.test(s))
		{
			alert("不合法的有效期限格式"); 
			document.getElementById("lblExpired").focus();
			return "";
		}
		rets=rets+"&expired="+s;
	}else 
		rets=rets+"&expired=";
	
	if(!isvalidInteger(document.getElementById("lblMaxSignin").value))
	{
		document.getElementById("lblMaxSignin").focus();
		return "";
	}else
		rets=rets+"&maxSignin="+document.getElementById("lblMaxSignin").value;
	if(!isvalidInteger(document.getElementById("lblMaxUp").value))
	{
		document.getElementById("lblMaxUp").focus();
		return "";
	}else
		rets=rets+"&maxup="+document.getElementById("lblMaxUp").value;
	if(!isvalidInteger(document.getElementById("lblMaxUpfile").value))
	{
		document.getElementById("lblMaxUpfile").focus();
		return "";
	}else
		rets=rets+"&maxupfile="+document.getElementById("lblMaxUpfile").value;
	
	if(!isvalidInteger(document.getElementById("lblMaxDw").value))
	{
		document.getElementById("lblMaxDw").focus();
		return "";
	}else
		rets=rets+"&maxdw="+document.getElementById("lblMaxDw").value;
	if(!isvalidNumber(document.getElementById("lblMaxDisk").value))
	{
		document.getElementById("lblMaxDisk").focus();
		return "";
	}else
		rets=rets+"&maxdisk="+document.getElementById("lblMaxDisk").value;
	if(!isvalidNumber(document.getElementById("lblUseDisk").value))
	{
		document.getElementById("lblUseDisk").focus();
		return "";
	}else
		rets=rets+"&usedisk="+document.getElementById("lblUseDisk").value;
	if(document.getElementById("chkDispHidden").checked)
		rets=rets+"&disphidden=1";
	else	rets=rets+"&disphidden=0";
	if(document.getElementById("chkForbid").checked)
		rets=rets+"&forbid=1";
	else	rets=rets+"&forbid=0";
	
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
	
	if(document.getElementById("lblRoot").value!="")
	{
		var s=document.getElementById("lblRoot").value;
		var re=/^([c-z]|[C-Z]){1}:{1}\\{1}/;
		if(!re.test(s))
		{
			alert("不合法的主目录路径");
			document.getElementById("lblRoot").focus();
			return "";
		}
	}
	rets=rets+"&rootdir="+document.getElementById("lblRoot").value;
	for(i=0;i<8;i++)
	{
		var idx=1<<i;
		if(document.getElementById("lAccess"+idx).checked) qx=qx+idx;
	}
	if(qx==0){ alert("请设置主目录的访问权限"); return ""; }
	rets=rets+"&rootqx="+qx;
	
	//虚目录的设置
	rets=rets+"&vpath=";
	var oSelect=document.getElementById("selVPath");
	for(i=1;i<oSelect.options.length;i++)
		rets=rets+oSelect.options.item(i).text+";";
	return rets;
}

function user_change(el)
{
	initAccount();
	if(el.selectedIndex==0) return;
	showPopup(250, 200, 150, 20);
	//获取指定帐号的信息
	xmlHttp.open("GET", "/ftpusers?cmd=info&user="+el[el.selectedIndex].value, true);
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send( null );
	return;
}

function user_dele()
{
	var oSelect=document.getElementById("selAccount");
	if(oSelect.selectedIndex==0)
		alert("请选择要删除的帐号!");
	else if(confirm("确信删除帐号 "+oSelect[oSelect.selectedIndex].value+"?"))
	{
		showPopup(250, 200, 150, 20);
		xmlHttp.open("GET", "/ftpusers?cmd=dele&user="+oSelect[oSelect.selectedIndex].value, true);
		oSelect.selectedIndex=0;
		xmlHttp.onreadystatechange = processRequest;
		xmlHttp.send( null );
	}
}

function user_save()
{
	var strEncode=chkvalid();
	if(strEncode!="")
	{
		var oSelect=document.getElementById("selAccount");
		var struser=document.getElementById("lblUser").value;
		var idx=0; 
		struser=struser.toLowerCase();
		for(i=1;i<oSelect.options.length;i++)
		{
			var s=oSelect.options.item(i).value;
			s=s.toLowerCase();
			if(struser==s){ idx=i; break; }
		}
		if(idx==0)
		{
			if( !confirm("确信添加帐号 "+struser+"?") ) return;
		}
		else
		{
			if( !confirm("确信修改帐号 "+struser+"?") ) return;
		}
		showPopup(250, 200, 150, 20);
		oSelect.selectedIndex=0;
		xmlHttp.open("POST", "/ftpusers?cmd=save", true);
		xmlHttp.setRequestHeader("Content-Type","application/x-www-form-urlencoded; charset=utf-8");
		xmlHttp.onreadystatechange = processRequest;
		xmlHttp.send( strEncode );
	}
}

function vpath_change(el)
{
	if(el.selectedIndex==0)
	{
		document.getElementById("lblVName").value="";
		document.getElementById("lblVPath").value="";
		for(i=0;i<7;i++)
			document.getElementById("lVAccess"+(1<<i)).checked=true;
		document.getElementById("lVAccess128").checked=false;
		return;
	}
	var s=el[el.selectedIndex].text;
	var ss=s.split(",");
	document.getElementById("lblVName").value=ss[0];
	document.getElementById("lblVPath").value=ss[2];
	var qx=0;  qx=ss[1];
	for(i=0;i<8;i++)
	{
		var idx=1<<i;
		if(qx & idx) 
			document.getElementById("lVAccess"+idx).checked=true;
		else	document.getElementById("lVAccess"+idx).checked=false;
	}
}

function vpath_dele()
{
	var oSelect=document.getElementById("selVPath");
	if(oSelect.selectedIndex==0)
		alert("请选择要删除的虚目录!");
	else if(confirm("确信删除虚目录 "+oSelect[oSelect.selectedIndex].value+"?"))
	{
		oSelect.options.remove(oSelect.selectedIndex);
		oSelect.selectedIndex=0;
	}
}

function vpath_save()
{
	var vname=document.getElementById("lblVName").value;
	vname=vname.toLowerCase();
	var vpath=document.getElementById("lblVPath").value;
	var qx=0; 
	for(i=0;i<8;i++)
	{
		var idx=1<<i;
		if(document.getElementById("lVAccess"+idx).checked) qx=qx+idx;
	}
	if(vpath==""){alert("虚目录名设置无效"); return;}
	if(qx==0) { alert("请设置虚目录的访问权限"); return; }
	var re=/^([c-z]|[C-Z]){1}:{1}\\{1}/;
	if(vpath=="" || !re.test(vpath))
	{
		alert("物理路径设置无效");
		return;
	}
	var oSelect=document.getElementById("selVPath");
	var idx=0;
	for(i=1;i<oSelect.options.length;i++)
		if(vname==oSelect.options.item(i).value){ idx=i; break; }
	if(idx==0){
		if( confirm("确信添加虚目录 "+vname+"?") )
		{
			var oOption = document.createElement("OPTION");
			oOption.value=vname;
			oOption.text=vname+","+qx+","+vpath;
			oSelect.add(oOption);
			oSelect.selectedIndex=oSelect.options.length-1;
		}
	}else{
		if( confirm("确信修改虚目录 "+vname+"?") )
		{
			oSelect.options.item(i).text=vname+","+qx+","+vpath;
		}
	}
}
