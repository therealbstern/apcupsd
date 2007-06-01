// This file has been adapted to the Win32 version of Apcupsd
// by Kern E. Sibbald.  Many thanks to ATT and James Weatherall,
// the original author, for providing an excellent template.
//
// Rewrite/Refactoring by Adam Kropelin
//
// Copyright (2007) Adam D. Kropelin
// Copyright (2000) Kern E. Sibbald
//


// WinUPS header file

#include <windows.h>

// Application specific messages

// Message used for system tray notifications
#define WM_TRAYNOTIFY            WM_USER+1

// Message used to inform tray that a balloon tip was displayed
#define WM_BALLOONSHOW           WM_USER+2

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
