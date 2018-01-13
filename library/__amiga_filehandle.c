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
 *  __amiga_filehandle.c,v 1.1.1.1 1994/04/04 04:30:57 amiga Exp
 *
 *  __amiga_filehandle.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:57  amiga
 * Initial CVS check in.
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

/* Please use this function very restrictively!! It is meant only to be
   able to use dos.library functions on ixemul.library file descriptors.
   Be prepared to get a 0 return, as not every existing descriptor has
   to be implemented as an AmigaOS file */

BPTR
ix_filehandle (int fd)
{
  usetup;
  struct file *fp;

  if ((unsigned)fd >= NOFILE || (fp = u.u_ofile[fd]) == NULL)
    {
      errno = EBADF;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return 0;
    }

  if (fp->f_type == DTYPE_FILE)
    return CTOBPTR (fp->f_fh);
  else
    return 0;
}

