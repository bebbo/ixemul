/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1991, 1992  Markus M. Wild
 *  Portions Copyright (C) 1994 Rafael W. Luebbert
 *  Portions Copyright (C) 1996 Jeff Shepherd
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
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"
#include <stdlib.h>

int
fchown(int fd, uid_t uid, gid_t gid)
{
  usetup;
  struct file *f = u.u_ofile[fd];
  struct stat stb;

  if (fd >= 0 && fd < NOFILE && f)
    {
      if (syscall (SYS_fstat, fd, &stb) == -1) return -1;
      if (stb.st_mode & 0600)
        {
          if (syscall (SYS_fchmod, fd, stb.st_mode & ~0176000) == -1) return -1;
        }

      if (uid == (uid_t)(-1)) uid = stb.st_uid;
      if (gid == (gid_t)(-1)) gid = stb.st_gid;

      f->f_stb.st_uid = uid;
      f->f_stb.st_gid = gid;
      f->f_stb_dirty |= FSDF_OWNER;
      return 0;
    }

  errno = EBADF;
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
  return -1;
}
