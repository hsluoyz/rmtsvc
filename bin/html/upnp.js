//upnp.js

function processRequest() 
{
	if (xmlHttp.readyState == 4) { // 判断对象状态
		if (xmlHttp.status == 200) { // 信息已经成功返回，开始处理信息
			
			var xmlobj = xmlHttp.responseXML;
			var sta=xmlobj.getElementsByTagName("status");
			if(sta.length>0 && sta.item(0).text=="1"){
				document.getElementById("lblHelp").innerText="  正在查找中...";
				document.getElementById("btnStop").disabled=false;
			}else{
				document.getElementById("lblHelp").innerText=" ";
				document.getElementById("btnStop").disabled=true;
			}
			var wanip=xmlobj.getElementsByTagName("wanip");
			if(wanip.length>0) 
				document.getElementById("lblWanIP").innerText=wanip.item(0).text;
			var devname=xmlobj.getElementsByTagName("devname");
			var devmacf=xmlobj.getElementsByTagName("manufacturer");
			if(devname.length>0 && devname.item(0).text!="")
			{
				var s=devname.item(0).text+" / "+devmacf.item(0).text;
				document.getElementById("lblDevName").innerHTML=s+"&nbsp;&nbsp;<a href=upnpxml target=_blank>详细信息</a>";
			}
			else
				document.getElementById("lblDevName").innerHTML="";
			
			var upnplist=xmlobj.selectSingleNode("//upnplist")
			if(upnplist!=null)
			{
				if(upnpXML.documentElement!=null)
					upnpXML.removeChild(upnpXML.documentElement);
				if(upnplist.childNodes.length>0) upnpXML.appendChild(upnplist);
			}
			
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
	upnp_status();
}

function upnp_find()
{
	showPopup(250, 200, 150, 20);
	xmlHttp.open("GET", "/upnp?cmd=find", true);
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send( null );
}

function upnp_stop()
{
	showPopup(250, 200, 150, 20);
	xmlHttp.open("GET", "/upnp?cmd=stop", true);
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send( null );
}

function upnp_status()
{
	showPopup(250, 200, 150, 20);
	xmlHttp.open("GET", "/upnp?cmd=status", true);
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send( null );
}

function upnp_add()
{
	var v=window.showModalDialog("upnp_add.htm","" , "dialogHeight: 200px;dialogWidth: 220px;center: yes;resizable: no;scroll: no;status: no");
	if(v!=null && v!="")
	{
		showPopup(250, 200, 150, 20);
		xmlHttp.open("POST", "/upnp?cmd=add", true);
		xmlHttp.setRequestHeader("Content-Type","application/x-www-form-urlencoded; charset=utf-8");
		xmlHttp.onreadystatechange = processRequest;
		xmlHttp.send( v );
		return;
	}
}

function upnp_del()
{
	var btcp=1;
	if(upnpXML.recordset("ptype")=="UDP") btcp=0;
	var wport=upnpXML.recordset("wport");
	document.getElementById("lblItem").innerText="";
	
	showPopup(250, 200, 150, 20);
	xmlHttp.open("GET", "/upnp?cmd=del&tcp="+btcp+"&wport="+wport, true);
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send();
	return;
}

function itemClick(tblElement)
{
	var row=tblElement.rowIndex;
	upnpXML.recordset.absoluteposition=row;
	document.getElementById("lblItem").innerText=upnpXML.recordset("ptype") + " - " + upnpXML.recordset("wport");
}

function errorDesc(tblElement)
{
	
	var row=upnpXML.recordset.absoluteposition;
	upnpXML.recordset.absoluteposition=tblElement.rowIndex;
	var s=upnpXML.recordset("error");
	upnpXML.recordset.absoluteposition=row;
	alert(s);
}


