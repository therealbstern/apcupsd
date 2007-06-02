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

#define BALLOON_ENBALE_REG_PATH \
   "Software\\Microsoft\\Windows\\CurrentVersion\\Explorer\\Advanced"

#define BALLOON_ENABLE_REG_KEY "EnableBalloonTips"


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

   // Assume we're doing a balloon tip
   int balloon = 1;

   // Locate Apcupsd tray icon window.
   // If we can't find it, we can't do a balloon tip
   HWND tray = FindWindowEx(
      NULL, NULL, APCTRAY_WINDOW_CLASS, APCTRAY_WINDOW_NAME);
   if (!tray)
      balloon = 0;

   // Only Win2k and higher can use balloon notification
   OSVERSIONINFO vers;
   vers.dwOSVersionInfoSize = sizeof(vers);
   if (GetVersionEx(&vers) == 0 || vers.dwMajorVersion < 5)
      balloon = 0;

   // Finally, check for a group policy on balloon tips
   HKEY hkey;
   DWORD type, val, size;
   LONG result;

   // First in HKEY_CURRENT_USER
   type = REG_DWORD;
   size = sizeof(val);
   result = RegOpenKeyEx(
      HKEY_CURRENT_USER, BALLOON_ENBALE_REG_PATH, 0, KEY_QUERY_VALUE, &hkey);
   if (result == ERROR_SUCCESS) {
      result = RegQueryValueEx(
         hkey, BALLOON_ENABLE_REG_KEY, NULL, &type, (BYTE*)&val, &size);
      if (result == ERROR_SUCCESS)
         balloon = val;
      RegCloseKey(hkey);
   }

   // And again in HKEY_LOCAL_MACHINE
   type = REG_DWORD;
   size = sizeof(val);
   result = RegOpenKeyEx(
      HKEY_LOCAL_MACHINE, BALLOON_ENBALE_REG_PATH, 0, KEY_QUERY_VALUE, &hkey);
   if (result == ERROR_SUCCESS) {
      result = RegQueryValueEx(
         hkey, BALLOON_ENABLE_REG_KEY, NULL, &type, (BYTE*)&val, &size);
      if (result == ERROR_SUCCESS)
         balloon = val;
      RegCloseKey(hkey);
   }

   // Now display the popup
   if (balloon) {
      NOTIFYICONDATA nid;

      // Create the tray icon message
      nid.hWnd = tray;
      nid.cbSize = sizeof(nid);
      nid.uID = IDI_APCUPSD;

      // Setup balloon tip with detailed status info
      snprintf(nid.szInfo, sizeof(nid.szInfo), "%s", msg);
      snprintf(nid.szInfoTitle, sizeof(nid.szInfoTitle), "%s", "Apcupsd Notice");
      nid.uFlags = NIF_INFO;
      nid.uTimeout = 10000;
      nid.dwInfoFlags = NIIF_INFO;

      // Send the message
      Shell_NotifyIcon(NIM_MODIFY, &nid);

      // Notify apcupsd tray that a balloon was shown and
      // ask for a 10000 msec timeout;
      SendNotifyMessage(tray, WM_BALLOONSHOW, 10000, 0);
   } else {
      MessageBox(NULL, msg, "Apcupsd message", MB_OK);
   }

   return 0;
}
