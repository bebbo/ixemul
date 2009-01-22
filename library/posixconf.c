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
 *  posixconf.c,v 1.1.1.1 1994/04/04 04:30:59 amiga Exp
 *
 *  posixconf.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:59  amiga
 * Initial CVS check in.
 *
 *  Revision 1.2  1992/07/04  22:08:39  mwild
 *  machlimits is now limits...
 *
 *  Revision 1.1  1992/05/22  01:50:03  mwild
 *  Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"
#include <unistd.h>
#include <time.h>
#include <sys/syslimits.h>
#include <machine/limits.h>

long
sysconf (int name)
{
  usetup;

  switch (name)
    {
    case _SC_ARG_MAX:
      return ARG_MAX;
      
    case _SC_CHILD_MAX:
      return CHILD_MAX;
      
    case _SC_CLK_TCK:
      return CLK_TCK;
      
    case _SC_NGROUPS_MAX:
      return NGROUPS_MAX;
      
    case _SC_OPEN_MAX:
      return FD_SETSIZE; /* Was OPEN_MAX. See comments before getdtablesize()
			    why this was changed. */

    case _SC_JOB_CONTROL:
      return _POSIX_JOB_CONTROL;
    
    case _SC_SAVED_IDS:
      return _POSIX_SAVED_IDS;

    case _SC_VERSION:
      return _POSIX_VERSION;
      
    default:
      errno = EINVAL;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return -1;
    }
}


long
fpathconf (int fd, int name)
{
  usetup;
  struct file *f = u.u_ofile[fd];

  if ((unsigned)fd >= NOFILE || !f)
    {
      errno = EBADF;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return -1;
    }

  switch (name)
    {
    case _PC_LINK_MAX:
      return LINK_MAX;
      
    case _PC_MAX_CANON:
      return MAX_CANON;
      
    case _PC_MAX_INPUT:
      return MAX_INPUT;
      
    case _PC_NAME_MAX:
      return NAME_MAX;  /* or 32 on AmigaOS? What about NFS ? */
      
    case _PC_PATH_MAX:
      return PATH_MAX;
      
    case _PC_PIPE_BUF:
      return PIPE_BUF;
      
    case _PC_CHOWN_RESTRICTED:
      return _POSIX_CHOWN_RESTRICTED;
      
    case _PC_NO_TRUNC:
      return _POSIX_NO_TRUNC;
    
    case _PC_VDISABLE:
      return _POSIX_VDISABLE;

    default:
      errno = EINVAL;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return -1;
    }
}


long
pathconf (const char *file, int name)
{
  int fd = syscall(SYS_open, file, 0);
  usetup;
  
  if (fd >= 0)
    {
      int err, res;
      res = fpathconf (fd, name);
      err = errno;
      close (fd);
      errno = err;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return res;
    }

  return -1;
}
