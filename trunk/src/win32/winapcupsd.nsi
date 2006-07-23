; winapcupsd.nsi
;
; Adapted by Kern Sibbald for apcupsd from Bacula code
; Further modified by Adam Kropelin
;
; Command line options:

!define PRODUCT "Apcupsd"

;			    
; Include the Modern UI
;
!include "MUI.nsh"
;!include "params.nsh"
!include "util.nsh"

;
; Use Logic Library to improve readability
;
!include "LogicLib.nsh"

;
; Basics
;
  Name "Apcupsd"
  OutFile "winapcupsd-${VERSION}.exe"
  SetCompressor lzma
  InstallDir "c:\apcupsd"

;
; Page customization
;
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_TEXT "Start Apcupsd (Be sure to edit apcupsd.conf first!)"
!define MUI_FINISHPAGE_RUN_FUNCTION "StartApcupsd"
!define MUI_FINISHPAGE_RUN_NOTCHECKED
!define MUI_FINISHPAGE_SHOWREADME
!define MUI_FINISHPAGE_SHOWREADME_TEXT "View the ReleaseNotes"
!define MUI_FINISHPAGE_SHOWREADME_FUNCTION "ShowReadme"
!define MUI_FINISHPAGE_LINK "Visit Apcupsd Website"
!define MUI_FINISHPAGE_LINK_LOCATION "http://www.apcupsd.com"

;
; Pull in pages
;
 !insertmacro MUI_PAGE_WELCOME
 !insertmacro MUI_PAGE_LICENSE "..\..\COPYING"
 !insertmacro MUI_PAGE_COMPONENTS
 !insertmacro MUI_PAGE_DIRECTORY
 !insertmacro MUI_PAGE_INSTFILES
 !insertmacro MUI_PAGE_FINISH

 !insertmacro MUI_UNPAGE_WELCOME
 !insertmacro MUI_UNPAGE_CONFIRM
 !insertmacro MUI_UNPAGE_INSTFILES
 !insertmacro MUI_UNPAGE_FINISH
 
 !define      MUI_ABORTWARNING

 !insertmacro MUI_LANGUAGE "English"



DirText "Setup will install Apcupsd ${VERSION} to the directory \
         specified below."

Function StartApcupsd
  ExecShell "" "$SMPROGRAMS\Apcupsd\Start Apcupsd.lnk" "" SW_HIDE
FunctionEnd

Function ShowReadme
  Exec 'write "$INSTDIR\ReleaseNotes"'
FunctionEnd

Function .onInit
  ;
  ; Default INSTDIR to %SystemDrive%\apcupsd
  ;
  ReadEnvStr $0 SystemDrive
  ${If} $0 == ''
     StrCpy $0 'c:'
  ${EndIf}
  StrCpy $INSTDIR $0\apcupsd
FunctionEnd

Section "Apcupsd Service" SecService
  ; Check for existing installation
  StrCpy $7 0
  ${If} ${FileExists} "$INSTDIR\etc\apcupsd\apcupsd.conf"
    StrCpy $7 1
    ; Shutdown any apcupsd that could be running
    ExecWait '"$INSTDIR\bin\apcupsd.exe" /kill'
    ; give it some time to shutdown
    Sleep 3000
  ${EndIf}

  ; Set output path to the installation directory.
  SetOutPath "$INSTDIR\bin"
  CreateDirectory "$INSTDIR"
  CreateDirectory "$INSTDIR\bin"
  CreateDirectory "$INSTDIR\driver"
  CreateDirectory "$INSTDIR\etc"
  CreateDirectory "$INSTDIR\etc\apcupsd"
  CreateDirectory "$INSTDIR\examples"
  CreateDirectory "c:\tmp"

  ; Put files there
  ;
  ; NOTE: If you add new files here, be sure to remove them
  ;       in the uninstaller!
  ;
  
  File mingwm10.dll
  File pthreadGCE.dll
  File ${DEPKGS}\libusb-win32\libusb0.dll
  File apcupsd.exe
;  File smtp.exe
  File apcaccess.exe
;  File apctest.exe
  File popup.exe 
  File shutdown.exe
;  File email.exe

  SetOutPath "$INSTDIR\driver"
  File ..\..\platforms\mingw\apcupsd.inf
  File ..\..\platforms\mingw\apcupsd.cat
  File ${DEPKGS}\libusb-win32\libusb0.sys
  File ${DEPKGS}\libusb-win32\libusb0.dll
  File ..\..\platforms\mingw\install.txt

  SetOutPath "$INSTDIR\examples"
  File ..\..\examples\*

  SetOutPath "$INSTDIR"
;  File ..\..\platforms\cygwin\README.txt
  File ..\..\COPYING
  File ..\..\ChangeLog
  File ..\..\ReleaseNotes

  SetOutPath "$INSTDIR\etc\apcupsd"
  File ..\..\platforms\mingw\apccontrol.bat

  ; Install apcupsd.conf as apcupsd.conf.new if apcupsd.conf already exists
  ${If} ${FileExists} "$INSTDIR\etc\apcupsd\apcupsd.conf"
    File /oname=apcupsd.conf.new ..\..\platforms\etc\apcupsd.conf
  ${Else}
    File ..\..\platforms\etc\apcupsd.conf
  ${EndIf}

  ; If already installed as service skip the option
  ReadRegDWORD $9 HKLM "Software\Apcupsd" "InstalledService"
  ${Unless} $9 == 1
    ; Install as service?
    ${If} ${Cmd} 'MessageBox MB_YESNO|MB_ICONQUESTION "Do you want to install Apcupsd as a service$\n(automatically starts with your PC)?" IDYES'
      ExecWait '"$INSTDIR\bin\apcupsd.exe" /install'
      StrCpy $9 "1"
      WriteRegDWORD HKLM "Software\Apcupsd" "InstalledService" "1"
    ${EndIf}
  ${EndUnless}

  ; Create Start Menu Directory
  SetShellVarContext all
  CreateDirectory "$SMPROGRAMS\Apcupsd"

  ; Create a start menu link to start apcupsd (possibly as a service)
  ${If} $9 != 1
    ; Not installed as a service
    CreateShortCut "$SMPROGRAMS\Apcupsd\Start Apcupsd.lnk" "$INSTDIR\bin\apcupsd.exe"
    CreateShortCut "$SMPROGRAMS\Apcupsd\Stop Apcupsd.lnk" "$INSTDIR\bin\apcupsd.exe" "/kill"
  ${Else}
    Call IsNt
    Pop $R0
    ${If} $R0 == false
      ; Installed as a service, but not on NT
      CreateShortCut "$SMPROGRAMS\Apcupsd\Start Apcupsd.lnk" "$INSTDIR\bin\apcupsd.exe" "/service"
      CreateShortCut "$SMPROGRAMS\Apcupsd\Stop Apcupsd.lnk" "$INSTDIR\bin\apcupsd.exe" "/kill"
    ${Else}
      ; Installed as a service and we're on NT
      CreateShortCut "$SMPROGRAMS\Apcupsd\Start Apcupsd.lnk" "$SYSDIR\net.exe" "start apcupsd" "$INSTDIR\bin\apcupsd.exe"
      CreateShortCut "$SMPROGRAMS\Apcupsd\Stop Apcupsd.lnk" "$SYSDIR\net.exe" "stop apcupsd" "$INSTDIR\bin\apcupsd.exe"
    ${EndIf}
  ${EndIf}

  ; Write the uninstall keys for Windows & create Start Menu entry
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Apcupsd" "DisplayName" "Apcupsd"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Apcupsd" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  CreateShortCut "$SMPROGRAMS\Apcupsd\Uninstall Apcupsd.lnk" "$INSTDIR\Uninstall.exe"

  ${If} $7 != 1
    MessageBox MB_OK "Please edit the client configuration file $INSTDIR\etc\apcupsd\apcupsd.conf \
                      to fit your installation. When you click the OK button Wordpad will open to \
                      allow you to do this. Be sure to save your changes before closing Wordpad."
    Exec 'write "$INSTDIR\etc\apcupsd\apcupsd.conf"'  ; spawn wordpad with the file to be edited
  ${EndUnless}
SectionEnd

Section "USB Driver" SecUsbDrv
  Call IsNt
  Pop $R0
  ${If} $R0 != false
    SetOutPath "$WINDIR\system32"
    File ${DEPKGS}\libusb-win32\libusb0.dll
    ExecWait 'rundll32 libusb0.dll,usb_install_driver_np_rundll $INSTDIR\driver\apcupsd.inf'
  ${Else}
    MessageBox MB_OK "The USB driver cannot be automatically installed on Win98 or WinMe. \
                      Please see $INSTDIR\driver\install.txt for instructions on installing \
                      the driver by hand."
  ${EndIf}
SectionEnd

Section "Documentation" SecDoc
  SetOutPath "$INSTDIR\doc"
  CreateDirectory "$INSTDIR\doc"
  File ..\..\doc\latex\manual.html
  File ..\..\doc\latex\*.png
  ; Create Start Menu entry
  SetShellVarContext all
  CreateShortCut "$SMPROGRAMS\Apcupsd\Manual.lnk" "$INSTDIR\doc\manual.html"
SectionEnd


;
; Extra Page descriptions
;

LangString DESC_SecService ${LANG_ENGLISH} "Install Apcupsd on this system."
LangString DESC_SecUsbDrv ${LANG_ENGLISH} "Install USB driver. Required if you have a USB UPS."
LangString DESC_SecDoc ${LANG_ENGLISH} "Install Documentation on this system."

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SecService} $(DESC_SecService)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecUsbDrv} $(DESC_SecUsbDrv)
  !insertmacro MUI_DESCRIPTION_TEXT ${SecDoc} $(DESC_SecDoc)
!insertmacro MUI_FUNCTION_DESCRIPTION_END



; Uninstall section

UninstallText "This will uninstall Apcupsd. Hit next to continue."

Section "Uninstall"

  ; Shutdown any apcupsd that could be running
  ExecWait '"$INSTDIR\bin\apcupsd.exe" /kill'

  ReadRegDWORD $9 HKLM "Software\Apcupsd" "InstalledService"
  ${If} $9 == 1
    ; Remove apcuspd service
    ExecWait '"$INSTDIR\bin\apcupsd.exe" /remove'
  ${EndIf}

  ; remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Apcupsd"
  DeleteRegKey HKLM "Software\Apcupsd"

  ; remove start menu items
  SetShellVarContext all
  Delete /REBOOTOK "$SMPROGRAMS\Apcupsd\*"
  RMDir /REBOOTOK "$SMPROGRAMS\Apcupsd"

  ; remove files and uninstaller (preserving config for now)
  Delete /REBOOTOK "$INSTDIR\bin\mingwm10.dll"
  Delete /REBOOTOK "$INSTDIR\bin\pthreadGCE.dll"
  Delete /REBOOTOK "$INSTDIR\bin\libusb0.dll"
  Delete /REBOOTOK "$INSTDIR\bin\apcupsd.exe"
  Delete /REBOOTOK "$INSTDIR\bin\smtp.exe"
  Delete /REBOOTOK "$INSTDIR\bin\apcaccess.exe"
  Delete /REBOOTOK "$INSTDIR\bin\apctest.exe"
  Delete /REBOOTOK "$INSTDIR\bin\popup.exe"
  Delete /REBOOTOK "$INSTDIR\bin\shutdown.exe"
  Delete /REBOOTOK "$INSTDIR\bin\email.exe"
  Delete /REBOOTOK "$INSTDIR\driver\libusb0.dll"
  Delete /REBOOTOK "$INSTDIR\driver\libusb0.sys"
  Delete /REBOOTOK "$INSTDIR\driver\apcupsd.inf"
  Delete /REBOOTOK "$INSTDIR\driver\apcupsd.cat"
  Delete /REBOOTOK "$INSTDIR\driver\install.txt"
  Delete /REBOOTOK "$INSTDIR\examples\*"
  Delete /REBOOTOK "$INSTDIR\README.txt"
  Delete /REBOOTOK "$INSTDIR\COPYING"
  Delete /REBOOTOK "$INSTDIR\ChangeLog"
  Delete /REBOOTOK "$INSTDIR\ReleaseNotes"
  Delete /REBOOTOK "$INSTDIR\Uninstall.exe"
  Delete /REBOOTOK "$INSTDIR\etc\apcupsd\apccontrol.bat"
  Delete /REBOOTOK "$INSTDIR\etc\apcupsd\apcupsd.conf.new"
  Delete /REBOOTOK "$INSTDIR\doc\*"

  ; Delete conf if user approves
  ${If} ${Cmd} 'MessageBox MB_YESNO|MB_ICONQUESTION "Would you like to delete the current configuration and events files?" IDYES'
    Delete /REBOOTOK "$INSTDIR\etc\apcupsd\apcupsd.conf"
    Delete /REBOOTOK "$INSTDIR\etc\apcupsd\apcupsd.events"
  ${EndIf}

  ; remove directories used
  RMDir "$INSTDIR\bin"
  RMDir "$INSTDIR\driver"
  RMDir "$INSTDIR\etc\apcupsd"
  RMDir "$INSTDIR\etc"
  RMDir "$INSTDIR\doc"
  RMDir "$INSTDIR\examples"
  RMDir "$INSTDIR"
  RMDir "C:\tmp"
  
SectionEnd

; eof
