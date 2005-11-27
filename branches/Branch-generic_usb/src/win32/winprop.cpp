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


// winprop.cpp

// Implementation of the Properties dialog!

#ifdef properties_implemented

#include "winhdrs.h"
#include "lmcons.h"
#include "winservice.h"

#include "winups.h"
#include "winprop.h"

const char Apcupsd_REGISTRY_KEY [] = "Software\\ORL\\Apcupsd";
const char NO_PASSWORD_WARN [] = "WARNING : Running Apcupsd without setting a password is "
                                                                "a dangerous security risk!\n"
                                                                "Until you set a password, Apcupsd will not accept incoming connections.";
const char NO_OVERRIDE_ERR [] = "This machine has been preconfigured with Apcupsd settings, "
                                                                "which cannot be overridden by individual users.  "
                                                                "The preconfigured settings may be modified only by a System Administrator.";
const char NO_PASSWD_NO_OVERRIDE_ERR [] =
                                                                "No password has been set & this machine has been "
                                                                "preconfigured to prevent users from setting their own.\n"
                                                                "You must contact a System Administrator to configure Apcupsd properly.";
const char NO_PASSWD_NO_OVERRIDE_WARN [] =
                                                                "WARNING : This machine has been preconfigured to allow un-authenticated\n"
                                                                "connections to be accepted and to prevent users from enabling authentication.";
const char NO_PASSWD_NO_LOGON_WARN [] =
                                                                "WARNING : This machine has no default password set.  Apcupsd will present the "
                                                                "Default Properties dialog now to allow one to be entered.";
const char NO_CURRENT_USER_ERR [] = "The Apcupsd settings for the current user are unavailable at present.";
const char CANNOT_EDIT_DEFAULT_PREFS [] = "You do not have sufficient priviliges to edit the default local Apcupsd settings.";

// Constructor & Destructor
upsProperties::upsProperties()
{
        m_allowproperties = TRUE;
        m_allowshutdown = TRUE;
        m_dlgvisible = FALSE;
        m_usersettings = TRUE;
}

upsProperties::~upsProperties()
{
}

// Initialisation
BOOL
upsProperties::Init()
{
        
        // Load the settings from the registry
        Load(TRUE);


        return TRUE;
}

// Dialog box handling functions
void
upsProperties::Show(BOOL show, BOOL usersettings)
{
        if (show)
        {
                if (!m_allowproperties)
                {
                        // If the user isn't allowed to override the settings then tell them
                        MessageBox(NULL, NO_OVERRIDE_ERR, "Apcupsd Error", MB_OK | MB_ICONEXCLAMATION);
                        return;
                }

                // Verify that we know who is logged on
                if (usersettings) {
                        char username[UNLEN+1];
                        if (!upsService::CurrentUser(username, sizeof(username)))
                                return;
                        if (strcmp(username, "") == 0) {
                                MessageBox(NULL, NO_CURRENT_USER_ERR, "Apcupsd Error", MB_OK | MB_ICONEXCLAMATION);
                                return;
                        }
                } else {
                        // We're trying to edit the default local settings - verify that we can
                        HKEY hkLocal, hkDefault;
                        BOOL canEditDefaultPrefs = 1;
                        DWORD dw;
                        if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                Apcupsd_REGISTRY_KEY,
                                0, REG_NONE, REG_OPTION_NON_VOLATILE,
                                KEY_READ, NULL, &hkLocal, &dw) != ERROR_SUCCESS)
                                canEditDefaultPrefs = 0;
                        else if (RegCreateKeyEx(hkLocal,
                                "Default",
                                0, REG_NONE, REG_OPTION_NON_VOLATILE,
                                KEY_WRITE | KEY_READ, NULL, &hkDefault, &dw) != ERROR_SUCCESS)
                                canEditDefaultPrefs = 0;
                        if (hkLocal) RegCloseKey(hkLocal);
                        if (hkDefault) RegCloseKey(hkDefault);

                        if (!canEditDefaultPrefs) {
                                MessageBox(NULL, CANNOT_EDIT_DEFAULT_PREFS, "Apcupsd Error", MB_OK | MB_ICONEXCLAMATION);
                                return;
                        }
                }

                // Now, if the dialog is not already displayed, show it!
                if (!m_dlgvisible)
                {

                        // Load in the settings relevant to the user or system
                        Load(usersettings);

                        for (;;)
                        {
                                m_returncode_valid = FALSE;

                                // Do the dialog box
                                int result = DialogBoxParam(hAppInstance,
                                    MAKEINTRESOURCE(IDD_PROPERTIES), 
                                    NULL,
                                    (DLGPROC) DialogProc,
                                    (LONG) this);

                                if (!m_returncode_valid)
                                    result = IDCANCEL;


                                if (result == -1)
                                {
                                        // Dialog box failed, so quit
                                        PostQuitMessage(0);
                                        return;
                                }

                                if (result == IDCANCEL)
                                   break;

                        }

                        // Load in all the settings
                        Load(TRUE);
                }
        }
}

BOOL CALLBACK
upsProperties::DialogProc(HWND hwnd,
                                                  UINT uMsg,
                                                  WPARAM wParam,
                                                  LPARAM lParam )
{
        // We use the dialog-box's USERDATA to store a _this pointer
        // This is set only once WM_INITDIALOG has been recieved, though!
        upsProperties *_this = (upsProperties *) GetWindowLong(hwnd, GWL_USERDATA);

        switch (uMsg)
        {

        case WM_INITDIALOG:
                {
                        // Retrieve the Dialog box parameter and use it as a pointer
                        // to the calling upsProperties object
                        SetWindowLong(hwnd, GWL_USERDATA, lParam);
                        _this = (upsProperties *) lParam;

                        // Set the dialog box's title to indicate which Properties we're editting
                        if (_this->m_usersettings) {
                                SetWindowText(hwnd, "Apcupsd: Current User Properties");
                        } else {
                                SetWindowText(hwnd, "Apcupsd: Default Local System Properties");
                        }




                        SetForegroundWindow(hwnd);

                        _this->m_dlgvisible = TRUE;

                        return TRUE;
                }

        case WM_COMMAND:
                switch (LOWORD(wParam))
                {

                case IDOK:
                case IDC_APPLY:
                        {

                                // And to the registry
                                _this->Save();

                                // Was ok pressed?
                                if (LOWORD(wParam) == IDOK)
                                {
                                        // Yes, so close the dialog
//                                      log.Print(LL_INTINFO, upsLOG("enddialog (OK)\n"));

                                        _this->m_returncode_valid = TRUE;

                                        EndDialog(hwnd, IDOK);
                                        _this->m_dlgvisible = FALSE;
                                }

                                return TRUE;
                        }

                case IDCANCEL:
//                      log.Print(LL_INTINFO, upsLOG("enddialog (CANCEL)\n"));

                        _this->m_returncode_valid = TRUE;

                        EndDialog(hwnd, IDCANCEL);
                        _this->m_dlgvisible = FALSE;
                        return TRUE;



                }

                break;
        }
        return 0;
}

// Functions to load & save the settings
LONG
upsProperties::LoadInt(HKEY key, LPCSTR valname, LONG defval)
{
        LONG pref;
        ULONG type = REG_DWORD;
        ULONG prefsize = sizeof(pref);

        if (RegQueryValueEx(key,
                valname,
                NULL,
                &type,
                (LPBYTE) &pref,
                &prefsize) != ERROR_SUCCESS)
                return defval;

        if (type != REG_DWORD)
                return defval;

        if (prefsize != sizeof(pref))
                return defval;

        return pref;
}

void
upsProperties::LoadPassword(HKEY key, char *buffer)
{
        DWORD type = REG_BINARY;
        int slen=MAXPWLEN;
        char inouttext[MAXPWLEN];

        // Retrieve the encrypted password
        if (RegQueryValueEx(key,
                "Password",
                NULL,
                &type,
                (LPBYTE) &inouttext,
                (LPDWORD) &slen) != ERROR_SUCCESS)
                return;

        if (slen > MAXPWLEN)
                return;

        memcpy(buffer, inouttext, MAXPWLEN);
}

char *
upsProperties::LoadString(HKEY key, LPCSTR keyname)
{
        DWORD type = REG_SZ;
        DWORD buflen = 0;
        BYTE *buffer = 0;

        // Get the length of the AuthHosts string
        if (RegQueryValueEx(key,
                keyname,
                NULL,
                &type,
                NULL,
                &buflen) != ERROR_SUCCESS)
                return 0;

        if (type != REG_SZ)
                return 0;
        buffer = new BYTE[buflen];
        if (buffer == 0)
                return 0;

        // Get the AuthHosts string data
        if (RegQueryValueEx(key,
                keyname,
                NULL,
                &type,
                buffer,
                &buflen) != ERROR_SUCCESS) {
                delete [] buffer;
                return 0;
        }

        // Verify the type
        if (type != REG_SZ) {
                delete [] buffer;
                return 0;
        }

        return (char *)buffer;
}

void
upsProperties::Load(BOOL usersettings)
{
        char username[UNLEN+1];
        HKEY hkLocal, hkLocalUser, hkDefault;
        DWORD dw;

        // NEW (R3) PREFERENCES ALGORITHM
        // 1.   Look in HKEY_LOCAL_MACHINE/Software/ORL/Apcupsd3/%username%
        //              for sysadmin-defined, user-specific settings.
        // 2.   If not found, fall back to %username%=Default
        // 3.   If AllowOverrides is set then load settings from
        //              HKEY_CURRENT_USER/Software/ORL/Apcupsd3

        // GET THE CORRECT KEY TO READ FROM

        // Get the user name / service name
        if (!upsService::CurrentUser((char *)&username, sizeof(username)))
                return;

        // If there is no user logged on them default to SYSTEM
        if (strcmp(username, "") == 0)
                astrncpy((char *)&username, "SYSTEM", sizeof(username));

        // Try to get the machine registry key for Apcupsd
        if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                Apcupsd_REGISTRY_KEY,
                0, REG_NONE, REG_OPTION_NON_VOLATILE,
                KEY_READ, NULL, &hkLocal, &dw) != ERROR_SUCCESS)
                return;

        // Now try to get the per-user local key
        if (RegOpenKeyEx(hkLocal,
                username,
                0, KEY_READ,
                &hkLocalUser) != ERROR_SUCCESS)
                hkLocalUser = NULL;

        // Get the default key
        if (RegCreateKeyEx(hkLocal,
                "Default",
                0, REG_NONE, REG_OPTION_NON_VOLATILE,
                KEY_READ,
                NULL,
                &hkDefault,
                &dw) != ERROR_SUCCESS)
                hkDefault = NULL;

        // LOAD THE MACHINE-LEVEL PREFS

        // Logging/debugging prefs
//      log.Print(LL_INTINFO, upsLOG("loading local-only settings\n"));
//      log.SetMode(LoadInt(hkLocal, "DebugMode", 0));
//      log.SetLevel(LoadInt(hkLocal, "DebugLevel", 0));


        // LOAD THE USER PREFERENCES

        // Set the default user prefs
//      log.Print(LL_INTINFO, upsLOG("clearing user settings\n"));
        m_pref_AutoPortSelect=TRUE;
        m_pref_SockConnect=TRUE;
        m_pref_QuerySetting=2;
        m_pref_QueryTimeout=10;
        m_pref_EnableRemoteInputs=TRUE;
        m_pref_DisableLocalInputs=FALSE;
        m_pref_LockSettings=-1;
        m_pref_PollUnderCursor=FALSE;
        m_pref_PollForeground=TRUE;
        m_pref_PollFullScreen=FALSE;
        m_pref_PollConsoleOnly=TRUE;
        m_pref_PollOnEventOnly=FALSE;
        m_allowshutdown = TRUE;
        m_allowproperties = TRUE;

        // Load the local prefs for this user
        if (hkDefault != NULL)
        {
//              log.Print(LL_INTINFO, upsLOG("loading DEFAULT local settings\n"));
                LoadUserPrefs(hkDefault);
                m_allowshutdown = LoadInt(hkDefault, "AllowShutdown", m_allowshutdown);
                m_allowproperties = LoadInt(hkDefault, "AllowProperties", m_allowproperties);
        }

        // Are we being asked to load the user settings, or just the default local system settings?
        if (usersettings) {
                // We want the user settings, so load them!

                if (hkLocalUser != NULL)
                {
//                      log.Print(LL_INTINFO, upsLOG("loading \"%s\" local settings\n"), username);
                        LoadUserPrefs(hkLocalUser);
                        m_allowshutdown = LoadInt(hkLocalUser, "AllowShutdown", m_allowshutdown);
                        m_allowproperties = LoadInt(hkLocalUser, "AllowProperties", m_allowproperties);
                }

                // Now override the system settings with the user's settings
                // If the username is SYSTEM then don't try to load them, because there aren't any...
                if (m_allowproperties && (strcmp(username, "SYSTEM") != 0))
                {
                        HKEY hkGlobalUser;
                        if (RegCreateKeyEx(HKEY_CURRENT_USER,
                                Apcupsd_REGISTRY_KEY,
                                0, REG_NONE, REG_OPTION_NON_VOLATILE,
                                KEY_READ, NULL, &hkGlobalUser, &dw) == ERROR_SUCCESS)
                        {
//                              log.Print(LL_INTINFO, upsLOG("loading \"%s\" global settings\n"), username);
                                LoadUserPrefs(hkGlobalUser);
                                RegCloseKey(hkGlobalUser);

                                // Close the user registry hive so it can unload if required
                                RegCloseKey(HKEY_CURRENT_USER);
                        }
                }
        } else {
//              log.Print(LL_INTINFO, upsLOG("bypassing user-specific settings (both local and global)\n"));
        }

        if (hkLocalUser != NULL) RegCloseKey(hkLocalUser);
        if (hkDefault != NULL) RegCloseKey(hkDefault);
        RegCloseKey(hkLocal);

        // Make the loaded settings active..
        ApplyUserPrefs();

        // Note whether we loaded the user settings or just the default system settings
        m_usersettings = usersettings;
}

void
upsProperties::LoadUserPrefs(HKEY appkey)
{
        // LOAD USER PREFS FROM THE SELECTED KEY

        // Connection prefs
        m_pref_SockConnect=LoadInt(appkey, "SocketConnect", m_pref_SockConnect);
        m_pref_AutoPortSelect=LoadInt(appkey, "AutoPortSelect", m_pref_AutoPortSelect);
        m_pref_PortNumber=LoadInt(appkey, "PortNumber", m_pref_PortNumber);

        // Connection querying settings
        m_pref_QuerySetting=LoadInt(appkey, "QuerySetting", m_pref_QuerySetting);
        m_pref_QueryTimeout=LoadInt(appkey, "QueryTimeout", m_pref_QueryTimeout);

        // Load the password
        LoadPassword(appkey, m_pref_passwd);
        
        // CORBA Settings
        m_pref_CORBAConn=LoadInt(appkey, "CORBAConnect", m_pref_CORBAConn);

        // Remote access prefs
        m_pref_EnableRemoteInputs=LoadInt(appkey, "InputsEnabled", m_pref_EnableRemoteInputs);
        m_pref_LockSettings=LoadInt(appkey, "LockSetting", m_pref_LockSettings);
        m_pref_DisableLocalInputs=LoadInt(appkey, "LocalInputsDisabled", m_pref_DisableLocalInputs);

        // Polling prefs
        m_pref_PollUnderCursor=LoadInt(appkey, "PollUnderCursor", m_pref_PollUnderCursor);
        m_pref_PollForeground=LoadInt(appkey, "PollForeground", m_pref_PollForeground);
        m_pref_PollFullScreen=LoadInt(appkey, "PollFullScreen", m_pref_PollFullScreen);
        m_pref_PollConsoleOnly=LoadInt(appkey, "OnlyPollConsole", m_pref_PollConsoleOnly);
        m_pref_PollOnEventOnly=LoadInt(appkey, "OnlyPollOnEvent", m_pref_PollOnEventOnly);
}

void
upsProperties::ApplyUserPrefs()
{
        // APPLY THE CACHED PREFERENCES TO THE SERVER

}

void
upsProperties::SaveInt(HKEY key, LPCSTR valname, LONG val)
{
        RegSetValueEx(key, valname, 0, REG_DWORD, (LPBYTE) &val, sizeof(val));
}

void
upsProperties::SavePassword(HKEY key, char *buffer)
{
        RegSetValueEx(key, "Password", 0, REG_BINARY, (LPBYTE) buffer, MAXPWLEN);
}

void
upsProperties::Save()
{
        HKEY appkey;
        DWORD dw;

        if (!m_allowproperties)
                return;

        // NEW (R3) PREFERENCES ALGORITHM
        // The user's prefs are only saved if the user is allowed to override
        // the machine-local settings specified for them.  Otherwise, the
        // properties entry on the tray icon menu will be greyed out.

        // GET THE CORRECT KEY TO READ FROM

        // Have we loaded user settings, or system settings?
        if (m_usersettings) {
                // Verify that we know who is logged on
                char username[UNLEN+1];
                if (!upsService::CurrentUser((char *)&username, sizeof(username)))
                        return;
                if (strcmp(username, "") == 0)
                        return;

                // Try to get the per-user, global registry key for Apcupsd
                if (RegCreateKeyEx(HKEY_CURRENT_USER,
                        Apcupsd_REGISTRY_KEY,
                        0, REG_NONE, REG_OPTION_NON_VOLATILE,
                        KEY_WRITE | KEY_READ, NULL, &appkey, &dw) != ERROR_SUCCESS)
                        return;
        } else {
                // Try to get the default local registry key for Apcupsd
                HKEY hkLocal;
                if (RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                        Apcupsd_REGISTRY_KEY,
                        0, REG_NONE, REG_OPTION_NON_VOLATILE,
                        KEY_READ, NULL, &hkLocal, &dw) != ERROR_SUCCESS) {
                        MessageBox(NULL, "MB1", "Apcupsd", MB_OK);
                        return;
                }
                if (RegCreateKeyEx(hkLocal,
                        "Default",
                        0, REG_NONE, REG_OPTION_NON_VOLATILE,
                        KEY_WRITE | KEY_READ, NULL, &appkey, &dw) != ERROR_SUCCESS) {
                        RegCloseKey(hkLocal);
                        return;
                }
                RegCloseKey(hkLocal);
        }

        // SAVE PER-USER PREFS IF ALLOWED
        SaveUserPrefs(appkey);

        RegCloseKey(appkey);

        // Close the user registry hive, to allow it to unload if reqd
        RegCloseKey(HKEY_CURRENT_USER);
}

void
upsProperties::SaveUserPrefs(HKEY appkey)
{
        // SAVE THE PER USER PREFS
//      log.Print(LL_INTINFO, upsLOG("saving current settings to registry\n"));

}

#endif /* properties_implemented */
