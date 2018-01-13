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
 *  write.c,v 1.1.1.1 1994/04/04 04:30:38 amiga Exp
 *
 *  write.c,v
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

int
write (int fd, char *buf, int len)
{
  usetup;
  struct file *f;
  int result;

  if (fd < 0 || fd >= NOFILE || !(f = u.u_ofile[fd]))
    errno_return(EBADF, -1);

  if (f->f_write == NULL)
    errno_return(EIO, -1);
  
  result = (*f->f_write)(f, buf, len);

  if (result)
    f->f_stb_dirty &= ~FSDF_UTIME;

  return result;
}
