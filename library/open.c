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
 *  $Id: open.c,v 1.4 1994/06/19 15:14:07 rluebbert Exp $
 *
 *  $Log: open.c,v $
 *  Revision 1.4  1994/06/19  15:14:07  rluebbert
 *  *** empty log message ***
 *
 *  Revision 1.2  1992/07/28  00:32:04  mwild
 *  pass convert_dir the original signal mask, to check for pending signals
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#include <string.h>

/* standard functions.. could get overridden in a network environment */
extern int __ioctl(), __fselect(), __close();

/* "normal" functions, means do half-async writes & sync reads */
extern int __write(), __read(), __open();

/* incore functions */
extern int __mread(), __mclose(), __mselect();

static struct ix_mutex open_sem;

int
open(char *name, int mode, int perms)
{
  int fd;
  struct file *f;
  BPTR fh;
  int late_stat;
  int omask, error;
  char ptymask = 0;
  int ptyindex = 0;
  int amode = 0, i;
  usetup;

  if (name == NULL)     /* sanity check */
    return EACCES;
  mode = FFLAGS(mode);

  /* inhibit signals */
  omask = syscall (SYS_sigsetmask, ~0);

  error = falloc (&f, &fd);
  if (error)
    {
      syscall (SYS_sigsetmask, omask);
      errno = error;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return -1;
    }
  /* we now got the file, ie. since its count is > 0, no other process
   * will get it with falloc() */

  late_stat = 0;

  // The code between the stat() and the actual open() is critical
  ix_mutex_lock(&open_sem);

  if (stat(name, &f->f_stb) < 0)
    {
      /* there can mainly be two reasons for stat() to fail. Either the
       * file really doesn't exist (ENOENT), or then the filesystem/handler
       * doesn't support file locks. */

      /* if we should get out of here without an error, init the stat
       * buffer after having opened the file with __fstat, this sets some
       * reasonable parameters (see end of function). */
      late_stat = 1;

      if ((errno == ENOENT) && (mode & O_CREAT))
	{
	  /* can't set permissions on an open file, so this has to be done
	   * by 'close' */
	  f->f_stb.st_mode = (perms & ~u.u_cmask);
	  f->f_stb_dirty |= FSDF_MODE;

          if (!muBase)
            {
              f->f_stb.st_uid = geteuid();
              f->f_stb.st_gid = getegid();
              if (f->f_stb.st_uid != (uid_t)(-2) ||
                  f->f_stb.st_gid != (gid_t)(-2))
                f->f_stb_dirty |= FSDF_OWNER;
            }
	}
    }

  f->f_flags = mode & FMASK;
  f->f_ttyflags = IXTTY_ICRNL | IXTTY_OPOST | IXTTY_ONLCR;

  /* initialise the packet. The only thing needed at this time is its
   * header, filling in of port, action & args will be done when it's
   * used */
  __init_std_packet (&f->f_sp);
  __init_std_packet ((void *)&f->f_select_sp);

  /* check for case-sensitive filename */
  if ((mode & O_CASE) && !late_stat && !strchr(name, ':') && filenamecmp(name))
    {
      error = ENOENT;
      goto error;
    }

  /* ok, so lets try to open the file... */

  /* do this *only* if the stat() above was successful !! */
  if (!late_stat && S_ISDIR (f->f_stb.st_mode) && !(mode & FWRITE))
    {
      if (convert_dir (f, name, omask))
        {
          ix_mutex_unlock(&open_sem);
          goto ret_ok;
        }
      else
        {
	  goto error;
	}
    }

  /* filter invalid modes */
  switch (mode & (O_CREAT|O_TRUNC|O_EXCL))
    {
    case O_EXCL:
    case O_EXCL|O_TRUNC:
      /* can never succeed ! */
      error = EINVAL;
      goto error;

    case O_CREAT|O_EXCL:
    case O_CREAT|O_EXCL|O_TRUNC:
      if (! late_stat)
        {
	  error = EEXIST;
	  goto error;
	}
      break;
    }

  amode = (mode & O_CREAT) ? MODE_READWRITE : MODE_OLDFILE;

  if (!strcmp(name, "/dev/tty"))
    name = "*";
  else if ((i = is_pseudoterminal(name)))
    {
      char *orig_name = name;
      char mask;

      name = "/fifo/ptyXX/rweksm";
      memcpy(name + 7, orig_name + i + 1, 4);
      name[17] = (orig_name[i] == 'p' ? 'm' : 'c');
      mask = (name[17] == 'm' ? IX_PTY_MASTER : IX_PTY_SLAVE) | IX_PTY_CLOSE;
      ptyindex = (name[9] - 'p') * 16 + name[10] - (name[10] >= 'a' ? 'a' - 10 : '0');
      ix_lock_base();
      if (ix.ix_ptys[ptyindex] & mask)
        {
          ix_unlock_base();
          error = EIO;
          goto error;
        }
      ptymask = mask;
      ix.ix_ptys[ptyindex] |= (mask & IX_PTY_OPEN); /* mark pty in use */
      ix_unlock_base();
    }

  do
  {
    if (!strcmp(name, "*") || !strcasecmp(name, "console:"))
      /* Temporary patch for KingCON 1.3, which seems to have problems with
	 ACTION_FINDINPUT of "*"/"console:" when "dp_Port" of the packet is
	 not set to sender's "pr_MsgPort" - that's what IXEmul makes on
	 clients' startup during initialization of "stderr". */
      fh = Open(name, amode);
    else
      fh = __open (name, amode);

    if (! fh)
      {
        int err = IoErr();

        /* For those handlers that do not understand MODE_READWRITE (e.g. PAR: ) */
        if (err == ERROR_ACTION_NOT_KNOWN && amode == MODE_READWRITE)
        {
          amode = MODE_NEWFILE;
        }
        else
        {
          error = __ioerr_to_errno (err);
          goto error;
        }
      }
  } while (!fh);

  // End of critical section
  ix_mutex_unlock(&open_sem);

  /* now.. we're lucky, we actually opened the file! */
  f->f_fh = (struct FileHandle *) BTOCPTR(fh);

  if (mode & FWRITE)
    f->f_write  = __write;
  if (mode & FREAD)
    f->f_read   = __read;

  f->f_ioctl  = __ioctl;
  f->f_select = __fselect;
  f->f_close  = __close;
  f->f_type   = DTYPE_FILE;

  /*
   * have to use kmalloc() instead of malloc(), because this is no task-private
   * data, it could (in the future) be shared by other tasks 
   */
  f->f_name = (void *) kmalloc (strlen (name) + 1);
  if (f->f_name)
    strcpy (f->f_name, name);

ret_ok:
  /* ok, we're almost done. If desired, init the stat buffer to the
   * information we can get from an open file descriptor */
  if (late_stat) __fstat (f);
  
  /* if the file qualifies, try to change it into a DTYPE_MEM file */
  if (!late_stat && f->f_type == DTYPE_FILE 
      && f->f_stb.st_size < ix.ix_membuf_limit && mode == FREAD)
    {
      void *buf;
      
      /* try to obtain the needed memory */
      buf = (void *) kmalloc (f->f_stb.st_size);
      if (buf)
	if (syscall (SYS_read, fd, buf, f->f_stb.st_size) == f->f_stb.st_size)
	  {
	    __Close (CTOBPTR (f->f_fh));
	    f->f_type 		= DTYPE_MEM;
	    f->f_mf.mf_offset 	= 0;
	    f->f_mf.mf_buffer 	= buf;
	    f->f_read		= __mread;
	    f->f_close 		= __mclose;
	    f->f_ioctl		= 0;
	    f->f_select		= __mselect;
	  }
	else
	  kfree (buf);
    }

  syscall (SYS_sigsetmask, omask);

  if (error)
    {
      errno = error;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
    }
  if (!error && (mode & O_TRUNC) && (amode != MODE_NEWFILE))
    {
      syscall(SYS_ftruncate, fd, 0);
      f->f_stb_dirty |= FSDF_UTIME;
    }

  /* return the descriptor */
  return fd;

error:
  // End of critical section
  ix_mutex_unlock(&open_sem);

  /* free the file */
  u.u_ofile[fd] = 0;
  if (ptymask)
    {
      ix_lock_base();
      ix.ix_ptys[ptyindex] &= ~(ptymask & IX_PTY_OPEN);
      ix_unlock_base();
    }
  f->f_count--;
  syscall (SYS_sigsetmask, omask);
  errno = error;
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
  return -1;
}
