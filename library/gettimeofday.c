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
 *  gettimeofday.c,v 1.1.1.1 1994/04/04 04:30:24 amiga Exp
 *
 *  gettimeofday.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:24  amiga
 * Initial CVS check in.
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#include <devices/timer.h>
#include <sys/time.h>

#define __time_req (u.u_time_req)

int
gettimeofday(struct timeval *tp, struct timezone *tzp)
{
  int omask;
  usetup;

  if (tp)
    {
      __time_req->tr_node.io_Command = TR_GETSYSTIME;
      omask = syscall (SYS_sigsetmask, ~0);
      DoIO((struct IORequest *)__time_req);
      syscall (SYS_sigsetmask, omask);
      *tp = __time_req->tr_time;
      /* add the offset from UNIX to AmigaOS time system */
      tp->tv_sec += OFFSET_FROM_1970 * 24 * 3600 + ix_get_gmt_offset();
    }
  if (tzp)
    {
      /* since AmigaOS doesn't support timezones, we always return
       * GMT...
       */
       
      tzp->tz_minuteswest = 0;
      tzp->tz_dsttime = DST_NONE;
    }
  return 0;
}
