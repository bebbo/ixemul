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
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#if 0
static int
__mkdir_func (struct lockinfo *info, void *dummy, int *error)
{
  struct StandardPacket *sp = &info->sp;

  sp->sp_Pkt.dp_Type = ACTION_CREATE_DIR;
  sp->sp_Pkt.dp_Arg1 = info->parent_lock;
  sp->sp_Pkt.dp_Arg2 = info->bstr;

  PutPacket (info->handler, sp);
  __wait_sync_packet (sp);
  info->result = sp->sp_Pkt.dp_Res1;

  *error = info->result == 0;

  /* continue if we failed because of symlink - reference */
  return 1;
}


int
mkdir (const char *path, mode_t perm)
{
  BPTR lock;
  int result = -1;
  int omask;
  usetup;

  /* protect the obtained lock, we have to free it later */
  omask = syscall (SYS_sigsetmask, ~0);
  lock = __plock (path, __mkdir_func, 0);

  if (lock)
    {
      __unlock (lock);
      result = syscall (SYS_chmod, path, (int)(perm & ~u.u_cmask));

      if (!muBase)
	{
	  uid_t uid = geteuid();
	  gid_t gid = getegid();

	  if (uid != (uid_t)(-2) || gid != (gid_t)(-2))
	    syscall (SYS_chown, path, uid, gid);
	}
    }
  syscall (SYS_sigsetmask, omask);

  errno = __ioerr_to_errno (IoErr ());
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
  return result;
}

#else

char *ix_to_ados(char *, const char *);
char *check_root(char *);

int
mkdir (const char *path, mode_t perm)
{
  BPTR lock;
  int result = -1;
  int omask;
  usetup;
  char *buf = alloca(strlen(path) + 4);
  LONG err;

  buf = ix_to_ados(buf, path);

  /* protect the obtained lock, we have to free it later */
  omask = syscall (SYS_sigsetmask, ~0);
  lock = CreateDir (buf);

  if (!lock)
    {
      err = IoErr();
      if (err == ERROR_OBJECT_NOT_FOUND)
	{
	  buf = check_root(buf);
	  if (buf && *buf)
	    {
	      lock = CreateDir (buf);
	      if (!lock)
		err = IoErr();
	    }
	}
    }

  if (lock)
    {
      UnLock (lock);
      result = syscall (SYS_chmod, path, (int)(perm & ~u.u_cmask));

      if (!muBase)
	{
	  uid_t uid = geteuid();
	  gid_t gid = getegid();

	  if (uid != (uid_t)(-2) || gid != (gid_t)(-2))
	    syscall (SYS_chown, path, uid, gid);
	}
    }
  syscall (SYS_sigsetmask, omask);

  errno = __ioerr_to_errno (err);
  KPRINTF (("mkdir(%s): &errno = %lx, errno = %ld\n", path, &errno, errno));
  return result;
}

#endif
