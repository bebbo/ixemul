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
 *  ftime.c,v 1.1.1.1 1994/04/04 04:30:17 amiga Exp
 *
 *  ftime.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:17  amiga
 * Initial CVS check in.
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"

#include <sys/time.h>
#include <sys/timeb.h>

/* this seems to be a relict of pre-gettimeofday times.. */

int
ftime (struct timeb *tb)
{
  struct timeval tv;
  struct timezone tz;
  
  if (syscall (SYS_gettimeofday, &tv, &tz) == 0)
    {
      tb->time = tv.tv_sec;
      tb->millitm = tv.tv_usec / 1000;
      tb->timezone = tz.tz_minuteswest;
      tb->dstflag = tz.tz_dsttime;
      return 0;
    }
  return -1;
}
