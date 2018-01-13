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
 *  malloc.c,v 1.1.1.1 1994/04/04 04:30:29 amiga Exp
 *
 *  malloc.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:29  amiga
 * Initial CVS check in.
 *
 *  Revision 1.5  1992/10/20  16:26:17  mwild
 *  change to single indirection to memory management list, allowing us
 *  to switch lists on the fly (important for vfork).
 *
 *  Revision 1.4  1992/09/14  01:44:41  mwild
 *  reduce magic cookie information by one long. By this, gcc's obstack will
 *  allocate in neat 4k blocks, which makes GigaMem happy (GigaMem is a commercial
 *  VMem package for AmigaOS).
 *
 *  Revision 1.3  1992/08/09  20:58:57  amiga
 *  add cast
 *
 *  Revision 1.2  1992/05/22  01:43:07  mwild
 *  use buddy-alloc memory management
 *
 * Revision 1.1  1992/05/14  19:55:40  mwild
 * Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#include <stddef.h>

#define mem_list (u.u_mdp->md_list)
#define mem_used (u.u_mdp->md_malloc_sbrk_used)
#define own_mem_list (u.u_md.md_list)
#define own_mem_used (u.u_md.md_malloc_sbrk_used)

#define MAGIC1 0x55555555
#define MAGIC2 0xaaaaaaaa

/* use only one magic cookie at the end, since then the most frequently
   done allocation of gcc, 4072 byte, fits neatly into 4096 when we've added
   our information. Gigamem will thank you that it doesn't have to give
   you an additional page for 4 byte ;-)) */

struct mem_block {
  struct ixnode    node;
  u_int		   size;
  u_int		   magic[2];
  u_int		   realblock[1];
  /* realblock should really be [0], and at the end we'd have a magic2[1] */
};

struct memalign_ptr {
  u_char	magic;
  u_int		alignment:24;
};

#define MEMALIGN_MAGIC 0xdd

/* perhaps important later ;-) */
#define PAGESIZE 2048

void *
malloc (size_t size)
{
  usetup;
  struct mem_block *res;

  /* We increase SIZE below which could cause an dangerous (system
     crash) overflow. -bw/09-Jun-98 */
  if ((signed long)size < 0)
    return 0;

  /* guarantee long sizes (so we can use CopyMemQuick in realloc) */
  size = (size + 3) & ~3; /* next highest multiple of 4 */
  
  /* include management information */
  res = (struct mem_block *) b_alloc(size + sizeof (struct mem_block), 0); /* not MEMF_PUBLIC ! */
  
  if (res)
    {
      u_int *lp = &res->size;
      *lp++ = size;
      *lp++ = MAGIC1; *lp++ = MAGIC1;
      lp = (u_int *)((u_char *)lp + size);
      *lp++ = MAGIC2; 
      
      Forbid ();
      ixaddtail ((struct ixlist *)&mem_list, (struct ixnode *)res);
      Permit ();

      mem_used += size;
      return &res->realblock;
    }
  return 0;
}

void *
memalign (size_t alignment, size_t size)
{
  u_char *p = (u_char *) malloc (size + alignment + sizeof (struct memalign_ptr));
  struct memalign_ptr *al_start;

  if (! p)
    return p;

  /* if the block is already aligned right, just return it */
  if (! ((u_int)p & (alignment - 1)))
    return p;

  /* else find the start of the aligned block in our larger block */
  al_start = (struct memalign_ptr *)(((u_int)p + alignment - 1) & -alignment);

  /* this could be problematic on 68000, when an odd alignment is requested,
   * should I just don't allow odd alignments?? */
  al_start[-1].magic = MEMALIGN_MAGIC;
  al_start[-1].alignment = (u_char *)al_start - p;

  return al_start;
}

void *
valloc (size_t size)
{
  return (void *) memalign (PAGESIZE, size);
}

void
free (void *mem)
{
  struct mem_block *block;
  u_int *end_magic;
  usetup;

  /* this seems to be standard... don't like it though ! */
  if (! mem)
    return;

  block = (struct mem_block *)((u_char *)mem - offsetof (struct mem_block, realblock));

  if (((struct memalign_ptr *)mem - 1)->magic == MEMALIGN_MAGIC)
    {
      block = (struct mem_block *)
	      ((u_char *)block - ((struct memalign_ptr *)mem - 1)->alignment);
    }

   

  if (((u_int)block & 1) ||
      (block->magic[0] != MAGIC1 || block->magic[1] != MAGIC1))
    {
      ix_panic ("free: start of block corrupt!");
      syscall (SYS_exit, 20);
    }

  end_magic = (u_int *)((u_char *)&block->realblock + block->size);
  if (end_magic[0] != MAGIC2)
    {
      ix_panic ("free: end of block corrupt!");
      syscall (SYS_exit, 20);
    }

  Forbid ();
  ixremove ((struct ixlist *)&mem_list, (struct ixnode *) block);
  Permit ();

  mem_used -= block->size;
  b_free(block, block->size + sizeof (struct mem_block));
}


void all_free (void)
{
  struct mem_block *b, *nb;
  static int errno_dummy;
  usetup;

  /* Since errno (= *u.u_errno) might still be used, we point it to our own
     temporary errno dummy variable. */
  u.u_errno = &errno_dummy;

  /* be sure to use *our* memory lists in here, never all_free() the parents
     list.... */
  
  for (b = (struct mem_block *)own_mem_list.head; b; b = nb)
    {
      nb = (struct mem_block *)b->node.next;
      b_free(b, b->size + sizeof (struct mem_block));
    }

  /* this makes it possible to call all_free() more than once */
  ixnewlist ((struct ixlist *)&own_mem_list);
}

void *
realloc (void *mem, size_t size)
{
  struct mem_block *block;
  u_int *end_magic;
  void *res;
  usetup;

  if (!mem)
    return (void *) malloc (size);
    
  if (!size)
  {
    free(mem);
    return NULL;
  }
    
  block = (struct mem_block *)((u_char *)mem - offsetof (struct mem_block, realblock));

  
  /* duplicate the code in free() here so we don't have to check those magic
   * numbers twice */
  
  if (((struct memalign_ptr *)mem - 1)->magic == MEMALIGN_MAGIC)
    {
      block = (struct mem_block *)
	      ((u_char *)block - ((struct memalign_ptr *)mem - 1)->alignment);
    }

   
  if (((u_int)block & 1) ||
      (block->magic[0] != MAGIC1 || block->magic[1] != MAGIC1))
    {
      ix_panic ("realloc: start of block corrupt!");
      syscall (SYS_exit, 20);
    }

  end_magic = (u_int *)((u_char *)&block->realblock + block->size);
  if (end_magic[0] != MAGIC2)
    {
      ix_panic ("realloc: end of block corrupt!");
      syscall (SYS_exit, 20);
    }

  /* now that the block is validated, check whether we have to really
   * realloc, or if we just can return the old block */
  if (block->size >= size)
    res = mem;
  else
    {
      res = (void *) malloc (size);
      if (res)
	{
	  Forbid ();
          ixremove ((struct ixlist *)&mem_list, (struct ixnode *) block);
          Permit ();

	  CopyMemQuick (mem, res, block->size);

	  /* according to the manpage, the old buffer should only be
	   * freed, if the allocation of the new buffer was successful */
	  mem_used -= block->size;
	  b_free(block, block->size + sizeof (struct mem_block));
	}
    }

  return res;
}
