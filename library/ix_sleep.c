/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1991, 1992  Markus M. Wild
 *  Portions Copyright (C) 1994 Rafael W. Luebbert
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
 *  $Id: ix_sleep.c,v 1.4 1994/06/19 15:13:19 rluebbert Exp $
 *
 *  $Log: ix_sleep.c,v $
 *  Revision 1.4  1994/06/19  15:13:19  rluebbert
 *  *** empty log message ***
 *
 * 
 * 
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#define __time_req (u.u_time_req)
#define __tport    (u.u_sync_mp)

/* this is the `message' we queue on the sleep queues. */
struct sleep_msg {
  struct ixnode 	sm_node;
  short			sm_signal;
  struct Task*		sm_sigtask;
  u_int			sm_waitchan;
};


static inline u_short
ix_hash (u_int waitchan)
{
  unsigned short res;

  res = (waitchan >> 16) ^ (waitchan & 0xffff);
  res %= IX_NUM_SLEEP_QUEUES;
  return res; 
}

int
tsleep(caddr_t waitchan, char *wmesg, int timo)
{
  /* we run in the context of the calling task, we generate a sleep msg and
   * add it to the right sleep queue. wakeup() will do the rest.
   */
  struct sleep_msg sm;
  struct ixlist *the_list;
  u_int wait_sigs;
  int res = -1;
  struct Task *me = FindTask(0);
  struct user *u_ptr = getuser(me);

  if (CURSIG(&u))
    {
      errno = EINTR;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return -1;
    }

  sm.sm_sigtask = me;
  sm.sm_waitchan = (u_int)waitchan;

  u.p_stat = SSLEEP;	/* so that machdep.c can interrupt us */
  u.p_wchan = (caddr_t) waitchan;
  u.p_wmesg = wmesg;
  the_list = &ixemulbase->ix_sleep_queues[ix_hash((u_int)waitchan)];
  
  sm.sm_signal = u.u_sleep_sig;

  wait_sigs =  (1 << sm.sm_signal) | SIGBREAKF_CTRL_C;
  
  if (timo)
    {
      __time_req->tr_time.tv_sec = timo % 60;
      __time_req->tr_time.tv_usec = timo / 60;
      __time_req->tr_node.io_Command = TR_ADDREQUEST;
      SetSignal (0, 1 << __tport->mp_SigBit);
      SendIO((struct IORequest *)__time_req);
      wait_sigs |= 1 << __tport->mp_SigBit;
    }
  Forbid();
  ixaddtail ((struct ixlist *)the_list, (struct ixnode *)&sm);

  /* this will break the Disable () and reestablish it afterwards */
  res = Wait (wait_sigs);
  /* this conversion is inhibited in the Launch handler as long as we're
     in SSLEEP state. Since the SetSignal() below will remove all traces
     of a perhaps present SIGBREAKF_CTRL_C, we'll have to do the conversion
     here ourselves */
  if (((u.p_sigignore & sigmask(SIGMSG)) || !(u.p_sigcatch & sigmask(SIGMSG)))
      && (res & SIGBREAKF_CTRL_C))
    {
      struct Process *proc = (struct Process *)(u.u_session ? u.u_session->pgrp : getpid());
      _psignalgrp(proc, SIGINT);
    }
  SetSignal (0, res);
  res = CURSIG (&u) ? -1 : 0;

  ixremove ((struct ixlist *)the_list, (struct ixnode *)&sm);
  Permit();

  if (timo)
    {
      if (! CheckIO ((struct IORequest *)__time_req))
        AbortIO ((struct IORequest *)__time_req);
      WaitIO ((struct IORequest *)__time_req);
    }

  u.p_wchan = 0;
  u.p_wmesg = 0;
  u.p_stat = SRUN;
  if (res)
    errno = EINTR;
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
  return res;
}

int
ix_sleep (caddr_t chan, char *wmesg)
{
  return tsleep (chan, wmesg, 0);
}


/*
 * ix_wakeup() can be called from an interrupt (and is called that way;-))
 */

void
ix_wakeup (u_int waitchan)
{
  struct ixlist *the_list = &ixemulbase->ix_sleep_queues[ix_hash (waitchan)];
  struct sleep_msg *sm;
  
  Forbid();

  for (sm = (struct sleep_msg *)the_list->head;
       sm;
       sm = (struct sleep_msg *)sm->sm_node.next)
    {
      if (sm->sm_waitchan == waitchan)
        Signal (sm->sm_sigtask, 1 << sm->sm_signal);
    }

  Permit();
}
