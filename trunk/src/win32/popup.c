/* 
 * Dumb Windows program to put up a message box
 * containing the command line.  Any leading and
 * trailing quotes are stripped.
 * 
 *  Kern E. Sibbald
 *   July MM  
 */
#include "windows.h"
#include "winres.h"
#include "winups.h"
#include <stdio.h>

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
		   PSTR szCmdLine, int iCmdShow)
{
   int len = strlen(szCmdLine);
   char *msg, *wordPtr;

   // Funny things happen with the command line if the
   // execution comes from c:/Program Files/apcupsd/apcupsd.exe
   // We get a command line like: Files/apcupsd/apcupsd.exe" options
   // I.e. someone stops scanning command line on a space, not
   // realizing that the filename is quoted!!!!!!!!!!
   // So if first character is not a double quote and
   // the last character before first space is a double
   // quote, we throw away the junk.
   wordPtr = szCmdLine;
   while (*wordPtr && *wordPtr != ' ')
      wordPtr++;
   if (wordPtr > szCmdLine)	 // backup to char before space
      wordPtr--;
   // if first character is not a quote and last is, junk it
   if (*szCmdLine != '"' && *wordPtr == '"') {
      wordPtr++;
      while (*wordPtr && *wordPtr == ' ')
	 wordPtr++;		 /* strip leading spaces */
      szCmdLine = wordPtr;
      len = strlen(szCmdLine);
   }

   msg = szCmdLine;
   if (*szCmdLine == '"' && len > 0 && szCmdLine[len-1] == '"') {
      msg = szCmdLine + 1;
      szCmdLine[len-1] = 0;
   }

   // Locate Apcupsd tray icon window
   HWND tray = FindWindowEx(NULL, NULL, "Apcupsd Tray Icon", NULL);
   
   // Win2k and higher use balloon notification,
   // others have to use MessageBox
   OSVERSIONINFO vers;
   vers.dwOSVersionInfoSize = sizeof(vers);
   if (tray == NULL || GetVersionEx(&vers) == 0 || vers.dwMajorVersion < 5) {
      MessageBox(NULL, msg, "Apcupsd message", MB_OK);
   } else {
      NOTIFYICONDATA nid;

      // Create the tray icon message
      nid.hWnd = tray;
      nid.cbSize = sizeof(nid);
      nid.uID = IDI_APCUPSD;        // never changes after construction

      // Setup balloon tip with detailed status info
      snprintf(nid.szInfo, sizeof(nid.szInfo), "%s", msg);
      snprintf(nid.szInfoTitle, sizeof(nid.szInfoTitle), "%s", "Apcupsd Notice");
      nid.uFlags = NIF_INFO;
      nid.uTimeout = 5000;
      nid.dwInfoFlags = NIIF_INFO;

      // Send the message
      Shell_NotifyIcon(NIM_MODIFY, &nid);

      // Notify apcupsd tray that a balloon was shown and
      // ask for a 5000 msec timeout;
      SendNotifyMessage(tray, WM_BALLOONSHOW, 10000, 0);
   }

   return 0;
}
