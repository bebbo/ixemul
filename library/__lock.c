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
 *  __lock.c,v 1.1.1.1 1994/04/04 04:30:10 amiga Exp
 *
 *  __lock.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:10  amiga
 * Initial CVS check in.
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

#define _KERNEL
#include <string.h>
#include "ixemul.h"

struct lock_vec {
  int    mode;
  int    resolve_last;
};

static int
__lock_func (struct lockinfo *info, struct lock_vec *lv, int *error)
{
  struct StandardPacket *sp = &info->sp;

  sp->sp_Pkt.dp_Type = ACTION_LOCATE_OBJECT;
  sp->sp_Pkt.dp_Arg1 = info->parent_lock;
  sp->sp_Pkt.dp_Arg2 = info->bstr;
  sp->sp_Pkt.dp_Arg3 = lv->mode;

  PutPacket (info->handler, sp);
  __wait_sync_packet (sp);
  info->result = sp->sp_Pkt.dp_Res1;

  *error = info->result <= 0;
  
  /* continue if we failed because of symlink - reference ? */
  return lv->resolve_last;
}

BPTR __lock (char *name, int mode)
{
  struct lock_vec lv;

  lv.mode = mode;
  lv.resolve_last = 1;
  
  return __plock (name, __lock_func, &lv);
}

BPTR __llock (char *name, int mode)
{
  struct lock_vec lv;

  lv.mode = mode;
  lv.resolve_last = 0;

  return __plock (name, __lock_func, &lv);
}
