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
 *  __swrite.c,v 1.1.1.1 1994/04/04 04:30:12 amiga Exp
 *
 *  __swrite.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:12  amiga
 * Initial CVS check in.
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#ifdef USE_VMEM
#define PUBBUFSIZE   16384
char pub_buf[PUBBUFSIZE];
#  ifndef MEMF_SWAP
#     define MEMF_SWAP   (1L<<11)
#  endif
#endif

static int __do_sync_write(struct file *f, char *buf, int len)
{
  usetup;
  int err=errno, res = 0;
  int omask;

  omask = syscall (SYS_sigsetmask, ~0);
  __get_file (f);

  if (len > 0)
    {
      /* full append-mode means, before each write do an explicit
       * seek to eof */
      if (f->f_flags & FAPPEND)
	Seek(CTOBPTR(f->f_fh), 0, OFFSET_END);
#ifdef USE_VMEM
      if (TypeOfMem(buf) & MEMF_SWAP)
	{
	  while (len)
	    {
	      int l = len > PUBBUFSIZE ? PUBBUFSIZE : len;
	      int r;

	      memcpy(pub_buf, buf, l);
	      r = Write(CTOBPTR(f->f_fh), pub_buf, l);
	      if (r == -1)
		{
		  res = -1;
		  break;
                }
	      buf += r;
	      len -= r;
	      res += r;
	      if (r < l)
		break;
            }
	}
      else
	res = Write(CTOBPTR(f->f_fh), buf, len);
#else
      res = Write(CTOBPTR(f->f_fh), buf, len);
#endif
      if (res == -1) err = __ioerr_to_errno(IoErr());
    }
  __release_file (f);
  syscall (SYS_sigsetmask, omask);
  errno = err;
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
  return res;
}

int __write(struct file *f, char *buf, int len)
{
  char *p;
  int l, bytes, res = 0, tmp;

  if (HANDLER_NIL(f))
    return len;

  if (!IsInteractive(CTOBPTR(f->f_fh))) /* if not interactive */
    return __do_sync_write(f, buf, len);

  /* write the buffer line by line, otherwise the user isn't able to stop
     the console output until the whole buffer was flushed to the console */
  /* also check the f->f_ttyflags */

#define TTY_NLCR_ENABLE (IXTTY_RAW |  IXTTY_OPOST | IXTTY_ONLCR)

  for (p = buf, l = 0; l < len; p += bytes, l += bytes)
    {
      if (p[0] == '\n')
	{
	  bytes = 1;
	  if ((f->f_ttyflags & TTY_NLCR_ENABLE) == TTY_NLCR_ENABLE)
	    {
	      tmp = __do_sync_write(f, "\n\r", 2);
	      if (tmp == -1)
		return tmp;
	      tmp = (tmp == 2 ? 1 : 0);
	      res += tmp;
	      if (tmp != bytes)
		return res;
	      continue;
	    }

	  /* jDc: ONLCR unset = \n must not work like \n\r, replace it with INDEX instead */
	  if ((!(f->f_ttyflags & IXTTY_SPECIAL)) && ((!(f->f_ttyflags & IXTTY_ONLCR)) || (!(f->f_ttyflags & IXTTY_OPOST))))
	    {
	      tmp = __do_sync_write(f, "\204", 1);
	      if (tmp == -1)
		return tmp;
	      res += tmp;
	      if (tmp != bytes)
		return res;
	      continue;
	    }

	}
      else
	for (bytes = 0; l + bytes < len && p[bytes] != '\n' && bytes < 256; bytes++) ;
      if (bytes)
	{
	  tmp = __do_sync_write(f, p, bytes);
	  if (tmp == -1)
	    return tmp;
	  res += tmp;
	  if (tmp != bytes)
	    return res;
	}
    }
  return res;
}
