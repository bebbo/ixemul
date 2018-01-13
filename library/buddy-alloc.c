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
 *  $Id: buddy-alloc.c,v 1.4 1994/06/19 15:02:51 rluebbert Exp $
 *
 *  $Log: buddy-alloc.c,v $
 *  Revision 1.4  1994/06/19  15:02:51  rluebbert
 *  *** empty log message ***
 *
 *  Revision 1.2  1992/09/14  01:40:24  mwild
 *  change from using aligned blocks (obtained thru an AllocMem/FreeMem/AllocAbs
 *  hack) to using non-aligned blocks. The price for this is an additional
 *  field in every allocated block.
 *
 *  In the same run, change Forbid/Permit into Semaphore locking.
 *
 *  Revision 1.1  1992/05/22  01:42:33  mwild
 *  Initial revision
 *
 */
#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"
#include <exec/memory.h>
#include <stddef.h>

/* this provides a straight replacement for AllocMem() and FreeMem().
   Being this, it does *not* remember the size of allocation, the
   clients have to do this instead. */

/* NOTE: currently only two pools are supported, MEMF_PUBLIC and
         ! MEMF_PUBLIC. No MEMF_CHIP pools are needed by the library
         and are thus not supported */


/* TUNING: The two parameters that can be adjusted to fine tune
           allocation strategy are MAXSIZE and BUDDY_LIMIT. By setting
           MAXSIZE larger than BUDDY_LIMIT results in less Exec
           overhead, since blocks stay longer in the buddy system.
           Setting MAXSIZE==BUDDY_LIMIT sets memory usage to the
           minimum, at the cost of more Exec calls. */


/* no request for memory can be lower than this */
#define MINLOG2		4
#define MINSIZE		(1 << MINLOG2)

/* this is the size the buddy system gets memory pieces from Exec */
#define MAXLOG2		15	/* get 32K chunks */
#define MAXSIZE		(1 << MAXLOG2)

/* this is the limit for b_alloc to go straight to Exec */
#define BUDDY_LIMIT	(1 << (MAXLOG2 - 5))	/* but serve only upto 1K */

#define PRIVATE_POOL	0
#define PUBLIC_POOL	1
#define NUMPOOLS	2	/* public and !public */
/* attention: don't go larger than 3 pools, or you'll have to change the
              encoding in free_block (only 2 bits for now) */

struct free_list {
  u_int  exec_attr;
  struct ix_mutex sem;
  struct ixlist buckets[MAXLOG2 - MINLOG2];
} free_list[NUMPOOLS] = { { 0, }, { MEMF_PUBLIC, } };


struct free_block {
  /* to make the smallest allocatable block 16, and not 32 byte, stuff both
     the freelist information and the exec-block address into one long. */
  u_int	pool:2,		/* 0: block is free, > 0: POOL + 1 */
  	exec_block:30;	/* shift left twice to get the real address */

  /* from here on, fields only exist while the block is on the free list.
     The application sees a block as a chunk of memory starting at &next */
  struct free_block *next, *prev;	/* ixnode compatible */
  int index;
};


void
init_buddy (void)
{
  int i, l; 

  /* don't want such a nightmare of bug-hunt any more... */
  if (sizeof (struct free_block) > MINSIZE)
    {
      ix_panic ("buddy-system: MINSIZE/MINLOG2 too small, increase!");
      Wait (0);
    }

  for (l = 0; l < NUMPOOLS; l++)
    {
      for (i = 0; i < MAXLOG2 - MINLOG2; i++)
	ixnewlist ((struct ixlist *)&free_list[l].buckets[i]);
    }
}

static inline struct free_block *
unlink_block (u_int free_pool, u_char ind, void *block)
{
  struct free_block *fb = (struct free_block *) block;
  struct free_list *fl = free_list + free_pool;

  if (! fb)
    {
      fb = (struct free_block *)ixremhead((struct ixlist *)&fl->buckets[ind]);
      if (fb)
	{
	  fb = (struct free_block *) ((int)fb - offsetof (struct free_block, next));
	  fb->pool = free_pool + 1;
	  KPRINTF(("    unlink_block (%s, %ld) == $%lx\n", 
	     free_pool == PRIVATE_POOL ? "PRIVATE" : (free_pool == PUBLIC_POOL ? "PUBLIC" : "BOGOUS"), ind, fb));

	}
    }
  else
    {
      KPRINTF(("    unlink_block (%s, %ld, $%lx)\n", 
	 free_pool == PRIVATE_POOL ? "PRIVATE" : (free_pool == PUBLIC_POOL ? "PUBLIC" : "BOGOUS"), ind, fb));

      fb->pool = free_pool + 1;
      ixremove ((struct ixlist *)&fl->buckets[fb->index], (struct ixnode *)&fb->next);
    }

  return fb;
}

static void inline
link_block (u_int free_pool, u_char ind, void *block)
{
  struct free_block *fb = (struct free_block *) block;
  struct free_list *fl = free_list + free_pool;

  KPRINTF(("    link_block (%s, %ld, $%lx)\n", 
     free_pool == PRIVATE_POOL ? "PRIVATE" : (free_pool == PUBLIC_POOL ? "PUBLIC" : "BOGOUS"), ind, fb));

  fb->pool = 0;	/* we're on the freelist of this pool */
  fb->index = ind; /* and of this size */
  ixaddhead ((struct ixlist *)&fl->buckets[ind], (struct ixnode *)&fb->next);
}

/* this is a very special log2() function that knows the upper bound
   of its argument, and also automatically rounds to the next upper
   power of two */

static inline int const
log2 (int size)
{
  int pow = MAXLOG2;
  int lower_bound = 1 << (MAXLOG2 - 1);

  for (;;)
    {
      if (size > lower_bound)
        return pow;

      lower_bound >>= 1;
      pow--;
    }
}


static inline struct free_block *
get_block (u_int free_pool, u_char index)
{
  struct free_block *fb, *buddy;
  struct free_list *fl = free_list + free_pool;

  KPRINTF(("  get_block (%s, %ld)\n", 
     free_pool == PRIVATE_POOL ? "PRIVATE" : (free_pool == PUBLIC_POOL ? "PUBLIC" : "BOGOUS"), index, fb));

  if (index == (MAXLOG2 - MINLOG2))
    {
      fb = (struct free_block *) AllocMem (MAXSIZE, fl->exec_attr);
      if (! fb)
        return 0;

      fb->exec_block = (int)fb >> 2; /* buddies are relative to this base address */
      fb->pool = free_pool + 1; /* not free */

      return fb;
    }
  else 
    {
      if ((fb = unlink_block (free_pool, index, 0)))
        return fb;
    }


  fb = get_block (free_pool, index + 1);

  if (fb)
    {
      /* when splitting a block, we always free the upper buddy. So
         we can just add the size, instead of or'ing the offset to the
         Exec memory block */
      buddy = (struct free_block *)((int)fb + (1 << (index + MINLOG2)));

      buddy->exec_block = fb->exec_block;

      link_block (free_pool, index, buddy);
    }

  return fb;
}


static inline void
free_block (u_int free_pool, u_char index, struct free_block *fb)
{
  struct free_block *buddy;

  buddy = (struct free_block *)
	  ((((int)fb - (fb->exec_block<<2)) ^ (1 << (index + MINLOG2)))
	   + (fb->exec_block<<2));

  if (index == (MAXLOG2 - MINLOG2))
    {
      FreeMem (fb, MAXSIZE);
      return;
    }
  else if (buddy->pool || buddy->index != index)
    {
      /* too bad, buddy is not on freelist or of wrong size */
      link_block (free_pool, index, fb);
      return;
    }

  /* reserve the buddy, then recombine both */
  unlink_block (free_pool, index, buddy);

  /* since the buddy is free as well, recombine both blocks
     and free the twice as large block */
  free_block (free_pool, index + 1, fb < buddy ? fb : buddy);
}


void *
b_alloc (int size, unsigned pool)
{
  u_char bucket;
  struct free_block *block;
  struct free_list *fl = free_list + pool;

  if (size < 0)	/* Ridiculous size */
    return 0;
  if (size < MINSIZE)
    size = MINSIZE;

  /* the additional bytes are needed for the freelist pointer at
     the beginning of each block in use and the base block originally
     obtained from Exec. */

  if (size >= BUDDY_LIMIT - offsetof (struct free_block, next))
    return AllocMem (size, pool == PUBLIC_POOL ? MEMF_PUBLIC : 0);

  size += offsetof (struct free_block, next);

  bucket = log2 (size) - MINLOG2;

  /* have to differentiate between PUBLIC and PRIVATE memory here, sigh. 
     PRIVATE memory can safely be accessed by using a semaphore, PUBLIC
     memory however is allocated and free'd inside Forbid(), and using a
     semaphore there would possibly break a Forbid..
     Note: this is safe for use in GigaMem, as GigaMem only uses non-PUBLIC
	   memory, if you don't fiddle with attribute masks.. */
  if (pool == PRIVATE_POOL)
  {
    ix_mutex_lock(&fl->sem);
    block = get_block (pool, bucket);
    ix_mutex_unlock(&fl->sem);
  }
  else
  {
    Forbid();
    block = get_block (pool, bucket);
    Permit();
  }

  if (block)
    return (void *) & block->next;
  else
    return block;
}


void
b_free (void *mem, int size)
{
  u_char bucket;
  struct free_list *fl;
  struct free_block *fb;
  int free_pool;

  if (size < MINSIZE)
    size = MINSIZE;
    
  if (size >= BUDDY_LIMIT - offsetof (struct free_block, next))
    {
      FreeMem (mem, size);
      return;
    }

  size += offsetof (struct free_block, next);

  bucket = log2 (size) - MINLOG2;
  fb = (struct free_block *) ((int)mem - offsetof (struct free_block, next));
  free_pool = fb->pool - 1;
  fl = free_list + free_pool;

  if (free_pool == PRIVATE_POOL)
  {
    ix_mutex_lock(&fl->sem);
    free_block (free_pool, bucket, fb);
    ix_mutex_unlock(&fl->sem);
  }
  else
  {
    Forbid();
    free_block (free_pool, bucket, fb);
    Permit();
  }
}
