/*
 *  This file is part of ixnet.library for the Amiga.
 *  Copyright (C) 1996 by Jeff Shepherd
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
 *  $Id:$
 *
 *  $Log:$
 *
 */

#define _KERNEL
#include "ixnet.h"
#include "kprintf.h"

#include <exec/memory.h>
#include <dos/var.h>

#include <string.h>
#include <unistd.h>

/* not changed after the library is initialized */
struct ixnet_base		*ixnetbase = NULL;
struct ixemul_base		*ixemulbase = NULL;
struct ExecBase 		*SysBase = NULL;
struct DosLibrary		*DOSBase = NULL;

static struct
{
  void **base;
  char *name;
}
ix_libs[] =
{
  { (void **)&DOSBase, "dos.library" },
  { NULL, NULL }
};


int open_libraries(void)
{
  int i;

  for (i = 0; ix_libs[i].base; i++)
    if (!(*(ix_libs[i].base) = (void *)OpenLibrary(ix_libs[i].name, 0)))
      {
	return 0;
      }
  return 1;
}

void close_libraries(void)
{
  int i;

  for (i = 0; ix_libs[i].base; i++)
    if (*ix_libs[i].base)
      CloseLibrary(*ix_libs[i].base);
}


struct ixnet_base *ixnet_init (struct ixnet_base *ixbase)
{
  /* First set SysBase, because it is used by the 'u' macro. The Enforcer
     manual recommends caching ExecBase because low-memory accesses are slower
     when using Enforcer, besides the extra penalty of being in CHIP memory.
     Also, lots of accesses to address 4 can hurt interrupt performance. */
  SysBase = *(struct ExecBase **)4;
  ixnetbase = ixbase;

  /* This can't go here since we are running as ramlib */
  /* ixemulbase = u.u_ixbase; */

  if (!open_libraries())
    {
      close_libraries();
      return 0;
    }
  return ixbase;
}

