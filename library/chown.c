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
__chown_func (struct lockinfo *info, int ugid, int *error)
{
  struct StandardPacket *sp = &info->sp;

  sp->sp_Pkt.dp_Type = ACTION_SET_OWNER;
  sp->sp_Pkt.dp_Arg1 = 0;
  sp->sp_Pkt.dp_Arg2 = info->parent_lock;
  sp->sp_Pkt.dp_Arg3 = info->bstr;
  sp->sp_Pkt.dp_Arg4 = ugid;

  PutPacket (info->handler, sp);
  __wait_sync_packet (sp);

  info->result = sp->sp_Pkt.dp_Res1;
  *error = info->result != -1;
  return 1;
}

int
chown(const char *name, uid_t uid, gid_t gid)
{
  int user;
  int result;
  struct stat stb;
  usetup;

  if (syscall (SYS_lstat, name, &stb) == -1) return -1;
  if (stb.st_mode & 0600)
    {
      if (syscall (SYS_chmod, name, stb.st_mode & ~0176000) == -1) return -1;
    }

  if (uid == (uid_t)(-1)) uid = stb.st_uid;
  if (gid == (gid_t)(-1)) gid = stb.st_gid;

  user = (__unix2amigaid(uid) << 16) | __unix2amigaid(gid);

  result = __plock(name,__chown_func, (void *)user);
  if (result == 0)
    {
      errno = __ioerr_to_errno (IoErr ());
      
      // If device doesn't support this action, then just let it succeed
      if (errno == ENODEV)
        return 0;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
    }

  return result ? 0 : -1;
}
