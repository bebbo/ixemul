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
 *  kern_descrip.c,v 1.1.1.1 1994/04/04 04:30:42 amiga Exp
 *
 *  kern_descrip.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:42  amiga
 * Initial CVS check in.
 *
 *  Revision 1.2  1992/07/04  19:19:04  mwild
 *  add support for F_INTERNALIZE/F_EXTERNALIZE, which are used by IXPIPE
 *  and execve().
 *
 * Revision 1.1  1992/05/14  19:55:40  mwild
 * Initial revision
 *
 *
 *  Since the code originated from Berkeley, the following copyright
 *  header applies as well. The code has been changed, it's not the
 *  original Berkeley code!
 */

/*
 * Copyright (c) 1982, 1986, 1989 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution is only permitted until one year after the first shipment
 * of 4.4BSD by the Regents.  Otherwise, redistribution and use in source and
 * binary forms are permitted provided that: (1) source distributions retain
 * this entire copyright notice and comment, and (2) distributions including
 * binaries display the following acknowledgement:  This product includes
 * software developed by the University of California, Berkeley and its
 * contributors'' in the documentation or other materials provided with the
 * distribution and in all advertising materials mentioning features or use
 * of this software.  Neither the name of the University nor the names of
 * its contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)kern_descrip.c	7.16 (Berkeley) 6/28/90
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <string.h>

/*
 * Descriptor management.
 */

/*
 * System calls on descriptors.
 */
/* ARGSUSED */
int
getdtablesize (void)
{
  /* This was NOFILE, but getdtablesize is also used to determine the number
     number of filehandles select() should test. And if you pass a value
     larger than FD_SETSIZE to select(), you'll get fireworks. And it is very
     hard to discover why your program won't work. I know: I've been through
     this process twice now, so I thought I'd better fix this here by just
     returning FD_SETSIZE. 
     
     I could also change the FD_SETSIZE macro to the value of NOFILE (changing
     256 to 512), but that would be a waste of memory, and besides, in the
     future the number of filehandles might become dynamic, so it wouldn't work
     in any case. */
  return FD_SETSIZE;
}

/*
 * Duplicate a file descriptor.
 */
/* ARGSUSED */
int
dup (unsigned i)
{
  struct file *fp;
  int fd, error;
  usetup;

  if (i >= NOFILE || (fp = u.u_ofile[i]) == NULL)
    errno_return(EBADF, -1);

  if (fp->f_type == DTYPE_SOCKET)
    {
      struct file *fp2;
      int fd2;
      int err;

      if ((err = falloc (&fp2, &fd2)))
        errno_return(err, -1);

      fp2->f_so = netcall(NET__dup, fp);
      if (fp2->f_so == -1)
      {
        /* free the allocated fd */
        u.u_ofile[fd2] = 0;
        fp2->f_count = 0;
        return -1;
      }
      fp2->f_socket_domain = fp->f_socket_domain;
      fp2->f_socket_type = fp->f_socket_type;
      fp2->f_socket_protocol = fp->f_socket_protocol;
      _set_socket_params(fp2, fp->f_socket_domain, fp->f_socket_type, fp->f_socket_protocol);
      return fd2;
    }

  if ((error = ufalloc (0, &fd)))
    errno_return(error, -1);

  u.u_ofile[fd] = fp;
  u.u_pofile[fd] = u.u_pofile[i] &~ UF_EXCLOSE;
  fp->f_count++;

  if (fd > u.u_lastfile)
    u.u_lastfile = fd;

  return fd;
}

/*
 * Duplicate a file descriptor to a particular value.
 */
/* ARGSUSED */
int
dup2 (unsigned i, unsigned j)
{
  register struct file *fp;
  int old_err;
  usetup;

  if (i >= NOFILE || (fp = u.u_ofile[i]) == NULL)
    errno_return(EBADF, -1);

  if (j < 0 || j >= NOFILE)
    errno_return(EBADF, -1);

  if (i == j) return (int)j;

  ix_lock_base ();

  old_err = errno;
  if (u.u_ofile[j] && u.u_ofile[j]->f_close)
    u.u_ofile[j]->f_close (u.u_ofile[j]);

  u.u_ofile[j] = fp;
  u.u_pofile[j] = u.u_pofile[i] &~ UF_EXCLOSE;
  fp->f_count++;
  if (j > u.u_lastfile)
    u.u_lastfile = j;
  
  ix_unlock_base ();

  /*
   * dup2() must suceed even though the close had an error.
   */
  errno = old_err;	/* XXX */
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
  return (int)j;
}

/*
 * The file control system call.
 */
/* ARGSUSED */
int
fcntl(int fdes, int cmd, int arg)
{
  register struct file *fp;
  register char *pop;
  int i, error;
  usetup;

  /* F_INTERNALIZE doesn't need a valid descriptor. Check for this first */
  if (cmd == F_INTERNALIZE)
    {
      if ((error = ufalloc (0, &i)))
        {
          errno = error;
	  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
          return -1;
        }
      fp = (struct file *) arg;
      u.u_ofile[i] = fp;
      u.u_pofile[i] = 0;
      fp->f_count++;
      return i;
    }


  if (fdes >= NOFILE || (fp = u.u_ofile[fdes]) == NULL)
    {
      errno = EBADF;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return -1;
    }

  pop = &u.u_pofile[fdes];
  switch (cmd) 
    {
      case F_DUPFD:
	if (arg < 0 || arg >= NOFILE)
	  {
	    errno = EINVAL; 
	    KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
	    return -1;
	  }
	if ((error = ufalloc (arg, &i)))
	  {
	    errno = error;
	    KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
	    return -1;
	  }
	ix_lock_base ();
	u.u_ofile[i] = fp;
	u.u_pofile[i] = *pop &~ UF_EXCLOSE;
	fp->f_count++;
	ix_unlock_base ();
	if (i > u.u_lastfile) u.u_lastfile = i;
	return i;

      case F_GETFD:
	return *pop & 1;

      case F_SETFD:
	*pop = (*pop &~ 1) | (arg & 1);
	return 0;

      case F_GETFL:
	return OFLAGS(fp->f_flags);

      case F_SETFL:
	fp->f_flags &= ~FCNTLFLAGS;
	fp->f_flags |= FFLAGS(arg) & FCNTLFLAGS;
	if (fp->f_type == DTYPE_SOCKET)
	{
	  arg = (fp->f_flags & FASYNC) ? 1 : 0;
	  ioctl(fdes, FIOASYNC, &arg);
	  arg = (fp->f_flags & FNONBLOCK) ? 1 : 0;
	  ioctl(fdes, FIONBIO, &arg);
	}
	return 0;

      case F_EXTERNALIZE:
        return (int)fp;

      default:
        errno = EINVAL;
	KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
	return -1;
    }
	/* NOTREACHED */
}

/*
 * Return status information about a file descriptor.
 */
/* ARGSUSED */
int
fstat (int fdes, struct stat *sb)
{
  struct file *fp;
  usetup;

  if (fdes >= NOFILE || (fp = u.u_ofile[fdes]) == NULL)
    {
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      errno = EBADF;
      return -1;
    }

  /* I found this code from the AmiTCP sdk. It *should* work with AS225 */
  if (fp->f_type == DTYPE_SOCKET && u.u_ixnetbase)
    return netcall(NET__fstat, fp, sb);

  /* if this file is writable, then it's quite probably that the stat buffer
     information stored at open time is no longer valid. So if the file really
     is a file, update that information */
  if ((fp->f_flags & FWRITE) && fp->f_type == DTYPE_FILE)
    __fstat (fp);

  *sb = fp->f_stb;
  return 0;
}

/*
 * Allocate a user file descriptor.
 */
int
ufalloc(int want, int *result)
{
  usetup;

  for (; want < NOFILE; want++) 
    {
      if (u.u_ofile[want] == NULL) 
        {
	  u.u_pofile[want] = 0;
	  if (want > u.u_lastfile) u.u_lastfile = want;
			
	  *result = want;
	  return 0;
	}
    }
  return EMFILE;
}

/*
 * Allocate a user file descriptor
 * and a file structure.
 * Initialize the descriptor
 * to point at the file structure.
 */
int
falloc(struct file **resultfp, int *resultfd)
{
  register struct file *fp;
  int error, i;
  usetup;

  if ((error = ufalloc(0, &i)))
    return (error);

  ix_lock_base ();

  if (ix.ix_lastf == 0)
    ix.ix_lastf = ix.ix_file_tab;

  for (fp = ix.ix_lastf; fp < ix.ix_fileNFILE; fp++)
    if (fp->f_count == 0)
      goto slot;

  for (fp = ix.ix_file_tab; fp < ix.ix_lastf; fp++)
    if (fp->f_count == 0)
      goto slot;

  /* YES I know.. it's not optimal, we should resize the table...
   * unfortunately all code accessing file structures will then have
   * to be changed as well, and this is a job for later improvement,
   * first goal is to get this baby working... */
  ix_warning("ixemul.library file table full!");
  error = ENFILE;
  goto do_ret;

slot:
  u.u_ofile[i] = fp;
  fp->f_stb_dirty = 0;
  fp->f_name = 0;
  fp->f_count = 1;
  fp->f_type = 0;		/* inexistant type ;-) */
  memset(&fp->f__fh, 0, sizeof(fp->f__fh));
  ix.ix_lastf = fp + 1;
  if (resultfp)
    *resultfp = fp;
  if (resultfd)
    *resultfd = i;

  error = 0;

do_ret:
  ix_unlock_base();

  return error;
}


/*
 * Apply an advisory lock on a file descriptor.
 */
/* ARGSUSED */
int
flock (int fdes, int how)
{
  return 0;	/* always succeed */
}

