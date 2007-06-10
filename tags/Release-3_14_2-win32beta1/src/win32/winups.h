// This file has been adapted to the Win32 version of Apcupsd
// by Kern E. Sibbald.  Many thanks to ATT and James Weatherall,
// the original author, for providing an excellent template.
//
// Rewrite/Refactoring by Adam Kropelin
//
// Copyright (2007) Adam D. Kropelin
// Copyright (2000) Kern E. Sibbald
//

#ifndef WINUPS_H
#define WINUPS_H

// WinUPS header file

#include <windows.h>

// Application specific messages

// Message used for system tray notifications
#define WM_TRAYNOTIFY            WM_USER+1

// Message used to inform tray that a balloon tip was displayed
#define WM_BALLOONSHOW           WM_USER+2

// Message used to close a given apctray instance
#define WM_CLOSEINST             WM_USER+3

// Message used to remove all apctray instances from the registry
#define WM_REMOVEALL             WM_USER+4

// Message used to remove specified apctray instance from the registry
#define WM_REMOVE                WM_USER+5

// Message used to set balloon notification state
#define WM_BNOTIFY               WM_USER+6

// Apcupsd application window constants
#define APCUPSD_WINDOW_CLASS		"apcupsd"
#define APCUPSD_WINDOW_NAME		"apcupsd"

// apctray window constants
#define APCTRAY_WINDOW_CLASS		"apctray"
#define APCTRAY_WINDOW_NAME		"apctray"

// Command line option to start in service mode
#define ApcupsdRunService        "/service"

// Main UPS server routine - Exported by winmain for use by winservice
extern int ApcupsdAppMain(int service);

#endif // WINUPS_H
