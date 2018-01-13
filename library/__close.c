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
 *  $Id: __close.c,v 1.2 1994/06/19 15:18:48 rluebbert Exp $
 *
 *  $Log: __close.c,v $
 *  Revision 1.2  1994/06/19  15:18:48  rluebbert
 *  *** empty log message ***
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"
#include "select.h"

#ifndef ACTION_STACK
// From rexx/rexxio.h
#define ACTION_STACK 2002L	       /* stack a line			*/
#endif

int
__close(struct file *f)
{
  if (f->f_count == 1)
    {
      usetup;

      if (!memcmp(f->f_name, "/fifo/pty", 9) && SELPKT_IN_USE(f))
        {
          SendPacket3(f, __srwport, ACTION_STACK, f->f_fh->fh_Arg1, (int)"\n", 1);
          __wait_sync_packet(&f->f_sp);
        }
      __wait_select_packet((struct StandardPacket *)&f->f_select_sp);
    }

  ix_lock_base ();

  f->f_count--;

  KPRINTF (("__close: closing file $%lx, f_count now %ld.\n", f, f->f_count));
  if (f->f_count > 0) goto unlock;

  if (!(f->f_flags & FEXTOPEN))
    {
      __Close (CTOBPTR (f->f_fh));

      if (!memcmp(f->f_name, "/fifo/pty", 9))
        {
	  int i = (f->f_name[9] - 'p') * 16 + f->f_name[10] - (f->f_name[10] >= 'a' ? 'a' - 10 : '0');
      	  char mask = (f->f_name[17] == 'm' ? IX_PTY_MASTER : IX_PTY_SLAVE);
	  
	  ix.ix_ptys[i] &= ~(mask & IX_PTY_OPEN);
	  ix.ix_ptys[i] |= mask & IX_PTY_CLOSE;
	  if (!(ix.ix_ptys[i] & IX_PTY_OPEN))
	    ix.ix_ptys[i] = 0;  /* both slave and master are closed, so free this pty */
        }
    }

  if (f->f_flags & FUNLINK)
    {
      syscall (SYS_unlink, f->f_name);
    }
  else
    {
      /* if we have to set some modes, do it now */
      if (f->f_stb_dirty & FSDF_MODE)
        syscall (SYS_chmod, f->f_name, f->f_stb.st_mode);
    
      /* if we have to set some ids, do it now */
      if (f->f_stb_dirty & FSDF_OWNER)
        syscall (SYS_chown, f->f_name, f->f_stb.st_uid, f->f_stb.st_gid);

      /* see if we have to set the timestamp ourselves */
      if (f->f_write && (f->f_stb_dirty & FSDF_UTIME))
        syscall (SYS_utimes, f->f_name, NULL);
    }

  /* NOTE: the FEXTNAME flag tells us that the name in the
   * file structure wasn't allocated by kmalloc(), so we
   * shouldn't kfree() it either. */
  if (!(f->f_flags & FEXTNAME) && f->f_name) 
    {
      KPRINTF (("f->f_name(%s) ", f->f_name));
      kfree (f->f_name);
    }

unlock:
  ix_unlock_base ();

  return 0;
}
