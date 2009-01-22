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
 *  __mclose.c,v 1.1.1.1 1994/04/04 04:30:10 amiga Exp
 *
 *  __mclose.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:10  amiga
 * Initial CVS check in.
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

int
__mclose(struct file *f)
{
  ix_lock_base ();
  
  f->f_count--;
  if (f->f_count == 0)
    {
      if (!(f->f_flags & FEXTOPEN) && f->f_name) 
	{
	  KPRINTF (("f->f_name (%s) ", f->f_name));
	  kfree (f->f_name);
	}
      KPRINTF (("f->f_mf.mf_buffer "));
      kfree (f->f_mf.mf_buffer);

      ffree(f);
    }

  ix_unlock_base ();
 
  return 0;
}
