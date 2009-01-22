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
 *  ioctl.c,v 1.1.1.1 1994/04/04 04:30:26 amiga Exp
 *
 *  ioctl.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:26  amiga
 * Initial CVS check in.
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

#define _KERNEL
#define ioctl _ioctl
#include "ixemul.h"
#include "kprintf.h"
#include <sys/ioctl.h>
#undef ioctl

int
ioctl (int fd, unsigned long cmd, void *arg)
{
  usetup;
  struct file *f = u.u_ofile[fd];

  /* if this is an open fd */
  if (fd >= 0 && fd < NOFILE && f)
      if (f->f_ioctl)
	{
	  /* split the ioctl-cmd into its subparts */
	  int arglen, inout;
	  
	  inout  = cmd >> 30; /* this ignores IOC_VOID */
	  arglen = (cmd >> 16) & IOCPARM_MASK;
	  
	  return (*f->f_ioctl)(f, cmd, inout, arglen, arg);
	}
      else
	{
	  errno = EIO;
	  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
	}
  else
    {
      errno = EBADF;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
    }

  return -1;
}
