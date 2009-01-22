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
 *  kmalloc.c,v 1.1.1.1 1994/04/04 04:30:54 amiga Exp
 *
 *  kmalloc.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:54  amiga
 * Initial CVS check in.
 *
 *  Revision 1.3  1992/08/09  20:57:37  amiga
 *  add declaration
 *
 *  Revision 1.2  1992/05/22  01:43:35  mwild
 *  use buddy-alloc memory management
 *
 * Revision 1.1  1992/05/14  19:55:40  mwild
 * Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#include <exec/memory.h>
#include <stdio.h>

/* This is a very simple and crude malloc package, only intended to
   be used inside the library. We don't record what we allocated, as all
   allocations are inside objects that are resource tracked, so no memory
   should be lost (currently write buffers and memory files are allocated
   with these functions) */

void *
kmalloc (size_t size)
{
  u_int *res;

  /* always allocate a quantity of long words so we can CopyMemQuick() later */
  size = (size + 3) & ~3;

  res = (u_int *) b_alloc(size + 4, MEMF_PUBLIC);
  if (res) *res++ = size;

  return res;
}

void *
krealloc (void *mem, size_t size)
{
  u_int *res;

  if (! mem) return kmalloc (size);

  /* always allocate a quantity of long words */
  size = (size + 3) & ~3;

  /* in that case the block is already large enough */
  if (((u_int *)mem)[-1] >= size)
    return mem;

  res = (u_int *) b_alloc(size + 4, MEMF_PUBLIC);
  if (res)
    {
      *res++ = size;
      CopyMemQuick (mem, res, ((u_int *)mem)[-1]);

      /* according to the manpage, the old buffer should only be
       * freed, if the allocation of the new buffer was successful */
      kfree (mem);
    }

  return res;
}

void
kfree (void *mem)
{
  u_int *res;
  
  if (! mem) return;

  res = mem;
  res--;
  b_free(res, *res + 4);
}
