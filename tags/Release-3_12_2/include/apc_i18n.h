/*
 * apc_i18n.h
 *
 * Internationalization header
 *
 * Original file: fetchmail-4.7.4/i18n.h
 *
 */

/*
 *  Copyright (C) 1999-2000 Riccardo Facchetti <riccardo@master.oasi.gpa.it>
 *  Copyright (C) 1996-99 Andre M. Hedrick <andre@suse.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of version 2 of the GNU General
 * Public License as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#ifdef ENABLE_NLS

# undef __OPTIMIZE__
# include <libintl.h>
# define _(String) gettext((String))
# define N_(String) (String)

#else

# ifndef HAVE_AIX_OS
#  define _(String) (String)
#  define N_(String) (String)
# else
   /* 
    * When using pthreads, AIX 5.1 (possible other versions?) occasionally 
    * dies in log_event(). Eliminating the parens appears to fix it.
    */
#  define _(String) String
#  define N_(String) String
# endif

#endif
