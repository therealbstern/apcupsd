@rem This is a batch file which allows you to run the html2latex 
@rem Perl script on a Windows machine.
@rem
@rem NOTE! The variable perlhome must be set to the directory 
@rem containing perl.exe.  
@rem If you do not have "Perl for Win32"
@rem installed on your computer, you can get it from 
@rem the ActiveState Tool Corp. at http://www.ActiveState.com/pw32/


@echo off
set perlhome=C:\Perl\bin\
if not exist %perlhome%perl.exe goto perlnotfound
%perlhome%perl.exe html2latex %1 %2 %3 %4 %5 %6 %7 %8 %9 
echo Finished execution of html2latex.
goto end

:perlnotfound
echo *** HTML2LAT.BAT ERROR ***
echo Could not find perl.exe in the directory %perlhome%.
echo Put the correct path of Perl in html2lat.bat.

echo The output file was NOT created by html2latex.


:end
@rem Skipped over error messages if everything is ok.
