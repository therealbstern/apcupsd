@echo off
setlocal

rem
rem  This is the Windows apccontrol file.
rem

set prefix=\apcupsd
set sbindir=%prefix%\bin
set sysconfdir=%prefix%\etc\apcupsd

set APCUPSD=%sbindir%\apcupsd
set SHUTDOWN=%sbindir%\shutdown
set SCRIPTDIR=%sysconfdir%
set POPUP=%sbindir%\popup

rem On WinNT and higher, we can use 'start /b' to launch POPUP
rem in the background. Unfortunately 95, 98, and Me do not have
rem such a capability. Ideally, POPUP should background itself
rem and then we wouldn't have worry about this at all.
VER | FIND " 95 " > NUL
IF NOT ERRORLEVEL 1 GOTO nostart
VER | FIND " 98 " > NUL
IF NOT ERRORLEVEL 1 GOTO nostart
VER | FIND "Millennium" > NUL
IF NOT ERRORLEVEL 1 GOTO nostart
set POPUP=start /b %POPUP%
:nostart

rem
rem This piece is to substitute the default behaviour with your own script,
rem   perl, C program, etc.
rem
rem You can customize any command by creating an executable file (may be a
rem   script or a compiled program) and naming it the same as the %1 parameter
rem   passed by apcupsd to this script. We will accept files with any extension
rem   included in PATHEXT (*.exe, *.bat, *.cmd, etc).
rem
rem After executing your script, apccontrol continues with the default action.
rem   If you do not want apccontrol to continue, exit your script with exit 
rem   code 99. E.g. "exit /b 99".
rem
rem WARNING: please be aware that if you add any commands before the shutdown
rem   in the downshutdown) case and your command errors or stalls, it will
rem   prevent your machine from being shutdown, so test, test, test to
rem   make sure it works correctly.
rem
rem The apccontrol.bat file will be replaced every time apcupsd is installed,
rem   so do NOT make event modifications in this file. Instead, override the
rem   event actions using event scripts as described above.
rem

rem Use CALL here because event script might be a batch file itself
CALL %SCRIPTDIR%\%1 2> NUL

rem This is retarded. "IF ERRORLEVEL 99" means greater-than-or-
rem equal-to 99, so we have to synthesize an == using two IFs. 
rem Ahh, the glory of Windows batch programming. At least they 
rem gave us a NOT op.
IF NOT ERRORLEVEL 99 GOTO :events
IF NOT ERRORLEVEL 100 GOTO :done

:events

rem
rem powerout, onbattery, offbattery, mainsback events occur
rem   in that order.
rem

IF "%1" == "commfailure"   GOTO :commfailure
IF "%1" == "commok"        GOTO :commok
IF "%1" == "powerout"      GOTO :powerout
IF "%1" == "onbattery"     GOTO :onbattery
IF "%1" == "offbattery"    GOTO :offbattery
IF "%1" == "mainsback"     GOTO :mainsback
IF "%1" == "failing"       GOTO :failing
IF "%1" == "timeout"       GOTO :timeout
IF "%1" == "loadlimit"     GOTO :loadlimit
IF "%1" == "runlimit"      GOTO :runlimit
IF "%1" == "doshutdown"    GOTO :doshutdown
IF "%1" == "mainsback"     GOTO :mainsback
IF "%1" == "annoyme"       GOTO :annoyme
IF "%1" == "emergency"     GOTO :emergency
IF "%1" == "changeme"      GOTO :changeme
IF "%1" == "remotedown"    GOTO :remotedown
IF "%1" == "restartme"     GOTO :restartme
IF "%1" == "startselftest" GOTO :startselftest
IF "%1" == "endselftest"   GOTO :endselftest
IF "%1" == "battdetach"    GOTO :battdetach
IF "%1" == "battattach"    GOTO :battattach

echo Unknown command '%1'
echo.
echo Usage: %0 command
echo.
echo Warning: this script is intended to be launched by
echo apcupsd and should never be launched by users.
GOTO :done

:commfailure
   %POPUP% "Communications with UPS lost."
   GOTO :done

:commok
   %POPUP% "Communciations with UPS restored."
   GOTO :done

:powerout
   GOTO :done

:onbattery
   %POPUP% "Power failure. Running on UPS batteries."
   GOTO :done

:offbattery
   %POPUP% "Power has returned. No longer running on UPS batteries."
   GOTO :done

:mainsback
   GOTO :done

:failing
   %POPUP% "UPS battery power exhaused. Doing shutdown."
   GOTO :done

:timeout
   %POPUP% "UPS battery runtime limit exceeded. Doing shutdown."
   GOTO :done

:loadlimit
   %POPUP% "UPS battery discharge limit reached. Doing shutdown."
   GOTO :done

:runlimit
   %POPUP% "UPS battery runtime percent reached. Doing shutdown."
   GOTO :done

:doshutdown
rem
rem  If you want to try to power down your UPS, uncomment
rem    out the following lines, but be warned that if the
rem    following shutdown -h now doesn't work, you may find
rem    the power being shut off to a running computer :-(
rem  Also note, we do this in the doshutdown case, because
rem    there is no way to get control when the machine is
rem    shutdown to call this script with --killpower. As
rem    a consequence, we do both killpower and shutdown
rem    here.
rem  Note that Win32 lacks a portable way to delay for a
rem    given time, so we use the trick of pinging a
rem    non-existent IP address with a given timeout.
rem
rem   %APCUPSD% /kill
rem   ping -n 1 -w 5000 10.255.255.254 > NUL
rem   %POPUP% "Doing %APCUPSD% --killpower"
rem   %APCUPSD% --killpower
rem   ping -n 1 -w 20000 10.255.255.254 > NUL
rem
   %POPUP% "Doing %SHUTDOWN% -h now"
   %SHUTDOWN% -h now
   GOTO :done

:annoyme
   %POPUP% "Power problems: please logoff."
   GOTO :done

:emergency
   %SHUTDOWN% -h now
   GOTO :done

:changeme
   %POPUP% "Emergency! UPS batteries have failed: Change them NOW"
   GOTO :done

:remotedown
   %SHUTDOWN% -h now
   GOTO :done

:restartme
   GOTO :done

:startselftest
   %POPUP% "Self-test starting"
   GOTO :done

:endselftest
   %POPUP% "Self-test completed"
   GOTO :done

:battdetach
   %POPUP% "Battery disconnected"
   GOTO :done

:battattach
   %POPUP% "Battery reattached"
   GOTO :done

:done
rem That's all, folks
