var curSname="";
var svrQX=true;
var ACCESS_SERVICE_ALL=0x0030;

function window_onload()
{
	if(!oPopup) createpopup(); 
	var qx=parent.frmLeft.userQX;
	if((qx & ACCESS_SERVICE_ALL)==ACCESS_SERVICE_ALL)
		svrQX=false;
}

function serviceClick(tblElement)
{
	var row=tblElement.rowIndex;
	slistXML.recordset.absoluteposition=row;
	stat=slistXML.recordset("status");
	if(stat=="已启动")
	{
		document.getElementById("btnStart").disabled=true;
		document.getElementById("btnStop").disabled=svrQX;
	}else{
		document.getElementById("btnStart").disabled=svrQX;
		document.getElementById("btnStop").disabled=true;
	}
	runtype=slistXML.recordset("rtype");
	if(runtype=="自动")
	{
		document.getElementById("btnAuto").disabled=true;
		document.getElementById("btnManual").disabled=svrQX;
		document.getElementById("btnForbid").disabled=svrQX;
	}
	else if(runtype=="手动")
	{
		document.getElementById("btnManual").disabled=true;
		document.getElementById("btnAuto").disabled=svrQX;
		document.getElementById("btnForbid").disabled=svrQX;
	}
	else //禁用
	{
		document.getElementById("btnManual").disabled=svrQX;
	document.getElementById("btnAuto").disabled=svrQX;
	document.getElementById("btnForbid").disabled=true;
		document.getElementById("btnStart").disabled=true;
	}
	curSname=slistXML.recordset("sname");
	document.all("lblService").innerText="Service : "+curSname;
}

//----------------排序 func--------------------------
function sort(xmlObj, xslObj, sortByColName) 
{ 
var xmlData=eval("document.all."+xmlObj).XMLDocument;
var xslData=eval("document.all."+xslObj).XMLDocument;
var nodes=xslData.documentElement.selectSingleNode("xsl:for-each"); 
var s=nodes.selectSingleNode("@order-by").value;
if(s.substr(1)==sortByColName)
{
	if(s.charAt(0)=="+")
		s="-"+sortByColName;
	else s="+"+sortByColName;
}
else
	s="+"+sortByColName;
nodes.selectSingleNode("@order-by").value=s;

xmlData.documentElement.transformNodeToObject(xslData.documentElement,xmlData); 
} 
