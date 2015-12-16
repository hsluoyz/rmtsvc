# Microsoft Developer Studio Project File - Name="net4cpp21" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=net4cpp21 - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "net4cpp21.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "net4cpp21.mak" CFG="net4cpp21 - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "net4cpp21 - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "net4cpp21 - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE "net4cpp21 - Win32 nossl Release" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "net4cpp21 - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /FR /YX /FD /c
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /out:"..\libs\bin\net4cpp21.lib"
# SUBTRACT LIB32 /nologo

!ELSEIF  "$(CFG)" == "net4cpp21 - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /FR /YX /FD /GZ /c
# ADD BASE RSC /l 0x804 /d "_DEBUG"
# ADD RSC /l 0x804 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /out:"..\libs\bin\net4cpp21_d.lib"
# SUBTRACT LIB32 /nologo

!ELSEIF  "$(CFG)" == "net4cpp21 - Win32 nossl Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "net4cpp21___Win32_nossl_Release"
# PROP BASE Intermediate_Dir "net4cpp21___Win32_nossl_Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "nossl_Release"
# PROP Intermediate_Dir "nossl_Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /FR /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "_NOSSL_D" /FR /YX /FD /c
# ADD BASE RSC /l 0x804 /d "NDEBUG"
# ADD RSC /l 0x804 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /out:"..\libs\bin\net4cpp21_nossl.lib"
# SUBTRACT LIB32 /nologo

!ENDIF 

# Begin Target

# Name "net4cpp21 - Win32 Release"
# Name "net4cpp21 - Win32 Debug"
# Name "net4cpp21 - Win32 nossl Release"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\utils\Buffer.cpp
# End Source File
# Begin Source File

SOURCE=.\utils\cCmdShell.cpp
# End Source File
# Begin Source File

SOURCE=.\utils\cCoder.cpp
# End Source File
# Begin Source File

SOURCE=.\utils\cLogger.cpp
# End Source File
# Begin Source File

SOURCE=.\tools\cTelnet.cpp
# End Source File
# Begin Source File

SOURCE=.\utils\cThread.cpp
# End Source File
# Begin Source File

SOURCE=.\utils\cTime.cpp
# End Source File
# Begin Source File

SOURCE=.\protocol\dnsclnt.cpp
# End Source File
# Begin Source File

SOURCE=.\protocol\ftpclnt.cpp
# End Source File
# Begin Source File

SOURCE=.\protocol\ftpsvr.cpp
# End Source File
# Begin Source File

SOURCE=.\protocol\ftpsvr_docmd.cpp
# End Source File
# Begin Source File

SOURCE=.\protocol\httpclnt.cpp
# End Source File
# Begin Source File

SOURCE=.\protocol\httpreq.cpp
# End Source File
# Begin Source File

SOURCE=.\protocol\httprsp.cpp
# End Source File
# Begin Source File

SOURCE=.\protocol\httpsvr.cpp
# End Source File
# Begin Source File

SOURCE=.\protocol\httpsvr_response.cpp
# End Source File
# Begin Source File

SOURCE=.\IPRules.cpp
# End Source File
# Begin Source File

SOURCE=.\tools\mapport.cpp
# End Source File
# Begin Source File

SOURCE=.\utils\OTP.cpp
# End Source File
# Begin Source File

SOURCE=.\protocol\proxyclnt.cpp
# End Source File
# Begin Source File

SOURCE=.\protocol\proxysvr.cpp
# End Source File
# Begin Source File

SOURCE=.\protocol\proxysvr_https.cpp
# End Source File
# Begin Source File

SOURCE=.\protocol\proxysvr_socks4.cpp
# End Source File
# Begin Source File

SOURCE=.\protocol\proxysvr_socks5.cpp
# End Source File
# Begin Source File

SOURCE=.\protocol\smtpClnt.cpp
# End Source File
# Begin Source File

SOURCE=.\protocol\smtpsvr.cpp
# End Source File
# Begin Source File

SOURCE=.\tools\sniffer.cpp
# End Source File
# Begin Source File

SOURCE=.\protocol\sntpclnt.cpp
# End Source File
# Begin Source File

SOURCE=.\socketBase.cpp
# End Source File
# Begin Source File

SOURCE=.\protocol\socketIcmp.cpp
# End Source File
# Begin Source File

SOURCE=.\socketRaw.cpp
# End Source File
# Begin Source File

SOURCE=.\socketSvr.cpp
# End Source File
# Begin Source File

SOURCE=.\socketTcp.cpp
# End Source File
# Begin Source File

SOURCE=.\socketUdp.cpp
# End Source File
# Begin Source File

SOURCE=.\protocol\upnp.cpp
# End Source File
# Begin Source File

SOURCE=.\utils\utils.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\include\Buffer.h
# End Source File
# Begin Source File

SOURCE=.\utils\cCmdShell.h
# End Source File
# Begin Source File

SOURCE=.\include\cCoder.h
# End Source File
# Begin Source File

SOURCE=.\include\cLogger.h
# End Source File
# Begin Source File

SOURCE=.\include\cMutex.h
# End Source File
# Begin Source File

SOURCE=.\include\cTelnet.h
# End Source File
# Begin Source File

SOURCE=.\include\cThread.h
# End Source File
# Begin Source File

SOURCE=.\utils\cTime.h
# End Source File
# Begin Source File

SOURCE=.\include\dnsclnt.h
# End Source File
# Begin Source File

SOURCE=.\include\dnsdef.h
# End Source File
# Begin Source File

SOURCE=.\include\ftpclnt.h
# End Source File
# Begin Source File

SOURCE=.\include\ftpdef.h
# End Source File
# Begin Source File

SOURCE=.\include\ftpsvr.h
# End Source File
# Begin Source File

SOURCE=.\include\Handle.h
# End Source File
# Begin Source File

SOURCE=.\include\httpclnt.h
# End Source File
# Begin Source File

SOURCE=.\include\httpdef.h
# End Source File
# Begin Source File

SOURCE=.\include\httpreq.h
# End Source File
# Begin Source File

SOURCE=.\include\httprsp.h
# End Source File
# Begin Source File

SOURCE=.\include\httpsvr.h
# End Source File
# Begin Source File

SOURCE=.\include\icmpdef.h
# End Source File
# Begin Source File

SOURCE=.\include\ipdef.h
# End Source File
# Begin Source File

SOURCE=.\include\IPRules.h
# End Source File
# Begin Source File

SOURCE=.\include\mapport.h
# End Source File
# Begin Source File

SOURCE=.\utils\OTP.h
# End Source File
# Begin Source File

SOURCE=.\include\proxyclnt.h
# End Source File
# Begin Source File

SOURCE=.\include\proxydef.h
# End Source File
# Begin Source File

SOURCE=.\include\proxysvr.h
# End Source File
# Begin Source File

SOURCE=.\include\smtpclnt.h
# End Source File
# Begin Source File

SOURCE=.\include\smtpdef.h
# End Source File
# Begin Source File

SOURCE=.\include\smtpsvr.h
# End Source File
# Begin Source File

SOURCE=.\include\sniffer.h
# End Source File
# Begin Source File

SOURCE=.\include\sntpclnt.h
# End Source File
# Begin Source File

SOURCE=.\include\sntpdef.h
# End Source File
# Begin Source File

SOURCE=.\include\socketBase.h
# End Source File
# Begin Source File

SOURCE=.\include\socketdef.h
# End Source File
# Begin Source File

SOURCE=.\include\socketIcmp.h
# End Source File
# Begin Source File

SOURCE=.\include\socketRaw.h
# End Source File
# Begin Source File

SOURCE=.\include\socketSvr.h
# End Source File
# Begin Source File

SOURCE=.\include\socketTcp.h
# End Source File
# Begin Source File

SOURCE=.\include\socketUdp.h
# End Source File
# Begin Source File

SOURCE=.\include\stundef.h
# End Source File
# Begin Source File

SOURCE=.\include\sysconfig.h
# End Source File
# Begin Source File

SOURCE=.\include\upnp.h
# End Source File
# Begin Source File

SOURCE=.\include\upnpdef.h
# End Source File
# Begin Source File

SOURCE=.\utils\utils.h
# End Source File
# End Group
# End Target
# End Project
