//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//
//  This file was part of the VNC system.
//
//  The VNC system is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307,
//  USA.
//
// If the source code for the VNC system is not available from the place 
// whence you received this file, check http://www.uk.research.att.com/vnc or contact
// the authors on vnc@uk.research.att.com for information on obtaining it.
//
// This file has been adapted to the Win32 version of Apcupsd
// by Kern E. Sibbald.  Many thanks to ATT and James Weatherall,
// the original author, for providing an excellent template.
//
// Copyright (2000-2005) Kern E. Sibbald
//


// winService

// Implementation of service-oriented functionality of Apcupsd
// I.e. command line options that contact a running version of
// Apcupsd and ask it to do something (show about, show status,
// show events, ...)

#ifdef HAVE_MINGW
#include "compat.h"
#undef STRICT
#endif

#include "winhdrs.h"

// Header

#include "winservice.h"

#include <lmcons.h>
#include "winups.h"
#include "wintray.h"

// Error message logging
void LogErrorMsg(char *message, int eventID);
extern void logonfail(int ok);

// OS-SPECIFIC ROUTINES

// Create an instance of the upsService class to cause the static fields to be
// initialised properly

upsService init;

#ifdef HAVE_MINGW
extern  DWORD g_platform_id;
#else
DWORD g_platform_id;
#endif
BOOL g_impersonating_user = 0;

upsService::upsService()
{
   OSVERSIONINFO osversioninfo;
   osversioninfo.dwOSVersionInfoSize = sizeof(osversioninfo);

   // Get the current OS version
   if (!GetVersionEx(&osversioninfo)) {
      g_platform_id = 0;
   } else {
      g_platform_id = osversioninfo.dwPlatformId;
   }
}


// IsWin95 - returns a BOOL indicating whether the current OS is Win95
BOOL upsService::IsWin95()
{
   return (g_platform_id == VER_PLATFORM_WIN32_WINDOWS);
}

// IsWinNT - returns a bool indicating whether the current OS is WinNT
BOOL upsService::IsWinNT()
{
   return (g_platform_id == VER_PLATFORM_WIN32_NT);
}

// Internal routine to find the Apcupsd menu class window and
// post a message to it!

BOOL PostToApcupsd(UINT message, WPARAM wParam, LPARAM lParam)
{
   // Locate the hidden Apcupsd menu window
   HWND hservwnd = FindWindow(MENU_CLASS_NAME, NULL);
   if (hservwnd == NULL) {
      return FALSE;
   }
   // Post the message to Apcupsd
   PostMessage(hservwnd, message, wParam, lParam);
   return TRUE;
}


// Static routine to show the Properties dialog for a currently-running
// copy of Apcupsd, (usually a servicified version.)

BOOL upsService::ShowProperties()
{
   return TRUE;
}

// Static routine to show the Default Properties dialog for a currently-running
// copy of Apcupsd, (usually a servicified version.)

BOOL upsService::ShowDefaultProperties()
{
   return TRUE;
}

// Static routine to show the About dialog for a currently-running
// copy of Apcupsd, (usually a servicified version.)

BOOL upsService::ShowAboutBox()
{
   // Post to the Apcupsd menu window
   if (!PostToApcupsd(MENU_ABOUTBOX_SHOW, 0, 0)) {
      MessageBox(NULL, "No existing instance of Apcupsd could be contacted",
                 szAppName, MB_ICONEXCLAMATION | MB_OK);
      return FALSE;
   }
   return TRUE;
}

// Static routine to show the Status dialog for a currently-running
// copy of Apcupsd, (usually a servicified version.)

BOOL upsService::ShowStatus()
{
   // Post to the Apcupsd menu window
   if (!PostToApcupsd(MENU_STATUS_SHOW, 0, 0)) {
      MessageBox(NULL, "No existing instance of Apcupsd could be contacted",
                 szAppName, MB_ICONEXCLAMATION | MB_OK);
      return FALSE;
   }
   return TRUE;
}

// Static routine to show the Events dialog for a currently-running
// copy of Apcupsd, (usually a servicified version.)

BOOL upsService::ShowEvents()
{
   // Post to the Apcupsd menu window
   if (!PostToApcupsd(MENU_EVENTS_SHOW, 0, 0)) {
      MessageBox(NULL, "No existing instance of Apcupsd could be contacted",
                 szAppName, MB_ICONEXCLAMATION | MB_OK);
      return FALSE;
   }
   return TRUE;
}


// SERVICE-MODE ROUTINES

// Service-mode defines:

// Executable name
#define UPS_APPNAME            "apcupsd"

// Internal service name
#define UPS_SERVICENAME        "Apcupsd"

// Displayed service name
#define UPS_SERVICEDISPLAYNAME "Apcupsd UPS Server"

// List of other required services
#define UPS_DEPENDENCIES   __TEXT("tcpip\0afd\0+File System\0")

// Internal service state
SERVICE_STATUS g_srvstatus;        // current status of the service
SERVICE_STATUS_HANDLE g_hstatus;
DWORD g_error = 0;
DWORD g_servicethread = 0;
char *g_errortext[256];

// Forward defines of internal service functions
void WINAPI ServiceMain(DWORD argc, char **argv);

DWORD WINAPI ServiceWorkThread(LPVOID lpwThreadParam);
void ServiceStop();
void WINAPI ServiceCtrl(DWORD ctrlcode);

bool WINAPI CtrlHandler(DWORD ctrltype);

BOOL ReportStatus(DWORD state, DWORD exitcode, DWORD waithint);

// ROUTINE TO QUERY WHETHER THIS PROCESS IS RUNNING AS A SERVICE OR NOT

BOOL g_servicemode = FALSE;

BOOL upsService::RunningAsService()
{
   return g_servicemode;
}

BOOL upsService::KillRunningCopy()
{
   while (PostToApcupsd(WM_CLOSE, 0, 0)) 
     {    }
   return TRUE;
}


// SERVICE MAIN ROUTINE
int upsService::ApcupsdServiceMain()
{
   // Mark that we are a service
   g_servicemode = TRUE;

   // How to run as a service depends upon the OS being used
   switch (g_platform_id) {

   // Windows 95/98
   case VER_PLATFORM_WIN32_WINDOWS:{
      // Obtain a handle to the kernel library
      HINSTANCE kerneldll = LoadLibrary("KERNEL32.DLL");
      if (kerneldll == NULL) {
         LogErrorMsg("KERNEL32.DLL not found", 0);
         MessageBox(NULL, "KERNEL32.DLL not found: Apcupsd service not started",
                    "Apcupsd Service", MB_OK);
         break;
      }
      // And find the RegisterServiceProcess function
      DWORD WINAPI(*RegisterService) (DWORD, DWORD);
      RegisterService = (DWORD WINAPI(*)(DWORD, DWORD))
         GetProcAddress(kerneldll, "RegisterServiceProcess");
      if (RegisterService == NULL) {
         LogErrorMsg("Kernel service entry not found", 0);
         MessageBox(NULL,
                    "Kernel service entry point not found: Apcupsd service not started",
                    "Apcupsd Service", MB_OK);
         break;
      }
      // Register this process with the OS as a service!
      RegisterService(0, 1);

      // Run the service itself
      ApcupsdAppMain(1);

      // Then remove the service from the system service table
      RegisterService(0, 0);

      // Free the kernel library
      FreeLibrary(kerneldll);
      break;
   }
   // Windows NT, Win2K, WinXP
   case VER_PLATFORM_WIN32_NT:
      // Create a service entry table
      SERVICE_TABLE_ENTRY dispatchTable[] = {
         {UPS_SERVICENAME, (LPSERVICE_MAIN_FUNCTION) ServiceMain},
         {NULL, NULL}
      };

      // Call the service control dispatcher with our entry table
      if (!StartServiceCtrlDispatcher(dispatchTable)) {
         LogErrorMsg("StartServiceCtrlDispatcher failed.", 0);
      }
      break;
   }                               /* end switch */
   return 0;
}

// SERVICE MAIN ROUTINE - NT ONLY !!!
// NT ONLY !!!
void WINAPI ServiceMain(DWORD argc, char **argv)
{
   DWORD dwThreadID;

   // Register the service control handler
   g_hstatus = RegisterServiceCtrlHandler(UPS_SERVICENAME, ServiceCtrl);

   if (g_hstatus == 0) {
      LogErrorMsg("RegisterServiceCtrlHandler error", 0);
      MessageBox(NULL, "Contact Register Service Handler failure",
                 "Apcupsd service", MB_OK);
      return;
   }
   // Set up some standard service state values
   g_srvstatus.dwServiceType = SERVICE_WIN32 | SERVICE_INTERACTIVE_PROCESS;
   g_srvstatus.dwServiceSpecificExitCode = 0;

   // Give this status to the SCM
   if (!ReportStatus(SERVICE_START_PENDING,     // Service state
                     NO_ERROR,     // Exit code type
                     45000)) {     // Hint as to how long Apcupsd should have hung before you assume error

      LogErrorMsg("ReportStatus StartPending Failure.", 0);
      ReportStatus(SERVICE_STOPPED, g_error, 0);
      return;
   }
   // Now start the service for real
   (void)CreateThread(NULL, 0, ServiceWorkThread, NULL, 0, &dwThreadID);
   return;
}

// SERVICE START ROUTINE - thread that calls ApcupsdAppMain
//   NT ONLY !!!!
DWORD WINAPI ServiceWorkThread(LPVOID lpwThreadParam)
{
   // Save the current thread identifier
   g_servicethread = GetCurrentThreadId();

   // report the status to the service control manager.
   //
   if (!ReportStatus(SERVICE_RUNNING,   // service state
                     NO_ERROR,     // exit code
                     0)) {         // wait hint

      LogErrorMsg("ReportStatus Running Failure.", 0);
      MessageBox(NULL, "Report Service failure", "Apcupsd Service", MB_OK);
      return 0;
   }


   // RUN!
   ApcupsdAppMain(1);

   // Mark that we're no longer running
   g_servicethread = 0;

   // Tell the service manager that we've stopped.
   ReportStatus(SERVICE_STOPPED, g_error, 0);
   return 0;
}

// SERVICE STOP ROUTINE - post a quit message to the relevant thread
void ServiceStop()
{
   // Post a quit message to the main service thread
   if (g_servicethread != 0) {
      PostThreadMessage(g_servicethread, WM_QUIT, 0, 0);
   }
}

// SERVICE INSTALL ROUTINE
int upsService::InstallService()
{
   const int pathlength = 2048;
   char path[pathlength];
   char servicecmd[pathlength];

   // Get the filename of this executable
   if (GetModuleFileName(NULL, path, pathlength - (strlen(ApcupsdRunService) + 2)) ==
       0) {
      MessageBox(NULL, "Unable to install Apcupsd service", szAppName,
                 MB_ICONEXCLAMATION | MB_OK);
      return 0;
   }
   // Append the service-start flag to the end of the path:
   if ((int)strlen(path) + 20 + (int)strlen(ApcupsdRunService) < pathlength) {
      snprintf(servicecmd, sizeof(servicecmd), "\"%s\" %s", path, ApcupsdRunService);
   } else {
      MessageBox(NULL, "Apcupsd path too long to register service", "Service error",
                 MB_OK);
      return 0;
   }

   // How to add the Apcupsd service depends upon the OS
   switch (g_platform_id) {

      // Windows 95/98
   case VER_PLATFORM_WIN32_WINDOWS:
      // Locate the RunService registry entry
      HKEY runservices;
      if (RegCreateKey(HKEY_LOCAL_MACHINE,
                       "Software\\Microsoft\\Windows\\CurrentVersion\\RunServices",
                       &runservices) != ERROR_SUCCESS) {
         MessageBox(NULL,
                    "The System Registry could not be updated - the Apcupsd service was not installed",
                    szAppName, MB_ICONEXCLAMATION | MB_OK);
         break;
      }
      // Attempt to add a Apcupsd key
      if (RegSetValueEx
          (runservices, szAppName, 0, REG_SZ, (unsigned char *)servicecmd,
           strlen(servicecmd) + 1) != ERROR_SUCCESS) {
         RegCloseKey(runservices);
         MessageBox(NULL, "The Apcupsd service could not be installed", szAppName,
                    MB_ICONEXCLAMATION | MB_OK);
         break;
      }

      RegCloseKey(runservices);

      // We have successfully installed the service!
      break;

      // Windows NT
   case VER_PLATFORM_WIN32_NT:
      SC_HANDLE hservice;
      SC_HANDLE hsrvmanager;

      // Open the default, local Service Control Manager database
      hsrvmanager = OpenSCManager(NULL, NULL, SC_MANAGER_ALL_ACCESS);
      if (hsrvmanager == NULL) {
         MessageBox(NULL,
                    "The Service Control Manager could not be contacted - the Apcupsd service was not installed",
                    szAppName, MB_ICONEXCLAMATION | MB_OK);
         break;
      }
      // Create an entry for the Apcupsd service
      hservice = CreateService(hsrvmanager,     // SCManager database
                               UPS_SERVICENAME, // name of service
                               UPS_SERVICEDISPLAYNAME,  // name to display
                               SERVICE_ALL_ACCESS,      // desired access
                               SERVICE_WIN32_OWN_PROCESS |
                               SERVICE_INTERACTIVE_PROCESS,
                               // service type
                               SERVICE_AUTO_START,      // start type
                               SERVICE_ERROR_NORMAL,    // error control type
                               servicecmd,      // service's binary
                               NULL,    // no load ordering group
                               NULL,    // no tag identifier
                               UPS_DEPENDENCIES,        // dependencies
                               NULL,    // LocalSystem account
                               NULL);   // no password
      CloseServiceHandle(hsrvmanager);
      if (hservice == NULL) {
         MessageBox(NULL,
                    "The Apcupsd service could not be installed",
                    szAppName, MB_ICONEXCLAMATION | MB_OK);
         break;
      }
      CloseServiceHandle(hservice);

      // Everything went fine
      MessageBox(NULL,
                 "The Apcupsd service was successfully installed.\n"
                 "The service may be started from the Control Panel and will\n"
                 "automatically be run the next time this machine is rebooted.",
                 szAppName, MB_ICONINFORMATION | MB_OK);

      break;
   default:
      MessageBox(NULL,
                 "Unknown Windows operating system.\n"
                 "Cannot install Apcupsd service.\n",
                 szAppName, MB_ICONEXCLAMATION | MB_OK);
      break;
   }                               /* end switch */

   return 0;
}

// SERVICE REMOVE ROUTINE
int upsService::RemoveService()
{
   // How to remove the Apcupsd service depends upon the OS
   switch (g_platform_id) {

      // Windows 95/98
   case VER_PLATFORM_WIN32_WINDOWS:
      // Locate the RunService registry entry
      HKEY runservices;
      if (RegOpenKey(HKEY_LOCAL_MACHINE,
                     "Software\\Microsoft\\Windows\\CurrentVersion\\RunServices",
                     &runservices) != ERROR_SUCCESS) {
         MessageBox(NULL,
                    "Could not find registry entry.\nService probably not registerd - the Apcupsd service was not removed",
                    szAppName, MB_ICONEXCLAMATION | MB_OK);
      } else {
         // Attempt to delete the Apcupsd key
         if (RegDeleteValue(runservices, szAppName) != ERROR_SUCCESS) {
            RegCloseKey(runservices);
            MessageBox(NULL,
                       "Could not delete Registry key.\nThe Apcupsd service could not be removed",
                       szAppName, MB_ICONEXCLAMATION | MB_OK);
         }

         RegCloseKey(runservices);
         break;
      }

      // Try to kill any running copy of Apcupsd
      if (!KillRunningCopy()) {
         MessageBox(NULL,
                    "Apcupsd could not be contacted, probably not running",
                    szAppName, MB_ICONEXCLAMATION | MB_OK);
         break;
      }
      // We have successfully removed the service!
      MessageBox(NULL, "The Apcupsd service has been removed", szAppName,
                 MB_ICONINFORMATION | MB_OK);
      break;

      // Windows NT
   case VER_PLATFORM_WIN32_NT:
      SC_HANDLE hservice;
      SC_HANDLE hsrvmanager;

      // Open the SCM
      hsrvmanager = OpenSCManager(NULL, // machine (NULL == local)
                                  NULL, // database (NULL == default)
                                  SC_MANAGER_ALL_ACCESS // access required
         );
      if (hsrvmanager) {
         hservice = OpenService(hsrvmanager, UPS_SERVICENAME, SERVICE_ALL_ACCESS);
         if (hservice != NULL) {
            SERVICE_STATUS status;

            // Try to stop the Apcupsd service
            if (ControlService(hservice, SERVICE_CONTROL_STOP, &status)) {
               while (QueryServiceStatus(hservice, &status)) {
                  if (status.dwCurrentState == SERVICE_STOP_PENDING) {
                     Sleep(1000);
                  } else {
                     break;
                  }
               }

               if (status.dwCurrentState != SERVICE_STOPPED) {
                  MessageBox(NULL, "The Apcupsd service could not be stopped",
                             szAppName, MB_ICONEXCLAMATION | MB_OK);
               }
            }
            // Now remove the service from the SCM
            if (DeleteService(hservice)) {
               MessageBox(NULL, "The Apcupsd service has been removed", szAppName,
                          MB_ICONINFORMATION | MB_OK);
            } else {
               MessageBox(NULL, "The Apcupsd service could not be removed",
                          szAppName, MB_ICONEXCLAMATION | MB_OK);
            }

            CloseServiceHandle(hservice);
         } else {
            MessageBox(NULL, "The Apcupsd service could not be found", szAppName,
                       MB_ICONEXCLAMATION | MB_OK);
         }

         CloseServiceHandle(hsrvmanager);
      } else {
         MessageBox(NULL,
                    "The SCM could not be contacted - the Apcupsd service was not removed",
                    szAppName, MB_ICONEXCLAMATION | MB_OK);
      }
      break;
   }
   return 0;
}

// USEFUL SERVICE SUPPORT ROUTINES

// Service control routine
void WINAPI ServiceCtrl(DWORD ctrlcode)
{
   // What control code have we been sent?
   switch (ctrlcode) {

   case SERVICE_CONTROL_STOP:
      // STOP : The service must stop
      g_srvstatus.dwCurrentState = SERVICE_STOP_PENDING;
      ServiceStop();
      break;

   case SERVICE_CONTROL_INTERROGATE:
      // QUERY : Service control manager just wants to know our state
      break;

   default:
      // Control code not recognised
      break;
   }

   // Tell the control manager what we're up to.
   ReportStatus(g_srvstatus.dwCurrentState, NO_ERROR, 0);
}

// Service manager status reporting
BOOL ReportStatus(DWORD state, DWORD exitcode, DWORD waithint)
{
   static DWORD checkpoint = 1;
   BOOL result = TRUE;

   // If we're in the start state then we don't want the control manager
   // sending us control messages because they'll confuse us.
   if (state == SERVICE_START_PENDING) {
      g_srvstatus.dwControlsAccepted = 0;
   } else {
      g_srvstatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
   }

   // Save the new status we've been given
   g_srvstatus.dwCurrentState = state;
   g_srvstatus.dwWin32ExitCode = exitcode;
   g_srvstatus.dwWaitHint = waithint;

   // Update the checkpoint variable to let the SCM know that we
   // haven't died if requests take a long time
   if ((state == SERVICE_RUNNING) || (state == SERVICE_STOPPED)) {
      g_srvstatus.dwCheckPoint = 0;
   } else {
      g_srvstatus.dwCheckPoint = checkpoint++;
   }

   // Tell the SCM our new status
   if (!(result = SetServiceStatus(g_hstatus, &g_srvstatus))) {
      LogErrorMsg("SetServiceStatus failed", 0);
   }

   return result;
}

// Error reporting
void LogErrorMsg(char *message, int eventID)
{
   char msgbuff[256];
   HANDLE heventsrc;
   char *strings[3];
   LPTSTR msg;


   // Save the error code
   g_error = GetLastError();
   FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                 FORMAT_MESSAGE_FROM_SYSTEM,
                 NULL, g_error, 0, (LPTSTR) & msg, 0, NULL);

   syslog(0, "%s", msg);

   // Use event logging to log the error
   heventsrc = RegisterEventSource(NULL, UPS_SERVICENAME);

   snprintf(msgbuff, sizeof(msgbuff), "%s error: %ld", UPS_SERVICENAME, g_error);
   strings[0] = msgbuff;
   strings[1] = message;
   strings[2] = msg;

   if (heventsrc != NULL) {
      MessageBeep(MB_OK);

      ReportEvent(heventsrc,       // handle of event source
                  EVENTLOG_ERROR_TYPE,  // event type
                  0,               // event category
                  eventID,         // event ID
                  NULL,            // current user's SID
                  3,               // strings in 'strings'
                  0,               // no bytes of raw data
                  (const char **)strings,       // array of error strings
                  NULL);           // no raw data

      DeregisterEventSource(heventsrc);
   }
   LocalFree(msg);
}
