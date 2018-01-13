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
 *  opendir.c,v 1.1.1.1 1994/04/04 04:30:50 amiga Exp
 *
 *  opendir.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:50  amiga
 * Initial CVS check in.
 *
 *  Revision 1.2  1992/08/09  20:59:35  amiga
 *  add cast
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"
#include <sys/stat.h>
#include <dirent.h>

extern void *kmalloc ();

DIR *
opendir (const char *name)
{
  DIR *d;
  int err;
  struct stat stb;
  usetup;

  if (!(d = (DIR *) syscall (SYS_malloc, sizeof (*d)))) return 0;

  if ((d->dd_fd = syscall (SYS_open, name, 0)) >= 0)
    {
      if (syscall (SYS_fstat, d->dd_fd, &stb) == 0)
	{
	  if (! S_ISDIR (stb.st_mode))
	    {
              syscall (SYS_close, d->dd_fd);
	      syscall (SYS_free, d);
	      errno = ENOTDIR;
	      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
	      return 0;
	    }

	  return d;
	}
	
      err = errno;

      syscall (SYS_close, d->dd_fd);
    }
  else
    err = errno;

  syscall (SYS_free, d);
  errno = err;
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
  return 0;
}
