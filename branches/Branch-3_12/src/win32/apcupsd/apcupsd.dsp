# Microsoft Developer Studio Project File - Name="apcupsd" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Application" 0x0101

CFG=apcupsd - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "apcupsd.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "apcupsd.mak" CFG="apcupsd - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "apcupsd - Win32 Release" (based on "Win32 (x86) Application")
!MESSAGE "apcupsd - Win32 Debug" (based on "Win32 (x86) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "apcupsd - Win32 Release"

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
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /c
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386
# ADD LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /machine:I386

!ELSEIF  "$(CFG)" == "apcupsd - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 2
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_WINDOWS" /D "_MBCS" /YX /FD /GZ /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I "../compat" /I "../../../include" /I "../../../../depkgs-win32/pthreads" /I "../../../../depkgs-win32/zlib" /I "." /D "_DEBUG" /D "_WINMAIN_" /D "PTW32_BUILD" /D "WIN32" /D "_CONSOLE" /D "_MBCS" /D "HAVE_WIN32" /D "_AFXDLL" /FR /FD /GZ /c
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /win32
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /fo"Debug/winres.res" /d "_DEBUG" /d "_AFXDLL"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LINK32=link.exe
# ADD BASE LINK32 kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib /nologo /subsystem:windows /debug /machine:I386 /pdbtype:sept
# ADD LINK32 wsock32.lib pthreadVCE.lib zlib.lib /nologo /subsystem:windows /pdb:none /debug /machine:I386 /libpath:"../../../../depkgs-win32/pthreads" /libpath:"../../../../depkgs-win32/zlib"

!ENDIF 

# Begin Target

# Name "apcupsd - Win32 Release"
# Name "apcupsd - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=..\..\apcaccess.c
# End Source File
# Begin Source File

SOURCE=..\..\apcaction.c
# End Source File
# Begin Source File

SOURCE=..\..\lib\apcconfig.c
# End Source File
# Begin Source File

SOURCE=..\..\apcdevice.c
# End Source File
# Begin Source File

SOURCE=..\..\lib\apcerror.c
# End Source File
# Begin Source File

SOURCE=..\..\lib\apcevents.c
# End Source File
# Begin Source File

SOURCE=..\..\lib\apcexec.c
# End Source File
# Begin Source File

SOURCE=..\..\lib\apcfile.c
# End Source File
# Begin Source File

SOURCE=..\..\lib\apcipc.c
# End Source File
# Begin Source File

SOURCE=..\..\lib\apclibnis.c
# End Source File
# Begin Source File

SOURCE=..\..\lib\apclist.c
# End Source File
# Begin Source File

SOURCE=..\..\lib\apclock.c
# End Source File
# Begin Source File

SOURCE=..\..\lib\apclog.c
# End Source File
# Begin Source File

SOURCE=..\..\apcnet.c
# End Source File
# Begin Source File

SOURCE=..\..\apcnis.c
# End Source File
# Begin Source File

SOURCE=..\..\apcnisd.c
# End Source File
# Begin Source File

SOURCE=..\..\apcoptd.c
# End Source File
# Begin Source File

SOURCE=..\..\apcreports.c
# End Source File
# Begin Source File

SOURCE=..\..\lib\apcsignal.c
# End Source File
# Begin Source File

SOURCE=..\..\lib\apcstatus.c
# End Source File
# Begin Source File

SOURCE=..\..\apcupsd.c
# End Source File
# Begin Source File

SOURCE=..\apcupsd.rc
# End Source File
# Begin Source File

SOURCE=..\..\lib\apcwinipc.c
# End Source File
# Begin Source File

SOURCE=..\..\lib\asys.c
# End Source File
# Begin Source File

SOURCE=..\compat\compat.cpp
# End Source File
# Begin Source File

SOURCE=..\..\devicedbg.c
# End Source File
# Begin Source File

SOURCE=..\..\drivers\drivers.c
# End Source File
# Begin Source File

SOURCE=..\..\drivers\dumb\dumboper.c
# End Source File
# Begin Source File

SOURCE=..\..\drivers\dumb\dumbsetup.c
# End Source File
# Begin Source File

SOURCE=..\compat\getopt.c
# End Source File
# Begin Source File

SOURCE=..\..\drivers\net\net.c
# End Source File
# Begin Source File

SOURCE=..\compat\print.cpp
# End Source File
# Begin Source File

SOURCE=..\..\drivers\apcsmart\smart.c
# End Source File
# Begin Source File

SOURCE=..\..\drivers\apcsmart\smarteeprom.c
# End Source File
# Begin Source File

SOURCE=..\..\drivers\apcsmart\smartoper.c
# End Source File
# Begin Source File

SOURCE=..\..\drivers\apcsmart\smartsetup.c
# End Source File
# Begin Source File

SOURCE=..\..\drivers\apcsmart\smartsetup2.c
# End Source File
# Begin Source File

SOURCE=..\winabout.cpp
# End Source File
# Begin Source File

SOURCE=..\winevents.cpp
# End Source File
# Begin Source File

SOURCE=..\winmain.cpp
# End Source File
# Begin Source File

SOURCE=..\winprop.cpp
# End Source File
# Begin Source File

SOURCE=..\winres.rc
# PROP Exclude_From_Build 1
# End Source File
# Begin Source File

SOURCE=..\winservice.cpp
# End Source File
# Begin Source File

SOURCE=..\winstat.cpp
# End Source File
# Begin Source File

SOURCE=..\wintray.cpp
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=..\compat\alloca.h
# End Source File
# Begin Source File

SOURCE=..\..\drivers\apcsmart\apcsmart.h
# End Source File
# Begin Source File

SOURCE=..\compat\compat.h
# End Source File
# Begin Source File

SOURCE=..\compat\dirent.h
# End Source File
# Begin Source File

SOURCE=..\..\drivers\dumb\dumb.h
# End Source File
# Begin Source File

SOURCE=..\compat\sys\file.h
# End Source File
# Begin Source File

SOURCE=..\compat\getopt.h
# End Source File
# Begin Source File

SOURCE=..\compat\grp.h
# End Source File
# Begin Source File

SOURCE=..\compat\netinet\in.h
# End Source File
# Begin Source File

SOURCE=..\compat\arpa\inet.h
# End Source File
# Begin Source File

SOURCE=..\compat\sys\ioctl.h
# End Source File
# Begin Source File

SOURCE=..\compat\mswinver.h
# End Source File
# Begin Source File

SOURCE=..\..\drivers\net\net.h
# End Source File
# Begin Source File

SOURCE=..\compat\netdb.h
# End Source File
# Begin Source File

SOURCE=..\compat\pwd.h
# End Source File
# Begin Source File

SOURCE=..\compat\sched.h
# End Source File
# Begin Source File

SOURCE=..\compat\sys\socket.h
# End Source File
# Begin Source File

SOURCE=..\compat\stdint.h
# End Source File
# Begin Source File

SOURCE=..\compat\strings.h
# End Source File
# Begin Source File

SOURCE=..\compat\syslog.h
# End Source File
# Begin Source File

SOURCE=..\compat\sys\time.h
# End Source File
# Begin Source File

SOURCE=..\compat\unistd.h
# End Source File
# Begin Source File

SOURCE=..\compat\sys\wait.h
# End Source File
# Begin Source File

SOURCE=..\winabout.h
# End Source File
# Begin Source File

SOURCE=..\compat\winconfig.h
# End Source File
# Begin Source File

SOURCE=..\winevents.h
# End Source File
# Begin Source File

SOURCE=..\winhdrs.h
# End Source File
# Begin Source File

SOURCE=..\compat\winhost.h
# End Source File
# Begin Source File

SOURCE=..\winprop.h
# End Source File
# Begin Source File

SOURCE=..\winres.h
# End Source File
# Begin Source File

SOURCE=..\winservice.h
# End Source File
# Begin Source File

SOURCE=..\winstat.h
# End Source File
# Begin Source File

SOURCE=..\wintray.h
# End Source File
# Begin Source File

SOURCE=..\winups.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;rgs;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=..\apcupsd.ico
# End Source File
# Begin Source File

SOURCE=..\charging.ico
# End Source File
# Begin Source File

SOURCE=..\onbatt.ico
# End Source File
# Begin Source File

SOURCE=..\online.ico
# End Source File
# End Group
# End Target
# End Project
