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
 *  ftruncate.c,v 1.1.1.1 1994/04/04 04:30:18 amiga Exp
 *
 *  ftruncate.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:18  amiga
 * Initial CVS check in.
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#ifndef ACTION_SET_FILE_SIZE
#define ACTION_SET_FILE_SIZE 1022
#endif

#define USE_SETFILESIZE_SEEK_WORKAROUND 1

int
ftruncate (int fd, off_t len)
{
  usetup;
  struct file *f;
  if (u.u_parent_userdata)f = u.u_parent_userdata->u_ofile[fd];
  else f = u.u_ofile[fd]; 
  int err, res;
  int omask;

  errno = EBADF;
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));

  /* if this is an open fd */
  if (fd >= 0 && fd < NOFILE && f)
    {
      if (f->f_type == DTYPE_FILE)
	{
	  #if USE_SETFILESIZE_SEEK_WORKAROUND
	  BPTR fh;
	  LONG opos;
	  #endif

	  if (HANDLER_NIL (f))
	    {
	      errno = EINVAL;
	      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
	      return -1;
	    }

	  err = 0;
	  omask = syscall (SYS_sigsetmask, ~0);
	  __get_file (f);
	  #if USE_SETFILESIZE_SEEK_WORKAROUND
	  fh = CTOBPTR(f->f_fh);
	  opos = Seek(fh, 0, OFFSET_BEGINNING);
	  res = SetFileSize(fh, len, OFFSET_BEGINNING);
	  if (res == -1)
	    err = __ioerr_to_errno(IoErr());
	  else if (res < opos)
	    opos = res;
	  if (opos != -1)
	    Seek(fh, opos, OFFSET_BEGINNING);

	  if (res != -1)
	  #else
	  res = SetFileSize(CTOBPTR(f->f_fh), len, OFFSET_BEGINNING);
	  if (res == -1)
	    err = __ioerr_to_errno(IoErr());
	  else
	  #endif
	  {
	    /* 15-Jul-2003 bugfix: 0 indicates success! used to return
	     * whatever SetFileSize returned (new filesize) - Piru
	     */
	    res = 0;

	    err = 0;
	  }
	  __release_file (f);
	  syscall (SYS_sigsetmask, omask);
	  errno = err;
	  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
	  return res;
	}
      else
	{
	  errno = ESPIPE;
	  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
	}
    }

  return -1;
}
