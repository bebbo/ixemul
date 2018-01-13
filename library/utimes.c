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
 *  utimes.c,v 1.1.1.1 1994/04/04 04:30:38 amiga Exp
 *
 *  utimes.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:38  amiga
 * Initial CVS check in.
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

static int
__utimes_func (struct lockinfo *info, struct DateStamp *ds, int *error)
{
  struct StandardPacket *sp = &info->sp;

  sp->sp_Pkt.dp_Type = ACTION_SET_DATE;
  sp->sp_Pkt.dp_Arg1 = 0;
  sp->sp_Pkt.dp_Arg2 = info->parent_lock;
  sp->sp_Pkt.dp_Arg3 = info->bstr;
  sp->sp_Pkt.dp_Arg4 = (long)ds;
  
  PutPacket (info->handler, sp);
  __wait_sync_packet (sp);

  info->result = sp->sp_Pkt.dp_Res1;

  *error = info->result != -1;

  /* we want to set the date of the file pointed to, not that of the link */
  return 1;
}


int
utimes(char *name, struct timeval *tvp)
{
  struct DateStamp *ds;
  struct timeval tv;
  int result;
  usetup;
  
  if (!tvp)
    /* in this case, fill in the current time */
    syscall(SYS_gettimeofday, &tv, 0);
  else
    /* we have to ignore the value of "accesstime" and only
     * look at "modificationtime", that's tvp[1] */
    tv = tvp[1];

  tv.tv_sec -= ix_get_gmt_offset();
  /* long-alignment here could be overkill, but with DOS you never know.. */
  ds = alloca (sizeof (struct DateStamp) + 2);
  ds = LONG_ALIGN (ds);

  ds->ds_Days   = tv.tv_sec / (24 * 60 * 60);
  /* subtract UNIX/AmigaOS time offset */
  ds->ds_Days  -= OFFSET_FROM_1970;
  tv.tv_sec    %= 24 * 60 * 60;
  ds->ds_Minute = tv.tv_sec / 60;
  tv.tv_sec    %= 60;
  ds->ds_Tick   = (tv.tv_sec * TICKS_PER_SECOND +
	           (tv.tv_usec * TICKS_PER_SECOND)/1000000);

  result = __plock (name, __utimes_func, ds);

  if (! result) errno = __ioerr_to_errno (IoErr ());
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));

  return result ? 0 : -1;
}
