#include "winapi.h"
#include <unistd.h>
#include <windows.h>
#include <lmcons.h>
#include <ctype.h>
#include <signal.h>
#include <errno.h>

#include "apc.h"
#include "wintray.h"
#include "winres.h"
#include "winups.h"
#include "statmgr.h"


#define CMDOPT_PORT     "/port"
#define CMDOPT_HOST     "/host"
#define CMDOPT_REFRESH  "/refresh"
#define CMDOPT_INSTALL  "/install"
#define CMDOPT_REMOVE   "/remove"
#define CMDOPT_KILL     "/kill"

#define USAGE_TEXT   "[" CMDOPT_HOST    " <hostname>] " \
                     "[" CMDOPT_PORT    " <port>] "     \
                     "[" CMDOPT_REFRESH " <sec>] "      \
                     "[" CMDOPT_INSTALL "] "            \
                     "[" CMDOPT_REMOVE  "] "            \
                     "[" CMDOPT_KILL    "]"

#define DEFAULT_HOST    "127.0.0.1"
#define DEFAULT_PORT    3551
#define DEFAULT_REFRESH 1

int Install(char *host, unsigned short port, int refresh)
{
   // Get the filename of this executable
   char path[1024];
   GetModuleFileName(NULL, path, sizeof(path));

   // Build option flags only where using non-defaults
   char hoststr[256] = "";
   char portstr[32] = "";
   char refstr[32] = "";
   if (host)
      asnprintf(hoststr, sizeof(hoststr), "%s \"%s\"", CMDOPT_HOST, host);
   if (port)
      asnprintf(portstr, sizeof(portstr), "%s %u", CMDOPT_PORT, port);
   if (refresh > 0)
      asnprintf(refstr, sizeof(refstr), "%s %u", CMDOPT_REFRESH, refresh);

   // Append configuration flags
   char cmd[1024];
   asnprintf(cmd, sizeof(cmd), "\"%s\" %s %s %s",
      path, hoststr, portstr, refstr);

   // Open registry key for auto-run programs
   HKEY runkey;
   if (RegCreateKey(HKEY_LOCAL_MACHINE, 
                    "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                    &runkey) != ERROR_SUCCESS) {
      MessageBox(NULL, "The System Registry could not be updated.\n"
                        "Apctray was not installed.",
                 "Apctray", MB_ICONEXCLAMATION | MB_OK);
      return 1;
   }

   // Attempt to add Apctray key
   if (RegSetValueEx(runkey, "Apctray", 0, REG_SZ,
         (unsigned char *)cmd, strlen(cmd)+1) != ERROR_SUCCESS) {
      RegCloseKey(runkey);
      MessageBox(NULL, "The System Registry could not be updated.\n"
                        "Apctray was not installed.",
                 "Apctray", MB_ICONEXCLAMATION | MB_OK);
      return 1;
   }

   RegCloseKey(runkey);

   MessageBox(NULL,
              "Apctray was successfully installed and will automatically\n"
              "be run when users log in to this machine.",
              "Apctray", MB_ICONINFORMATION | MB_OK);

   return 0;
}

int Remove()
{
   // Open registry key for auto-run programs
   HKEY runkey;
   if (RegCreateKey(HKEY_LOCAL_MACHINE, 
                    "Software\\Microsoft\\Windows\\CurrentVersion\\Run",
                    &runkey) != ERROR_SUCCESS) {
      MessageBox(NULL, "Apctray does not appear to be installed\n"
                       "in the system registry or you do not have\n"
                       "permission to remove it.",
                 "Apctray", MB_ICONEXCLAMATION | MB_OK);
      return 0;
   }

   // Attempt to delete the Apctray key
   if (RegDeleteValue(runkey, "Apctray") != ERROR_SUCCESS) {
      RegCloseKey(runkey);
      MessageBox(NULL, "Apctray does not appear to be installed\n"
                       "in the system registry or you do not have\n"
                       "permission to remove it.",
                 "Apctray", MB_ICONEXCLAMATION | MB_OK);
      return 0;
   }

   RegCloseKey(runkey);

   MessageBox(NULL,
              "Apctray was successfully removed and will no longer\n"
              "be run when users log on to this machine.",
              "Apctray", MB_ICONINFORMATION | MB_OK);

   return 0;
}

int Kill()
{
   HWND wnd = FindWindow(APCTRAY_WINDOW_CLASS, APCTRAY_WINDOW_NAME);
   if (wnd)
      PostMessage(wnd, WM_CLOSE, 0, 0);
   return 0;
}

void Usage(const char *text1, const char* text2)
{
   MessageBox(NULL, text1, text2, MB_OK);
   MessageBox(NULL, USAGE_TEXT, "Apctray Usage",
              MB_OK | MB_ICONINFORMATION);
}

// WinMain parses the command line and either calls the main App
// routine or, under NT, the main service routine.
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   PSTR CmdLine, int iCmdShow)
{
   InitWinAPIWrapper();
   WSA_Init();

   char *host = NULL;
   unsigned short port = 0;
   int interval = -1;

   // Check command line options
   char *arg;
   char *opt = CmdLine;
   while ((arg = GetArg(&opt))) {
      if (strcasecmp(arg, CMDOPT_HOST) == 0) {
         if (!(arg = GetArg(&opt))) {
            Usage(CMDOPT_HOST, "Option requires argument");
            return 1;
         }
         host = arg;
      } else if (strcasecmp(arg, CMDOPT_PORT) == 0) {
         if (!(arg = GetArg(&opt))) {
            Usage(CMDOPT_PORT, "Option requires argument");
            return 1;
         }
         port = strtoul(arg, NULL, 0);
      } else if (strcasecmp(arg, CMDOPT_REFRESH) == 0) {
         if (!(arg = GetArg(&opt))) {
            Usage(CMDOPT_REFRESH, "Option requires argument");
            return 1;
         }
         interval = strtoul(arg, NULL, 0);
      } else if (strcasecmp(arg, CMDOPT_INSTALL) == 0) {
         return Install(host, port, interval);
      } else if (strcasecmp(arg, CMDOPT_REMOVE) == 0) {
         return Remove();
      } else if (strcasecmp(arg, CMDOPT_KILL) == 0) {
         return Kill();
      } else {
         Usage(arg, "Unknown option");
         return 1;
      }
   }

   // Assign defaults where necessary
   if (!host) host = DEFAULT_HOST;
   if (!port) port = DEFAULT_PORT;
   if (interval < 1) interval = DEFAULT_REFRESH;

   // Create a StatMgr for handling UPS status queries
   StatMgr *statmgr = new StatMgr(host, port);

   // Create tray icon & menu
   upsMenu *menu = new upsMenu(hInstance, statmgr, interval);
   if (menu == NULL) {
      PostQuitMessage(0);
   }

   // Now enter the Windows message handling loop until told to quit!
   MSG msg;
   while (GetMessage(&msg, NULL, 0, 0)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
   }

   delete menu;
   delete statmgr;
   WSACleanup();
   return 0;
}
