/* multimon - prototypes and structures for multimon.c

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

void do_model (char *monhost, char *suffix);
void do_system (char *monhost, char *suffix);
void do_fulldata (char *monhost, char *suffix);
void do_status (char *monhost, char *suffix);
void do_upstemp (char *monhost, char *suffix);
void do_upstempc (char *monhost, char *suffix);
void do_upstempf (char *monhost, char *suffix);
void do_humidity (char *monhost, char *suffix);
void do_ambtemp (char *monhost, char *suffix);
void do_ambtempc (char *monhost, char *suffix);
void do_ambtempf (char *monhost, char *suffix);
void do_utility (char *monhost, char *suffix);

struct {
        char    *name;
        void    (*func)(char *monhost, char *suffix);
}       fields[] =
{
        { "MODEL",      do_model                },
        { "SYSTEM",     do_system               },
        { "STATUS",     do_status               },
        { "DATA",       do_fulldata             },
        { "UPSTEMP",    do_upstemp              },
        { "UPSTEMPC",   do_upstempc             },
        { "UPSTEMPF",   do_upstempf             },
        { "HUMIDITY",   do_humidity             },
        { "AMBTEMP",    do_ambtemp              },
        { "AMBTEMPC",   do_ambtempc             },
        { "AMBTEMPF",   do_ambtempf             },
        { "UTILITY",    do_utility              },
        { NULL,         (void(*)(char *, char *))(NULL)       }
};
