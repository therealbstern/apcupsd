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
//



// Tray

// Implementation of a system tray icon & menu for Apcupsd

#include "winhdrs.h"
#include "winups.h"
#include "winservice.h"
#include <lmcons.h>

// Header

#include "wintray.h"

// Constants
#ifdef properties_implemented
const UINT MENU_PROPERTIES_SHOW = RegisterWindowMessage("Apcupsd.Properties.User.Show");
const UINT MENU_DEFAULT_PROPERTIES_SHOW = RegisterWindowMessage("Apcupsd.Properties.Default.Show");
#endif
const UINT MENU_ABOUTBOX_SHOW = RegisterWindowMessage("Apcupsd.AboutBox.Show");
const UINT MENU_STATUS_SHOW = RegisterWindowMessage("Apcupsd.Status.Show");
const UINT MENU_EVENTS_SHOW = RegisterWindowMessage("Apcupsd.Events.Show");
const UINT MENU_SERVICEHELPER_MSG = RegisterWindowMessage("Apcupsd.ServiceHelper.Message");
const UINT MENU_ADD_CLIENT_MSG = RegisterWindowMessage("Apcupsd.AddClient.Message");
const char *MENU_CLASS_NAME = "Apcupsd Tray Icon";

extern char *ups_status(int stat);

extern int battstat;

// Implementation

upsMenu::upsMenu()
{
    // Create a dummy window to handle tray icon messages
    WNDCLASSEX wndclass;

    wndclass.cbSize         = sizeof(wndclass);
    wndclass.style          = 0;
    wndclass.lpfnWndProc    = upsMenu::WndProc;
    wndclass.cbClsExtra     = 0;
    wndclass.cbWndExtra     = 0;
    wndclass.hInstance      = hAppInstance;
    wndclass.hIcon          = LoadIcon(NULL, IDI_APPLICATION);
    wndclass.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wndclass.hbrBackground  = (HBRUSH) GetStockObject(WHITE_BRUSH);
    wndclass.lpszMenuName   = (const char *) NULL;
    wndclass.lpszClassName  = MENU_CLASS_NAME;
    wndclass.hIconSm        = LoadIcon(NULL, IDI_APPLICATION);

    RegisterClassEx(&wndclass);

    /* Create System Tray menu Window */
    m_hwnd = CreateWindow(MENU_CLASS_NAME,
                            MENU_CLASS_NAME,
                            WS_OVERLAPPEDWINDOW,
                            CW_USEDEFAULT,
                            CW_USEDEFAULT,
                            200, 200,
                            NULL,
                            NULL,
                            hAppInstance,
                            NULL);
    if (m_hwnd == NULL) {
       PostQuitMessage(0);
       return;
    }

    // record which client created this window
    SetWindowLong(m_hwnd, GWL_USERDATA, (LONG)this);

    // Timer to trigger icon updating
    SetTimer(m_hwnd, 1, 5000, NULL);

    // Load the icons for the tray
    m_online_icon = LoadIcon(hAppInstance, MAKEINTRESOURCE(IDI_ONLINE));
    m_onbatt_icon = LoadIcon(hAppInstance, MAKEINTRESOURCE(IDI_ONBATT));
    m_charging_icon = LoadIcon(hAppInstance, MAKEINTRESOURCE(IDI_CHARGING));

    // Load the popup menu
    m_hmenu = LoadMenu(hAppInstance, MAKEINTRESOURCE(IDR_TRAYMENU));

    // Install the tray icon!
    AddTrayIcon();
}

upsMenu::~upsMenu()
{
    // Remove the tray icon
    SendTrayMsg(NIM_DELETE, 0);
        
    // Destroy the loaded menu
    if (m_hmenu != NULL)
        DestroyMenu(m_hmenu);
}

void
upsMenu::AddTrayIcon()
{
    SendTrayMsg(NIM_ADD, battstat);
}

void
upsMenu::DelTrayIcon()
{
    SendTrayMsg(NIM_DELETE, 0);
}


void
upsMenu::UpdateTrayIcon(int battstat)
{
    (void *)ups_status(0);
    SendTrayMsg(NIM_MODIFY, battstat);
}

void
upsMenu::SendTrayMsg(DWORD msg, int battstat)
{
    // Create the tray icon message
    m_nid.hWnd = m_hwnd;
    m_nid.cbSize = sizeof(m_nid);
    m_nid.uID = IDI_APCUPSD;                 // never changes after construction
    /* If battstat == 0 we are on batteries, otherwise we are online
     * and the value of battstat is the percent charge.
     */
    if (battstat == 0)
       m_nid.hIcon = m_onbatt_icon;
    else if (battstat >= 99)
       m_nid.hIcon = m_online_icon;
    else
       m_nid.hIcon = m_charging_icon;

    m_nid.uFlags = NIF_ICON | NIF_MESSAGE;
    m_nid.uCallbackMessage = WM_TRAYNOTIFY;


    // Use resource string as tip if there is one
    if (LoadString(hAppInstance, IDI_APCUPSD, m_nid.szTip, sizeof(m_nid.szTip))) {
        m_nid.uFlags |= NIF_TIP;
    }
    
    // Try to add the UPS's status to the tip string, if possible
    if (m_nid.uFlags & NIF_TIP) {
        strncat(m_nid.szTip, " - ", (sizeof(m_nid.szTip)-1)-strlen(m_nid.szTip));
        strncat(m_nid.szTip, ups_status(0), (sizeof(m_nid.szTip)-1)-strlen(m_nid.szTip));
    }

    // Send the message
    if (Shell_NotifyIcon(msg, &m_nid)) {
       EnableMenuItem(m_hmenu, ID_CLOSE, MF_ENABLED);
    } else {
       if (!upsService::RunningAsService()) {
          if (msg == NIM_ADD) {
             // The tray icon couldn't be created, so use the Properties dialog
             // as the main program window
             PostQuitMessage(0);
          }
       }
    }
}

// Process window messages
LRESULT CALLBACK upsMenu::WndProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
        // This is a static method, so we don't know which instantiation we're 
        // dealing with. We use Allen Hadden's (ahadden@taratec.com) suggestion 
        // from a newsgroup to get the pseudo-this.
        upsMenu *_this = (upsMenu *) GetWindowLong(hwnd, GWL_USERDATA);

        switch (iMsg)
        {

                // Every five seconds, a timer message causes the icon to update
        case WM_TIMER:
                // *** HACK for running servicified
                if (upsService::RunningAsService()) {
                    // Attempt to add the icon if it's not already there
                    _this->AddTrayIcon();
                    // Trigger a check of the current user
                    PostMessage(hwnd, WM_USERCHANGED, 0, 0);
                }

                // Update the icon
                _this->UpdateTrayIcon(battstat);
                break;

                // DEAL WITH NOTIFICATIONS FROM THE SERVER:
        case WM_SRV_CLIENT_AUTHENTICATED:
        case WM_SRV_CLIENT_DISCONNECT:
                // Adjust the icon accordingly
                _this->UpdateTrayIcon(battstat);
                return 0;

                // STANDARD MESSAGE HANDLING
        case WM_CREATE:
                return 0;

        case WM_COMMAND:
                // User has clicked an item on the tray menu
                switch (LOWORD(wParam))
                {

#ifdef properties_implemented
                case ID_DEFAULT_PROPERTIES:
                        // Show the default properties dialog, unless it is already displayed
                        _this->m_properties.Show(TRUE, FALSE);
                        _this->UpdateTrayIcon(battstat);
                        break;
                

                case ID_PROPERTIES:
                        // Show the properties dialog, unless it is already displayed
                        _this->m_properties.Show(TRUE, TRUE);
                        _this->UpdateTrayIcon(battstat);
                        break;
#endif
                
                case ID_STATUS:
                        // Show the status dialog
                        _this->m_status.Show(TRUE);
                        _this->UpdateTrayIcon(battstat);
                        break;

                case ID_EVENTS:
                        // Show the Events dialog
                        _this->m_events.Show(TRUE);
                        _this->UpdateTrayIcon(battstat);
                        break;


                case ID_KILLCLIENTS:
                        // Disconnect all currently connected clients
                        break;

                case ID_ABOUT:
                        // Show the About box
                        _this->m_about.Show(TRUE);
                        break;

                case ID_CLOSE:
                        // User selected Close from the tray menu
                        PostMessage(hwnd, WM_CLOSE, 0, 0);
                        break;

                }
                return 0;

        case WM_TRAYNOTIFY:
                // User has clicked on the tray icon or the menu
                {
                        // Get the submenu to use as a pop-up menu
                        HMENU submenu = GetSubMenu(_this->m_hmenu, 0);

                        // What event are we responding to, RMB click?
                        if (lParam==WM_RBUTTONUP) {
                                if (submenu == NULL) {
                                        return 0;
                                }

                                // Make the first menu item the default (bold font)
                                SetMenuDefaultItem(submenu, 0, TRUE);
                                
                                // Get the current cursor position, to display the menu at
                                POINT mouse;
                                GetCursorPos(&mouse);

                                // There's a "bug"
                                // (Microsoft calls it a feature) in Windows 95 that requires calling
                                // SetForegroundWindow. To find out more, search for Q135788 in MSDN.
                                //
                                SetForegroundWindow(_this->m_nid.hWnd);

                                // Display the menu at the desired position
                                TrackPopupMenu(submenu,
                                                0, mouse.x, mouse.y, 0,
                                                _this->m_nid.hWnd, NULL);

                                return 0;
                        }
                        
                        // Or was there a LMB double click?
                        if (lParam==WM_LBUTTONDBLCLK) {
                                // double click: execute first menu item
                                SendMessage(_this->m_nid.hWnd,
                                                        WM_COMMAND, 
                                                        GetMenuItemID(submenu, 0),
                                                        0);
                        }

                        return 0;
                }

        case WM_CLOSE:
                break;

        case WM_DESTROY:
                // The user wants Apcupsd to quit cleanly...
                PostQuitMessage(0);
                return 0;

        case WM_QUERYENDSESSION:
                // Are we running as a system service?
                // Or is the system shutting down (in which case we should check anyway!)
                if ((!upsService::RunningAsService()) || (lParam == 0)) {
                        // No, so we are about to be killed

                        // If there are remote connections then we should verify
                        // that the user is happy about killing them.

                        // Finally, post a quit message, just in case
                        PostQuitMessage(0);
                        return TRUE;
                }

                // Tell the OS that we've handled it anyway
//              PostQuitMessage(0);
                return TRUE;

        
        default:
#ifdef properties_implemented
                // Deal with any of our custom message types
                if (iMsg == MENU_PROPERTIES_SHOW) {
                        // External request to show our Properties dialog
                        PostMessage(hwnd, WM_COMMAND, MAKELONG(ID_PROPERTIES, 0), 0);
                        return 0;
                }
                if (iMsg == MENU_DEFAULT_PROPERTIES_SHOW) {
                        // External request to show our Properties dialog
                        PostMessage(hwnd, WM_COMMAND, MAKELONG(ID_DEFAULT_PROPERTIES, 0), 0);
                        return 0;
                }
#endif

                if (iMsg == MENU_ABOUTBOX_SHOW) {
                        // External request to show our About dialog
                        PostMessage(hwnd, WM_COMMAND, MAKELONG(ID_ABOUT, 0), 0);
                        return 0;
                }
                if (iMsg == MENU_STATUS_SHOW) {
                        // External request to show our status
                        PostMessage(hwnd, WM_COMMAND, MAKELONG(ID_STATUS, 0), 0);
                        return 0;
                }

                if (iMsg == MENU_EVENTS_SHOW) {
                        // External request to show our Events dialogue
                        PostMessage(hwnd, WM_COMMAND, MAKELONG(ID_EVENTS, 0), 0);
                        return 0;
                }

                if (iMsg == MENU_SERVICEHELPER_MSG) {
                        // External ServiceHelper message.
                        // This message holds a process id which we can use to
                        // impersonate a specific user.  In doing so, we can load their
                        // preferences correctly
                        upsService::ProcessUserHelperMessage(wParam, lParam);

                        // - Trigger a check of the current user
                        PostMessage(hwnd, WM_USERCHANGED, 0, 0);
                        return 0;
                }
                if (iMsg == MENU_ADD_CLIENT_MSG) {
                        // Add Client message.  This message includes an IP address
                        // of a listening client, to which we should connect.

                        return 0;
                }
        }

        // Message not recognised
        return DefWindowProc(hwnd, iMsg, wParam, lParam);
}
