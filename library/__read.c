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
 *  __read.c,v 1.1.1.1 1994/04/04 04:30:12 amiga Exp
 *
 *  __read.c,v
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

/*
 * Starting with ixemul 42.0 we switched from asynchronous to synchronous
 * reads.  Apparently the ADOS CON handler uses the mp_SigTask field of the
 * MsgPort struct of an ACTION_READ package to find the Task which it should
 * signal when the user presses Ctrl-C.  However, async reads use a special
 * port where the mp_SigTask field points to an Interrupt struct.  The CON
 * handler seems to either use that address as the new Task pointer (I hope
 * not) or disable Ctrl-C signalling entirely.  I never noticed this until it
 * was pointed out to me, since I always use KingCON. KingCON seems to handle
 * Ctrl-C differently. Just to be sure I also changed the ACTION_WAIT_CHAR
 * to synchronous mode.
 *
 * Starting with 43.1 I no longer use async writes at all. The overhead in
 * handling async writes turned out to be bigger than the gain.
 */

#ifdef USE_VMEM
#define PUBBUFSIZE   16384
extern char pub_buf[];
#  ifndef MEMF_SWAP
#     define MEMF_SWAP   (1L<<11)
#  endif
#endif


int __read(struct file *f, char *buf, int len)
{
  usetup;
  int err = errno, res = 0;
  int omask;
  char *orig_buf;

  /* always return EOF */
  if (HANDLER_NIL(f)) return 0;

  omask = syscall (SYS_sigsetmask, ~0);
  __get_file (f);
  
  if (f->f_ttyflags & IXTTY_PKT)
    {
      len--;
      *buf++ = 0;
    }

  orig_buf = buf;

  if (len > 0)
    {
      /* if interactive and don't block */
      if (IsInteractive(CTOBPTR(f->f_fh)) && (f->f_flags & FNDELAY))
	{
	  /* shudder.. this is the most inefficient part of the whole
	   * library, but I don't know of another way of doing it... */
	  while (res < len)
	    {
	      char c;

	      if (!WaitForChar(CTOBPTR(f->f_fh), 0))
	      {
		if (!res)
		{
		  res = -1;
		  err = EAGAIN;
		}
		break;
	      }
	      if (Read(CTOBPTR(f->f_fh), &c, 1) != 1)
		{
		  err = __ioerr_to_errno(IoErr());
		  /* if there really was no character to read, we should
		   * have escaped already at the 'if WaitForChar' line */
		  res = -1;
		  break;
		}
	      *buf++ = c;
	      res++;
	    }
	}         
      else
	{
#ifdef USE_VMEM
	  if (TypeOfMem(buf) & MEMF_SWAP)
	    {
	      while (len)
		{
		  int l = len > PUBBUFSIZE ? PUBBUFSIZE : len;
		  int r;

		  r = Read(CTOBPTR(f->f_fh), pub_buf, l);
		  if (r == -1)
		    {
		      res = -1;
		      break;
		    }
		  memcpy(buf, pub_buf, r);
		  buf += r;
		  len -= r;
		  res += r;
		  if (r < l)
		    break;
		}
	    }
	  else
	    res = Read(CTOBPTR(f->f_fh), buf, len);
#else
	  res = Read(CTOBPTR(f->f_fh), buf, len);
#endif
	  if (res == -1)
	    err = __ioerr_to_errno(IoErr());
	}
   }

  __release_file (f);
  syscall (SYS_sigsetmask, omask);
  errno = err;
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
  
  /*  If we read something from a file, and the file is opened RAW (e.g., a stdin
   *  stream in RAW mode), then we replace all '\r' characters with '\n' characters.
   *  This is a courtesy for those programs that turn off ECHO for the input of a
   *  password. Turning off ECHO sets the input mode to RAW, and in RAW mode the
   *  Enter key inserts a '\r' into the stream. However, the stdio functions expect
   *  a '\n' to end a line, so we patch the input buffer.
   *
   * Actually, the application sets the CR->NL or NL->CR translation
   * through the termios interface. Amiga cooked mode already performs the
   * normal CR->NL conversion, so we only do that transformation when in
   * raw mode.
   */
  if (res > 0 && 
      ((f->f_ttyflags & IXTTY_INLCR) ||
       ((f->f_ttyflags & IXTTY_RAW) && (f->f_ttyflags & IXTTY_ICRNL))))
    {
      int i;
      char match, subst;

      if (f->f_ttyflags & IXTTY_INLCR)
	{
	  match = '\n';
	  subst = '\r';
	}
      else
	{
	  match = '\r';
	  subst = '\n';
	}
      for (i = 0; i < res; i++)
	if (orig_buf[i] == match)
	  orig_buf[i] = subst;
    }
  if (res > 0 && (f->f_ttyflags & IXTTY_PKT))
    res++;
  return res;
}
