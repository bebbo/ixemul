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
 *  access.c,v 1.1.1.1 1994/04/04 04:30:14 amiga Exp
 *
 *  access.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:14  amiga
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
access(const char *name, int mode)
{
  struct stat stb;
  int res;
  usetup;

  res = -1;
  if (syscall (SYS_stat, name, &stb) == 0)
    {
      /* this doesn't check, whether the volume on which this files
       * lies itself permits the desired actions, on a readonly-fs
       * the write-permission on the file doesn't count.. HOW should
       * I get at this information?? */
       if (!muBase || stb.st_uid == (uid_t)syscall(SYS_geteuid) ||
	   (uid_t)syscall(SYS_geteuid) == 0)
	 {
	   res = (((stb.st_mode >> 6) & 07) & mode) == mode ? 0 : -1;
	 }
       else
	 {
	   int i, ngroups, gidset[NGROUPS_MAX];
 
	   ngroups = syscall(SYS_getgroups, NGROUPS_MAX, gidset);
 
	   for (i = 0; i < ngroups; i++)
	     if (stb.st_gid == (gid_t)gidset[i])
	       break;
 
	   if (i != ngroups)
	     res = (((stb.st_mode >> 3) & 07) & mode) == mode ? 0 : -1;
	   else
	     res = ((stb.st_mode & 07) & mode) == mode ? 0 : -1;
	 }

      if (res) errno = EACCES;
    }
  else
    {
      errno = ENOENT;  
    }
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));

  return res;
}
