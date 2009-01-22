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
 *  __fstat.c,v 1.1.1.1 1994/04/04 04:30:08 amiga Exp
 *
 *  __fstat.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:08  amiga
 * Initial CVS check in.
 *
 *  Revision 1.2  1993/11/05  21:49:59  mw
 *  "grp/oth-perms,
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

#define _KERNEL
#include <string.h>
#include <stdio.h>
#include "ixemul.h"
#include "kprintf.h"

#ifndef ACTION_EXAMINE_FH
#define ACTION_EXAMINE_FH 1034
#endif

/************************************************************************/
/*                                                                      */
/*    fstat() function.                                                 */
/*                                                                      */
/************************************************************************/

int
__fstat(struct file *f)
{
  long len, pos, bytesperblock;
  struct FileInfoBlock *fib;
  struct InfoData *info;
  BPTR lock;
  int omask;
  BOOL res;
  int is_interactive = IsInteractive(CTOBPTR(f->f_fh));
  struct stat *st = &f->f_stb;
  usetup;

  omask = syscall (SYS_sigsetmask, ~0);
  __get_file (f);
  if (!(f->f_stb_dirty & FSDF_OWNER))
    {
      st->st_uid = -2;
      st->st_gid = -2;
    }

  /* take special care of NIL:, /dev/null and friends ;-) */
  if (HANDLER_NIL(f))
    {
      st->st_mode = S_IFCHR | 0777;
      st->st_nlink = 1;
      st->st_blksize = 512;
      st->st_blocks = 0;
      goto end;
    }

  info = alloca (sizeof (*info) + 2);
  info = LONG_ALIGN (info);
  fib  = alloca (sizeof (*fib) + 2);
  fib  = LONG_ALIGN (fib);
  fib->fib_OwnerUID = fib->fib_OwnerGID = 0;

  /* we now have two possibilities.. either the filesystem understands the
   * new ACTION_EXAMINE_FH packet, or we have to do some guesses at fields.. */
  if (ExamineFH(CTOBPTR(f->f_fh), fib))
    {
      int mode = fill_stat_mode(st, fib);       /* see stat.c */

      /* read the uid/gid data */
      if (!(f->f_stb_dirty & FSDF_OWNER))
	{
	  st->st_uid = __amiga2unixid(fib->fib_OwnerUID);
	  st->st_gid = __amiga2unixid(fib->fib_OwnerGID);
	}

      if (!(f->f_stb_dirty & FSDF_MODE))
	st->st_mode = mode;
      else
	st->st_mode |= (mode & 0170000);
  
      /* ARGLLLLL !!!
	 Some (newer, hi Bill Hawes ;-)) handlers support EXAMINE_FH, but
	 don't know yet about ST_PIPEFILE. So console windows claim they're
	 plain files... Until this problem is fixed in a majority of
	 handlers, do an explicit SEEK here to find those fakers.. */
      if (is_interactive || Seek(CTOBPTR(f->f_fh), 0, OFFSET_CURRENT) == -1)
	st->st_mode = (st->st_mode & ~S_IFREG) | S_IFCHR;

      /* some kind of a default-size for directories... */
      st->st_size = fib->fib_DirEntryType < 0 ? fib->fib_Size : 1024; 
      st->st_handler = (long)f->f_fh->fh_Type;
      st->st_dev = (dev_t)st->st_handler; /* trunc to 16 bit */
      st->st_ino = get_unique_id(NULL, f->f_fh);
      st->st_atime =
      st->st_ctime =
      st->st_mtime = OFFSET_FROM_1970 * 24 * 3600 + ix_get_gmt_offset() +
				  fib->fib_Date.ds_Days * 24 * 60 * 60 +
				  fib->fib_Date.ds_Minute * 60 +
				  fib->fib_Date.ds_Tick/TICKS_PER_SECOND;
      /* in a try to count the blocks used for filemanagement, we add one for
       * the fileheader. Note, that this is wrong for large files, where there
       * are some extension-blocks as well */
      st->st_blocks = fib->fib_NumBlocks + 1;
    }
  else
    {
      /* ATTENTION: see lseek.c for Bugs in Seek() and ACTION_SEEK ! */
      /* seek to EOF */
      pos = (is_interactive ? -1 : Seek(CTOBPTR(f->f_fh), 0, OFFSET_END));
      if (pos >= 0)
	len = Seek(CTOBPTR(f->f_fh), pos, OFFSET_BEGINNING);
      else
	len = 0;

      bzero (st, sizeof(struct stat));

      if (!(f->f_stb_dirty & FSDF_OWNER))
	{
	  st->st_uid = syscall(SYS_geteuid);
	  st->st_gid = syscall(SYS_getegid);
	}

      if (!(f->f_stb_dirty & FSDF_MODE))
	{
	  st->st_mode = (len >= 0 && !is_interactive) ?
			S_IFREG | (0666 & ~u.u_cmask) : S_IFCHR | 0777;
	}

      st->st_handler = (long)f->f_fh->fh_Type;
      /* the following is a limited try to support programs, that assume that
       * st_dev is always valid */
      st->st_dev = (dev_t)(long)st->st_handler; /* truncate to 16 bit */
      /* yet another kludge.. if we call this with different descriptors, we
       * should get different inode numbers.. grmpf.. */
      st->st_ino = (ino_t)~(long)(f->f_fh);
      st->st_nlink = 1; /* for now no problem.. 2.0... */
      /* len could be -1, if the device doesn't allow seeking.. */
      st->st_size = len >= 0 ? len : 0;
      st->st_atime =  
      st->st_mtime =
      st->st_ctime = syscall (SYS_time, 0);
    }

  /* try to find out block size of device, hey postman, it's packet-time
   * again:-)) */

  /* clear the info-structure. Since this packet is used by the console
   * handler to transmit the window pointer, it actually answers the
   * request, but doesn't set the not used fields to 0.. this gives HUGE
   * block lengths :-)) */
  bzero (info, sizeof(*info));
  res = 0;
  bytesperblock = 0;
  if (S_ISREG(st->st_mode))
  {
    lock = DupLockFromFH(CTOBPTR(f->f_fh));
    if (lock)
      {
	res = Info(lock, (void *)info);
	UnLock(lock);
      }
    if (res && info->id_BytesPerBlock)
      bytesperblock = info->id_BytesPerBlock;
  }

  st->st_blksize = 0;
  if (res && bytesperblock)
    {
      st->st_blksize = bytesperblock;
      if (!st->st_blocks) 
	st->st_blocks = ((st->st_size + bytesperblock - 1) / bytesperblock);
      else
	st->st_blocks = (st->st_blocks * bytesperblock) / 512;
    }
  if (!st->st_blksize) 
    {
      st->st_blksize = 512;
      st->st_blocks = (st->st_size + 511) >> 9;
    }

end:
  __release_file (f);
  syscall (SYS_sigsetmask, omask);
  return 0;
}
