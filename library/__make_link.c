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
 *  $Id: __make_link.c,v 1.1.1.1 2005/03/15 15:57:09 laire Exp $
 */

#define _KERNEL
#include "ixemul.h"

#ifndef ACTION_MAKE_LINK
#define ACTION_MAKE_LINK 1021
#endif

#if 0

struct link_vec {
  BPTR target;
  int  link_mode;
};

static int
__link_func (struct lockinfo *info, struct link_vec *lv, int *error)
{
  struct StandardPacket *sp = &info->sp;

  sp->sp_Pkt.dp_Type = ACTION_MAKE_LINK;
  sp->sp_Pkt.dp_Arg1 = info->parent_lock;
  sp->sp_Pkt.dp_Arg2 = info->bstr;
  sp->sp_Pkt.dp_Arg3 = lv->target;
  sp->sp_Pkt.dp_Arg4 = lv->link_mode;

  PutPacket (info->handler, sp);
  __wait_sync_packet (sp);
  info->result = sp->sp_Pkt.dp_Res1;

  *error = info->result != -1;

  /* shouldn't be possible to get the ERR_SOFT_LINK here.. */
  return 0;
}

int
__make_link (char *path, BPTR targ, int mode)
{
  struct link_vec lv;

  lv.target = targ;
  lv.link_mode = mode;

  return __plock (path, __link_func, &lv);
}

#else

char *ix_to_ados(char *, const char *);

int
__make_link (char *path, BPTR targ, int mode)
{
  usetup;
  int omask;
  char *buf = alloca(strlen(path) + 3);
  LONG result;

  buf = ix_to_ados(buf, path);

  omask = syscall (SYS_sigsetmask, ~0);
  result = MakeLink(buf, targ, mode);
  syscall (SYS_sigsetmask, omask);

  if (result == 0)
    {
      errno = __ioerr_to_errno (IoErr ());
    }

  return result ? 0 : -1;
}

#endif
