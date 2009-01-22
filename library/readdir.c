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
 *  readdir.c,v 1.1.1.1 1994/04/04 04:30:51 amiga Exp
 *
 *  readdir.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:51  amiga
 * Initial CVS check in.
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include <sys/stat.h>
#include <dirent.h>

struct dirent *
readdir (DIR *dp)
{
  u_int l;

  if (! dp || dp->dd_fd < 0) return 0;

  if (syscall (SYS_read, dp->dd_fd,
	       & dp->dd_ent.d_fileno, 4) <= 0)
    return 0;

  if (syscall (SYS_read, dp->dd_fd, &l, 4) <= 0)
    return 0;

  dp->dd_ent.d_type = (l ? DT_DIR : DT_REG);

  if (syscall (SYS_read, dp->dd_fd, &l, 4) <= 0)
    return 0;

  /* since we don't know, whether the name is zero padded, provide a dummy */
  dp->dd_ent.d_name[l] = 0;
  if (syscall (SYS_read, dp->dd_fd, dp->dd_ent.d_name, l) <= 0)
    return 0;

  dp->dd_ent.d_namlen = l;
  /* if the name was stored without terminating zero, account for the additional
   * byte in the name length field */
  if (dp->dd_ent.d_name [l-1])
    dp->dd_ent.d_namlen ++;
  dp->dd_ent.d_reclen = l + 8;
  dp->dd_ent.d_namlen--;        /* don't count zero pad */
  return & dp->dd_ent;
}
