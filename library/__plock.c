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

/*
 * Lock() and LLock() emulation. Takes care of expanding paths that contain
 * symlinks.
 * Call __plock() if you need a lock to the parent directory as used in
 * other packets, that way you always get the "right" thing
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#include <stdlib.h>
#include <string.h>

#define NEW_NAME   1
#define DONE       2

#if 0

static struct DevProc   *get_device_proc(char *, struct DevProc *, struct lockinfo *, void *);
static void             unslashify(char *);
static void             resolve_name(struct lockinfo *info, int (*last_func)(), void *last_arg);
static int              get_component(struct lockinfo *info);
static int              resolve_name_on_device(struct lockinfo *info, int (*last_func)(), void *last_arg);

static BPTR lock(struct lockinfo *info)
{
  struct StandardPacket *sp = &info->sp;

  sp->sp_Pkt.dp_Type = ACTION_LOCATE_OBJECT;
  sp->sp_Pkt.dp_Arg1 = info->parent_lock;
  sp->sp_Pkt.dp_Arg2 = info->bstr;
  sp->sp_Pkt.dp_Arg3 = ACCESS_READ;

  PutPacket(info->handler, sp);
  __wait_sync_packet(sp);
  SetIoErr(sp->sp_Pkt.dp_Res2);
  return sp->sp_Pkt.dp_Res1;
}

static int readlink(struct lockinfo *info)
{
  usetup;
  struct StandardPacket *sp = &info->sp;
  char buf[256];

  /* SFS doesn't like it if dp_Arg2==dp_Arg3. Copy the string */
  strncpy(buf, info->str + 1, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';

  sp->sp_Pkt.dp_Port = __srwport;
  sp->sp_Pkt.dp_Type = ACTION_READ_LINK;
  sp->sp_Pkt.dp_Arg1 = info->parent_lock;
  sp->sp_Pkt.dp_Arg2 = (long)buf; /* read as cstr */
  sp->sp_Pkt.dp_Arg3 = (long)info->str + 1; /* write as cstr */
  sp->sp_Pkt.dp_Arg4 = 255; /* what a BSTR can address */

  PutPacket(info->handler, sp);
  __wait_sync_packet(sp);
  SetIoErr(sp->sp_Pkt.dp_Res2);
  return sp->sp_Pkt.dp_Res1;
}

#endif

int is_pseudoterminal(const char *name)
{
  int i = 1;

  if (!memcmp(name, "/dev/", 5) || !(i = memcmp(name, "dev:", 4)))
    {
      if (i)
	name++;
      if ((name[4] == 'p' || name[4] == 't') && name[5] == 't'
	  && name[6] == 'y' && name[7] >= 'p' && name[7] <= 'u'
	  && strchr("0123456789abcdef", name[8]) && !name[9])
	return i + 4;
    }
  return 0;
}

#if 0

BPTR
__plock (const char *file_name, int (*last_func)(), void *last_arg)
{
  usetup;
  struct lockinfo *info;
  int omask;

  KPRINTF (("__plock: file_name = %s, last_func = $%lx\n",
	    file_name ? file_name : "(none)", last_func));

  if (!file_name)
    return 0;

  /* now get a LONG aligned packet */
  info = alloca(sizeof(*info) + 2);
  info = LONG_ALIGN(info);
  __init_std_packet(&info->sp);

  /* need to operate on a backup of the passed name, so I can do
   * /sys -> sys: conversion in place */
  info->name = info->buf;
  strcpy(info->name + 1, file_name);
  info->is_root = u.u_is_root;
  if (info->is_root)
    info->name[0] = '/';
  else
    info->name++;
  info->link_levels = 0;
  info->bstr = CTOBPTR(info->str);

  // Turn the name into an AmigaOS name, except for . and ..
  if (ix.ix_flags & ix_translate_slash)
    unslashify(info->name);

  /* NOTE: although we don't use any DOS calls here, we have to block
   * any signals, since the locks we obtain in this function have to
   * be freed before anything else can be done. This function is
   * *not* reentrant in the sense that it can be interrupted without
   * being finished */
  omask = syscall(SYS_sigsetmask, ~0);

  resolve_name(info, last_func, last_arg);

  syscall(SYS_sigsetmask, omask);
  return info->result;
}

static void resolve_name(struct lockinfo *info, int (*last_func)(), void *last_arg)
{
  char *name = info->name;
  struct DevProc *dp = NULL;
  int done = FALSE;
  char PI = 0;  // dummy

  while (!done)
  {
    info->result = 0;

    if (!dp)
    {
      if (!strcasecmp(name, "nil:") || !strcasecmp(name, "/nil") ||
	  !strcmp(name, "/dev/null") || !strcmp(name, "dev:null"))
	{
	  SetIoErr(4242); /* special special ;-) */
	  return;
	}
      if (is_pseudoterminal(name))
	{
	  SetIoErr(5252);       /* special special ;-) */
	  return;
	}
      if (!strcmp(name, ":"))
	{
	  SetIoErr(6262); /* another special special (the root directory) */
	  return;
	}

      if (!strcasecmp(name, "console:") || !strcasecmp(name, "/console") || !strcmp(name, "/dev/tty"))
	name = "*";
      bzero(&PI, sizeof(PI));
    }

    info->unlock_parent = 0;

    dp = get_device_proc(name, dp, info, &PI);

    if (!info->handler)
      {
	SetIoErr(ERROR_OBJECT_NOT_FOUND);
	break;
      }

    info->name = name;

    switch (resolve_name_on_device(info, last_func, last_arg))
    {
      case NEW_NAME:
	FreeDeviceProc(dp);
	dp = NULL;
	name = info->name;
	break;

      case DONE:
	done = TRUE;
	break;
    }
    if (info->unlock_parent)
      UnLock(info->parent_lock);
  }

  if (dp)
    FreeDeviceProc(dp);
}

static int get_component(struct lockinfo *info)
{
  char *name = info->name, *str = info->str, *sep, *next;
  int is_last = 0, len;

  if (info->is_fs)
  {
    /* fetch the first part of "name", thus stopping at either a : or a /
     * next points at the start of the next directory component to be
     * processed in the next run
     */

    sep = index(name, ':');
    if (sep)
    {
      sep++; /* the : is part of the filename */
      next = sep;
    }
    else
    {
      sep = index(name, '/');

      /* map foo/bar/ into foo/bar, but keep foo/bar// */
      if (sep && sep[1] == 0)
      {
	is_last = 1;
	next = sep + 1;
      }
      else if (!sep)
      {
	sep = name + strlen(name);
	next = sep;
	is_last = 1;
      }
      else
      {
	if (ix.ix_flags & ix_translate_slash)
	  for (next = sep + 1; *next == '/'; next++) ;
	else
	  next = sep + 1;

	/* if the slash is the first character, it means "parent",
	 * so we have to pass it literally to Lock() */
	if (sep == name)
	  sep = next;
      }
    }
  }
  else
  {
    sep = name + strlen(name);
    next = sep;
    is_last = 1;
  }

  len = sep - name;
  if (len)
    bcopy(name, str + 1, len);
  *str = len;
  str[len + 1] = 0;

  /* turn a ".." into a "/", and a "." into a "" */

  if (strcmp(str + 1, "..") == 0)
  {
    str[0] = 1; str[1] = '/'; str[2] = 0;
  }
  else if (strcmp(str + 1, ".") == 0)
  {
    str[0] = 0; str[1] = 0;
  }

  info->name = next;
  return is_last;
}

static int resolve_name_on_device(struct lockinfo *info, int (*last_func)(), void *last_arg)
{
  usetup;
  int is_last = FALSE;
  int error = FALSE;
  int res = 0;
  char *sep, *str = info->str;

  sep = index(info->name, ':');
  if (sep)
    info->name = sep + 1;

  while (!error && !is_last)
  {
    KPRINTF(("__plock: solving for %s.\n", info->name));

    if (info->link_levels >= MAXSYMLINKS)
    {
      SetIoErr(ERROR_TOO_MANY_LEVELS);
      return DONE;
    }

    is_last = get_component(info);
    info->sp.sp_Pkt.dp_Port = __srwport;
    error = FALSE;

    if (!is_last)
    {
      BPTR new_parent;

      if ((new_parent = lock(info)))
      {
	if (info->unlock_parent)
	{
	  int err = IoErr();

	  UnLock(info->parent_lock);
	  SetIoErr(err);
	}
	info->parent_lock = new_parent;
	info->unlock_parent = TRUE;
      }
      else
      {
	error = TRUE;
	res = 1;
      }
    }
    else
    {
      res = (*last_func)(info, last_arg, &error);
      if (error)
	SetIoErr(info->sp.sp_Pkt.dp_Res2);
    }

    if (info->is_fs && IoErr() == ERROR_OBJECT_NOT_FOUND &&
	((!*str && info->is_root) || !strcmp(str + 1, "/")))
    {
      info->is_root = 1;

      while (!strncmp(info->name, "./", 2) || !strncmp(info->name, "../", 3))
      {
	info->name = index(info->name, '/') + 1;
      }
      if (!strcmp(info->name, ".") || !strcmp(info->name, ".."))
      {
	info->name = index(info->name, 0);
      }

      if (*info->name)
      {
	sep = index(info->name, '/');
	if (!sep)
	{
	  sep = index(info->name, 0);
	  sep[1] = 0;
	}
	*sep = ':';
      }
      else {
	*--info->name = ':';
      }
      return NEW_NAME;
    }

    info->is_root = 0;

    /* if no error, fine */
    if (!error)
      continue;

    if (!res)
      continue;

    /* else check whether ordinary error or really symlink */
    if (IoErr() != ERROR_IS_SOFT_LINK)
      continue;

    /* read the link. temporarily use our str as a cstr, thus setting
     * a terminating zero byte and skipping the length byte */
    res = readlink(info);

    /* error (no matter which...) couldn't read the link */
    if (res <= 0)
      return DONE;

    if (info->name)
    {
      char buf[1024], *p;
      res = a2u(buf, str + 1);
      if (buf[res - 1] != '/')
	strcat(buf, "/");
      strcat(buf, info->name);
      if (buf[0] == '/')
      {
	strcpy(info->buf, buf + 1);
	if ((p = strchr(info->buf, '/')))
	  *p = ':';
      }
      else
      {
	strcpy(info->buf, buf);
      }
    }
    else
    {
      strcpy(info->buf, str + 1);
    }

    info->name = info->buf;
    is_last = error = 0;
    info->link_levels++;

    /* if the read name is absolute, we may have to change the
     * handler and the parent lock. Check for this */
    if (index(info->name, ':'))
      return NEW_NAME;
  }
  if (error && IoErr() == ERROR_OBJECT_NOT_FOUND)
    return 0;

  return DONE;
}

static struct DevProc *
get_device_proc (char *name, struct DevProc *prev, struct lockinfo *info, void *PI)
{
  struct DevProc *dp;
  char *p, old = 0;
  struct Process *this_proc = NULL;
  APTR oldwin = NULL;

  if (ix.ix_flags & ix_no_insert_disk_requester)
    {
      this_proc = (struct Process *)SysBase->ThisTask;
      oldwin = this_proc->pr_WindowPtr;
      this_proc->pr_WindowPtr = (APTR)-1;
    }

  p = strchr(name, ':');
  if (p)
  {
    old = p[1];
    p[1] = '\0';
  }

  if (prev == NULL)
    info->is_fs = p ? IsFileSystem(name) : 1;

  dp = GetDeviceProc(name, prev);
  info->handler = dp ? dp->dvp_Port : 0;
  if (dp)
    info->parent_lock = dp->dvp_Lock;

  if (p)
    p[1] = old;

  if (ix.ix_flags & ix_no_insert_disk_requester)
    this_proc->pr_WindowPtr = oldwin;
  return dp;
}

static void unslashify(char *name)
{
  char *oname = name;

  if (index (name, ':'))
    return;

  while (oname[0] == '/' &&
	 (oname[1] == '/' || !memcmp(oname + 1, "./", 2) || !memcmp(oname + 1, "../", 3)))
    while (*++oname == '.') ;

  if (!strcmp(oname, "/.") || !strcmp(oname, "/.."))
    oname[1] = '\0';

  /* don't (!) use strcpy () here, this is an overlapping copy ! */
  if (oname > name)
    bcopy(oname, name, strlen(oname) + 1);

  /* root directory */
  if (name[0] == '/' && name[1] == 0)
    {
      name[0] = ':';
      return;
    }

  if (name[0] == '/')
    {
      /* get the delimiter */
      char *cp = index(name + 1, '/');
      int shift = 0;

      /* if there is a separating (and not terminating) slash, shift a bit ;-) */
      if (cp)
	while (*cp == '/')
	  {
	    shift++;
	    cp++;
	  }

      /* is it a terminator (then discard it) or a separator ? */
      if (!cp || !*cp)
	{
	  /* terminator */
	  cp = name + strlen(name);
	  bcopy(name + 1, name, cp - name);
	  cp[-1 - shift] = ':';
	  cp[-shift] = 0;
	}
      else
	{
	  /* separator */
	  bcopy(name + 1, name, strlen(name) + 1);
	  cp--;
	  bcopy(cp, cp - (shift - 1), strlen(cp) + 1);
	  cp[-shift] = ':';
	}
    }
}

#endif

