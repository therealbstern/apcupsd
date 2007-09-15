// This file has been adapted to the Win32 version of Apcupsd
// by Kern E. Sibbald.  Many thanks to ATT and James Weatherall,
// the original author, for providing an excellent template.
//
// Rewrite/Refactoring by Adam Kropelin
//
// Copyright (2007) Adam D. Kropelin
// Copyright (2000-2006) Kern E. Sibbald
//     20 July 2000

// System Headers
#include <unistd.h>
#include <windows.h>
#include <lmcons.h>
#include <ctype.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include "defines.h"

// Apcupsd UNIX main entrypoint
extern int ApcupsdMain(int argc, char **argv);

// Custom headers
#include "winups.h"
#include "winservice.h"
#include "compat.h"

// Standard command-line flag definitions
#define ApcupsdRunService        "/service"
#define ApcupsdRunAsUserApp      "/run"
#define ApcupsdInstallService    "/install"
#define ApcupsdRemoveService     "/remove"
#define ApcupsdKillRunningCopy   "/kill"
#define ApcupsdShowHelp          "/help"
#define ApcupsdQuiet             "/quiet"

// Usage string
static const char *ApcupsdUsageText =
   "apcupsd [/quiet] [/run] [/kill] [/install] [/remove] [/help]\n";

// Application instance
static HINSTANCE hAppInstance;

// Command line argument storage
#define MAX_COMMAND_ARGS 100
static char *command_args[MAX_COMMAND_ARGS] = { "apcupsd", NULL };
static int num_command_args = 1;
static char *winargs[MAX_COMMAND_ARGS];
static int num_winargs = 0;

// WinMain parses the command line and either calls the main App
// routine or, under NT, the main service routine.
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
                   PSTR CmdLine, int iCmdShow)
{
   bool quiet = false;

   InitWinAPIWrapper();
   WSA_Init();

   // Save the application instance and main thread id
   hAppInstance = hInstance;

   /* Build Unix style argc *argv[] */

   /* Don't NULL command_args[0] !!! */
   for (int i = 1; i < MAX_COMMAND_ARGS; i++)
      command_args[i] = NULL;

   // Split command line to windows and non-windows arguments
   char *arg;
   char *szCmdLine = CmdLine;
   while ((arg = GetArg(&szCmdLine))) {
      // Save the argument in appropriate list
      if (*arg != '/' && num_command_args < MAX_COMMAND_ARGS)
         command_args[num_command_args++] = arg;
      else if (num_winargs < MAX_COMMAND_ARGS)
         winargs[num_winargs++] = arg;
   }

   // Default Windows argument
   if (num_winargs == 0)
      winargs[num_winargs++] = ApcupsdRunAsUserApp;

   // Act on Windows arguments...
   for (int i = 0; i < num_winargs; i++) {
      // /service
      if (strcasecmp(winargs[i], ApcupsdRunService) == 0) {
         // Run Apcupsd as a service
         return upsService::ApcupsdServiceMain();
      }
      // /run  (this is the default if no command line arguments)
      if (strcasecmp(winargs[i], ApcupsdRunAsUserApp) == 0) {
         // Apcupsd is being run as a user-level program
         return ApcupsdAppMain(0);
      }
      // /install
      if (strcasecmp(winargs[i], ApcupsdInstallService) == 0) {
         // Install Apcupsd as a service
         return upsService::InstallService(quiet);
      }
      // /remove
      if (strcasecmp(winargs[i], ApcupsdRemoveService) == 0) {
         // Remove the Apcupsd service
         return upsService::RemoveService(quiet);
      }
      // /kill
      if (strcasecmp(winargs[i], ApcupsdKillRunningCopy) == 0) {
         // Kill any already running copy of Apcupsd
         return upsService::KillRunningCopy();
      }
      // /quiet
      if (strcasecmp(winargs[i], ApcupsdQuiet) == 0) {
         // Set quiet flag and go on to next argument
         quiet = true;
         continue;
      }
      // /help
      if (strcasecmp(winargs[i], ApcupsdShowHelp) == 0) {
         MessageBox(NULL, ApcupsdUsageText, "Apcupsd Usage",
                    MB_OK | MB_ICONINFORMATION);
         return 0;
      }

      // Unknown option: Show the usage dialog
      MessageBox(NULL, winargs[i], "Bad Command Line Options", MB_OK);
      MessageBox(NULL, ApcupsdUsageText, "Apcupsd Usage",
                 MB_OK | MB_ICONINFORMATION);
      return 1;
   }
}

// Callback for processing Windows messages
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   switch (uMsg)
   {
   // Clean exit requested
   case WM_DESTROY:
      PostQuitMessage(0);
      return 0;

   // Everything else uses default handling
   default:
      return DefWindowProc(hwnd, uMsg, wParam, lParam);
   }
}

// Called as a thread from ApcupsdAppMain()
// Here we invoke apcupsd UNIX main loop
//
void *ApcupsdMain(LPVOID lpwThreadParam)
{
   pthread_detach(pthread_self());

   // Call the "real" apcupsd
   ApcupsdMain(num_command_args, command_args);

   // In case apcupsd returns
   DWORD main_tid = (DWORD)lpwThreadParam;
   PostThreadMessage(main_tid, WM_QUIT, 0, 0);
}

// This is the main routine for Apcupsd when running as an application
// (under Windows 95 or Windows NT)
// Under NT, Apcupsd can also run as a service.  The ApcupsdServerMain routine,
// defined in the upsService header, is used instead when running as a service.

int ApcupsdAppMain(int service)
{
   // Set this process to be the last application to be shut down.
   SetProcessShutdownParameters(0x100, 0);

   // Check to see if we're already running
   HANDLE sem = CreateSemaphore(NULL, 0, 1, "apcupsd");
   if (sem == NULL || GetLastError() == ERROR_ALREADY_EXISTS) {
      MessageBox(NULL, "Another instance of Apcupsd is already running", 
                 "Apcupsd Error", MB_OK);
      return 0;
   }

   // Dummy window class
   WNDCLASSEX wndclass;
   wndclass.cbSize = sizeof(wndclass);
   wndclass.style = 0;
   wndclass.lpfnWndProc = WindowProc;
   wndclass.cbClsExtra = 0;
   wndclass.cbWndExtra = 0;
   wndclass.hInstance = hAppInstance;
   wndclass.hIcon = NULL;
   wndclass.hCursor = NULL;
   wndclass.hbrBackground = NULL;
   wndclass.lpszMenuName = NULL;
   wndclass.lpszClassName = APCUPSD_WINDOW_CLASS;
   wndclass.hIconSm = NULL;
   if (RegisterClassEx(&wndclass) == 0)
      return 0;

   // Create dummy window so we can receive Windows messages
   if (CreateWindow(APCUPSD_WINDOW_CLASS,  // class
                    APCUPSD_WINDOW_NAME,   // name/title
                    0,                     // style
                    0,                     // X pos
                    0,                     // Y pos
                    0,                     // width
                    0,                     // height
                    NULL,                  // parent
                    NULL,                  // menu
                    hAppInstance,          // app instance
                    NULL                   // create param
                   ) == NULL)
       return 0;

   // Create a thread on which to run apcupsd UNIX main loop
   pthread_t tid;
   pthread_create(&tid, NULL, ApcupsdMain, (void *)GetCurrentThreadId());

   // Now enter the Windows message handling loop until told to quit
   MSG msg;
   while (GetMessage(&msg, NULL, 0, 0)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
   }

   pthread_kill(tid, SIGTERM);
}
