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
 *  __fioctl.c,v 1.1.1.1 1994/04/04 04:29:44 amiga Exp
 *
 *  __fioctl.c,v
 * Revision 1.1.1.1  1994/04/04  04:29:44  amiga
 * Initial CVS check in.
 *
 *  Revision 1.1  1992/05/17  21:00:51  mwild
 *  Initial revision
 *
 * Revision 1.1  1992/05/14  19:55:40  mwild
 * Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"
#include <sys/ioctl.h>

extern int __read(), __aread(), __write(), __awrite();

/* IOCTLs on files in general */

int
__fioctl(struct file *f, unsigned int cmd, unsigned int inout,
         unsigned int arglen, unsigned int arg)
{
  usetup;
  int omask;
  int result = 0;
  int err=errno;
  
  omask = syscall (SYS_sigsetmask, ~0);
  __get_file (f);

  switch (cmd)
    {
    case FIONREAD:
      {
	unsigned int *pt = (unsigned int *)arg;
	if (!IsInteractive(CTOBPTR(f->f_fh)))
	  {
	    int this_pos, eof_pos;
	    /* so this must be some file-oriented device, could be
	     * a pipe, could be a normal file. Lets try to seek to
	     * the eof, if we can, we know, how many characters there
	     * are to be read. */
	    this_pos = Seek(CTOBPTR(f->f_fh), 0, OFFSET_CURRENT);

	    if (this_pos >= 0)
	      {
		/* fine, the device seems at least to understand the
		 * Seek-Packet */
	        eof_pos = Seek(CTOBPTR(f->f_fh), 0, OFFSET_END);

		/* since this was a real seek, the device could have
		 * signaled an error, if it just can't seek .. */
		if (eof_pos >= 0)
		  {
		    *pt = eof_pos - this_pos;
		    Seek(CTOBPTR(f->f_fh), this_pos, OFFSET_BEGINNING);
		    result = 0;
		    goto ret;
		  }
	      }
	    /* well, since the device can't seek, AND it's not	    
	     * interactive, chances are bad, we ever will get at the
	     * right result, but we'll try nevertheless the WaitForChar
	     * Packet, it can only fail... */
	  }
	/* if the docs would all speak the same language... some
	 * say, that the timeout should be in 1/50s, others say
	 * its actually in micro/s.. who knows.. */
	*pt = WaitForChar(CTOBPTR(f->f_fh), 0) != 0;
	result = 0;
        goto ret;
      }

    case FIONBIO:
      {
	/* that's probably the most inefficient part of the whole
	 * library... */
	result = f->f_flags & FNDELAY ? 1 : 0;
	if (*(unsigned int *)arg)
	  f->f_flags |= FNDELAY;
	else
	  f->f_flags &= ~FNDELAY;
	/* I didn't find it documented in a manpage, but I assume, we
	 * should return the former state, not just zero.. */
	goto ret;
      }

    case FIOASYNC:
      {
	/* DOESN'T WORK YET */

	int flags = *(unsigned long*)arg;
	result = f->f_flags & FASYNC ? 1 : 0;
	if (flags)
	  f->f_flags |= FASYNC;
	else
	  f->f_flags &= ~FASYNC;

	/* ATTENTION: have to call some function here in the future !!! */

	/* I didn't find it documented in a manpage, but I assume, we
	 * should return the former state, not just zero.. */
	goto ret;
      }

    case FIOCLEX:
    case FIONCLEX:
    case FIOSETOWN:
    case FIOGETOWN:
      /* this is no error, but nevertheless we don't take any actions.. */      
      result = 0; goto ret;
    }

ret:
    __release_file (f);
    syscall (SYS_sigsetmask, omask);
    errno = err;
    KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
    return result;
}
