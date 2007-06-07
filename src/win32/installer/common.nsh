;
; Shutdown a given app
; Caller provides window class name and timeout in msec
;
; $0 - Class
; $1 - Timeout
; $2 - Hwnd (internal only)
Function ${UN}ShutdownApp
  Exch $1
  Exch 1
  Exch $0
  Push $2

  FindWindow $2 $0
  ${If} $2 != 0
    SendMessage $2 ${WM_CLOSE} 0 0
    ${While} $1 > 0
      Sleep 100
      FindWindow $2 $0
      ${If} $2 == 0
        ${Break}
      ${EndIf}
      SendMessage $2 ${WM_CLOSE} 0 0
      IntOp $1 $1 - 100
    ${EndWhile}
    Sleep 2000
  ${EndIf}

  Pop $2
  Pop $1
  Pop $0
FunctionEnd

!macro ${UN}_shutdownAppConstructor CLASS TIMEOUTMSEC
  Push "${CLASS}"
  Push "${TIMEOUTMSEC}"
  Call ${UN}ShutdownApp
!macroend

!ifdef ShutdownApp
!undef ShutdownApp
!endif
!define ShutdownApp '!insertmacro "${UN}_shutdownAppConstructor"'
