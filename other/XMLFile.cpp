// XMLSettings.cpp : implementation file
//

//#include "stdafx.h"

#include "xmlfile.h"
#include <algorithm>

/////////////////////////////////////////////////////////////////////////////
// CXMLFile
CXMLFile::CXMLFile()
{
	::CoInitialize(NULL);
	XmlDocPtr = NULL;
	xml_file_name ="";
}

CXMLFile::CXMLFile(const char* filename)
{
	::CoInitialize(NULL);
	if(XmlDocPtr!=NULL)
		XmlDocPtr.Detach();
	XmlDocPtr = NULL;
	if(filename)
		xml_file_name=filename;
	load(filename);
}

CXMLFile::~CXMLFile()
{
	if(XmlDocPtr!=NULL)
		XmlDocPtr.Detach();
	::CoUninitialize();
}

void CXMLFile::clear()
{
	if(XmlDocPtr!=NULL)
		XmlDocPtr.Detach();
	DeleteFile(xml_file_name.c_str());
	load(xml_file_name.c_str());
}

// get a long value
long CXMLFile::GetLong(const char* cstrBaseKeyName, const char* cstrValueName, long lDefaultValue)
{
	/* 
		Since XML is text based and we have no schema, just convert to a string and 
		call the GetString method.
	*/
	long lRetVal = lDefaultValue;
	char chs[256];
	sprintf(chs,"%d", lRetVal);
	lRetVal = atol(GetString(cstrBaseKeyName, cstrValueName, chs).c_str() );
	return lRetVal;
}

std::string CXMLFile::GetAttribute(const char* cstrBaseKeyName, const char* cstrValueName, 
		const char* cstrAttributeName, const char* cstrDefaultAttributeValue)
{
	std::string strAttributeValue;
	std::string strDummy;
	GetNodeValue(cstrBaseKeyName, cstrValueName, NULL, strDummy, cstrAttributeName, 
		cstrDefaultAttributeValue, strAttributeValue);
	return strAttributeValue;

}

// get a string value
std::string CXMLFile::GetString(const char* cstrBaseKeyName, const char* cstrValueName, const char* cstrDefaultValue)
{
	std::string strValue;
	std::string strDummy;
	GetNodeValue(cstrBaseKeyName, cstrValueName, cstrDefaultValue, strValue, 
		NULL, NULL, strDummy);
	return strValue;
}

// get a string value
std::string CXMLFile::GetStringC(const char* cstrBaseKeyName, const char* cstrValueName, const char* cstrDefaultValue)
{
	std::string strValue;
	std::string strDummy;
	GetNodeValue(cstrBaseKeyName, cstrValueName, cstrDefaultValue, strValue, 
		NULL, NULL, strDummy);
	//从xml解析出来的字符串将\r去掉了，只有\n.而向msn发送消息是\n不换行，因此
	//yyc add 将\n转换为\r 2006-02-27
	char *ptr=(char *)strValue.c_str();
	while(*ptr){ if(*ptr=='\n') *ptr='\r'; ptr++; }
	return strValue;
}

// set a long value
long CXMLFile::SetLong(const char* cstrBaseKeyName, const char* cstrValueName, long lValue)
{
	long lRetVal = 0;
	char chsVal[256];
	sprintf(chsVal,"%d", lValue);

	lRetVal = SetString(cstrBaseKeyName, cstrValueName, chsVal);

	return lRetVal;
}

// set a string value
long CXMLFile::SetString(const char* cstrBaseKeyName, const char* cstrValueName, const char* cstrValue)
{
	return SetNodeValue(cstrBaseKeyName, cstrValueName,cstrValue);
}

// set a string Attribute
long CXMLFile::SetAttribute(const char* cstrBaseKeyName, const char* cstrValueName,
					const char* cstrAttributeName, const char* cstrAttributeValue)
{
	return SetNodeValue(cstrBaseKeyName, cstrValueName, NULL, cstrAttributeName, cstrAttributeValue);
}

long CXMLFile::GetNodeValue(const char* cstrBaseKeyName, const char* cstrValueName, 
		const char* cstrDefaultValue, std::string& strValue, const char* cstrAttributeName, 
		const char* cstrDefaultAttributeValue,std::string& strAttributeValue)
{
	int iNumKeys = 0;
//	std::string cstrValue = cstrDefaultValue;//yyc remove
	std::string cstrValue;
	std::string* pCStrKeys = NULL;

	if(cstrDefaultValue) cstrValue.assign(cstrDefaultValue); //yyc add 
	std::string strBaseKeyName("//");
	strBaseKeyName +=cstrBaseKeyName;
	if( strBaseKeyName.at(strBaseKeyName.length() -1) !='/' )	
		strBaseKeyName += "/";
	strBaseKeyName += cstrValueName;
	MSXML2::IXMLDOMElementPtr rootElem = NULL;
	MSXML2::IXMLDOMNodePtr foundNode = NULL;
	foundNode=XmlDocPtr->selectSingleNode( _com_util::ConvertStringToBSTR(strBaseKeyName.c_str()) );
	if (foundNode)
	{
		// get the text of the node (will be the value we requested)
		BSTR bstr = NULL;
		HRESULT hr = foundNode->get_text(&bstr);
		strValue =_com_util::ConvertBSTRToString(bstr);
		if(cstrAttributeName!=NULL)
		{
			MSXML2::IXMLDOMElementPtr elptr=foundNode;
			try //yyc add 
			{
				strAttributeValue=_com_util::ConvertBSTRToString(
				_bstr_t( elptr->getAttribute(_bstr_t(cstrAttributeName)) )
				);
			}
			catch(...) //yyc add
			{
				if(cstrDefaultAttributeValue)
					strAttributeValue=cstrDefaultAttributeValue;
			}
			
		}
		
		if (bstr) 
		{
			SysFreeString(bstr); 
			bstr = NULL; 
		}

		return 0;
	}
	else
		return -1;

}

long CXMLFile::SetNodeValue(const char* cstrBaseKeyName, const char* cstrValueName, 
			const char* cstrValue, const char* cstrAttributeName, const char* cstrAttributeValue)
{
	/*  RETURN VALUES:
		 0 = SUCCESS		-1 = LOAD FAILED		-2 = NODE NOT FOUND
		-3 = PUT TEXT FAILED		-4 = SAVE FAILED
	*/
	long lRetVal = 0;
	int iNumKeys = 0;
	std::string* pCStrKeys = NULL;

	// Add the value to the base key separated by a '\'
	std::string strBaseKeyName(cstrBaseKeyName);
	if( strBaseKeyName.at(strBaseKeyName.length() -1) !='/' )	
		strBaseKeyName += "/";
	strBaseKeyName += cstrValueName;

	// Parse all keys from the base key name (keys separated by a '\')
	pCStrKeys = ParseKeys(strBaseKeyName.c_str(), iNumKeys);

	// Traverse the xml using the keys parsed from the base key name to find the correct node
	if (pCStrKeys)
	{	
		if (XmlDocPtr == NULL)
			return -2;

		MSXML2::IXMLDOMElementPtr rootElem = NULL;
		MSXML2::IXMLDOMNodePtr foundNode = NULL;
		
		XmlDocPtr->get_documentElement(&rootElem);  // root node
		
		if (rootElem)
		{
			// returns the last node in the chain
			foundNode = FindNode(rootElem, pCStrKeys, iNumKeys, true); 
			
			if (foundNode)
			{
				HRESULT hr;
				// set the text of the node (will be the value we sent)
				if(cstrValue!=NULL)
					hr = foundNode->put_text(_bstr_t(cstrValue));
				if(cstrAttributeName!=NULL )
				{
					MSXML2::IXMLDOMElementPtr elptr=foundNode;
					elptr->setAttribute(_bstr_t(cstrAttributeName),
						_bstr_t(cstrAttributeValue) );
				}		

				if (!SUCCEEDED(hr))				
					lRetVal = -3;
				foundNode = NULL;
			}
			else
				lRetVal = -2;
			
			rootElem = NULL;
		}

		delete [] pCStrKeys;
	}

	return lRetVal;
}

// xmlfile.DeleteSetting("Settings/who","");删除该键及其所有子键
// delete a key or chain of keys
long CXMLFile::DeleteSetting(const char* cstrBaseKeyName, const char* cstrValueName)
{
	long bRetVal = -1;
	int iNumKeys = 0;
	std::string* pCStrKeys = NULL;
	std::string strBaseKeyName(cstrBaseKeyName);
	if ( strBaseKeyName!="" )
	{
		if( strBaseKeyName.at(strBaseKeyName.length() -1) !='/' )	
		strBaseKeyName += "/";
		strBaseKeyName +=  std::string(cstrValueName);
	}
	
	// Parse all keys from the base key name (keys separated by a '\')
	pCStrKeys = ParseKeys(strBaseKeyName.c_str(), iNumKeys);

	// Traverse the xml using the keys parsed from the base key name to find the correct node.
	if (pCStrKeys)
	{
		if (XmlDocPtr == NULL)
			return bRetVal;
		MSXML2::IXMLDOMElementPtr rootElem = NULL;
		MSXML2::IXMLDOMNodePtr foundNode = NULL;
		XmlDocPtr->get_documentElement(&rootElem);  // root node
		if (rootElem)
		{
			// returns the last node in the chain
			foundNode = FindNode(rootElem, pCStrKeys, iNumKeys); 
			if (foundNode)
			{
				// get the parent of the found node and use removeChild to delete the found node
				MSXML2::IXMLDOMNodePtr parentNode = NULL;
				
				foundNode->get_parentNode(&parentNode);
				
				if (parentNode)
				{
					HRESULT hr = parentNode->removeChild(foundNode);
					parentNode = NULL;
				}
				foundNode = NULL;
			}
			rootElem = NULL;
		}
		delete [] pCStrKeys;
	}
	return bRetVal;
}

// get a basekey's all children's value
long CXMLFile::GetKeysValue(const char* cstrBaseKeyName, std::map<std::string, std::string>& keys_val)
{
	int iNumKeys = 0;
	std::string* pCStrKeys = NULL;
	std::string strValue;

	pCStrKeys = ParseKeys(cstrBaseKeyName, iNumKeys);

	if (pCStrKeys)
	{
		if (XmlDocPtr == NULL)  // load the xml document
			return -1;
		MSXML2::IXMLDOMElementPtr rootElem = NULL;
		MSXML2::IXMLDOMNodePtr foundNode = NULL;
		MSXML2::IXMLDOMNodeListPtr nodelst= NULL;
		MSXML2::IXMLDOMNodePtr pNode= NULL;
		XmlDocPtr->get_documentElement(&rootElem);  // root node

		if (rootElem)
		{
			foundNode = FindNode(rootElem, pCStrKeys, iNumKeys); 
			if (foundNode)
				nodelst=foundNode->GetchildNodes();
			if(nodelst!=NULL)
			{
				for (int i=0; i<nodelst->length; i++)
				{
					pNode = nodelst->item[i];
					keys_val[(const char*)(pNode->nodeName)]=pNode->text;//(const char*)pNode->xml;					
				}
				foundNode = NULL;
			}
			pNode=NULL;
			nodelst= NULL;
			foundNode = NULL;
			rootElem = NULL;
		}
		delete [] pCStrKeys;
	}
	return 0;
/*
std::map<std::string, std::string> mp;
	std::map<std::string, std::string>::const_iterator iter;
	std::string strmsg;
	xmlfile.GetKeysValue("xmlRoot/西安/空军工程大学/学生/入学新生", mp);
	for(iter=mp.begin(); iter!=mp.end(); ++iter)
	{
		strmsg+=" ("+iter->first+", "+ iter->second+") \n";
	}
	AfxMessageBox(strmsg.c_str());
*/
	
}

// get a basekey's all children's value
long CXMLFile::GetKeys(const char* cstrBaseKeyName, std::vector<std::string>& keys)
{
	int iNumKeys = 0;
	std::string* pCStrKeys = NULL;
	std::string strValue;

	pCStrKeys = ParseKeys(cstrBaseKeyName, iNumKeys);

	if (pCStrKeys)
	{
		if (XmlDocPtr == NULL)  // load the xml document
			return -1;
		MSXML2::IXMLDOMElementPtr rootElem = NULL;
		MSXML2::IXMLDOMNodePtr foundNode = NULL;
		MSXML2::IXMLDOMNodePtr pNode= NULL;
		XmlDocPtr->get_documentElement(&rootElem);  // root node

		if (rootElem)
		{
			foundNode = FindNode(rootElem, pCStrKeys, iNumKeys);
			pNode=foundNode->GetfirstChild();
			while(pNode!=NULL)
			{
				//pNode =pNode-> nodelst->item[i];
				keys.push_back ((const char*)pNode->nodeName);//(const char*)pNode->xml;	
				//ATLTRACE((const char*)pNode->text);ATLTRACE("\n==============");
				pNode=pNode->GetnextSibling();
			}
		}
		delete [] pCStrKeys;
	}
	return 0;
}

long CXMLFile::GetRootElem(MSXML2::IXMLDOMElementPtr rootElem)
{
	return XmlDocPtr->get_documentElement(&rootElem);
}

long CXMLFile::GetNode(const char* cstrKeyName,
					   MSXML2::IXMLDOMNodePtr& foundNode)
{
	int iNumKeys = 0;
	std::string* pCStrKeys = NULL;

	std::string strBaseKeyName( "//");
	strBaseKeyName +=cstrKeyName;

	MSXML2::IXMLDOMElementPtr rootElem = NULL;
	
	foundNode=XmlDocPtr->selectSingleNode( _com_util::ConvertStringToBSTR(strBaseKeyName.c_str()) );
	if (foundNode)
	{
		return 0;
	}
	else
		return -1;

}

// Parse all keys from the base key name.
std::string* CXMLFile::ParseKeys(const char* cstrFullKeyPath, int &iNumKeys)
{
	std::string cstrTemp;
	std::string* pCStrKeys = NULL;
	std::string strFullKeyPath(cstrFullKeyPath);
	// replace spaces with _ since xml doesn't like them
	std::replace(strFullKeyPath.begin(), strFullKeyPath.end(), ' ', '_');
	
	if (*(strFullKeyPath.end() - 1) == '/' )
		strFullKeyPath.erase(strFullKeyPath.end() -1 );// remove slashes on the end

	iNumKeys=std::count(strFullKeyPath.begin(), strFullKeyPath.end(), '/') +1;

	pCStrKeys = new std::string[iNumKeys];  // create storage for the keys

	if (pCStrKeys)
	{
		int iFind = 0, iLastFind = 0, iCount = -1;
		
		// get all of the keys in the chain
		while (iFind != -1)
		{
			iFind = strFullKeyPath.find('/', iLastFind);
			if (iFind > -1)
			{
				iCount++;
				pCStrKeys[iCount].assign(strFullKeyPath, iLastFind, iFind - iLastFind);
				iLastFind = iFind + 1;
			}
			else
			{
				// make sure we don't just discard the last key in the chain
				if (iLastFind < strFullKeyPath.length()) 
				{
					iCount++;
					pCStrKeys[iCount].assign(strFullKeyPath, iLastFind, strFullKeyPath.length() - iLastFind);
				}
			}
		}
	}

	return pCStrKeys;
}

//if the specific file exist, then return true, else return false
static bool FileExist(const char* pszFileName)
{
	bool bExist = false;
	HANDLE hFile;
	
	if (NULL != pszFileName)
	{
		// Use the preferred Win32 API call and not the older OpenFile.
		hFile = CreateFile(pszFileName, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL, OPEN_EXISTING, 0, 0);
		
		if (hFile != INVALID_HANDLE_VALUE)
		{
			// If the handle is valid then the file exists.
			CloseHandle(hFile);
			bExist = true;
		}
	}
	
	return (bExist);
}
//获取装载xml错误
std::string CXMLFile::loadError()
{
	std::string strError;
	if(XmlDocPtr==NULL){
		strError="load XML - failed to create XML Object\r\n";
		return strError;
	}
	// an XML load error occurred so display the reason
	MSXML2::IXMLDOMParseErrorPtr pIParseError = NULL;
	XmlDocPtr->get_parseError(&pIParseError);
	
	if (pIParseError)
	{
		long value;
		BSTR bstr = NULL;

		HRESULT hr = pIParseError->get_errorCode(&value);
		pIParseError->get_reason(&bstr);

		if (bstr) { 
			strError=(char *)_bstr_t(bstr, true);
			SysFreeString(bstr); bstr = NULL; 
		}
		else strError.assign("load XML - failed to get reason(%d)\r\n",value);
		pIParseError = NULL;		
	}
	else strError.assign("load XML - failed to get last error\r\n");
	return strError;
}

//装载xml字符流 //yyc add
bool CXMLFile::loadXML(const char *xmlBuffer, const char* root_name)
{
	if(xmlBuffer==NULL || XmlDocPtr != NULL)
		return false;
	VARIANT_BOOL vbSuccessful;
	// initialize the Xml parser
	HRESULT hr = XmlDocPtr.CreateInstance(__uuidof(MSXML2::DOMDocument30));
		//__uuidof(MSXML2::DOMDocument40));
	//(MSXML2::CLSID_DOMDocument);
		
	if (XmlDocPtr == NULL) return false;
	m_root_name=root_name;
	vbSuccessful=XmlDocPtr->loadXML(_bstr_t(xmlBuffer));

	if (vbSuccessful == VARIANT_TRUE)
	{
		return true;  // loaded		
	}
	return false;
}
// load the XML file into the parser
bool CXMLFile::load(const char* filename, const char* root_name)
{
	if(XmlDocPtr != NULL)
		return false;
	
	VARIANT_BOOL vbSuccessful;
	if(filename) xml_file_name=filename;
	if(filename==NULL || filename[0]==0) return false;
	// initialize the Xml parser
	HRESULT hr = XmlDocPtr.CreateInstance(__uuidof(MSXML2::DOMDocument30));
		//__uuidof(MSXML2::DOMDocument40));
	//(MSXML2::CLSID_DOMDocument);
		
	if (XmlDocPtr == NULL) return false;

	m_root_name=root_name;
	// see if the file exists
	if ( !FileExist(filename ) )  // if not
	{
		// create it
		std::string strtmp("<?xml version=\"1.0\" ?><");//encoding=\"UTF-16\"
		if(root_name!=NULL)
		{
			strtmp+=root_name;
			strtmp+="></";
			strtmp+=root_name;
			strtmp+=">";
		}
		else
			strtmp+="xmlRoot></xmlRoot>";

		vbSuccessful=XmlDocPtr->loadXML(_bstr_t(strtmp.c_str()));
	}
	else  // if so
	{
		// load it
		vbSuccessful=XmlDocPtr->load(CComVariant::CComVariant((const char*)filename ));
	}

	if (vbSuccessful == VARIANT_TRUE)
	{
		return true;  // loaded		
	}
	return false;
/*  yyc remove
	else
	{
		// an XML load error occurred so display the reason
		MSXML2::IXMLDOMParseErrorPtr pIParseError = NULL;
		XmlDocPtr->get_parseError(&pIParseError);

		if (pIParseError)
		{
			long value;
			BSTR bstr = NULL;

			HRESULT hr = pIParseError->get_errorCode(&value);
			pIParseError->get_reason(&bstr);
			//std::string cstrMessage=(char *)_bstr_t(bstr, true);

			MessageBox( NULL, (char *)_bstr_t(bstr, true), "错误提示",
				MB_OK|MB_ICONERROR);

			if (bstr) { SysFreeString(bstr); bstr = NULL; }	
			pIParseError = NULL;		
		}
		return false;
	}
*/
}

// save the XML file
bool CXMLFile::save(const char* filename)
{ 
	if(XmlDocPtr==NULL)
		return false;
	HRESULT hr;
	if(filename==NULL||filename=="")
		hr = XmlDocPtr->save(CComVariant::CComVariant(xml_file_name.c_str()));
	else
		hr = XmlDocPtr->save(CComVariant::CComVariant(filename));
	xml_file_name=filename;
	//XmlDocPtr=NULL;
	return SUCCEEDED(hr);
}

// discard any changes
void CXMLFile::DiscardChanges()
{
	XmlDocPtr=NULL;	
}

// find a node given a chain of key names
MSXML2::IXMLDOMNodePtr CXMLFile::FindNode(MSXML2::IXMLDOMNodePtr parentNode, 
											  std::string* pCStrKeys, int iNumKeys, 
												  bool bAddNodes /*= false*/)
{
	MSXML2::IXMLDOMNodePtr foundNode = NULL;
	MSXML2::IXMLDOMElementPtr tempElem = NULL;

	for (int i=0; i<iNumKeys; i++)
	{
		// find the node named X directly under the parent
		foundNode = parentNode->selectSingleNode(_bstr_t(pCStrKeys[i].c_str()));

		if (foundNode == NULL) 
		{
			// if its not found...
			if (bAddNodes)  // create the node and append to parent (Set only)
			{
				tempElem=XmlDocPtr->createElement(_bstr_t(pCStrKeys[i].c_str()));
				if (tempElem) 
				{
					foundNode=parentNode->appendChild(tempElem);
					// since we are traversing the nodes, we need to set the parentNode to our foundNode
					parentNode = NULL;
					parentNode = foundNode;
					foundNode = NULL;
				}
			}
			else
			{
				foundNode = NULL;
				parentNode = NULL;
				break;
			}
		}
		else
		{
			parentNode = NULL;
			parentNode = foundNode;
			foundNode = NULL;
		}
	}

	return parentNode;
}
