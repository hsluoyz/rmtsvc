//proxyaccount.js
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
	node=xmlobj.getElementsByTagName("expired");
	if(node.length>0 && node.item(0).text!="")
	{
		document.getElementById("chkExpired").checked=false;
		document.getElementById("lblExpired").readOnly=false;
		document.getElementById("lblExpired").value=node.item(0).text;
	}
	
	node=xmlobj.getElementsByTagName("maxsignin");
	if(node.length>0) document.getElementById("lblMaxSignin").value=node.item(0).text;
	node=xmlobj.getElementsByTagName("maxratio");
	if(node.length>0) document.getElementById("lblMaxRatio").value=node.item(0).text;
	node=xmlobj.getElementsByTagName("forbid");
	if(node.length>0 && node.item(0).text=="1") 
		document.getElementById("chkForbid").checked=true;
	
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
	
	var dstfilter_xml=xmlobj.selectSingleNode("//dstfilter")
	if(dstfilter_xml!=null && dstfilter_xml.childNodes.length>0)
	{
		node=dstfilter_xml.getElementsByTagName("ipaddr");
		document.getElementById("lblDstAddr").value=node.item(0).text;
		node=dstfilter_xml.getElementsByTagName("access");
		if(node.length>0 && node.item(0).text=="1")
		{
			document.getElementById("chkDstAccess1").checked=true;
			document.getElementById("chkDstAccess0").checked=false;
		}else{
			document.getElementById("chkDstAccess1").checked=false;
			document.getElementById("chkDstAccess0").checked=true;
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
	xmlHttp.open("GET", "/proxyusers?cmd=list", true);
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
	document.getElementById("lblDesc").value="";
	document.getElementById("lblExpired").value="";
	document.getElementById("lblExpired").readOnly=true;
	document.getElementById("chkExpired").checked=true;
	document.getElementById("lblMaxSignin").value="0";
	document.getElementById("lblMaxRatio").value="0";
	document.getElementById("chkForbid").checked=false;
	
	document.getElementById("chkIPAccess1").checked=false;
	document.getElementById("chkIPAccess0").checked=false;
	document.getElementById("lblIPAddr").value="";
	
	document.getElementById("chkDstAccess1").checked=false;
	document.getElementById("chkDstAccess0").checked=false;
	document.getElementById("lblDstAddr").value="";
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
	if(!isvalidInteger(document.getElementById("lblMaxRatio").value))
	{
		document.getElementById("lblMaxRatio").focus();
		return "";
	}else
		rets=rets+"&maxratio="+document.getElementById("lblMaxRatio").value;
	
	if(document.getElementById("chkForbid").checked)
		rets=rets+"&forbid=1";
	else	rets=rets+"&forbid=0";
	
	if(document.getElementById("lblIPAddr").value!="")
	{
		if(document.getElementById("chkIPAccess1").checked==false && 
		   document.getElementById("chkIPAccess0").checked==false )
		{
		   	alert("请设置访问许可/禁止");
		   	return "";
		}
		rets=rets+"&ipaddr="+document.getElementById("lblIPAddr").value;
		if(document.getElementById("chkIPAccess1").checked)
			rets=rets+"&ipaccess=1";
		else rets=rets+"&ipaccess=0";
	}else rets=rets+"&ipaccess=1&ipaddr=";
	if(document.getElementById("lblDstAddr").value!="")
	{
		if(document.getElementById("chkDstAccess1").checked==false && 
		   document.getElementById("chkDstAccess0").checked==false )
		{
		   	alert("请设置访问许可/禁止");
		   	return "";
		}
		rets=rets+"&dstaddr="+document.getElementById("lblDstAddr").value;
		if(document.getElementById("chkDstAccess1").checked)
			rets=rets+"&dstaccess=1";
		else rets=rets+"&dstaccess=0";
	}else rets=rets+"&dstaccess=1&dstaddr=";
	
	return rets;
}

function user_change(el)
{
	initAccount();
	if(el.selectedIndex==0) return;
	showPopup(250, 200, 150, 20);
	//获取指定帐号的信息
	xmlHttp.open("GET", "/proxyusers?cmd=info&user="+el[el.selectedIndex].value, true);
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
		xmlHttp.open("GET", "/proxyusers?cmd=dele&user="+oSelect[oSelect.selectedIndex].value, true);
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
		xmlHttp.open("POST", "/proxyusers?cmd=save", true);
		xmlHttp.setRequestHeader("Content-Type","application/x-www-form-urlencoded; charset=utf-8");
		xmlHttp.onreadystatechange = processRequest;
		xmlHttp.send( strEncode );
	}
}
