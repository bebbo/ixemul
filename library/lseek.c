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
 *  lseek.c,v 1.1.1.1 1994/04/04 04:30:29 amiga Exp
 *
 *  lseek.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:29  amiga
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

static inline int
__extend_file (struct file *f, int add_to_eof, int *err)
{
  int res = 0;
  int buf_size, written;
  char *buf;

  /* now you know perhaps, that starting with OS2.0 there's a packet that
     sets the size of a file (used in [f]truncate()). Too bad, that the 
     following statement from the autodocs render this packet useless... :

     `` Do NOT count on any specific values to be in the extended area. ''
   
     Since **IX requires the new area to be zero'd, we can just as well
     write zeros without the use of the new packet ;-(( */
  
      
  buf_size = add_to_eof > 32*1024 ? 32*1024 : add_to_eof;
  while (! (buf = (char *) kmalloc (buf_size)) && buf_size)
    buf_size >>= 1;
  if (buf)
    {
      bzero (buf, buf_size);

      for (written = 0; written < add_to_eof; )
        {
	  res = Write(CTOBPTR(f->f_fh), buf, buf_size);
          if (res < 0)
            {
              *err = __ioerr_to_errno (IoErr());
              break;
            }
          written += res;
          buf_size = add_to_eof - written > buf_size ? 
		       buf_size : add_to_eof - written;
	}
      kfree (buf);
    }
  else
    {
      *err = ENOMEM;
      res = -1;
    }

  if (res >= 0)
    {
      Seek(CTOBPTR(f->f_fh), 0, OFFSET_END);
      res = Seek(CTOBPTR(f->f_fh), 0, OFFSET_END);
      *err = __ioerr_to_errno (IoErr());
    }

  return res;
}


off_t
lseek (int fd, off_t off, int dir)
{
  usetup;
  struct file *f = u.u_ofile[fd];
  int omask;
  int err, res;
  int previous_pos = 0, shouldbe_pos;

  /* if this is an open fd */
  if (fd >= 0 && fd < NOFILE && f)
    {
      if (f->f_type == DTYPE_FILE)
	{
          if (HANDLER_NIL(f))
	    {
	      /* This is always possible with /dev/null */
	      if (off == 0)
	        return 0;
	      errno = ESPIPE;
	      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
	      return -1;
	    }

	  omask = syscall (SYS_sigsetmask, ~0);
	  __get_file (f);

	  /* there's a subtle difference between Unix lseek() and AmigaOS
	   * Seek(): lseek() returns the *current* position of the `pointer',
	   * Seek() returns the *previous* position. So in some cases, we
	   * have to do another Seek() to find out where we are.
	   * Thanks Mike for pointing me at this! */

	  switch (dir)
	    {
	    case SEEK_SET:
	      /* previous doesn't matter, don't need to seek */
	      previous_pos = 0;
	      break;

	    case SEEK_CUR:
	      /* first find out current position */
	      previous_pos = Seek(CTOBPTR(f->f_fh), 0, OFFSET_CURRENT);
	      if (previous_pos == -1)
	        {
	          err = __ioerr_to_errno(IoErr());
	        }
	      break;
	      
	    case SEEK_END:
	      /* first find out end position (have to do twice.. argl) */
	      Seek(CTOBPTR(f->f_fh), 0, OFFSET_END);
	      previous_pos = Seek(CTOBPTR(f->f_fh), 0, OFFSET_CURRENT);
	      if (previous_pos == -1)
	        {
	          err = __ioerr_to_errno(IoErr());
	        }
	      break;
	    }
	  
	  shouldbe_pos = previous_pos + off;
	  if (shouldbe_pos < 0)
	    {
	      /* that way we make sure that invalid seek errors later result
	         from seeking past eof, so we can enlarge the file */

	      err = EINVAL;
	      res = -1;
	    }
	  else if (previous_pos >= 0)
	    {
	      res = Seek(CTOBPTR(f->f_fh), off, dir - 1);
	      if (res == -1 && IoErr() == ERROR_SEEK_ERROR)
	        {
	          /* in this case, assume the user wanted to seek past eof.
	             Thus get the current eof position, so that we can
	             tell __extend_file how much to enlarge the file */
		  res = Seek(CTOBPTR(f->f_fh), 0, OFFSET_END);
	        }
	  
	      if (res == -1)
	        {
	          err = __ioerr_to_errno(IoErr());
	        }
	      else
	        {
	          err = 0;

	          if (previous_pos != shouldbe_pos)
		    {
		      res = Seek(CTOBPTR(f->f_fh), 0, OFFSET_CURRENT);
		      if (res == -1)
		        {
	                  err = __ioerr_to_errno(IoErr());
	                }

	              if (res >= 0 && res < shouldbe_pos && (f->f_flags & FWRITE))
	                {
	                  /* extend the file... */
		          res = __extend_file (f, shouldbe_pos - res, &err);
		        }
		    }
		  else
		    res = shouldbe_pos;
	        }
	    }
	  else
	    res = -1;
	  
	  __release_file (f);
	  syscall (SYS_sigsetmask, omask);
	  errno = err;
	  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
	  return res;
	}
      else if (f->f_type == DTYPE_MEM)
	{
	  int real_off;
	  int old_off;

	  omask = syscall (SYS_sigsetmask, ~0);
	  __get_file (f);
	  old_off = f->f_mf.mf_offset;
	  
	  real_off = (dir == L_SET ? off : 
		      (dir == L_INCR ? 
		       old_off + off : f->f_stb.st_size + off));
	  if (real_off < 0) real_off = 0;
	  else if (real_off > f->f_stb.st_size) real_off = f->f_stb.st_size;
	  f->f_mf.mf_offset = real_off;
	  __release_file (f);
	  syscall (SYS_sigsetmask, omask);
	  return old_off;
	}
      else
	{
	  errno = ESPIPE;
	  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
	}
    }
  else
    {
      errno = EBADF;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
    }

  return -1;
}

