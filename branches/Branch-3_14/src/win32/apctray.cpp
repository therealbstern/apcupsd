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
#define CMDOPT_INTERVAL "/interval"

#define USAGE_TEXT   "[/host <hostname>] [/port <port>] [/interval <sec>]"

char *GetArg(char **cmdline)
{
   // Skip leading whitespace
   while (isspace(**cmdline))
      (*cmdline)++;

   // Bail if there's nothing left
   if (**cmdline == '\0')
      return NULL;

   // Find end of this argument
   char *ret;
   if (**cmdline == '"') {
      // Find end of quoted argument
      ret = ++(*cmdline);
      while (**cmdline && **cmdline != '"')
         (*cmdline)++;
   } else {
      // Find end of non-quoted argument
      ret = *cmdline;
      while (**cmdline && !isspace(**cmdline))
         (*cmdline)++;
   }

   // NUL-terminate this argument
   if (**cmdline)
      *(*cmdline)++ = '\0';

   return ret;
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

   // Default settings
   char *host = "127.0.0.1";
   unsigned short port = 3551;
   int interval = 1000;

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
      } else if (strcasecmp(arg, CMDOPT_INTERVAL) == 0) {
         if (!(arg = GetArg(&opt))) {
            Usage(CMDOPT_INTERVAL, "Option requires argument");
            return 1;
         }
         interval = strtoul(arg, NULL, 0) * 1000;
      } else {
         Usage(arg, "Unknown option");
         return 1;
      }
   }

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
