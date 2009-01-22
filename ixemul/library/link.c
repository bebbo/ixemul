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
 *  link.c,v 1.1.1.1 1994/04/04 04:30:28 amiga Exp
 *
 *  link.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:28  amiga
 * Initial CVS check in.
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"

#ifndef LINK_HARD
#define LINK_HARD 0
#endif

int
link (char *old, char *new)
{
  BPTR lock;
  int res = -1;
  int omask;

  /* since we obtain a lock which we have to free later, block signals */
  omask = syscall (SYS_sigsetmask, ~0);
  if ((lock = __lock (old, ACCESS_READ)))
    {
      res = __make_link (new, lock, LINK_HARD);
      __unlock (lock);
    }
  syscall (SYS_sigsetmask, omask);

  return res;
}
