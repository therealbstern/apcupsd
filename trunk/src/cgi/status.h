/* status.h - translation of status abbreviations to descriptions

   Copyright (C) 1999  Russell Kroll <rkroll@exploits.org>

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.              
*/

struct {
        char    *name;
        char    *desc;
        int     severity;
}       stattab[] =
{
        { "OFF",        "OFF",                  1       },
        { "OL",         "ONLINE",               0       },
        { "OB",         "ON BATTERY",           2       },
        { "LB",         "LOW BATTERY",          2       },
        { "RB",         "REPLACE BATTERY",      2       },
        { "OVER",       "OVERLOAD",             2       },
        { "TRIM",       "VOLTAGE TRIM",         1       },
        { "BOOST",      "VOLTAGE BOOST",        1       },
        { "CAL",        "CALIBRATION",          1       },
        { "CL",         "COMM LOST",            2       },
        { "SD",         "SHUTTING DOWN",        2       },
        { "SLAVE",      "SLAVE",                0       },
        { NULL,         NULL,                   0       }
};
