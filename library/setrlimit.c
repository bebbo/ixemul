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
 *  setrlimit.c,v 1.1.1.1 1994/04/04 04:30:34 amiga Exp
 *
 *  setrlimit.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:34  amiga
 * Initial CVS check in.
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#include <sys/time.h>
#include <sys/resource.h>

/* just - after some validity-checks - always return OK.. it would
 * be suicidial if it really changed ANY parameters.. */

int
setrlimit(int resource, struct rlimit *rlp)
{
  usetup;

  if (resource < RLIMIT_CPU || resource > RLIMIT_RSS || !rlp)
    {
      errno = EINVAL;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return -1;
    }

  return 0;
}
