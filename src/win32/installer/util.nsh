 Function IsNT
   Push $R0
   Push $R1
   ReadRegStr $R0 HKLM "SOFTWARE\Microsoft\Windows NT\CurrentVersion" CurrentVersion
   StrCmp $R0 "" 0 lbl_winnt

   StrCpy $R0 'false'
   Goto lbl_done

   lbl_winnt:
    Strcpy $R0 'true'

   lbl_done:
   Pop $R1
   Exch $R0
 FunctionEnd
 
 ;
 ; Based on Yazno's function, http://yazno.tripod.com/powerpimpit/
 ; Updated by Joost Verburg
 ;
 ; Returns on top of stack
 ;
 ; Windows Version (95, 98, ME, NT x.x, 2000, XP, 2003)
 ; or
 ; '' (Unknown Windows Version)
 ;
 ; Usage:
 ;   Call GetWindowsVersion
 ;   Pop $R0
 ;   ; at this point $R0 is "NT 4.0" or whatnot
 
 Function GetWindowsVersion
 
   Push $R0
   Push $R1
 
   ClearErrors
 
   ReadRegStr $R0 HKLM \
   "SOFTWARE\Microsoft\Windows NT\CurrentVersion" CurrentVersion

   IfErrors 0 lbl_winnt
   
   ; we are not NT
   ReadRegStr $R0 HKLM \
   "SOFTWARE\Microsoft\Windows\CurrentVersion" VersionNumber
 
   StrCpy $R1 $R0 1
   StrCmp $R1 '4' 0 lbl_error
 
   StrCpy $R1 $R0 3
 
   StrCmp $R1 '4.0' lbl_win32_95
   StrCmp $R1 '4.9' lbl_win32_ME lbl_win32_98
 
   lbl_win32_95:
     StrCpy $R0 '95'
   Goto lbl_done
 
   lbl_win32_98:
     StrCpy $R0 '98'
   Goto lbl_done
 
   lbl_win32_ME:
     StrCpy $R0 'ME'
   Goto lbl_done
 
   lbl_winnt:
 
   StrCpy $R1 $R0 1
 
   StrCmp $R1 '3' lbl_winnt_x
   StrCmp $R1 '4' lbl_winnt_x
 
   StrCpy $R1 $R0 3
 
   StrCmp $R1 '5.0' lbl_winnt_2000
   StrCmp $R1 '5.1' lbl_winnt_XP
   StrCmp $R1 '5.2' lbl_winnt_2003 lbl_error
 
   lbl_winnt_x:
     StrCpy $R0 "NT $R0" 6
   Goto lbl_done
 
   lbl_winnt_2000:
     Strcpy $R0 '2000'
   Goto lbl_done
 
   lbl_winnt_XP:
     Strcpy $R0 'XP'
   Goto lbl_done
 
   lbl_winnt_2003:
     Strcpy $R0 '2003'
   Goto lbl_done
 
   lbl_error:
     Strcpy $R0 ''
   lbl_done:
 
   Pop $R1
   Exch $R0
 
 FunctionEnd


; StrReplace
; Replaces all ocurrences of a given needle within a haystack with another string
; Written by dandaman32
 
Var STR_REPLACE_VAR_0
Var STR_REPLACE_VAR_1
Var STR_REPLACE_VAR_2
Var STR_REPLACE_VAR_3
Var STR_REPLACE_VAR_4
Var STR_REPLACE_VAR_5
Var STR_REPLACE_VAR_6
Var STR_REPLACE_VAR_7
Var STR_REPLACE_VAR_8
 
Function StrReplace
  Exch $STR_REPLACE_VAR_2
  Exch 1
  Exch $STR_REPLACE_VAR_1
  Exch 2
  Exch $STR_REPLACE_VAR_0
    StrCpy $STR_REPLACE_VAR_3 -1
    StrLen $STR_REPLACE_VAR_4 $STR_REPLACE_VAR_1
    StrLen $STR_REPLACE_VAR_6 $STR_REPLACE_VAR_0
    loop:
      IntOp $STR_REPLACE_VAR_3 $STR_REPLACE_VAR_3 + 1
      StrCpy $STR_REPLACE_VAR_5 $STR_REPLACE_VAR_0 $STR_REPLACE_VAR_4 $STR_REPLACE_VAR_3
      StrCmp $STR_REPLACE_VAR_5 $STR_REPLACE_VAR_1 found
      StrCmp $STR_REPLACE_VAR_3 $STR_REPLACE_VAR_6 done
      Goto loop
    found:
      StrCpy $STR_REPLACE_VAR_5 $STR_REPLACE_VAR_0 $STR_REPLACE_VAR_3
      IntOp $STR_REPLACE_VAR_8 $STR_REPLACE_VAR_3 + $STR_REPLACE_VAR_4
      StrCpy $STR_REPLACE_VAR_7 $STR_REPLACE_VAR_0 "" $STR_REPLACE_VAR_8
      StrCpy $STR_REPLACE_VAR_0 $STR_REPLACE_VAR_5$STR_REPLACE_VAR_2$STR_REPLACE_VAR_7
      StrLen $STR_REPLACE_VAR_6 $STR_REPLACE_VAR_0
      Goto loop
    done:
  Pop $STR_REPLACE_VAR_1 ; Prevent "invalid opcode" errors and keep the
  Pop $STR_REPLACE_VAR_1 ; stack as it was before the function was called
  Exch $STR_REPLACE_VAR_0
FunctionEnd
 
!macro _strReplaceConstructor OUT NEEDLE NEEDLE2 HAYSTACK
  Push "${HAYSTACK}"
  Push "${NEEDLE}"
  Push "${NEEDLE2}"
  Call StrReplace
  Pop "${OUT}"
!macroend
 
!define StrReplace '!insertmacro "_strReplaceConstructor"'