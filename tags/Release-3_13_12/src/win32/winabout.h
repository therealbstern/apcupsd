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
// Copyright (2000) Kern E. Sibbald
//

#include <windows.h>
#include <stdio.h>
#include <process.h>


// upsAbout

// Object implementing the About dialog for WinUPS.

class upsAbout;

#if (!defined(_win_upsABOUT))
#define _win_upsABOUT

// The upsAbout class itself
class upsAbout
{
public:
        // Constructor/destructor
        upsAbout();
        ~upsAbout();

        // Initialisation
        BOOL Init();

        // The dialog box window proc
        static BOOL CALLBACK DialogProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

        // General
        void Show(BOOL show);

        // Implementation
        BOOL m_dlgvisible;
};

#endif // _win_upsPROPERTIES
