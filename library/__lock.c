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

#if 0

struct lock_vec {
  int    mode;
  int    resolve_last;
};

static int
__lock_func (struct lockinfo *info, struct lock_vec *lv, int *error)
{
  struct StandardPacket *sp = &info->sp;

  sp->sp_Pkt.dp_Type = ACTION_LOCATE_OBJECT;
  sp->sp_Pkt.dp_Arg1 = info->parent_lock;
  sp->sp_Pkt.dp_Arg2 = info->bstr;
  sp->sp_Pkt.dp_Arg3 = lv->mode;

  PutPacket (info->handler, sp);
  __wait_sync_packet (sp);
  info->result = sp->sp_Pkt.dp_Res1;

  *error = info->result <= 0;

  /* continue if we failed because of symlink - reference ? */
  return lv->resolve_last;
}

BPTR __lock (char *name, int mode)
{
  struct lock_vec lv;

  lv.mode = mode;
  lv.resolve_last = 1;

  return __plock (name, __lock_func, &lv);
}

BPTR __llock (char *name, int mode)
{
  struct lock_vec lv;

  lv.mode = mode;
  lv.resolve_last = 0;

  return __plock (name, __lock_func, &lv);
}

#else

char *ix_to_ados(char *, const char *);
char *check_root(char *);

static int is_special_name(const char *name)
{
  if (!strcasecmp(name, "NIL:"))
    {
      SetIoErr(4242);
      return 1;
    }
  else if (is_pseudoterminal(name))
    {
      SetIoErr(5252);
      return 1;
    }
  else if (!strcmp(name, ":"))
    {
      SetIoErr(6262);
      return 1;
    }

  return 0;
}

BPTR __lock (char *name, int mode)
{
  BPTR lock;
  APTR win;
  char *buf = alloca(strlen(name) + 4);
  buf = ix_to_ados(buf, name);
  if (is_special_name(buf))
    return 0;
  win = ((struct Process *)SysBase->ThisTask)->pr_WindowPtr;
  ((struct Process *)SysBase->ThisTask)->pr_WindowPtr = (APTR)-1;
  lock = Lock(buf, mode);
  if (lock == 0)
    {
      LONG err = IoErr();
      if (err == ERROR_OBJECT_NOT_FOUND)
        {
	  buf = check_root(buf);
	  if (buf)
	    {
	      if (*buf == 0)
		err = 6262;
	      else
		{
		  lock = Lock(buf, mode);
		  if (lock)
		    err = 0;
		}
	    }

	  SetIoErr(err);
	}
    }
  ((struct Process *)SysBase->ThisTask)->pr_WindowPtr = win;
  return lock;
}

BPTR __llock (char *name, int mode)
{
  struct DevProc *dvp;
  char *buf = alloca(strlen(name) + 4);
  char buf2[256] __attribute__((aligned(4)));
  BPTR lock = 0;
  LONG error;
  const char *p;
  char *q;
  int len;
  APTR win;
  int retry = 0;
  int last_err = 0;

  buf = ix_to_ados(buf, name);

  win = ((struct Process *)SysBase->ThisTask)->pr_WindowPtr;
  ((struct Process *)SysBase->ThisTask)->pr_WindowPtr = (APTR)-1;

  do
  {
//dprintf("llock(%s)\n", buf);
    if (is_special_name(buf))
    {
      last_err = IoErr();
      break;
    }

    p = buf;
    q = buf2 + 1;
    len = 0;

    while (*p && *p != ':')
      ++p;

    if (!*p)
      p = buf;

    while(*p && len < 256)
    {
      *q++ = *p++;
      ++len;
    }

    buf2[0] = len;

    dvp = NULL;

    for (;;)
    {
      dvp = GetDeviceProc(buf, dvp);

      if (!dvp)
	break;

      lock = DoPkt3(dvp->dvp_Port, ACTION_LOCATE_OBJECT, dvp->dvp_Lock, MKBADDR(buf2), mode);

      if (lock)
	break;

      error = IoErr();

      if (error == ERROR_IS_SOFT_LINK)
	{
	  break;
	}
      else if (error == ERROR_OBJECT_NOT_FOUND)
	{
	  if (!(dvp->dvp_Flags & DVPF_ASSIGN))
	    break;
	}
      else
	{
	  LONG res = ErrorReport(error, REPORT_LOCK, dvp->dvp_Lock, dvp->dvp_Port);
	  FreeDeviceProc(dvp);
	  dvp = NULL;
	  if (res)
	    break;
	}
    }

    FreeDeviceProc(dvp);

    if (lock == 0 && ++retry == 1)
    {
      last_err = IoErr();
      if (last_err == ERROR_OBJECT_NOT_FOUND)
      {
	buf = check_root(buf);
	if (buf)
	{
	  if (*buf == 0)
	  {
	    last_err = 6262;
	  }
	  else
	  {
//dprintf("retry\n");
	    retry = 10;
	  }
	}
      }
    }
  }
  while (lock == 0 && retry == 10);

  if (lock == 0)
    SetIoErr(last_err);

  ((struct Process *)SysBase->ThisTask)->pr_WindowPtr = win;

  return lock;
}

#endif
