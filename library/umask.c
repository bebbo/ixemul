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
 *  umask.c,v 1.1.1.1 1994/04/04 04:30:37 amiga Exp
 *
 *  umask.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:37  amiga
 * Initial CVS check in.
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"
#include "multiuser.h"

mode_t
umask (mode_t mode)
{
  int res;
  usetup;

  if (u.u_ixnetbase)
      return netcall(NET_umask, mode);

  if (mode & ~0777)
    {
      errno = EINVAL;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return -1;
    }

  res = u.u_cmask;
  u.u_cmask = mode;

  if (muBase) {
    long mumode = 0;

    if (mode & S_IXUSR)	mumode |= FIBF_EXECUTE | FIBF_SCRIPT;
    if (mode & S_IWUSR)	mumode |= FIBF_READ | FIBF_DELETE;
    if (mode & S_IRUSR)	mumode |= FIBF_READ;
#ifdef FIBF_GRP_EXECUTE
  /* FIBF_GRP_EXECUTE requires at least OS3 headers */
    if (mode & S_IXGRP)	mumode |= FIBF_GRP_EXECUTE;
    if (mode & S_IWGRP)	mumode |= FIBF_GRP_WRITE | FIBF_GRP_DELETE;
    if (mode & S_IRGRP)	mumode |= FIBF_GRP_READ;
    if (mode & S_IXOTH)	mumode |= FIBF_OTR_EXECUTE;
    if (mode & S_IWOTH)	mumode |= FIBF_OTR_WRITE | FIBF_OTR_DELETE;
    if (mode & S_IROTH)	mumode |= FIBF_OTR_READ;
    if (mode & S_ISTXT) mumode |= FIBF_HOLD;
    if (mode & S_ISUID) mumode |= muFIBF_SET_UID;
    if (mode & S_ISGID) mumode |= muFIBF_SET_GID;
#endif

    muSetDefProtection(muT_Task, (ULONG)FindTask(NULL),
	muT_DefProtection, mumode,
	TAG_DONE);
  }

  return res;
}
