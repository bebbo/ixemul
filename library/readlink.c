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
 *  readlink.c,v 1.1.1.1 1994/04/04 04:30:31 amiga Exp
 *
 *  readlink.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:31  amiga
 * Initial CVS check in.
 *
 *  Revision 1.2  1992/05/18  12:22:32  mwild
 *  set errno, and only send READLINK packet if the file really is a symlink
 *
 * Revision 1.1  1992/05/14  19:55:40  mwild
 * Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#include <string.h>

#ifndef ERROR_IS_SOFT_LINK
#define ERROR_IS_SOFT_LINK  233
#endif

#ifndef ACTION_READ_LINK
#define ACTION_READ_LINK 1024
#endif

struct readlink_vec {
  char *buf;
  int bufsize;
};

static int
__readlink_func (struct lockinfo *info, struct readlink_vec *rv, int *error)
{
  struct StandardPacket *sp = &info->sp;

  /* this baby uses CSTRings, absolutely unique... */
  sp->sp_Pkt.dp_Type = ACTION_READ_LINK;
  sp->sp_Pkt.dp_Arg1 = info->parent_lock;
  sp->sp_Pkt.dp_Arg2 = (long)info->str + 1;
  sp->sp_Pkt.dp_Arg3 = (long)rv->buf;
  sp->sp_Pkt.dp_Arg4 = rv->bufsize;

  PutPacket (info->handler, sp);
  __wait_sync_packet (sp);
  
  info->result = sp->sp_Pkt.dp_Res1;

  *error = info->result <= 0;

  /* this *can't* be a symlink... */
  return 0;
}

int readlink(const char *path, char *buf, int bufsiz)
{
  struct readlink_vec rv;
  struct stat stb;
  int rc;
  usetup;
  
  /* readlink is buggy in the current fs release (37.26 here), in that
     it reports OBJECT_NOT_FOUND if a file is present but not a 
     symbolic link */
  if (syscall(SYS_lstat, path, &stb) == 0)
    {
      if (S_ISLNK (stb.st_mode))
	{
	  rv.buf = alloca(bufsiz);
	  rv.bufsize = bufsiz;
  
	  rc = __plock ((char *)path, __readlink_func, &rv);
	  if (rc <= 0)
	    {
	      errno = __ioerr_to_errno (IoErr ());
	      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
	    }
          else
            {
              int len = a2u(NULL, rv.buf);
              char *p = alloca(len);

              a2u(p, rv.buf);
              if (p[0] == '.' && p[1] == '/')  /* skip ./ */
              {
                p += 2;
                len -= 2;
              }
              rc = (len - 1 < bufsiz ? len - 1 : bufsiz);
              memcpy(buf, p, rc);
              if (rc < bufsiz)
                buf[rc] = '\0';
            }
	  return rc > 0 ? rc : -1;
	}
      else
	{
	  errno = EINVAL;
	  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
	}
    }
  /* errno should be set already by lstat() */
  return -1;
}

int a2u(char *buf, char *src)
{
  int len = 0;

  if (index(src, ':'))
  {
    if (buf)
      buf[len] = '/';
    len++;
    while (*src != ':')
    {
      if (buf)
        buf[len] = *src;
      len++;
      src++;
    }
    src++;
  }
  else
  {
    if (buf)
      buf[len] = '.';
    len++;
  }
  while (*src)
  {
    if (*src == '/')
    {
      if (buf)
        strcpy(buf + len, "/..");
      len += 3;
      src++;
    }
    else
    {
      if (buf)
        buf[len] = '/';
      len++;
      while (*src && *src != '/')
      {
        if (buf)
          buf[len] = *src;
        src++;
        len++;
      }
      if (*src)
        src++;
    }
  }
  if (buf)
    buf[len] = '\0';
  return len + 1;
}
