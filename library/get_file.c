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
 *  get_file.c,v 1.1.1.1 1994/04/04 04:30:58 amiga Exp
 *
 *  get_file.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:58  amiga
 * Initial CVS check in.
 *
 *  Revision 1.2  1992/08/09  20:45:35  amiga
 *  add new sleep parameter
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"

int __get_file (struct file *f)
{
  int res = 0;

  Forbid ();
  for (;;)
    {
      if (!(f->f_sync_flags & FSFF_LOCKED))
        {
          f->f_sync_flags |= FSFF_LOCKED;
          /* got it ! */
          break;
	}
      f->f_sync_flags |= FSFF_WANTLOCK;
      if (ix_sleep((caddr_t)&f->f_sync_flags, "get_file") < 0)	/* error in sleep, might be INTR */
        {
	  res = -1;
          break;
        }
      /* have to always recheck whether we really got the lock */
    }
  Permit ();      
  return res;
}


void __release_file (struct file *f)
{
  Forbid ();
  if (f->f_sync_flags & FSFF_WANTLOCK)
    ix_wakeup ((u_int)& f->f_sync_flags);
    
  f->f_sync_flags &= ~(FSFF_WANTLOCK|FSFF_LOCKED);
  Permit ();
}
