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
 *  rename.c,v 1.1.1.1 1994/04/04 04:30:32 amiga Exp
 *
 *  rename.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:32  amiga
 * Initial CVS check in.
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

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
