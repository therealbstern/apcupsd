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
rem   perl, or C program.
rem
rem You can customize every single command creating an executable file (may be a
rem   script or a compiled program) and calling it the same as the %1 parameter
rem   passed by apcupsd to this script.
rem
rem After executing your script, apccontrol continues with the default action.
rem   If you do not want apccontrol to continue, exit your script with exit 
rem   code 99. E.g. "exit 99".
rem
rem WARNING: please be aware that if you add any commands before the shutdown
rem   in the downshutdown) case and your command errors or stalls, it will
rem   prevent your machine from being shutdown, so test, test, test to
rem   make sure it works correctly.
rem
rem The apccontrol file with no extension will be rebuilt (overwritten)
rem   every time that "make" is invoked if you are working with the
rem   source files. Thus if you build from a source distribution, we
rem   recommend you make your changes to the apccontrol.in file.
rem
IF EXIST %SCRIPTDIR%\%1% (
   rem Use CALL here because event script might be a batch file itself
   CALL %SCRIPTDIR%\%1%
   
   rem This is retarded. "IF ERRORLEVEL 99" means >= 99, so
   rem we have to synthesize an == using two IFs. Ahh, the glory
   rem of Win32 batch programming. At least they gave us a NOT op.
   IF ERRORLEVEL 99 (
      IF NOT ERRORLEVEL 100 (
         rem exit code 99 means he does not want us to do default action
         exit
      )
   )
)

rem
rem powerout, onbattery, offbattery, mainsback events occur
rem   in that order.
rem

IF "%1%" == "commfailure" (
   %POPUP% "apccontrol: Communications with UPS lost."
) ELSE IF "%1%" == "commok" (
   %POPUP% "apccontrol: Communciations with UPS restored."
) ELSE IF "%1%" == "powerout" (
   rem Normally we'd POPUP here, but that's a bit annoying on Windows,
   rem so don't popup until onbattery comes.
) ELSE IF "%1%" == "onbattery" (
   %POPUP% "apccontrol: Power failure. Running on UPS batteries."
) ELSE IF "%1%" == "offbattery" (
   %POPUP% "apccontrol: Power has returned. No longer running on UPS batteries."
) ELSE IF "%1%" == "mainsback" (
   rem ...nothing...
) ELSE IF "%1%" == "failing" (
   %POPUP% "apccontrol: UPS battery power exhaused. Doing shutdown."
) ELSE IF "%1%" == "timeout" (
   %POPUP% "apccontrol: UPS battery runtime limit exceeded. Doing shutdown."
) ELSE IF "%1%" == "loadlimit" (
   %POPUP% "apccontrol: UPS battery discharge limit reached. Doing shutdown."
) ELSE IF "%1%" == "runlimit" (
   %POPUP% "apccontrol: UPS battery runtime percent reached. Doing shutdown."
) ELSE IF "%1%" == "doshutdown" (
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
rem   %POPUP% "apccontrol: Doing %APCUPSD% --killpower"
rem   %APCUPSD% --killpower
rem   ping -n 1 -w 20000 10.255.255.254 > NUL
rem
   %POPUP% "apccontrol: Doing %SHUTDOWN% -h now"
   %SHUTDOWN% -h now
) ELSE IF "%1%" == "mainsback" (
   %POPUP% "apccontrol: Power has returned..."
) ELSE IF "%1%" == "annoyme" (
   %POPUP% "apccontrol: Power problems: please logoff."
) ELSE IF "%1%" == "emergency" (
   %POPUP% "apccontrol: Doing %SHUTDOWN% -h now"
   %SHUTDOWN% -h now
) ELSE IF "%1%" == "changeme" (
   %POPUP% "apccontrol: Emergency! UPS batteries have failed: Change them NOW"
) ELSE IF "%1%" == "remotedown" (
   %POPUP% "apccontrol: Doing %SHUTDOWN% -h now"
   %SHUTDOWN% -h now
) ELSE IF "%1%" == "restartme" (
   %POPUP% "apccontrol: restarting apcupsd would have been called."
) ELSE IF "%1%" == "startselftest" (
   CALL %POPUP% "apccontrol: startselftest."
) ELSE IF "%1%" == "endselftest" (
   %POPUP% "apccontrol: endselftest."
) ELSE IF "%1%" == "battdetach" (
   rem ...nothing...
) ELSE IF "%1%" == "battattach" (
   rem ...nothing...
) ELSE (
   echo Unknown command '%1%'
   echo.
   echo Usage: %0% ^<command^>
   echo.
   echo Warning: this script is intended to be launched by
   echo apcupsd and should never be launched by users.
)
