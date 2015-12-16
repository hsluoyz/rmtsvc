
function window_onload()
{
	if(!oPopup) createpopup();
	if(!xmlHttp) createXMLHttpRequest();
}

function processClick(tblElement)
{
	var row=tblElement.rowIndex;
	fportXML.recordset.absoluteposition=row;
	var curPid=fportXML.recordset("pid");
	document.all("lblProcess").innerText=" "+curPid;
}

//结束当前选中进程
function pkill()
{
	var curPid=fportXML.recordset("pid");
	if(curPid!=0 && confirm("确信结束进程"+curPid) )
	{
		xmlHttp.open("GET", "/pkill?pid="+curPid, false);
    		xmlHttp.send(null);
    		if(xmlHttp.responseText=="true")
    			fportXML.src='/fport';
    		else alert(xmlHttp.responseText);
    	}
}


function proFile()
{
	var qx=parent.frmLeft.userQX;
	var fpath=fportXML.recordset("pname");
	if(fpath=="")
		alert("请选择要查看属性的模块!");
	else
	{
		window.showModalDialog("proFile.htm",qx+","+fpath,"dialogHeight: 350px;dialogWidth: 400px;center: yes;resizable: no;scroll: no;status: no");
	}
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
