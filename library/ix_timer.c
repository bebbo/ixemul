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
 *  $Id: ix_timer.c,v 1.3 1994/06/19 15:13:28 rluebbert Exp $
 *
 *  $Log: ix_timer.c,v $
 *  Revision 1.3  1994/06/19  15:13:28  rluebbert
 *  *** empty log message ***
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"
#include <stddef.h>

/*
 * this is the interrupt code that distributes those itimer signals and
 * collects resource information
 */

/*
 * For all you "moralists" out there in Amiga land...
 * This code uses exec private information about what the stack frame looks
 * like inside an interrupt. However, this information is used read-only, and
 * it really doesn't matter whether it will be wrong in the future, in that
 * case system-time will be measured in other ways, but so what ? ;-))
 */

/*
 * by specifying the function as taking varargs parameter, we force gcc
 * to generate a framepointer...
 */
#if 1
int
ix_timer(char *foobar, ...)
{
  register struct Task	*t_pass	asm ("a1");
  struct Task		*me;
  struct user		*p;
  u_int			current_pc;
  register u_int	a5 asm ("a5");
  u_int			sp;
  struct itimerval	*tim;
  /* not necessarily "me" */
  struct Task		*current_task;

  me = t_pass;
  p = getuser(me);
  current_task = FindTask(0);

  /* find out value of sp on invocation of this function. This is easy,
   * since gcc generates a 
   *   link a5,#..
   * at the beginning. So we find sp with a5+4
   */
  sp = a5 + 4;

  tim = p->u_timer;

  /* The main work. Decrement the timers, and if they hit zero, generate
   * the approprate signal */

  /* real timer counts in real time */
  if (timerisset (&tim->it_value) && !itimerdecr (tim, timer_resolution))
    _psignal (me, SIGALRM);

  /* virtual timer only counts, when current_task == me AND the task is
   * not executing in system time. To get at the current PC, remember (or learn;-))
   * that the stack in an interrupt handler looks like follows:
   *   0(sp)  rts into ExitIntr
   *   4(sp),8(sp),12(sp),16(sp),20(sp),24(sp) -> d0/d1/a0/a1/a5/a6
   *    now the stuff for the correct rte instruction
   *   28(sp) -> SR
   *   30(sp) -> PC <- that's what we're interested in
   */
  /* heuristics for 2.0.. */
  current_pc = *(u_int *)(sp + 46);

  if (me == current_task)
    {
      struct timeval *tv;
      int is_user = current_pc >= p->u_start_pc && current_pc < p->u_end_pc;

      ++tim;
      if (is_user && timerisset(&tim->it_value) &&
          !itimerdecr (tim, timer_resolution))
        _psignal (me, SIGVTALRM);
      ++tim;

      /* profiling timer, runs while this process is executing, no matter
       * whether in system time or not */
      if (timerisset(&tim->it_value) &&
          !itimerdecr (tim, timer_resolution))
        _psignal (me, SIGPROF);

      /* now that we're done with the timers, if this is our task executing,
       * update it's rusage fields */
      tv = is_user ? &p->u_ru.ru_utime : &p->u_ru.ru_stime;
      tv->tv_usec += timer_resolution;
      if (tv->tv_usec >= 1000000)
        {
	  tv->tv_usec -= 1000000; /* - is much cheaper than % */
	  tv->tv_sec++;
        }
    }

  switch (ix.ix_flags & ix_profile_method_mask)
  {
    case IX_PROFILE_PROGRAM:
      if (p->u_prof.pr_scale) {
        addupc (current_pc, &p->u_prof, 1);
      }
      break;
    case IX_PROFILE_TASK:
      if (me == current_task && p->u_prof.pr_scale) {
        addupc (p->u_prof_last_pc, &p->u_prof, 1);
      }
      break;
    case IX_PROFILE_ALWAYS:
      if (p->u_prof.pr_scale)
        addupc (p->u_prof_last_pc, &p->u_prof, 1);
      break;
  }
  return 0;
}

#else

int
ix_timer (char *foobar, ...)
{
  u_int			current_pc;
  register u_int	a5 asm ("a5");
  u_int			sp;
  struct Task		*current_task;
  struct ixnode         *node;

  current_task = FindTask(0);

  /* find out value of sp on invocation of this function. This is easy,
   * since gcc generates a 
   *   link a5,#..
   * at the beginning. So we find sp with a5+4
   */
  sp = a5 + 4;

  /* virtual timer only counts, when current_task == me AND the task is
   * not executing in system time. To get at the current PC, remember (or learn;-))
   * that the stack in an interrupt handler looks like follows:
   *   0(sp)  rts into ExitIntr
   *   4(sp),8(sp),12(sp),16(sp),20(sp),24(sp) -> d0/d1/a0/a1/a5/a6
   *    now the stuff for the correct rte instruction
   *   28(sp) -> SR
   *   30(sp) -> PC <- that's what we're interested in
   */
  /* heuristics for 2.0.. */
  current_pc = *(u_int *)(sp + 46);

#define getuserptr(ixnode) ((struct user *)(((char *)ixnode) - offsetof(struct user, u_user_node)))
  for (node = timer_task_list.head; node; node = node->next)
    {
      struct user *p = getuserptr(node);
      int is_user = current_pc >= p->u_start_pc && current_pc < p->u_end_pc;
      struct timeval *tv;

      tv = is_user ? &p->u_ru.ru_utime : &p->u_ru.ru_stime;
      tv->tv_usec += timer_resolution;
      if (tv->tv_usec >= 1000000)
        {
	  tv->tv_usec -= 1000000; /* - is much cheaper than % */
	  tv->tv_sec++;
        }

      if (p->u_prof.pr_scale)
        switch (ix.ix_flags & ix_profile_method_mask)
        {
          case IX_PROFILE_PROGRAM:
            addupc(current_pc, &p->u_prof, 1);
            break;

          case IX_PROFILE_TASK:
            if (p->u_task == current_task) {
              addupc(p->u_prof_last_pc, &p->u_prof, 1);
            }
            break;

          case IX_PROFILE_ALWAYS:
            addupc(p->u_prof_last_pc, &p->u_prof, 1);
            break;
        }
    }

  return 0;
}

#endif
