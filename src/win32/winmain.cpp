//  Copyright (C) 1999 AT&T Laboratories Cambridge. All Rights Reserved.
//
//  This file was part of the ups system.
//
//  The ups system is free software; you can redistribute it and/or modify
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
// If the source code for the ups system is not available from the place 
// whence you received this file, check http://www.uk.research.att.com/vnc or contact
// the authors on ups@uk.research.att.com for information on obtaining it.
//
// This file has been adapted to the Win32 version of Apcupsd
// by Kern E. Sibbald.  Many thanks to ATT and James Weatherall,
// the original author, for providing an excellent template.
//
// Copyright (2000) Kern E. Sibbald
//     20 July 2000
//


// winmain.cpp

// 24/11/97             WEZ

// winmain.cpp for win32 version of apcupsd

////////////////////////////
// System headers
#include <unistd.h>
#include "winhdrs.h"
#include <lmcons.h>
#include <ctype.h>
#include <signal.h>
#include <pthread.h>

extern int ApcupsdMain(int argc, char **argv);
extern int terminate(int sig);
extern void clean_threads(void);
extern int kill_on_powerfail;
extern DWORD g_error;
extern BOOL ReportStatus(DWORD state, DWORD exitcode, DWORD waithint);

extern void LogErrorMsg(char *msg, int eventID);


////////////////////////////
// Custom headers
#include "winups.h"

#include "wintray.h"
#include "winservice.h"

// Application instance and name
HINSTANCE       hAppInstance;
const char      *szAppName = "Apcupsd";
DWORD           mainthreadId;

// Imported variables
extern DWORD    g_servicethread;

#define MAX_COMMAND_ARGS 100
static char *command_args[MAX_COMMAND_ARGS] = {"apcupsd", NULL};
static int num_command_args = 1;
static pid_t main_pid;
static pthread_t main_tid;

// WinMain parses the command line and either calls the main App
// routine or, under NT, the main service routine.
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, 
PSTR CmdLine, int iCmdShow)
{
        char *szCmdLine = CmdLine;
        char *wordPtr,*tempPtr;
        int i,quote;

        // Save the application instance and main thread id
        hAppInstance = hInstance;
        mainthreadId = GetCurrentThreadId();

        main_pid = getpid();
        main_tid = pthread_self();

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
        if (wordPtr > szCmdLine)      // backup to char before space
           wordPtr--;
        // if first character is not a quote and last is, junk it
        if (*szCmdLine != '"' && *wordPtr == '"')
           szCmdLine = wordPtr + 1;
//      MessageBox(NULL, szCmdLine, "Cmdline", MB_OK);

        /* Build Unix style argc *argv[] */      

        /* Don't NULL command_args[0] !!! */
        for (i=1;i<MAX_COMMAND_ARGS;i++)
           command_args[i] = NULL;

        wordPtr = szCmdLine;
        quote = 0;
        while  (*wordPtr && (*wordPtr == ' ' || *wordPtr == '\t'))
           wordPtr++;
        if (*wordPtr == '\"') {
           quote = 1;
           wordPtr++;
        } else if (*wordPtr == '/') {
           /* Skip Windows options */
           while (*wordPtr && (*wordPtr != ' ' && *wordPtr != '\t'))
              wordPtr++;
           while  (*wordPtr && (*wordPtr == ' ' || *wordPtr == '\t'))
              wordPtr++;
        }
        if (*wordPtr) {
           while (*wordPtr && num_command_args < MAX_COMMAND_ARGS) {
              tempPtr = wordPtr;
              if (quote) {
                 while (*tempPtr && *tempPtr != '\"')
                 tempPtr++;
                 quote = 0;
              } else {
                 while (*tempPtr && *tempPtr != ' ')
                 tempPtr++;
              }
              if (*tempPtr)
                 *(tempPtr++) = '\0';
              command_args[num_command_args++] = wordPtr;
              wordPtr = tempPtr;
              while (*wordPtr && (*wordPtr == ' ' || *wordPtr == '\t'))
                 wordPtr++;
              if (*wordPtr == '\"') {
                 quote = 1;
                 wordPtr++;
              }
           }
        }

        /* 
         * NOTE!!!! We set Apcupsd option to kill UPS power on powerfail
         *          because on Windows, we have no way to regain control
         *          after the system has synced the disks and killed all
         *          the processes.  Thus, apcupsd will ask the UPS to
         *          kill the power before terminating, and we hope that
         *          the user has set a sufficient delay in the UPS to 
         *          allow the system to halt before the power is cut.
         *
         *          On second thought this is not really a good idea.
         *          For example, in a master/slave, one does not want the
         *          master to immediately power down the UPS.
         */
//      kill_on_powerfail = TRUE;

        // Now process Windows command line options
        // 
        // Make the command-line lowercase and parse it
        for (i = 0; i < (int)strlen(szCmdLine); i++) {
           szCmdLine[i] = tolower(szCmdLine[i]);
        }

        BOOL argfound = FALSE;
        for (i = 0; i < (int)strlen(szCmdLine); i++) {
                if (szCmdLine[i] <= ' ')
                        continue;

                if (szCmdLine[i] == '-') {
                   while (szCmdLine[i] && szCmdLine[i] != ' ')
                      i++;
                   continue;
                }

                argfound = TRUE;

                // Now check for command-line arguments

                // /servicehelper
                //  Used on NT to connect to apcupsd
                if (strncmp(&szCmdLine[i], ApcupsdRunServiceHelper, strlen(ApcupsdRunServiceHelper)) == 0) {
                        // NB : This flag MUST be parsed BEFORE "-service", otherwise it will match
                        // the wrong option!  (This code should really be replaced with a simple
                        // parser machine and parse-table...)

                        // Run the Apcupsd Service Helper app
                        upsService::PostUserHelperMessage();
                        return 0;
                }
                // /service
                if (strncmp(&szCmdLine[i], ApcupsdRunService, strlen(ApcupsdRunService)) == 0) {
                        // Run Apcupsd as a service
                        return upsService::ApcupsdServiceMain();
                }
                // /run  (this is the default if no command line arguments)
                if (strncmp(&szCmdLine[i], ApcupsdRunAsUserApp, strlen(ApcupsdRunAsUserApp)) == 0) {
                        // Apcupsd is being run as a user-level program
                        return ApcupsdAppMain();
                }
                // /install
                if (strncmp(&szCmdLine[i], ApcupsdInstallService, strlen(ApcupsdInstallService)) == 0) {
                        // Install Apcupsd as a service
                        upsService::InstallService();
                        i+=strlen(ApcupsdInstallService);
                        continue;
                }
                // /remove
                if (strncmp(&szCmdLine[i], ApcupsdRemoveService, strlen(ApcupsdRemoveService)) == 0) {
                        // Remove the Apcupsd service
                        upsService::RemoveService();
                        i+=strlen(ApcupsdRemoveService);
                        continue;
                }

#ifdef properties_implemented
                // /settings
                if (strncmp(&szCmdLine[i], ApcupsdShowProperties, strlen(ApcupsdShowProperties)) == 0)
                {
                        // Show the Properties dialog of an existing instance of Apcupsd
                        upsService::ShowProperties();
                        i+=strlen(ApcupsdShowProperties);
                        continue;
                }
                // /defaultsettings
                if (strncmp(&szCmdLine[i], ApcupsdShowDefaultProperties, strlen(ApcupsdShowDefaultProperties)) == 0)
                {
                        // Show the Properties dialog of an existing instance of Apcupsd
                        upsService::ShowDefaultProperties();
                        i+=strlen(ApcupsdShowDefaultProperties);
                        continue;
                }
#endif
                // /about
                if (strncmp(&szCmdLine[i], ApcupsdShowAbout, strlen(ApcupsdShowAbout)) == 0) {
                        // Show the About dialog of an existing instance of Apcupsd
                        upsService::ShowAboutBox();
                        i+=strlen(ApcupsdShowAbout);
                        continue;
                }

                // /status
                if (strncmp(&szCmdLine[i], ApcupsdShowStatus, strlen(ApcupsdShowStatus)) == 0) {
                        // Show the Status dialog of an existing instance of Apcupsd
                        upsService::ShowStatus();
                        i+=strlen(ApcupsdShowStatus);
                        continue;
                }

                // /events
                if (strncmp(&szCmdLine[i], ApcupsdShowEvents, strlen(ApcupsdShowEvents)) == 0) {
                        // Show the Events dialog of an existing instance of Apcupsd
                        upsService::ShowEvents();
                        i+=strlen(ApcupsdShowEvents);
                        continue;
                }


                // /kill
                if (strncmp(&szCmdLine[i], ApcupsdKillRunningCopy, strlen(ApcupsdKillRunningCopy)) == 0) {
                        // Kill any already running copy of Apcupsd
                        upsService::KillRunningCopy();
                        i+=strlen(ApcupsdKillRunningCopy);
                        continue;
                }

                // /help
                if (strncmp(&szCmdLine[i], ApcupsdShowHelp, strlen(ApcupsdShowHelp)) == 0) {
                   MessageBox(NULL, ApcupsdUsageText, "Apcupsd Usage", MB_OK | MB_ICONINFORMATION);
                   i+=strlen(ApcupsdShowHelp);
                   continue;
                }
                
                MessageBox(NULL, szCmdLine, "Bad Command Line Options", MB_OK);

                // Show the usage dialog
                MessageBox(NULL, ApcupsdUsageText, "Apcupsd Usage", MB_OK | MB_ICONINFORMATION);
                break;
        }

        // If no arguments were given then just run
        if (!argfound)
                return ApcupsdAppMain();

        return 0;
}


// Called as a thread from ApcupsdAppMain()
// Here we handle the Windows messages
//
//DWORD WINAPI Main_Msg_Loop(LPVOID lpwThreadParam)
void *Main_Msg_Loop(LPVOID lpwThreadParam)
{
        DWORD old_servicethread = g_servicethread;

        pthread_detach(pthread_self());
 
        /* Since we are the only thread with a message loop
         * mark ourselves as the service thread so that
         * we can receive all the window events.
         */
        g_servicethread = GetCurrentThreadId();

        // Create tray icon & menu if we're running as an app
        upsMenu *menu = new upsMenu();
        if (menu == NULL) {
           PostQuitMessage(0);
        }


        // Now enter the Windows message handling loop until told to quit!
        MSG msg;
        while (GetMessage(&msg, NULL, 0,0) ) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
        }

        if (menu != NULL)
           delete menu;

        if (old_servicethread != 0) { /* started as NT service */
           // Mark that we're no longer running
           g_servicethread = 0;

           // Tell the service manager that we've stopped.
           ReportStatus(SERVICE_STOPPED, g_error, 0);
        }   
        pthread_kill(main_tid, SIGTERM);
        sleep(1);
        kill(main_pid, SIGTERM);      /* ask main thread to terminate */
        _exit(0);
}
 

// This is the main routine for Apcupsd when running as an application
// (under Windows 95 or Windows NT)
// Under NT, Apcupsd can also run as a service.  The ApcupsdServerMain routine,
// defined in the upsService header, is used instead when running as a service.

int ApcupsdAppMain()
{
//        DWORD dwThreadID;
        pthread_t tid;

        // Set this process to be the last application to be shut down.
        SetProcessShutdownParameters(0x100, 0);
        
        HWND hservwnd = FindWindow(MENU_CLASS_NAME, NULL);
        if (hservwnd != NULL) {
           // We don't allow multiple instances!
           MessageBox(NULL, "Another instance of Apcupsd is already running", szAppName, MB_OK);
           return 0;
        }

        // Create a thread to handle the Windows messages
//        (void)CreateThread(NULL, 0, Main_Msg_Loop, NULL, 0, &dwThreadID);
        pthread_create(&tid, NULL, Main_Msg_Loop, (void *)0);

        // Call the "real" apcupsd
        ApcupsdMain(num_command_args, command_args);
        return 0;
}
