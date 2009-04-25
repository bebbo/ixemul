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
 *  close.c,v 1.1.1.1 1994/04/04 04:30:16 amiga Exp
 *
 *  close.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:16  amiga
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
close (int fd)
{
  usetup;  
  struct file **fp;
  if (u.u_parent_userdata)fp = &u.u_parent_userdata->u_ofile[fd];
  else fp = &u.u_ofile[fd];
  /*if (u.u_parent_userdata)
  {
	  u_ptr = u.u_parent_userdata;
  }*/
  //struct file **fp = &u.u_ofile[fd];

KPRINTF(("close(%d)\n", fd));
  /* if this is an open fd */
  if (fd >= 0 && fd < NOFILE && *fp)
    {
	  
      struct file *f;
      /* skip close if execve() atexit-handler close() call and UF_EXCLOSE
         is clear. - Piru */
      if ((u.p_flag & EXECVECLOSE) && u.u_ofile[fd] && (u.u_pofile[fd] & UF_EXCLOSE) == 0)
	{
	   KPRINTF(("close(%d) skipped\n", fd));
	   return 0;
	}
      /* free the slot in the user table */
      f = *fp;
      *fp = 0;
      if (f->f_close)
	return (*f->f_close)(f);
      else
	{
	  errno = EIO;
	  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
	}
    }
  else
    {
      errno = EBADF;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
    }
  return -1;
}
