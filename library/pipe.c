/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1991, 1992  Markus M. Wild
 *  Portions Copyright (C) 1994 Rafael W. Luebbert
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
 *  $Id: pipe.c,v 1.4 1994/06/19 15:14:19 rluebbert Exp $
 *
 *  $Log: pipe.c,v $
 *  Revision 1.4  1994/06/19  15:14:19  rluebbert
 *  *** empty log message ***
 *
 *  Revision 1.2  1992/07/04  19:21:08  mwild
 *  (finally..) fix the bug which could cause pipe readers/writers to deadlock
 *
 * Revision 1.1  1992/05/14  19:55:40  mwild
 * Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#include <sys/ioctl.h>
#include <string.h>
#include "select.h"
#include "unp.h"

/* information for the temporary implementation of pipes.
   PIPE: has the big disadvantage that it blocks in the most unpleasent
   situations, and doesn't send SIGPIPE to processes that write on
   readerless pipes. Unacceptable for this library ;-)) */

static int __pclose();

int
pipe (int pv[2])
{
  struct file *f1, *f2;
  struct sock_stream *ss;
  int res, err, omask;
  usetup;
  
  omask = syscall (SYS_sigsetmask, ~0);
  res = -1; 
  err = EMFILE;
  if ((ss = init_stream()))
    {
      if (! falloc (&f1, pv))
        {
          if (! falloc (&f2, pv+1))
            {
	      f1->f_ss = ss;
	      f1->f_stb.st_mode = 0666 | S_IFCHR;
	      f1->f_stb.st_size = UNIX_SOCKET_SIZE;
	      f1->f_stb.st_blksize = 512;
	      f1->f_flags  = FREAD;
	      f1->f_type   = DTYPE_PIPE;
	      f1->f_read   = stream_read;
	      f1->f_write  = 0;
	      f1->f_ioctl  = unp_ioctl;
	      f1->f_close  = __pclose;
	      f1->f_select = unp_select;

	      f2->f_ss = ss;
	      f2->f_stb.st_mode = 0666 | S_IFCHR;
	      f2->f_stb.st_size = UNIX_SOCKET_SIZE;
	      f2->f_stb.st_blksize = 512;
	      f2->f_flags  = FWRITE;
	      f2->f_type   = DTYPE_PIPE;
	      f2->f_read   = 0;
	      f2->f_write  = stream_write;
	      f2->f_ioctl  = unp_ioctl;
	      f2->f_close  = __pclose;
	      f2->f_select = unp_select;

	      res = err =0;
	      goto ret;
	    }
	  f1->f_count = 0;
	}

      kfree (ss);
    }

ret:
  syscall (SYS_sigsetmask, omask);

  errno = err;
  KPRINTF_DISABLED (("&errno = %lx, errno = %ld\n", &errno, errno));
  return res;
}

static int
__pclose (struct file *f)
{
  struct sock_stream *ss = f->f_ss;
  usetup;

  Forbid();

  f->f_count--;

  if (f->f_count == 0)
    {
      if (f->f_read)
        ss->flags |= UNF_NO_READER;
      else
	ss->flags |= UNF_NO_WRITER;
	
      if ((ss->flags & (UNF_NO_READER|UNF_NO_WRITER)) ==
	  	       (UNF_NO_READER|UNF_NO_WRITER))
	kfree (ss);
      else
        {
          if (ss->task)
            Signal(ss->task, 1UL << u.u_pipe_sig);
          ix_wakeup ((u_int)ss);
        }
    }

  Permit();

  return 0;
}
