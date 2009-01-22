/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1997 Hans Verkuil
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

struct mutex {
  int initialized;
  int oldmask;
  union {
    struct SignalSemaphore sem;
    char buf[120];
  } un;
};

/* Semaphore handling functions */

int ix_mutex_lock(struct ix_mutex *_mutex)
{
  usetup;
  struct mutex *mutex = (struct mutex *)_mutex;
  int old = u.p_sigmask;

  if (!mutex->initialized)
  {
    InitSemaphore(&mutex->un.sem);
    mutex->initialized = 1;
  }
  u.p_sigmask = ~0;
  ObtainSemaphore(&mutex->un.sem);
  if (mutex->un.sem.ss_NestCount == 1)
    mutex->oldmask = old;
  return 0;
}

int ix_mutex_attempt_lock(struct ix_mutex *_mutex)
{
  usetup;
  struct mutex *mutex = (struct mutex *)_mutex;
  int old = u.p_sigmask;

  if (!mutex->initialized)
  {
    InitSemaphore(&mutex->un.sem);
    mutex->initialized = 1;
  }
  u.p_sigmask = ~0;
  if (AttemptSemaphore(&mutex->un.sem))
  {
    if (mutex->un.sem.ss_NestCount == 1)
      mutex->oldmask = old;
    return 0;
  }
  errno = EAGAIN;
  u.p_sigmask = old;
  return -1;
}

void ix_mutex_unlock(struct ix_mutex *_mutex)
{
  usetup;
  struct mutex *mutex = (struct mutex *)_mutex;
  int old = u.p_sigmask;

  if (!mutex->initialized)
  {
    InitSemaphore(&mutex->un.sem);
    mutex->initialized = 1;
  }
  if (mutex->un.sem.ss_NestCount == 1)
    old = mutex->oldmask;
  if (mutex->un.sem.ss_NestCount)
    ReleaseSemaphore(&mutex->un.sem);
  u.p_sigmask = old;
}

static struct ix_mutex mutex_library_base;

void ix_lock_base(void)
{
  ix_mutex_lock(&mutex_library_base);
}

void ix_unlock_base(void)
{
  ix_mutex_unlock(&mutex_library_base);
}

