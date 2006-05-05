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

#ifdef properties_implemented

// upsProperties

// Object implementing the Properties dialog for WinUPS.
// The Properties dialog is displayed whenever the user selects the
// Properties option from the system tray menu.
// The Properties dialog also takes care of loading the program
// settings and saving them on exit.

class upsProperties;

#if (!defined(_win_upsPROPERTIES))
#define _win_upsPROPERTIES

// Includes
#include <windows.h>

// The upsProperties class itself
class upsProperties
{
public:
        // Constructor/destructor
        upsProperties();
        ~upsProperties();

        // Initialisation
        BOOL Init();

        // The dialog box window proc
        static BOOL CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

        // Display the properties dialog
        // If usersettings is TRUE then the per-user settings come up
        // If usersettings is FALSE then the default system settings come up
        void Show(BOOL show, BOOL usersettings);

        // Loading & saving of preferences
        void Load(BOOL usersettings);
        void Save();

        // TRAY ICON MENU SETTINGS
        BOOL AllowProperties() {return m_allowproperties;};
        BOOL AllowShutdown() {return m_allowshutdown;};

        // Implementation
protected:
        // The server object to which this properties object is attached.
//      upsServer *                     m_server;

        // Flag to indicate whether the currently loaded settings are for
        // the current user, or are default system settings
        BOOL                            m_usersettings;

        // Tray icon menu settings
        BOOL                            m_allowproperties;
        BOOL                            m_allowshutdown;

        // Password handling
        void LoadPassword(HKEY k, char *buffer);
        void SavePassword(HKEY k, char *buffer);

        // String handling
        char * LoadString(HKEY k, LPCSTR valname);
        void SaveString(HKEY k, LPCSTR valname, const char *buffer);

        // Manipulate the registry settings
        LONG LoadInt(HKEY key, LPCSTR valname, LONG defval);
        void SaveInt(HKEY key, LPCSTR valname, LONG val);

        // Loading/saving all the user prefs
        void LoadUserPrefs(HKEY appkey);
        void SaveUserPrefs(HKEY appkey);

        // Making the loaded user prefs active
        void ApplyUserPrefs();
        
        BOOL m_returncode_valid;
        BOOL m_dlgvisible;

        // STORAGE FOR THE PROPERTIES PRIOR TO APPLICATION
        BOOL m_pref_SockConnect;
        BOOL m_pref_AutoPortSelect;
        LONG m_pref_PortNumber;
#ifndef MAXPWLEN /* KES */
#define MAXPWLEN 100
#endif
        char m_pref_passwd[MAXPWLEN];
        BOOL m_pref_CORBAConn;
        UINT m_pref_QuerySetting;
        UINT m_pref_QueryTimeout;
        BOOL m_pref_EnableRemoteInputs;
        int m_pref_LockSettings;
        BOOL m_pref_DisableLocalInputs;
        BOOL m_pref_PollUnderCursor;
        BOOL m_pref_PollForeground;
        BOOL m_pref_PollFullScreen;
        BOOL m_pref_PollConsoleOnly;
        BOOL m_pref_PollOnEventOnly;
};

#endif // _win_upsPROPERTIES

#endif /* properties_implemented */
