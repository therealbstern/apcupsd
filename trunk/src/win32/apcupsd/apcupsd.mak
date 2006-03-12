# 
!IF "$(CFG)" == ""
CFG=apcupsd - Win32 Release
!MESSAGE No configuration specified. Defaulting to apcupsd - Win32 Release.
!ENDIF 

!IF "$(CFG)" != "apcupsd - Win32 Release" && "$(CFG)" != "apcupsd - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "apcupsd.mak" CFG="apcupsd - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "apcupsd - Win32 Release" (based on "Win32 (x86) Console Application")
!MESSAGE "apcupsd - Win32 Debug" (based on "Win32 (x86) Console Application")
!MESSAGE 
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "apcupsd - Win32 Release"

OUTDIR=.\Release
INTDIR=.\Release
# Begin Custom Macros
OutDir=.\Release
# End Custom Macros

ALL : "$(OUTDIR)\apcupsd.exe"


CLEAN :
	-@erase "$(INTDIR)\compat.obj"
	-@erase "$(INTDIR)\print.obj"
	-@erase "$(INTDIR)\winabout.obj"
	-@erase "$(INTDIR)\winevents.obj"
	-@erase "$(INTDIR)\winmain.obj"
	-@erase "$(INTDIR)\winprop.obj"
	-@erase "$(INTDIR)\newwinservice.obj"
	-@erase "$(INTDIR)\winstat.obj"
	-@erase "$(INTDIR)\wintray.obj"
	-@erase "$(INTDIR)\winapi.obj"
	-@erase "$(INTDIR)\action.obj"
	-@erase "$(INTDIR)\apcnis.obj"
	-@erase "$(INTDIR)\apcupsd.obj"
	-@erase "$(INTDIR)\device.obj"
	-@erase "$(INTDIR)\options.obj"
	-@erase "$(INTDIR)\reports.obj"
	-@erase "$(INTDIR)\drivers.obj"
	-@erase "$(INTDIR)\apcconfig.obj"
	-@erase "$(INTDIR)\apcerror.obj"
	-@erase "$(INTDIR)\apcevents.obj"
	-@erase "$(INTDIR)\apcexec.obj"
	-@erase "$(INTDIR)\apcfile.obj"
	-@erase "$(INTDIR)\apclibnis.obj"
	-@erase "$(INTDIR)\apclock.obj"
	-@erase "$(INTDIR)\apclog.obj"
	-@erase "$(INTDIR)\apcsignal.obj"
	-@erase "$(INTDIR)\apcstatus.obj"
	-@erase "$(INTDIR)\newups.obj"
	-@erase "$(INTDIR)\asys.obj"
	-@erase "$(INTDIR)\testdriver.obj"
        -@erase "$(OUTDIR)\apcupsd.exe"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MT /W3 /GX /O2 /I "../compat" /I "../.." /I "../../../include" /I "../../../../depkgs-win32/pthreads" /I "." /D "NDEBUG" /D "WIN32" /D "__WXMSW__" /D "_CONSOLE" /D "_MBCS" /D "HAVE_WIN32" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\winres.res" /d "NDEBUG" 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\apcupsd.bsc" 
BSC32_SBRS= \
        
LINK32=link.exe
LINK32_FLAGS=rpcrt4.lib oleaut32.lib ole32.lib uuid.lib winspool.lib winmm.lib \
  comctl32.lib comdlg32.lib Shell32.lib AdvAPI32.lib User32.lib Gdi32.lib wsock32.lib \
  wldap32.lib pthreadVCE.lib zlib.lib /nodefaultlib:libcmt.lib \
  /nologo /subsystem:windows /machine:I386 /out:"$(OUTDIR)\apcupsd.exe" /libpath:"../../../../depkgs-win32/pthreads"
LINK32_OBJS= \
	"$(INTDIR)\compat.obj" \
	"$(INTDIR)\print.obj" \
	"$(INTDIR)\winabout.obj" \
	"$(INTDIR)\winevents.obj" \
	"$(INTDIR)\winmain.obj" \
	"$(INTDIR)\winprop.obj" \
	"$(INTDIR)\newwinservice.obj" \
	"$(INTDIR)\winstat.obj" \
	"$(INTDIR)\wintray.obj" \
	"$(INTDIR)\winapi.obj" \
	"$(INTDIR)\action.obj" \
	"$(INTDIR)\apcnis.obj" \
	"$(INTDIR)\apcupsd.obj" \
	"$(INTDIR)\device.obj" \
	"$(INTDIR)\options.obj" \
	"$(INTDIR)\reports.obj" \
	"$(INTDIR)\drivers.obj" \
	"$(INTDIR)\apcconfig.obj" \
	"$(INTDIR)\apcerror.obj" \
	"$(INTDIR)\apcevents.obj" \
	"$(INTDIR)\apcexec.obj" \
	"$(INTDIR)\apcfile.obj" \
	"$(INTDIR)\apclibnis.obj" \
	"$(INTDIR)\apclock.obj" \
	"$(INTDIR)\apclog.obj" \
	"$(INTDIR)\apcsignal.obj" \
	"$(INTDIR)\apcstatus.obj" \
	"$(INTDIR)\newups.obj" \
	"$(INTDIR)\asys.obj" \
	"$(INTDIR)\testdriver.obj" \
        "$(INTDIR)\winres.res"

"$(OUTDIR)\apcupsd.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ELSEIF  "$(CFG)" == "apcupsd - Win32 Debug"

OUTDIR=.\Debug
INTDIR=.\Debug
# Begin Custom Macros
OutDir=.\Debug
# End Custom Macros

ALL : "$(OUTDIR)\apcupsd.exe" "$(OUTDIR)\apcupsd.bsc"

CLEAN :
	-@erase "$(INTDIR)\compat.obj
	-@erase "$(INTDIR)\compat.sbr"
	-@erase "$(INTDIR)\print.obj
	-@erase "$(INTDIR)\print.sbr"
	-@erase "$(INTDIR)\winabout.obj
	-@erase "$(INTDIR)\winabout.sbr"
	-@erase "$(INTDIR)\winevents.obj
	-@erase "$(INTDIR)\winevents.sbr"
	-@erase "$(INTDIR)\winmain.obj
	-@erase "$(INTDIR)\winmain.sbr"
	-@erase "$(INTDIR)\winprop.obj
	-@erase "$(INTDIR)\winprop.sbr"
	-@erase "$(INTDIR)\newwinservice.obj
	-@erase "$(INTDIR)\newwinservice.sbr"
	-@erase "$(INTDIR)\winstat.obj
	-@erase "$(INTDIR)\winstat.sbr"
	-@erase "$(INTDIR)\wintray.obj
	-@erase "$(INTDIR)\wintray.sbr"
	-@erase "$(INTDIR)\winapi.obj
	-@erase "$(INTDIR)\winapi.sbr"
	-@erase "$(INTDIR)\action.obj
	-@erase "$(INTDIR)\action.sbr"
	-@erase "$(INTDIR)\apcnis.obj
	-@erase "$(INTDIR)\apcnis.sbr"
	-@erase "$(INTDIR)\apcupsd.obj
	-@erase "$(INTDIR)\apcupsd.sbr"
	-@erase "$(INTDIR)\device.obj
	-@erase "$(INTDIR)\device.sbr"
	-@erase "$(INTDIR)\options.obj
	-@erase "$(INTDIR)\options.sbr"
	-@erase "$(INTDIR)\reports.obj
	-@erase "$(INTDIR)\reports.sbr"
	-@erase "$(INTDIR)\drivers.obj
	-@erase "$(INTDIR)\drivers.sbr"
	-@erase "$(INTDIR)\apcconfig.obj
	-@erase "$(INTDIR)\apcconfig.sbr"
	-@erase "$(INTDIR)\apcerror.obj
	-@erase "$(INTDIR)\apcerror.sbr"
	-@erase "$(INTDIR)\apcevents.obj
	-@erase "$(INTDIR)\apcevents.sbr"
	-@erase "$(INTDIR)\apcexec.obj
	-@erase "$(INTDIR)\apcexec.sbr"
	-@erase "$(INTDIR)\apcfile.obj
	-@erase "$(INTDIR)\apcfile.sbr"
	-@erase "$(INTDIR)\apclibnis.obj
	-@erase "$(INTDIR)\apclibnis.sbr"
	-@erase "$(INTDIR)\apclock.obj
	-@erase "$(INTDIR)\apclock.sbr"
	-@erase "$(INTDIR)\apclog.obj
	-@erase "$(INTDIR)\apclog.sbr"
	-@erase "$(INTDIR)\apcsignal.obj
	-@erase "$(INTDIR)\apcsignal.sbr"
	-@erase "$(INTDIR)\apcstatus.obj
	-@erase "$(INTDIR)\apcstatus.sbr"
	-@erase "$(INTDIR)\newups.obj
	-@erase "$(INTDIR)\newups.sbr"
	-@erase "$(INTDIR)\asys.obj
	-@erase "$(INTDIR)\asys.sbr"
	-@erase "$(INTDIR)\testdriver.obj
	-@erase "$(INTDIR)\testdriver.sbr"
        -@erase "$(OUTDIR)\apcupsd.exe"
        -@erase "$(OUTDIR)\apcupsd.bsc"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"


CPP_PROJ=/nologo /MTd /W3 /Gm /GX /ZI /Od /I "../compat" /I "../.." /I "../../../include" /I "../../../../depkgs-win32/pthreads" /I "."  /D "_DEBUG" /D "WIN32" /D "__WXMSW__" /D "_CONSOLE" /D "_MBCS" /D "HAVE_WIN32" /FR"$(INTDIR)\\" /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
RSC_PROJ=/l 0x409 /fo"$(INTDIR)\winres.res" /d "_DEBUG"
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\apcupsd.bsc" 
BSC32_SBRS= \
	"$(INTDIR)\compat.sbr" \
	"$(INTDIR)\print.sbr" \
	"$(INTDIR)\winabout.sbr" \
	"$(INTDIR)\winevents.sbr" \
	"$(INTDIR)\winmain.sbr" \
	"$(INTDIR)\winprop.sbr" \
	"$(INTDIR)\newwinservice.sbr" \
	"$(INTDIR)\winstat.sbr" \
	"$(INTDIR)\wintray.sbr" \
	"$(INTDIR)\winapi.sbr" \
	"$(INTDIR)\action.sbr" \
	"$(INTDIR)\apcnis.sbr" \
	"$(INTDIR)\apcupsd.sbr" \
	"$(INTDIR)\device.sbr" \
	"$(INTDIR)\options.sbr" \
	"$(INTDIR)\reports.sbr" \
	"$(INTDIR)\drivers.sbr" \
	"$(INTDIR)\apcconfig.sbr" \
	"$(INTDIR)\apcerror.sbr" \
	"$(INTDIR)\apcevents.sbr" \
	"$(INTDIR)\apcexec.sbr" \
	"$(INTDIR)\apcfile.sbr" \
	"$(INTDIR)\apclibnis.sbr" \
	"$(INTDIR)\apclock.sbr" \
	"$(INTDIR)\apclog.sbr" \
	"$(INTDIR)\apcsignal.sbr" \
	"$(INTDIR)\apcstatus.sbr" \
	"$(INTDIR)\newups.sbr" \
	"$(INTDIR)\asys.sbr" \
	"$(INTDIR)\testdriver.sbr" \

"$(OUTDIR)\apcupsd.bsc" : "$(OUTDIR)" $(BSC32_SBRS)
    $(BSC32) @<<
  $(BSC32_FLAGS) $(BSC32_SBRS)
<<

LINK32=link.exe
LINK32_FLAGS=rpcrt4.lib oleaut32.lib ole32.lib uuid.lib winspool.lib winmm.lib \
  comctl32.lib comdlg32.lib Shell32.lib AdvAPI32.lib User32.lib Gdi32.lib wsock32.lib \
  wldap32.lib pthreadVCE.lib zlib.lib /nodefaultlib:libcmtd.lib \
  /nologo /subsystem:windows /pdb:none /debug /machine:I386 /out:"$(OUTDIR)\apcupsd.exe" /libpath:"../../../../depkgs-win32/wx/lib" /libpath:"../../../../depkgs-win32/pthreads" /libpath:"../../../../depkgs-win32/zlib" 
LINK32_OBJS= \
	"$(INTDIR)\compat.obj" \
	"$(INTDIR)\print.obj" \
	"$(INTDIR)\winabout.obj" \
	"$(INTDIR)\winevents.obj" \
	"$(INTDIR)\winmain.obj" \
	"$(INTDIR)\winprop.obj" \
	"$(INTDIR)\newwinservice.obj" \
	"$(INTDIR)\winstat.obj" \
	"$(INTDIR)\wintray.obj" \
	"$(INTDIR)\winapi.obj" \
	"$(INTDIR)\action.obj" \
	"$(INTDIR)\apcnis.obj" \
	"$(INTDIR)\apcupsd.obj" \
	"$(INTDIR)\device.obj" \
	"$(INTDIR)\options.obj" \
	"$(INTDIR)\reports.obj" \
	"$(INTDIR)\drivers.obj" \
	"$(INTDIR)\apcconfig.obj" \
	"$(INTDIR)\apcerror.obj" \
	"$(INTDIR)\apcevents.obj" \
	"$(INTDIR)\apcexec.obj" \
	"$(INTDIR)\apcfile.obj" \
	"$(INTDIR)\apclibnis.obj" \
	"$(INTDIR)\apclock.obj" \
	"$(INTDIR)\apclog.obj" \
	"$(INTDIR)\apcsignal.obj" \
	"$(INTDIR)\apcstatus.obj" \
	"$(INTDIR)\newups.obj" \
	"$(INTDIR)\asys.obj" \
	"$(INTDIR)\testdriver.obj" \
        "$(INTDIR)\winres.res"

"$(OUTDIR)\apcupsd.exe" : "$(OUTDIR)" $(DEF_FILE) $(LINK32_OBJS)
    $(LINK32) @<<
  $(LINK32_FLAGS) $(LINK32_OBJS)
<<

!ENDIF 

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


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("apcupsd.dep")
!INCLUDE "apcupsd.dep"
!ELSE 
!MESSAGE Warning: cannot find "apcupsd.dep"
!ENDIF 
!ENDIF 

SOURCE=..\winres.rc

"$(INTDIR)\winres.res" : $(SOURCE) "$(INTDIR)"
        $(RSC) /l 0x409 /fo"$(INTDIR)\winres.res" /d "NDEBUG" $(SOURCE)

FILENAME=compat
SOURCE=..\compat\compat.cpp

!IF  "$(CFG)" == "apcupsd - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "apcupsd - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"     "$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

FILENAME=print
SOURCE=..\compat\print.cpp

!IF  "$(CFG)" == "apcupsd - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "apcupsd - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"     "$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

FILENAME=winabout
SOURCE=..\winabout.cpp

!IF  "$(CFG)" == "apcupsd - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "apcupsd - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"     "$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

FILENAME=winevents
SOURCE=..\winevents.cpp

!IF  "$(CFG)" == "apcupsd - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "apcupsd - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"     "$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

FILENAME=winmain
SOURCE=..\winmain.cpp

!IF  "$(CFG)" == "apcupsd - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "apcupsd - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"     "$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

FILENAME=winprop
SOURCE=..\winprop.cpp

!IF  "$(CFG)" == "apcupsd - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "apcupsd - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"     "$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

FILENAME=newwinservice
SOURCE=..\newwinservice.cpp

!IF  "$(CFG)" == "apcupsd - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "apcupsd - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"     "$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

FILENAME=winstat
SOURCE=..\winstat.cpp

!IF  "$(CFG)" == "apcupsd - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "apcupsd - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"     "$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

FILENAME=wintray
SOURCE=..\wintray.cpp

!IF  "$(CFG)" == "apcupsd - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "apcupsd - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"     "$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

FILENAME=winapi
SOURCE=..\winapi.cpp

!IF  "$(CFG)" == "apcupsd - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "apcupsd - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"     "$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

FILENAME=action
SOURCE=..\lib\action.cpp

!IF  "$(CFG)" == "apcupsd - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "apcupsd - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"     "$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

FILENAME=apcnis
SOURCE=..\lib\apcnis.cpp

!IF  "$(CFG)" == "apcupsd - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "apcupsd - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"     "$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

FILENAME=apcupsd
SOURCE=..\lib\apcupsd.cpp

!IF  "$(CFG)" == "apcupsd - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "apcupsd - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"     "$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

FILENAME=device
SOURCE=..\lib\device.cpp

!IF  "$(CFG)" == "apcupsd - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "apcupsd - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"     "$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

FILENAME=options
SOURCE=..\lib\options.cpp

!IF  "$(CFG)" == "apcupsd - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "apcupsd - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"     "$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

FILENAME=reports
SOURCE=..\lib\reports.cpp

!IF  "$(CFG)" == "apcupsd - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "apcupsd - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"     "$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

FILENAME=drivers
SOURCE=..\lib\drivers.cpp

!IF  "$(CFG)" == "apcupsd - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "apcupsd - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"     "$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

FILENAME=apcconfig
SOURCE=..\lib\apcconfig.cpp

!IF  "$(CFG)" == "apcupsd - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "apcupsd - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"     "$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

FILENAME=apcerror
SOURCE=..\lib\apcerror.cpp

!IF  "$(CFG)" == "apcupsd - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "apcupsd - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"     "$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

FILENAME=apcevents
SOURCE=..\lib\apcevents.cpp

!IF  "$(CFG)" == "apcupsd - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "apcupsd - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"     "$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

FILENAME=apcexec
SOURCE=..\lib\apcexec.cpp

!IF  "$(CFG)" == "apcupsd - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "apcupsd - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"     "$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

FILENAME=apcfile
SOURCE=..\lib\apcfile.cpp

!IF  "$(CFG)" == "apcupsd - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "apcupsd - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"     "$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

FILENAME=apclibnis
SOURCE=..\lib\apclibnis.cpp

!IF  "$(CFG)" == "apcupsd - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "apcupsd - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"     "$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

FILENAME=apclock
SOURCE=..\lib\apclock.cpp

!IF  "$(CFG)" == "apcupsd - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "apcupsd - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"     "$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

FILENAME=apclog
SOURCE=..\lib\apclog.cpp

!IF  "$(CFG)" == "apcupsd - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "apcupsd - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"     "$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

FILENAME=apcsignal
SOURCE=..\lib\apcsignal.cpp

!IF  "$(CFG)" == "apcupsd - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "apcupsd - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"     "$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

FILENAME=apcstatus
SOURCE=..\lib\apcstatus.cpp

!IF  "$(CFG)" == "apcupsd - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "apcupsd - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"     "$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

FILENAME=newups
SOURCE=..\lib\newups.cpp

!IF  "$(CFG)" == "apcupsd - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "apcupsd - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"     "$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

FILENAME=asys
SOURCE=..\lib\asys.cpp

!IF  "$(CFG)" == "apcupsd - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "apcupsd - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"     "$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

FILENAME=testdriver
SOURCE=..\lib\testdriver.cpp

!IF  "$(CFG)" == "apcupsd - Win32 Release"


"$(INTDIR)\$(FILENAME).obj" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ELSEIF  "$(CFG)" == "apcupsd - Win32 Debug"


"$(INTDIR)\$(FILENAME).obj"     "$(INTDIR)\$(FILENAME).sbr" : $(SOURCE) "$(INTDIR)"
        $(CPP) $(CPP_PROJ) $(SOURCE)


!ENDIF 

