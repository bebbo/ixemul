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
 *  $Id: ixnet_init.c,v 1.2 2005/03/29 02:42:39 piru Exp $
 *
 *  $Log: ixnet_init.c,v $
 *  Revision 1.2  2005/03/29 02:42:39  piru
 *  cosmetics
 *
 *  Revision 1.1.1.1  2005/03/15 15:57:09  laire
 *  a new beginning
 *
 *  Revision 1.3  2001/06/01 17:39:53  emm
 *  Simplified signal handling. Minor old fixes.
 *
 *  Revision 1.2  2000/09/05 20:59:55  emm
 *  Fixed some bugs. Deadlocks, memory trashing, ...
 *
 *  Revision 1.1.1.1  2000/05/07 19:37:45  emm
 *  Imported sources
 *
 *  Revision 1.1.1.1  2000/04/29 00:44:32  nobody
 *  Initial import
 *
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
struct ixnet_base               *ixnetbase = NULL;
struct ixemul_base              *ixemulbase = NULL;
struct ExecBase                 *SysBase = NULL;
struct DosLibrary               *DOSBase = NULL;

static const struct
{
  void **base;
  const char *name;
}
ix_libs[] =
{
  { (void **)&DOSBase, "dos.library" },
  { NULL, NULL }
};


int open_libraries(void)
{
  int i;

  KPRINTF(("open_libraries\n"));
  for (i = 0; ix_libs[i].base; i++)
    {
      KPRINTF(("%ld: opening(%s)\n",i,ix_libs[i].base));
      if (!(*(ix_libs[i].base) = (void *)OpenLibrary(ix_libs[i].name, 0)))
	{
	  KPRINTF(("failed !\n"));
	  return 0;
	}
    }
  KPRINTF(("ok\n"));
  return 1;
}

void close_libraries(void)
{
  int i;

  KPRINTF(("close_libraries\n"));
  for (i = 0; ix_libs[i].base; i++)
    if (*ix_libs[i].base)
      {
	KPRINTF(("%ld: closing(%lx,%s)\n",i,*ix_libs[i].base,ix_libs[i].name));
	CloseLibrary(*ix_libs[i].base);
      }
  KPRINTF(("ok\n"));
}


struct ixnet_base *ixnet_init (struct ixnet_base *ixbase)
{
  KPRINTF(("ixnet_init(%lx)\n",ixbase));

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
      KPRINTF(("closing libraries\n"));
      close_libraries();
      return 0;
    }
  return ixbase;
}

