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
 */

#define _KERNEL
#include <string.h>
#include "ixemul.h"
#include "kprintf.h"

#include <utility/tagitem.h>
#include <dos/dostags.h>

#if 0

struct open_vec {
  int action;
  struct FileHandle *fh;
};

static int
__open_func (struct lockinfo *info, struct open_vec *ov, int *error)
{
  struct StandardPacket *sp = &info->sp;

  sp->sp_Pkt.dp_Type = ov->action;
  sp->sp_Pkt.dp_Arg1 = CTOBPTR (ov->fh);
  sp->sp_Pkt.dp_Arg2 = info->parent_lock;
  sp->sp_Pkt.dp_Arg3 = info->bstr;

  PutPacket (info->handler, sp);
  __wait_sync_packet (sp);

  info->result = sp->sp_Pkt.dp_Res1;
  *error = info->result != -1;

  /* handler is supposed to supply this, but if it didn't... */
  if (!*error && !ov->fh->fh_Type) ov->fh->fh_Type = info->handler;

  /* continue if we failed because of symlink - reference */
  return 1;
}

BPTR
__open (char *name, int action)
{
  unsigned long *fh;
  struct open_vec ov;
  BPTR res;
  struct Process *this_proc = NULL;
  APTR oldwin = NULL;
  char buf[32], *s, *p = buf;

  if ((name[0] == '/' && strncmp(name, "/dev/", 5)) ||
      index(name, ':'))
  {
    s = (name[0] == '/') ? name + 1 : name;
    while (*s && *s != '/' && *s != ':')
      *p++ = *s++;
    *p++ = ':';
    *p = '\0';
    if (ix.ix_flags & ix_no_insert_disk_requester)
    {
      this_proc = (struct Process *)SysBase->ThisTask;
      oldwin = this_proc->pr_WindowPtr;
      this_proc->pr_WindowPtr = (APTR)-1;
    }
    if (!IsFileSystem(buf))     /* if NAME is not a file system but */
				/* something like a pipe or console, then */
				/* do NOT use __plock() but call Open directly */
    {
      if (!*s)
	res = Open(buf, action);
      else if (*s == '/')
      {
	*s = ':';
	res = Open(name + 1, action);
	*s = '/';
      }
      else
	res = Open(name, action);
      if (ix.ix_flags & ix_no_insert_disk_requester)
	this_proc->pr_WindowPtr = oldwin;
      return res;
    }
    if (ix.ix_flags & ix_no_insert_disk_requester)
      this_proc->pr_WindowPtr = oldwin;
  }

  /* needed to get berserk ReadArgs() to behave sane, GRRR */
  fh = AllocDosObjectTags (DOS_FILEHANDLE, ADO_FH_Mode, action, TAG_DONE);
  if (! fh)
    return 0;
  ov.fh = (struct FileHandle *) fh;
  ov.action = action;

  res = __plock (name, __open_func, &ov);

  /* check for opening of NIL:, in which case we just return
     a filehandle with a zero handle field */
  if (!res && IoErr() == 4242)
  {
    res = -1;
  }

  if (!res)
    FreeDosObject (DOS_FILEHANDLE, fh);

  return res ? CTOBPTR(ov.fh) : 0;
}
#else

#if 1

/*
 * ix_to_ados © 2005 Harry Sintonen <sintonen@iki.fi>
 */

char *
ix_to_ados(char *buf, const char *name)
{
  usetup;
  char *p;
  const char *q = name;
  int allowdot = 1;
  int dotcnt = 0;
  int root = 0;
  int absados = 0;

  KPRINTF(("name \"%s\"\n", name));

  if (u.u_is_root)
  {
    root = 1;
  }
  else if (*q == '/')
  {
    root = 1;
    q++;
  }

  #if 1
  /* allow amigados paths concatenated to nix ones */
  p = strrchr(q, ':');
  if (p)
  {
    dotcnt = -1;
    q = p + 1;
    while ((p + dotcnt >= name) && p[dotcnt] != ':' && p[dotcnt] != '/')
      dotcnt--;
    memcpy(buf, p + dotcnt + 1, -dotcnt);
    p = buf - dotcnt;

    root = 0;
    dotcnt = 0;
    absados = 1;
  }
  else
  #endif
    p = buf;

  for (;;)
  {
    char c = *q++;

    switch (c)
    {
      case '.':
        if (allowdot)
          dotcnt++;
        else
          *p++ = '.';
        break;

      case '/':
      case '\0':
        if (dotcnt > 2)
        {
          int cnt = dotcnt;
          while (cnt--)
            *p++ = '.';
          if (root == 1)
            root++;
        }
        if (root == 2 || (root == 1 && c == '\0'))
        {
          *p++ = ':';
          root = 0;
        }
        else if ((root == 0 && absados == 0) &&
                 (dotcnt == 2 ||                              /* "/../" */
                 ((dotcnt == 0 || dotcnt > 2) && c != '\0'))) /* normal "/" */
        {
          *p++ = '/';
        }

        if (c == '\0')
          goto out;

        while (*q == '/')
          q++;

        allowdot = 1;
        dotcnt = 0;
        break;

      #if 0
      case ':':
        dotcnt = -2;
        while (*q == ':')
        {
          q++; dotcnt--;
        }

        while ((q + dotcnt >= name) && q[dotcnt] != ':' && q[dotcnt] != '/')
          dotcnt--;
        dotcnt++;
        memcpy(buf, q + dotcnt, -dotcnt);
        p = buf - dotcnt;

        root = 0;
        allowdot = 1; /* allow foo:../ */
        dotcnt = 0;
        break;
      #endif

      default:
        if (root == 1)
          root++;
        while (dotcnt--)
          *p++ = '.';
        *p++ = c;
        allowdot = 0;
        dotcnt = 0;
        absados = 0;
        break;
    }
  }
  out:

  /* remove lonely '/' from the end */
  if (p > buf + 2 && p[-1] == '/' && p[-2] != '/' && p[-2] != ':')
    p--;

  *p = '\0';

  KPRINTF(("result \"%s\"\n", buf));

  if (!strcasecmp(buf, "console:"))
  {
    buf[0] = '*';
    buf[1] = '\0';
  }
  else if (!strncmp(buf, "dev:", 4))
  {
    if (!strcmp(buf + 4, "tty"))
    {
      buf[0] = '*';
      buf[1] = '\0';
    }
    else if (!strcmp(buf + 4, "null"))
    {
      strcpy(buf, "NIL:");
    }
    else if (!strcmp(buf + 4, "random") || !strcmp(buf + 4, "urandom") || !strcmp(buf + 4, "srandom") || !strcmp(buf + 4, "prandom"))
    {
      strcpy(buf, "RANDOM:");
    }
    else if (!strcmp(buf + 4, "zero"))
    {
      strcpy(buf, "ZERO:");
    }
  }

  return buf;
}

#else

char *
ix_to_ados(char *buf, const char *name)
{
  usetup;
  char *p = buf;
  const char *q = name;
  int parent_count = 0;

  KPRINTF(("name \"%s\"\n", name));

  if (u.u_is_root)
    {
      *p++ = '/';
    }

  while (*q)
    {
      if (*q == '/')
	{
	  if (p > buf && p[-1] == '/')
	    {
	      --p;
	    }
	  else
	    {
	      p = buf;
	      parent_count = 0;
	    }
	}
      else if (*q == '.' && (q[1] == '/' || q[1] == '\0'))
	{
	  ++q;
	  if (*q)
	    ++q;
	  continue;
	}
      else if (*q == '.' && q[1] == '.' && (q[2] == '/' || q[2] == '\0'))
	{
	  q += 2;

	  if (p == buf)
	    {
	      ++parent_count;
	      if (*q)
		++q;
	      continue;
	    }
	  else if (p == buf + 1 && *buf == '/')
	    {
	      p = buf;
	      parent_count = 0;
	    }
	  else
	    {
	      do
		{
		  --p;
		}
	      while (p > buf && p[-1] != '/');
	      if (p > buf + 1)
		--p;
	      else
		{
		  while (*q == '/')
		    {
		      ++q;
		    }
		}
	    }
	}
      else
	{
	  const char *start = q;

	  do
	    {
	      *p++ = *q++;
	    }
	  while (*q && *q != '/' && *q != ':');

	  if (*q == ':')
	    {
	      p = buf;
	      *p++ = '/';
	      while (start != q)
		*p++ = *start++;
	    }
	}

      if (*q == '/' || *q == ':')
        {
	  *p++ = '/';
	  ++q;
	}
    }

  if (p == buf + 1 && buf[0] == '/')
    {
      buf[0] = ':';
      *p = '\0';
    }
  else
    {
      if (p > buf && p[-1] == '/')
	--p;

      *p++ = '\0';

      if (parent_count)
        {
	  while (p != buf)
	    {
	      --p;
	      p[parent_count] = *p;
	    }
	  do
	    {
	      *p++ = '/';
	    }
	  while (--parent_count);
	}
      else if (*buf == '/')
        {
	  ++buf;
	  p = buf;
	  while (*p != '/' && *p)
	    ++p;
	  if (!*p)
	    p[1] = '\0';
	  *p = ':';
	}
    }

  KPRINTF(("result \"%s\"\n", buf));

  if (!strcasecmp(buf, "console:"))
  {
    buf[0] = '*';
    buf[1] = '\0';
  }
  else if (!strncmp(buf, "dev:", 4))
  {
    if (!strcmp(buf + 4, "tty"))
    {
      buf[0] = '*';
      buf[1] = '\0';
    }
    else if (!strcmp(buf + 4, "null"))
    {
      strcpy(buf, "NIL:");
    }
    else if (!strcmp(buf + 4, "random") || !strcmp(buf + 4, "urandom") || !strcmp(buf + 4, "srandom") || !strcmp(buf + 4, "prandom"))
    {
      strcpy(buf, "RANDOM:");
    }
    else if (!strcmp(buf + 4, "zero"))
    {
      strcpy(buf, "ZERO:");
    }
  }

   return buf;
}

#endif

char *check_root(char *name)
{
//char buf[256];
//dprintf("check_root(%s)\n", name);
  if (*name == '/')
  {
    BPTR olddir = CurrentDir(0);
    BPTR dir = DupLock(olddir);
    int root = 0;
//NameFromLock(olddir, buf, sizeof(buf));
//dprintf("curdir <%s>\n", buf);

    CurrentDir(olddir);

    do
    {
      BPTR parent = ParentDir(dir);

      ++name;
//dprintf("dir %p parent %p name <%s>\n", dir, parent, name);
      if (parent == 0)
      {
	root = 1;
	break;
      }

      UnLock(dir);
      dir = parent;
    }
    while (*name == '/');

    UnLock(dir);

    while (*name == '/')
      ++name;

    if (!root)
    {
      name = NULL;
    }
    else if (*name)
    {
      char *p = name;
      do
      {
	++p;
      }
      while (*p && *p != '/');

      if (*p)
      {
	*p = ':';
      }
      else
      {
	p[0] = ':';
	p[1] = '\0';
      }
    }
  }
  else
  {
    name = NULL;
  }
//dprintf("return(%s)\n",name ?name: "NULL");
  return name;
}

BPTR
__open (char *name, int action)
{
  BPTR file;
  APTR win;
  struct Process *me = (APTR) SysBase->ThisTask;
  char *buf = alloca(strlen(name) + 4);
  buf = ix_to_ados(buf, name);
  win = me->pr_WindowPtr;
  me->pr_WindowPtr = (APTR)-1;
  file = Open(buf, action);
  if (!file)
  {
    LONG err = IoErr();
    if (err == ERROR_OBJECT_NOT_FOUND)
    {
      buf = check_root(buf);
      if (buf && *buf)
      {
	file = Open(buf, action);
	if (file)
	  err = 0;
      }
      SetIoErr(err);
    }
  }
  me->pr_WindowPtr = win;
  return file;
}

#endif
