var regpath="";

function processRequest() 
{
	if (xmlHttp.readyState == 4) { // 判断对象状态
		if (xmlHttp.status == 200) { // 信息已经成功返回，开始处理信息
			
			var xmlobj = xmlHttp.responseXML;
			var rkeys=xmlobj.selectSingleNode("//regkeys")
			if(rkeys!=null && rkeys.childNodes.length>0)
			{
				document.getElementById("lblKeyNum").innerText=rkeys.childNodes.length;
				if(regkeyXML.documentElement!=null)
					regkeyXML.removeChild(regkeyXML.documentElement);
				regkeyXML.appendChild(rkeys);
				document.getElementById("lblRegPath").innerText=regpath;
			}
			var ritems=xmlobj.selectSingleNode("//regitems")
			if(ritems!=null && ritems.childNodes.length>0)
    			{
    				
    				if(regitemXML.documentElement!=null)
    					regitemXML.removeChild(regitemXML.documentElement);
				regitemXML.appendChild(ritems);
    			}
            	} //else alert("请求的页面有异常,status="+xmlHttp.status);
            	hidePopup();
        }
}

function submitIt(strEncode,strurl)
{
	showPopup(250, 200, 150, 20);
	xmlHttp.abort();
	xmlHttp.open("POST", strurl, true);
	xmlHttp.setRequestHeader("Content-Type","application/x-www-form-urlencoded; charset=utf-8");
	xmlHttp.onreadystatechange = processRequest;
    	xmlHttp.send( strEncode );	
}

function window_onload()
{
	if(!oPopup) createpopup();   	
	if(!xmlHttp) createXMLHttpRequest();
	xmlHttp.open("GET", "/reglist?listwhat=1", true);
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send( null );
	
	var qx=parent.frmLeft.userQX;
	if((qx & ACCESS_REGIST_ALL)==ACCESS_REGIST_ALL)
	{
		document.all("fDelkey").disabled=false;
		document.all("fAddkey").disabled=false;
		document.all("fAddItem").disabled=false;
		document.all("fDelItem").disabled=false;
	}
}

function regkeyClick(tblElement)
{
	var row=tblElement.rowIndex;
	regkeyXML.recordset.absoluteposition=row;
	var regkey=regkeyXML.recordset("regkey");
	var rpath=document.getElementById("lblRegPath").innerText;
	
	document.getElementById("lblRegKey").innerText=regkey;
	document.getElementById("lblRegItem").innerText="";
	//对特殊字符&进行mime编码.replace(/&/g,"%26")
	var rkey=rpath+"\\"+regkey;
	strEncode="listwhat=2&rkey="+rkey.replace(/&/g,"%26");
	submitIt(strEncode,"/reglist");
}


function regkeyDblClick(tblElement)
{
	var row=tblElement.rowIndex;
	regkeyXML.recordset.absoluteposition=row;
	var regkey=regkeyXML.recordset("regkey");
	var rpath=document.getElementById("lblRegPath").innerText;
	
	document.getElementById("lblRegKey").innerText="";
	document.getElementById("lblRegItem").innerText="";
	regpath=rpath+"\\"+regkey;
	//对特殊字符&进行mime编码.replace(/&/g,"%26")
	strEncode="listwhat=3&rkey="+regpath.replace(/&/g,"%26");
	submitIt(strEncode,"/reglist");
}

function regItemClick(tblElement)
{
	var row=tblElement.rowIndex;
	regitemXML.recordset.absoluteposition=row;
	var id=regitemXML.recordset("id");
	if(id!="")
		document.getElementById("lblRegItem").innerText=regitemXML.recordset("rname");
}

function goup()
{
	var rpath=document.getElementById("lblRegPath").innerText;
	var p=rpath.lastIndexOf("\\");
	if(p!=-1)
	{
		regpath=rpath.substr(0,p);
		//对特殊字符&进行mime编码.replace(/&/g,"%26")
    		strEncode="listwhat=3&rkey="+regpath.replace(/&/g,"%26");
		submitIt(strEncode,"/reglist");
	}
	else alert("到头了");
}

function goto()
{
	var rpath=document.getElementById("lblRegPath").innerText;
	var new_rpath=prompt("输入有效路径",rpath);
	if(new_rpath!=null && new_rpath!="" )
	{
		regpath=new_rpath;
		//对特殊字符&进行mime编码.replace(/&/g,"%26")
    		strEncode="listwhat=3&rkey="+regpath.replace(/&/g,"%26");
		submitIt(strEncode,"/reglist");
	}
}

function refreshKey()
{
	regpath=document.getElementById("lblRegPath").innerText;
	//对特殊字符&进行mime编码.replace(/&/g,"%26")
	strEncode="listwhat=1&rkey="+regpath.replace(/&/g,"%26");
	submitIt(strEncode,"/reglist");
}

function refreshItem()
{
	var regkey=document.getElementById("lblRegKey").innerText;
	var rpath=document.getElementById("lblRegPath").innerText;
	if(regkey!="") rpath=rpath+"\\"+regkey;
	//对特殊字符&进行mime编码.replace(/&/g,"%26")
	strEncode="listwhat=2&rkey="+rpath.replace(/&/g,"%26");
	submitIt(strEncode,"/reglist");
}

function delKey()
{
	var regkey=document.getElementById("lblRegKey").innerText;
	var rpath=document.getElementById("lblRegPath").innerText;
	if(regkey=="")
		alert("请选择要删除的子键!");
	else if( confirm("确信删除子键 "+regkey+"?") )
	{
		//对特殊字符&进行mime编码.replace(/&/g,"%26")
		strEncode="rpath="+rpath.replace(/&/g,"%26")+"&rkey="+regkey.replace(/&/g,"%26");
		submitIt(strEncode,"/regkey_del");
	}
}

function addKey()
{
	var rpath=document.getElementById("lblRegPath").innerText;
	var regkey=prompt("输入子键名","");
	if(regkey!=null && regkey!="" )
	{
		//对特殊字符&进行mime编码.replace(/&/g,"%26")
		strEncode="rpath="+rpath.replace(/&/g,"%26")+"&rkey="+regkey.replace(/&/g,"%26");
		submitIt(strEncode,"/regkey_add");
	}
}



function delItem()
{
	var regitem=document.getElementById("lblRegItem").innerText;
	if(regitem=="")
		alert("请选择要删除的项目!");
	else
	{
		var regkey=document.getElementById("lblRegKey").innerText;
		var rpath=document.getElementById("lblRegPath").innerText;
		if( confirm("确信删除子键 "+regkey+" 下的 "+regitem+" 项目?") )
		{
			//对特殊字符&进行mime编码.replace(/&/g,"%26")
			if(regkey!="")
			{
				var rtmp=rpath+"\\"+regkey;
				strEncode="rpath="+rtmp.replace(/&/g,"%26")+"&rname="+regitem.replace(/&/g,"%26");
			}
			else
				strEncode="rpath="+rpath.replace(/&/g,"%26")+"&rname="+regitem.replace(/&/g,"%26");
			document.getElementById("lblRegItem").innerText="";
			submitIt(strEncode,"/regitem_del");
		}
	}
}

function addItem()
{
	var v=window.showModalDialog("addRegitem.htm","","dialogHeight:300px;dialogWidth=300px;center:yes;status:no;scroll:no;");
	if(v!=null && v!="")
	{
		var regkey=document.getElementById("lblRegKey").innerText;
		var rpath=document.getElementById("lblRegPath").innerText;
		//对特殊字符&进行mime编码.replace(/&/g,"%26")
		if(regkey!="")
		{
			var rtmp=rpath+"\\"+regkey;
			strEncode="rpath="+rtmp.replace(/&/g,"%26")+"&"+v;
		}
		else
			strEncode="rpath="+rpath.replace(/&/g,"%26")+"&"+v;
		submitIt(strEncode,"/regitem_add");
	}
}

function keypress(txtElement)
{
	if(document.all("fAddItem").disabled) return;
	if(window.event.keyCode==10 && window.event.ctrlKey)
	{
		var tblElement=txtElement.parentElement.parentElement;
		var row=tblElement.rowIndex;
		regitemXML.recordset.absoluteposition=row;
		if(regitemXML.recordset("id")!="")
		{
			var rname=""+regitemXML.recordset("rname");
			var rtype=regitemXML.recordset("rtype")
			var rvalue=""+txtElement.value;
			var regkey=document.getElementById("lblRegKey").innerText;
			var rpath=document.getElementById("lblRegPath").innerText;
			var strEncode="";
			//对特殊字符&进行mime编码.replace(/&/g,"%26")
			if(regkey!="")
			{
				var rtmp=rpath+"\\"+regkey;
				strEncode="rpath="+rtmp.replace(/&/g,"%26");
				strEncode=strEncode+"&rtype="+rtype;
				strEncode=strEncode+"&rname="+rname.replace(/&/g,"%26");
				strEncode=strEncode+"&rdata="+rvalue.replace(/&/g,"%26");
			}
			else
			{
				strEncode="rpath="+rpath.replace(/&/g,"%26");
				strEncode=strEncode+"&rtype="+rtype;
				strEncode=strEncode+"&rname="+rname.replace(/&/g,"%26");
				strEncode=strEncode+"&rdata="+rvalue.replace(/&/g,"%26");
			}
			submitIt(strEncode,"/regitem_md");
		}
		window.event.keyCode=0;
	}
}
function cancelModifyItem(txtElement)
{
	txtElement.dataFld="rdata";
	txtElement.className="txtInput_none";
	txtElement.readOnly=true;
	document.all("lblHelp").innerHTML="";
}
function modifyItem(txtElement)
{
	if(document.all("fAddItem").disabled) return;
	if(txtElement.readOnly==false) return;
	var tblElement=txtElement.parentElement.parentElement;
	var row=tblElement.rowIndex;
	regitemXML.recordset.absoluteposition=row;
	if(regitemXML.recordset("id")!="")
	{
		txtElement.dataFld="";
		txtElement.value=regitemXML.recordset("rdata");
		txtElement.className="txtInput_normal";
		txtElement.readOnly=false;
		document.all("lblHelp").innerHTML="(<font color=red>按Ctrl+Enter键保存修改</font>)"
	}
}

