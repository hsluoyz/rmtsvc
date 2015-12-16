# Microsoft Developer Studio Generated NMAKE File, Based on rmtsvc.dsp
!IF "$(CFG)" == ""
CFG=rmtsvc - Win32 Debug
!MESSAGE No configuration specified. Defaulting to rmtsvc - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "rmtsvc - Win32 Release" && "$(CFG)" != "rmtsvc - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "rmtsvc.mak" CFG="rmtsvc - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "rmtsvc - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "rmtsvc - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

!IF  "$(CFG)" == "rmtsvc - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : ".\bin\rmtsvc.exe" "$(OUTDIR)\rmtsvc.bsc"


CLEAN :
	-@erase "$(INTDIR)\cInjectDll.obj"
	-@erase "$(INTDIR)\cInjectDll.sbr"
	-@erase "$(INTDIR)\execCommand.obj"
	-@erase "$(INTDIR)\execCommand.sbr"
	-@erase "$(INTDIR)\findpassword.obj"
	-@erase "$(INTDIR)\findpassword.sbr"
	-@erase "$(INTDIR)\ftpserver.obj"
	-@erase "$(INTDIR)\ftpserver.sbr"
	-@erase "$(INTDIR)\ftpserver_ini.obj"
	-@erase "$(INTDIR)\ftpserver_ini.sbr"
	-@erase "$(INTDIR)\getSysusage.obj"
	-@erase "$(INTDIR)\getSysusage.sbr"
	-@erase "$(INTDIR)\IPF.obj"
	-@erase "$(INTDIR)\IPF.sbr"
	-@erase "$(INTDIR)\msnbot.obj"
	-@erase "$(INTDIR)\msnbot.sbr"
	-@erase "$(INTDIR)\msnbot_docmd.obj"
	-@erase "$(INTDIR)\msnbot_docmd.sbr"
	-@erase "$(INTDIR)\msnbot_docmd_video.obj"
	-@erase "$(INTDIR)\msnbot_docmd_video.sbr"
	-@erase "$(INTDIR)\NTService.obj"
	-@erase "$(INTDIR)\NTService.sbr"
	-@erase "$(INTDIR)\parseCommand.obj"
	-@erase "$(INTDIR)\parseCommand.sbr"
	-@erase "$(INTDIR)\proxyserver.obj"
	-@erase "$(INTDIR)\proxyserver.sbr"
	-@erase "$(INTDIR)\proxyserver_ini.obj"
	-@erase "$(INTDIR)\proxyserver_ini.sbr"
	-@erase "$(INTDIR)\rmtsvc.obj"
	-@erase "$(INTDIR)\rmtsvc.sbr"
	-@erase "$(INTDIR)\Script1.res"
	-@erase "$(INTDIR)\ShellCommandEx.obj"
	-@erase "$(INTDIR)\ShellCommandEx.sbr"
	-@erase "$(INTDIR)\startProcessAsUser.obj"
	-@erase "$(INTDIR)\startProcessAsUser.sbr"
	-@erase "$(INTDIR)\stringMatch.obj"
	-@erase "$(INTDIR)\stringMatch.sbr"
	-@erase "$(INTDIR)\telnetserver.obj"
	-@erase "$(INTDIR)\telnetserver.sbr"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vfwcap.obj"
	-@erase "$(INTDIR)\vfwcap.sbr"
	-@erase "$(INTDIR)\vidcManager.obj"
	-@erase "$(INTDIR)\vidcManager.sbr"
	-@erase "$(INTDIR)\vidcManager_ini.obj"
	-@erase "$(INTDIR)\vidcManager_ini.sbr"
	-@erase "$(INTDIR)\webAction.obj"
	-@erase "$(INTDIR)\webAction.sbr"
	-@erase "$(INTDIR)\webAction_fport.obj"
	-@erase "$(INTDIR)\webAction_fport.sbr"
	-@erase "$(INTDIR)\webAction_fview.obj"
	-@erase "$(INTDIR)\webAction_fview.sbr"
	-@erase "$(INTDIR)\webAction_pview.obj"
	-@erase "$(INTDIR)\webAction_pview.sbr"
	-@erase "$(INTDIR)\webAction_rview.obj"
	-@erase "$(INTDIR)\webAction_rview.sbr"
	-@erase "$(INTDIR)\webAction_sview.obj"
	-@erase "$(INTDIR)\webAction_sview.sbr"
	-@erase "$(INTDIR)\webAction_upnp.obj"
	-@erase "$(INTDIR)\webAction_upnp.sbr"
	-@erase "$(INTDIR)\webAction_vidc.obj"
	-@erase "$(INTDIR)\webAction_vidc.sbr"
	-@erase "$(INTDIR)\websvr.obj"
	-@erase "$(INTDIR)\websvr.sbr"
	-@erase "$(INTDIR)\Wutils.obj"
	-@erase "$(INTDIR)\Wutils.sbr"
	-@erase "$(OUTDIR)\rmtsvc.bsc"
	-@erase ".\bin\rmtsvc.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MD /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_CONSOLE" /D "_MBCS" /FR"$(INTDIR)\\" /Fp"$(INTDIR)\rmtsvc.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
RSC_PROJ=/l 0x804 /fo"$(INTDIR)\Script1.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\rmtsvc.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\cInjectDll.sbr" \
	"$(INTDIR)\execCommand.sbr" \
	"$(INTDIR)\findpassword.sbr" \
	"$(INTDIR)\ftpserver.sbr" \
	"$(INTDIR)\ftpserver_ini.sbr" \
	"$(INTDIR)\getSysusage.sbr" \
	"$(INTDIR)\IPF.sbr" \
	"$(INTDIR)\msnbot.sbr" \
	"$(INTDIR)\msnbot_docmd.sbr" \
	"$(INTDIR)\msnbot_docmd_video.sbr" \
	"$(INTDIR)\NTService.sbr" \
	"$(INTDIR)\parseCommand.sbr" \
	"$(INTDIR)\proxyserver.sbr" \
	"$(INTDIR)\proxyserver_ini.sbr" \
	"$(INTDIR)\rmtsvc.sbr" \
	"$(INTDIR)\ShellCommandEx.sbr" \
	"$(INTDIR)\startProcessAsUser.sbr" \
	"$(INTDIR)\stringMatch.sbr" \
	"$(INTDIR)\telnetserver.sbr" \
	"$(INTDIR)\vfwcap.sbr" \
	"$(INTDIR)\vidcManager.sbr" \
	"$(INTDIR)\vidcManager_ini.sbr" \
	"$(INTDIR)\webAction.sbr" \
	"$(INTDIR)\webAction_fport.sbr" \
	"$(INTDIR)\webAction_fview.sbr" \
	"$(INTDIR)\webAction_pview.sbr" \
	"$(INTDIR)\webAction_rview.sbr" \
	"$(INTDIR)\webAction_sview.sbr" \
	"$(INTDIR)\webAction_upnp.sbr" \
	"$(INTDIR)\webAction_vidc.sbr" \
	"$(INTDIR)\websvr.sbr" \
	"$(INTDIR)\Wutils.sbr"

"$(OUTDIR)\rmtsvc.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:no /pdb:"$(OUTDIR)\rmtsvc.pdb" /machine:I386 /out:"bin/rmtsvc.exe" 
LINK32_OBJS= \
	"$(INTDIR)\cInjectDll.obj" \
	"$(INTDIR)\execCommand.obj" \
	"$(INTDIR)\findpassword.obj" \
	"$(INTDIR)\ftpserver.obj" \
	"$(INTDIR)\ftpserver_ini.obj" \
	"$(INTDIR)\getSysusage.obj" \
	"$(INTDIR)\IPF.obj" \
	"$(INTDIR)\msnbot.obj" \
	"$(INTDIR)\msnbot_docmd.obj" \
	"$(INTDIR)\msnbot_docmd_video.obj" \
	"$(INTDIR)\NTService.obj" \
	"$(INTDIR)\parseCommand.obj" \
	"$(INTDIR)\proxyserver.obj" \
	"$(INTDIR)\proxyserver_ini.obj" \
	"$(INTDIR)\rmtsvc.obj" \
	"$(INTDIR)\ShellCommandEx.obj" \
	"$(INTDIR)\startProcessAsUser.obj" \
	"$(INTDIR)\stringMatch.obj" \
	"$(INTDIR)\telnetserver.obj" \
	"$(INTDIR)\vfwcap.obj" \
	"$(INTDIR)\vidcManager.obj" \
	"$(INTDIR)\vidcManager_ini.obj" \
	"$(INTDIR)\webAction.obj" \
	"$(INTDIR)\webAction_fport.obj" \
	"$(INTDIR)\webAction_fview.obj" \
	"$(INTDIR)\webAction_pview.obj" \
	"$(INTDIR)\webAction_rview.obj" \
	"$(INTDIR)\webAction_sview.obj" \
	"$(INTDIR)\webAction_upnp.obj" \
	"$(INTDIR)\webAction_vidc.obj" \
	"$(INTDIR)\websvr.obj" \
	"$(INTDIR)\Wutils.obj" \
	"$(INTDIR)\Script1.res"

".\bin\rmtsvc.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "rmtsvc - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\rmtsvc.exe"


CLEAN :
	-@erase "$(INTDIR)\cInjectDll.obj"
	-@erase "$(INTDIR)\execCommand.obj"
	-@erase "$(INTDIR)\findpassword.obj"
	-@erase "$(INTDIR)\ftpserver.obj"
	-@erase "$(INTDIR)\ftpserver_ini.obj"
	-@erase "$(INTDIR)\getSysusage.obj"
	-@erase "$(INTDIR)\IPF.obj"
	-@erase "$(INTDIR)\msnbot.obj"
	-@erase "$(INTDIR)\msnbot_docmd.obj"
	-@erase "$(INTDIR)\msnbot_docmd_video.obj"
	-@erase "$(INTDIR)\NTService.obj"
	-@erase "$(INTDIR)\parseCommand.obj"
	-@erase "$(INTDIR)\proxyserver.obj"
	-@erase "$(INTDIR)\proxyserver_ini.obj"
	-@erase "$(INTDIR)\rmtsvc.obj"
	-@erase "$(INTDIR)\Script1.res"
	-@erase "$(INTDIR)\ShellCommandEx.obj"
	-@erase "$(INTDIR)\startProcessAsUser.obj"
	-@erase "$(INTDIR)\stringMatch.obj"
	-@erase "$(INTDIR)\telnetserver.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\vfwcap.obj"
	-@erase "$(INTDIR)\vidcManager.obj"
	-@erase "$(INTDIR)\vidcManager_ini.obj"
	-@erase "$(INTDIR)\webAction.obj"
	-@erase "$(INTDIR)\webAction_fport.obj"
	-@erase "$(INTDIR)\webAction_fview.obj"
	-@erase "$(INTDIR)\webAction_pview.obj"
	-@erase "$(INTDIR)\webAction_rview.obj"
	-@erase "$(INTDIR)\webAction_sview.obj"
	-@erase "$(INTDIR)\webAction_upnp.obj"
	-@erase "$(INTDIR)\webAction_vidc.obj"
	-@erase "$(INTDIR)\websvr.obj"
	-@erase "$(INTDIR)\Wutils.obj"
	-@erase "$(OUTDIR)\rmtsvc.exe"
	-@erase "$(OUTDIR)\rmtsvc.ilk"
	-@erase "$(OUTDIR)\rmtsvc.pdb"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MDd /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_CONSOLE" /D "_MBCS" /Fp"$(INTDIR)\rmtsvc.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

RSC=rc.exe
RSC_PROJ=/l 0x804 /fo"$(INTDIR)\Script1.res" /d "_DEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\rmtsvc.bsc" 
BSC32_SBRS= \
	
LINK32=link.exe
LINK32_FLAGS=kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:console /incremental:yes /pdb:"$(OUTDIR)\rmtsvc.pdb" /debug /machine:I386 /out:"$(OUTDIR)\rmtsvc.exe" /pdbtype:sept 
LINK32_OBJS= \
	"$(INTDIR)\cInjectDll.obj" \
	"$(INTDIR)\execCommand.obj" \
	"$(INTDIR)\findpassword.obj" \
	"$(INTDIR)\ftpserver.obj" \
	"$(INTDIR)\ftpserver_ini.obj" \
	"$(INTDIR)\getSysusage.obj" \
	"$(INTDIR)\IPF.obj" \
	"$(INTDIR)\msnbot.obj" \
	"$(INTDIR)\msnbot_docmd.obj" \
	"$(INTDIR)\msnbot_docmd_video.obj" \
	"$(INTDIR)\NTService.obj" \
	"$(INTDIR)\parseCommand.obj" \
	"$(INTDIR)\proxyserver.obj" \
	"$(INTDIR)\proxyserver_ini.obj" \
	"$(INTDIR)\rmtsvc.obj" \
	"$(INTDIR)\ShellCommandEx.obj" \
	"$(INTDIR)\startProcessAsUser.obj" \
	"$(INTDIR)\stringMatch.obj" \
	"$(INTDIR)\telnetserver.obj" \
	"$(INTDIR)\vfwcap.obj" \
	"$(INTDIR)\vidcManager.obj" \
	"$(INTDIR)\vidcManager_ini.obj" \
	"$(INTDIR)\webAction.obj" \
	"$(INTDIR)\webAction_fport.obj" \
	"$(INTDIR)\webAction_fview.obj" \
	"$(INTDIR)\webAction_pview.obj" \
	"$(INTDIR)\webAction_rview.obj" \
	"$(INTDIR)\webAction_sview.obj" \
	"$(INTDIR)\webAction_upnp.obj" \
	"$(INTDIR)\webAction_vidc.obj" \
	"$(INTDIR)\websvr.obj" \
	"$(INTDIR)\Wutils.obj" \
	"$(INTDIR)\Script1.res"

"$(OUTDIR)\rmtsvc.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("rmtsvc.dep")
!INCLUDE "rmtsvc.dep"
!ELSE 
!MESSAGE Warning: cannot find "rmtsvc.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "rmtsvc - Win32 Release" || "$(CFG)" == "rmtsvc - Win32 Debug"
SOURCE=.\cInjectDll.cpp

!IF  "$(CFG)" == "rmtsvc - Win32 Release"


"$(INTDIR)\cInjectDll.obj"	"$(INTDIR)\cInjectDll.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "rmtsvc - Win32 Debug"


"$(INTDIR)\cInjectDll.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\execCommand.cpp

!IF  "$(CFG)" == "rmtsvc - Win32 Release"


"$(INTDIR)\execCommand.obj"	"$(INTDIR)\execCommand.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "rmtsvc - Win32 Debug"


"$(INTDIR)\execCommand.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\other\findpassword.cpp

!IF  "$(CFG)" == "rmtsvc - Win32 Release"


"$(INTDIR)\findpassword.obj"	"$(INTDIR)\findpassword.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "rmtsvc - Win32 Debug"


"$(INTDIR)\findpassword.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\ftpserver.cpp

!IF  "$(CFG)" == "rmtsvc - Win32 Release"


"$(INTDIR)\ftpserver.obj"	"$(INTDIR)\ftpserver.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "rmtsvc - Win32 Debug"


"$(INTDIR)\ftpserver.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\ftpserver_ini.cpp

!IF  "$(CFG)" == "rmtsvc - Win32 Release"


"$(INTDIR)\ftpserver_ini.obj"	"$(INTDIR)\ftpserver_ini.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "rmtsvc - Win32 Debug"


"$(INTDIR)\ftpserver_ini.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\other\getSysusage.cpp

!IF  "$(CFG)" == "rmtsvc - Win32 Release"


"$(INTDIR)\getSysusage.obj"	"$(INTDIR)\getSysusage.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "rmtsvc - Win32 Debug"


"$(INTDIR)\getSysusage.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\other\IPF.cpp

!IF  "$(CFG)" == "rmtsvc - Win32 Release"


"$(INTDIR)\IPF.obj"	"$(INTDIR)\IPF.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "rmtsvc - Win32 Debug"


"$(INTDIR)\IPF.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\msnbot\msnbot.cpp

!IF  "$(CFG)" == "rmtsvc - Win32 Release"


"$(INTDIR)\msnbot.obj"	"$(INTDIR)\msnbot.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "rmtsvc - Win32 Debug"


"$(INTDIR)\msnbot.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\msnbot\msnbot_docmd.cpp

!IF  "$(CFG)" == "rmtsvc - Win32 Release"


"$(INTDIR)\msnbot_docmd.obj"	"$(INTDIR)\msnbot_docmd.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "rmtsvc - Win32 Debug"


"$(INTDIR)\msnbot_docmd.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\msnbot\msnbot_docmd_video.cpp

!IF  "$(CFG)" == "rmtsvc - Win32 Release"


"$(INTDIR)\msnbot_docmd_video.obj"	"$(INTDIR)\msnbot_docmd_video.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "rmtsvc - Win32 Debug"


"$(INTDIR)\msnbot_docmd_video.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\NTService.cpp

!IF  "$(CFG)" == "rmtsvc - Win32 Release"


"$(INTDIR)\NTService.obj"	"$(INTDIR)\NTService.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "rmtsvc - Win32 Debug"


"$(INTDIR)\NTService.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\parseCommand.cpp

!IF  "$(CFG)" == "rmtsvc - Win32 Release"


"$(INTDIR)\parseCommand.obj"	"$(INTDIR)\parseCommand.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "rmtsvc - Win32 Debug"


"$(INTDIR)\parseCommand.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\proxyserver.cpp

!IF  "$(CFG)" == "rmtsvc - Win32 Release"


"$(INTDIR)\proxyserver.obj"	"$(INTDIR)\proxyserver.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "rmtsvc - Win32 Debug"


"$(INTDIR)\proxyserver.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\proxyserver_ini.cpp

!IF  "$(CFG)" == "rmtsvc - Win32 Release"


"$(INTDIR)\proxyserver_ini.obj"	"$(INTDIR)\proxyserver_ini.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "rmtsvc - Win32 Debug"


"$(INTDIR)\proxyserver_ini.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\rmtsvc.cpp

!IF  "$(CFG)" == "rmtsvc - Win32 Release"


"$(INTDIR)\rmtsvc.obj"	"$(INTDIR)\rmtsvc.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "rmtsvc - Win32 Debug"


"$(INTDIR)\rmtsvc.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\Script1.rc

"$(INTDIR)\Script1.res" : $(SOURCE) "$(INTDIR)"
	$(RSC) $(RSC_PROJ) $(SOURCE)


SOURCE=.\ShellCommandEx.cpp

!IF  "$(CFG)" == "rmtsvc - Win32 Release"


"$(INTDIR)\ShellCommandEx.obj"	"$(INTDIR)\ShellCommandEx.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "rmtsvc - Win32 Debug"


"$(INTDIR)\ShellCommandEx.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\startProcessAsUser.cpp

!IF  "$(CFG)" == "rmtsvc - Win32 Release"


"$(INTDIR)\startProcessAsUser.obj"	"$(INTDIR)\startProcessAsUser.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "rmtsvc - Win32 Debug"


"$(INTDIR)\startProcessAsUser.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\other\stringMatch.cpp

!IF  "$(CFG)" == "rmtsvc - Win32 Release"


"$(INTDIR)\stringMatch.obj"	"$(INTDIR)\stringMatch.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "rmtsvc - Win32 Debug"


"$(INTDIR)\stringMatch.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\telnetserver.cpp

!IF  "$(CFG)" == "rmtsvc - Win32 Release"


"$(INTDIR)\telnetserver.obj"	"$(INTDIR)\telnetserver.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "rmtsvc - Win32 Debug"


"$(INTDIR)\telnetserver.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\msnbot\vfwcap.cpp

!IF  "$(CFG)" == "rmtsvc - Win32 Release"


"$(INTDIR)\vfwcap.obj"	"$(INTDIR)\vfwcap.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "rmtsvc - Win32 Debug"


"$(INTDIR)\vfwcap.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\vIDC\webI\vidcManager.cpp

!IF  "$(CFG)" == "rmtsvc - Win32 Release"


"$(INTDIR)\vidcManager.obj"	"$(INTDIR)\vidcManager.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "rmtsvc - Win32 Debug"


"$(INTDIR)\vidcManager.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=..\..\vIDC\webI\vidcManager_ini.cpp

!IF  "$(CFG)" == "rmtsvc - Win32 Release"


"$(INTDIR)\vidcManager_ini.obj"	"$(INTDIR)\vidcManager_ini.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "rmtsvc - Win32 Debug"


"$(INTDIR)\vidcManager_ini.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\webAction.cpp

!IF  "$(CFG)" == "rmtsvc - Win32 Release"


"$(INTDIR)\webAction.obj"	"$(INTDIR)\webAction.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "rmtsvc - Win32 Debug"


"$(INTDIR)\webAction.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\webAction_fport.cpp

!IF  "$(CFG)" == "rmtsvc - Win32 Release"


"$(INTDIR)\webAction_fport.obj"	"$(INTDIR)\webAction_fport.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "rmtsvc - Win32 Debug"


"$(INTDIR)\webAction_fport.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\webAction_fview.cpp

!IF  "$(CFG)" == "rmtsvc - Win32 Release"


"$(INTDIR)\webAction_fview.obj"	"$(INTDIR)\webAction_fview.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "rmtsvc - Win32 Debug"


"$(INTDIR)\webAction_fview.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\webAction_pview.cpp

!IF  "$(CFG)" == "rmtsvc - Win32 Release"


"$(INTDIR)\webAction_pview.obj"	"$(INTDIR)\webAction_pview.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "rmtsvc - Win32 Debug"


"$(INTDIR)\webAction_pview.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\webAction_rview.cpp

!IF  "$(CFG)" == "rmtsvc - Win32 Release"


"$(INTDIR)\webAction_rview.obj"	"$(INTDIR)\webAction_rview.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "rmtsvc - Win32 Debug"


"$(INTDIR)\webAction_rview.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\webAction_sview.cpp

!IF  "$(CFG)" == "rmtsvc - Win32 Release"


"$(INTDIR)\webAction_sview.obj"	"$(INTDIR)\webAction_sview.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "rmtsvc - Win32 Debug"


"$(INTDIR)\webAction_sview.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\webAction_upnp.cpp

!IF  "$(CFG)" == "rmtsvc - Win32 Release"


"$(INTDIR)\webAction_upnp.obj"	"$(INTDIR)\webAction_upnp.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "rmtsvc - Win32 Debug"


"$(INTDIR)\webAction_upnp.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=..\..\vIDC\webI\webAction_vidc.cpp

!IF  "$(CFG)" == "rmtsvc - Win32 Release"


"$(INTDIR)\webAction_vidc.obj"	"$(INTDIR)\webAction_vidc.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "rmtsvc - Win32 Debug"


"$(INTDIR)\webAction_vidc.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

SOURCE=.\websvr.cpp

!IF  "$(CFG)" == "rmtsvc - Win32 Release"


"$(INTDIR)\websvr.obj"	"$(INTDIR)\websvr.sbr" : $(SOURCE) "$(INTDIR)"


!ELSEIF  "$(CFG)" == "rmtsvc - Win32 Debug"


"$(INTDIR)\websvr.obj" : $(SOURCE) "$(INTDIR)"


!ENDIF 

SOURCE=.\other\Wutils.cpp

!IF  "$(CFG)" == "rmtsvc - Win32 Release"


"$(INTDIR)\Wutils.obj"	"$(INTDIR)\Wutils.sbr" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "rmtsvc - Win32 Debug"


"$(INTDIR)\Wutils.obj" : $(SOURCE) "$(INTDIR)"
	$(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 


!ENDIF 

