/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1996  Hans Verkuil
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
#include "ixemul.h"
#include "kprintf.h"
#include <sys/mman.h>

extern int read(), write();

caddr_t mmap (caddr_t addr, size_t len, int prot, int flags, int fd, off_t offset)
{
  struct mmap_mem *m;
  usetup;
  
  if (flags & MAP_FIXED)
    {
      errno = ENOMEM;
      return (daddr_t)-1;
    }
  if (!(flags & MAP_ANON) && (fd < 0 || fd >= NOFILE || !u.u_ofile[fd]))
    {
      errno = EBADF;
      return (caddr_t)-1;
    }
  m = malloc(sizeof(struct mmap_mem));
  if (m == NULL)
    {
      errno = ENOMEM;
      return (caddr_t)-1;
    }
  m->addr = malloc(len);
  if (m->addr == NULL)
    {
      free(m);
      errno = ENOMEM;
      return (caddr_t)-1;
    }
  if (!(flags & MAP_ANON))
    {
      off_t curoff = lseek(fd, 0, SEEK_CUR);

      lseek(fd, offset, SEEK_SET);
      read(fd, m->addr, len);
      lseek(fd, curoff, SEEK_SET);
    }
  m->length = len;
  m->prot = prot;
  m->fd = fd;
  m->flags = flags;
  m->offset = offset;
  m->next = u.u_mmap;
  u.u_mmap = m;
  return m->addr;
}

static struct mmap_mem *find_mmap(caddr_t addr)
{
  usetup;
  struct mmap_mem *m = u.u_mmap;
  
  for (m = u.u_mmap; m; m = m->next)
    if ((caddr_t)m->addr <= addr && (caddr_t)m->addr + m->length > addr)
      break;
  return m;
}

/* NetBSD also doesn't support this */
void madvise(caddr_t addr, int len, int behav)
{
}

int mlock(caddr_t addr, size_t len)
{
  return 0;
}

int munlock(caddr_t addr, size_t len)
{
  return 0;
}

void mprotect(caddr_t addr, size_t len, int prot)
{
  struct mmap_mem *m = find_mmap(addr);
  
  if (m)
    m->prot = prot;
}

void msync(caddr_t addr, int len)
{
  struct mmap_mem *m = find_mmap(addr);
  int cur_off;
  
  if (!m || (m->flags & MAP_ANON) || !(m->flags & MAP_SHARED) || !(m->prot & PROT_WRITE))
    return;
  cur_off = lseek(m->fd, 0, SEEK_CUR);
  lseek(m->fd, m->offset, SEEK_SET);
  write(m->fd, m->addr, m->length);
  lseek(m->fd, cur_off, SEEK_SET);
}

int munmap(caddr_t addr, size_t len)
{
  struct mmap_mem *m, *prev = NULL;
  usetup;

  for (m = u.u_mmap; m; m = m->next)
    {
      if ((caddr_t)m->addr <= addr && (caddr_t)m->addr + m->length > addr)
	break;
      prev = m;
    }
  if (!m)
    {
      errno = EINVAL;
      return -1;
    }
  msync(addr, len);
  if (prev)
    prev->next = m->next;
  else
    u.u_mmap = m->next;
  free(m->addr);
  free(m);
  return 0;
}
