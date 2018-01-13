/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1991, 1992  Markus M. Wild
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  select.h,v 1.1.1.1 1994/04/04 04:30:08 amiga Exp
 *
 *  select.h,v
 * Revision 1.1.1.1  1994/04/04  04:30:08  amiga
 * Initial CVS check in.
 *
 *  Revision 1.2  1993/11/05  22:14:53  mwild
 *  new code for inet.library support
 *
 * Revision 1.1  1992/05/14  20:36:14  mwild
 * Initial revision
 *
 */

#ifndef __SELECT_H__
#define __SELECT_H__

/*
 * definitions of arguments for select() implementation.
 *
 * Each file type that supports select has to look like this:
 *
 *  __..select (f, select_cmd, io_mode)
 *
 */

/*
 * possible values for select_cmd
 */

/* init a select, perhaps initiate a timeout call that can be checked with
 * SELECT_CHECK
 * Return a signal-mask to be used in Wait() that indicates that this file
 * has changed mode.
 */
#define SELCMD_PREPARE	1

/* return whether io_mode on this file won't block */
#define SELCMD_CHECK	2

/* same thing, but for a poll */
#define SELCMD_POLL	10

/*
 * possible values for io_mode
 */

#define SELMODE_IN	0
#define SELMODE_OUT	1
#define SELMODE_EXC	2

#define SELPKT_IN_USE(f) ((f)->f_select_sp.sp_Pkt.dp_Port)

#endif
