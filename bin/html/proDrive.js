
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
			node = xmlobj.getElementsByTagName("fvolu");
			if(node.length>0)
				document.all("fvolu").value=node.item(0).text;
			node = xmlobj.getElementsByTagName("fsyst");
			if(node.length>0)
				document.all("fsyst").innerText=node.item(0).text;
			node = xmlobj.getElementsByTagName("fused");
			if(node.length>0)
				document.all("fused").innerText=node.item(0).text;
			node = xmlobj.getElementsByTagName("ffree");
			if(node.length>0)
				document.all("ffree").innerText=node.item(0).text;
			node = xmlobj.getElementsByTagName("fsize");
			if(node.length>0)
				document.all("fsize").innerText=node.item(0).text;
            	}
            	hidePopup();
        }
}
var sdri="";
function window_onload()
{
	var qx=0;
	sdri=window.dialogArguments;
	var p=sdri.indexOf(',');
	if(p!=-1)
	{
		qx=sdri.substr(0,p);
		sdri=sdri.substr(p+1);
	}
	if(!oPopup) createpopup();
	if(!xmlHttp) createXMLHttpRequest();
	xmlHttp.open("GET", "/prodrive?path="+sdri, true);
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send( null );
	showPopup(100, 150, 150, 20);
	
	if((qx & ACCESS_FILE_ALL)==ACCESS_FILE_ALL)
	{
		document.all("fvolu").className="txtInput_b";
		document.all("fvolu").disabled=false;
		document.all("btnApply").disabled=false;
	}
}

function mdVolu()
{
	showPopup(100, 150, 150, 20);
	var svolu=document.all("fvolu").value;
	xmlHttp.open("GET", "/prodrive?path="+sdri+"&volu="+svolu, true);
	xmlHttp.onreadystatechange = processRequest;
	xmlHttp.send( null );
}
