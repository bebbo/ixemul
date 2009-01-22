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
 *  $Id: rename.c,v 1.1.1.1 2005/03/15 15:57:09 laire Exp $
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#if 0

struct rename_vec {
  char *source_name;
  BPTR target_dir_lock;
  BSTR target_name;
};


static int
__rename_func (struct lockinfo *info, struct rename_vec *rv, int *error)
{
  struct StandardPacket *sp = &info->sp;

  if (SameLock(info->parent_lock, rv->target_dir_lock) == LOCK_DIFFERENT)
    {
      sp->sp_Pkt.dp_Res1 = 0;
      sp->sp_Pkt.dp_Res2 = ERROR_RENAME_ACROSS_DEVICES;
    }
  else
    {
      sp->sp_Pkt.dp_Type = ACTION_RENAME_OBJECT;
      sp->sp_Pkt.dp_Arg1 = info->parent_lock;
      sp->sp_Pkt.dp_Arg2 = info->bstr;
      sp->sp_Pkt.dp_Arg3 = rv->target_dir_lock;
      sp->sp_Pkt.dp_Arg4 = rv->target_name;

      PutPacket (info->handler, sp);
      __wait_sync_packet (sp);
    }

  info->result = sp->sp_Pkt.dp_Res1;
  *error = info->result != -1;

  /* retry if necessary */
  return 1;
}

static int
__get_target_data (struct lockinfo *info, struct rename_vec *rv, int *error)
{
  struct StandardPacket *sp = &info->sp;

  rv->target_dir_lock = info->parent_lock;
  rv->target_name = info->bstr;

  info->result = __plock (rv->source_name, __rename_func, rv);
  sp->sp_Pkt.dp_Res2 = IoErr();

  *error = info->result != -1;

  /* don't retry */
  return 0;
}

int
rename(const char *from, const char *to)
{
  int res;
  struct rename_vec rv;
  usetup;

  rv.source_name = (char *)from;

  res = __plock ((char *)to, __get_target_data, &rv);

  if (res == 0 && IoErr() == ERROR_OBJECT_EXISTS)
    {
      syscall (SYS_unlink, to);
      res = __plock ((char *)to, __get_target_data, &rv);
    }

  if (!res)
    errno = __ioerr_to_errno (IoErr());

  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
  return res ? 0 : -1;
}

#else

char *ix_to_ados(char *, const char *);
char *check_root(char *);

int
rename(const char *from, const char *to)
{
  int res;
  usetup;
  int omask;
  char *buf1 = alloca(strlen(from) + 4);
  char *buf2 = alloca(strlen(to) + 4);
  LONG err;

  buf1 = ix_to_ados(buf1, from);
  buf2 = ix_to_ados(buf2, to);

  omask = syscall (SYS_sigsetmask, -1);

  res = Rename(buf1, buf2);

  if (!res)
    {
      err = IoErr();
      if (err == ERROR_OBJECT_NOT_FOUND)
        {
	  char *new_buf1 = check_root(buf1);
	  char *new_buf2 = check_root(buf2);
	  int retry = 0;
	  if (new_buf1 && *new_buf1)
	    {
	      buf1 = new_buf1;
	      retry = 1;
	    }
	  if (new_buf2 && *new_buf2)
	    {
	      buf2 = new_buf2;
	      retry = 1;
	    }
	  if (retry)
	    {
	      res = Rename(buf1, buf2);
	      if (!res)
		err = IoErr();
	    }
	}
    }

  if (!res && err == ERROR_OBJECT_EXISTS)
    {
      res = DeleteFile(buf2);
      if (!res && IoErr() == ERROR_DELETE_PROTECTED)
	{
	  res = SetProtection(buf2, 0);
	  if (res)
	    {
	      res = DeleteFile(buf2);
	    }
	}
      if (res)
	{
	  res = Rename(buf1, buf2);
	}
    }

  syscall (SYS_sigsetmask, omask);

  if (!res)
    errno = __ioerr_to_errno (IoErr());

  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
  return res ? 0 : -1;
}

#endif
