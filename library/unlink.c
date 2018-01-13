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
 *  unlink.c,v 1.1.1.1 1994/04/04 04:30:37 amiga Exp
 *
 *  unlink.c,v
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
#include <string.h>

static int
__delete_func (struct lockinfo *info, void *dummy, int *error)
{
  struct StandardPacket *sp = &info->sp;

  sp->sp_Pkt.dp_Type = ACTION_DELETE_OBJECT;
  sp->sp_Pkt.dp_Arg1 = info->parent_lock;
  sp->sp_Pkt.dp_Arg2 = info->bstr;

  PutPacket (info->handler, sp);
  __wait_sync_packet (sp);

  info->result = sp->sp_Pkt.dp_Res1;

  *error = info->result != -1;
 
  /* stop if we failed because of symlink - reference */
  return 0;
}

int
unlink (char *name)
{
  int err;
  struct file *fp;
  struct stat st;
  usetup;

  /* we walk thru our filetab to see, whether WE have this file
   * open, and set its unlink-flag, if not, return the error */
  ix_lock_base();
  for (fp = ix.ix_file_tab; fp < ix.ix_fileNFILE; fp++)
    if (fp->f_count && fp->f_name && !strcmp(fp->f_name, name))
      {
        fp->f_flags |= FUNLINK;
        ix_unlock_base();
        return 0;
      }
  ix_unlock_base();

  /* first try to normally delete the file, if we get an
   * 'delete_protected' error, we'll try another way out.. */
  if (__plock (name, __delete_func, 0))
    return 0;

  /* so there was an error.. */
  err = IoErr();
  if (err == ERROR_DELETE_PROTECTED)
    {
      if (!syscall(SYS_chmod, name, st.st_mode | S_IWUSR)) 
        if (__plock (name, __delete_func, 0))
          err = 0;
        else
          err = IoErr();
    }

  if (err)
    {
      errno = __ioerr_to_errno(err);
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return -1;
    }

  return 0;
}
