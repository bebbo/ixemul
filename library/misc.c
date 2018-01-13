/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1991, 1992  Markus M. Wild
 *  Portions Copyright (C) 1994 Rafael W. Luebbert
 *  Portions Copyright (C) 1996 Jeff Shepherd
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

/* Miscellaneous functions */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"
#include <stdlib.h>

int
getpid(void)
{
  return (int)FindTask (0);
}

int
getppid(void)
{
  usetup;
  
  return (int)u.p_pptr;
}

int
setpgid(int pid, int pgrp)
{
  usetup;

  if (pid)
      getuser(pid)->p_pgrp = pgrp;
  else
      u.p_pgrp = pgrp;
  return 0;
}

int
setpgrp(int pid, int pgrp)
{
  return setpgid(pid, pgrp);
}

pid_t
getpgrp(void)
{
  usetup;
  return u.p_pgrp;
}

pid_t
setsid(void)
{
  struct session *s;
  usetup;

  if (u.u_session && u.u_session->s_count <= 1)
    {
      errno = EPERM;
      return (pid_t)-1;
    }

  s = kmalloc(sizeof(struct session));
  if (s == NULL)
    {
      errno = ENOMEM;
      return (pid_t)-1;
    }
  if (u.u_session)
    u.u_session->s_count--;
  u.u_session = s;
  s->s_count = 1;
  s->pgrp = u.p_pgrp = getpid();

  return u.p_pgrp;
}

int
getrusage(int who, struct rusage *rusage)
{
  usetup;

  if (who != RUSAGE_SELF && who != RUSAGE_CHILDREN)
    {
      errno = EINVAL;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return -1;
    }

  *rusage = (who == RUSAGE_SELF) ? u.u_ru : u.u_cru;
  return 0;
}

char hostname[64] = "localhost";

int
gethostname(char *name, int namelen)
{
  usetup;

  if (u.u_ixnetbase)
    return netcall(NET_gethostname, name, namelen);
  strncpy (name, hostname, namelen);
  return 0;
}

int
sethostname(char *name, int namelen)
{
  int len;
  usetup;

  if (u.u_ixnetbase)
    return netcall(NET_sethostname, name, namelen);

  len = namelen < sizeof (hostname) - 1 ? namelen : sizeof (hostname) - 1;
  strncpy (hostname, name, len);
  hostname[len] = 0;
  return 0;
}

/* not really useful.. but it's there ;-)) */
int
getpagesize(void)
{
  return 2048;
}

void sync (void)
{
  usetup;
  /* could probably walk the entire file table and fsync each, but
     this is too much of a nuisance ;-)) */
  errno = ENOSYS;
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
}

int
fork (void)
{
  usetup;
  errno = ENOSYS;
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
  return -1;
}

int
mkfifo (const char *path, mode_t mode)
{
  usetup;
  errno = ENOSYS;
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
  return -1;
}

int
mknod (const char *path, mode_t mode, dev_t dev)
{
  usetup;
  errno = ENOSYS;
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
  return -1;
}

void __flush_cache(void *adr, int len)
{
  CacheClearE(adr, len, CACRF_ClearI | CACRF_ClearD);
}

void ix_flush_insn_cache(void *adr, int len)
{
  CacheClearE(adr, len, CACRF_ClearI);
}

void ix_flush_data_cache(void *adr, int len)
{
  CacheClearE(adr, len, CACRF_ClearD);
}

void ix_flush_caches(void)
{
  CacheClearU();
}
