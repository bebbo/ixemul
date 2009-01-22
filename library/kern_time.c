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
 *  kern_time.c,v 1.1.1.1 1994/04/04 04:30:41 amiga Exp
 *
 *  kern_time.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:41  amiga
 * Initial CVS check in.
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 *
 *  Since the code originated from Berkeley, the following copyright
 *  header applies as well. The code has been changed, it's not the
 *  original Berkeley code!
 */

/*
 * Copyright (c) 1982, 1986, 1989 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution is only permitted until one year after the first shipment
 * of 4.4BSD by the Regents.  Otherwise, redistribution and use in source and
 * binary forms are permitted provided that: (1) source distributions retain
 * this entire copyright notice and comment, and (2) distributions including
 * binaries display the following acknowledgement:  This product includes
 * software developed by the University of California, Berkeley and its
 * contributors'' in the documentation or other materials provided with the
 * distribution and in all advertising materials mentioning features or use
 * of this software.  Neither the name of the University nor the names of
 * its contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *      @(#)kern_time.c 7.13 (Berkeley) 6/28/90
 */

#if 1

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#include <sys/time.h>

#define err_return(code)                \
{                                       \
  errno = code;                         \
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno)); \
  return -1;                            \
}

void timevalfix(struct timeval *t1);
void timevaladd(struct timeval *t1, const struct timeval *t2);
void timevalsub(struct timeval *t1, const struct timeval *t2);

int
getitimer(u_int which, struct itimerval *itv)
{
  usetup;

  if (which > ITIMER_PROF || !itv)
    err_return (EINVAL);

  Disable ();
  *itv = u.u_timer[which];
  Enable();

  return 0;
}

int
setitimer(u_int which, const struct itimerval *itv, struct itimerval *oitv)
{
	usetup;

	if (which > ITIMER_PROF)
		err_return (EINVAL);

	if (oitv && getitimer(which, oitv))
		return -1;
	if (itv == 0)
		return 0;

	if (itimerfix((struct timeval *)&itv->it_value) ||
	    itimerfix((struct timeval *)&itv->it_interval))
		err_return (EINVAL);

	Disable ();
	u.u_timer[which] = *itv;
	Enable ();

	return 0;
}

/*
 * Check that a proposed value to load into the .it_value or
 * .it_interval part of an interval timer is acceptable, and
 * fix it to have at least minimal value (i.e. if it is less
 * than the resolution of the clock, round it up.)
 */
int itimerfix(struct timeval *tv)
{
	usetup;

	if (tv->tv_sec < 0 || tv->tv_sec > 100000000 ||
	    tv->tv_usec < 0 || tv->tv_usec >= 1000000)
		err_return (EINVAL);
	if (tv->tv_sec == 0 && tv->tv_usec != 0 && tv->tv_usec < timer_resolution)
		tv->tv_usec = timer_resolution;
	return 0;
}

/*
 * Decrement an interval timer by a specified number
 * of microseconds, which must be less than a second,
 * i.e. < 1000000.  If the timer expires, then reload
 * it.  In this case, carry over (usec - old value) to
 * reducint the value reloaded into the timer so that
 * the timer does not drift.  This routine assumes
 * that it is called in a context where the timers
 * on which it is operating cannot change in value.
 */

/* on 0 return, send a signal to process */

int
itimerdecr (struct itimerval *itp, int usec)    /* CALLED FROM INTERRUPT !! */
{
  if (itp->it_value.tv_usec < usec)
    {
      if (itp->it_value.tv_sec == 0)
	{
	  /* expired, and already in next interval */
	  usec -= itp->it_value.tv_usec;
	  goto expire;
	}

      itp->it_value.tv_usec += 1000000;
      itp->it_value.tv_sec--;
    }

  itp->it_value.tv_usec -= usec;
  usec = 0;
  if (timerisset(&itp->it_value))
    return (1);

  /* expired, exactly at end of interval */
expire:
  if (timerisset(&itp->it_interval))
    {
      itp->it_value = itp->it_interval;
      itp->it_value.tv_usec -= usec;
      if (itp->it_value.tv_usec < 0)
	{
	  itp->it_value.tv_usec += 1000000;
	  itp->it_value.tv_sec--;
	}
    }
  else
    itp->it_value.tv_usec = 0;          /* sec is already 0 */

  return (0);
}

/*
 * Add and subtract routines for timevals.
 * N.B.: subtract routine doesn't deal with
 * results which are before the beginning,
 * it just gets very confused in this case.
 * Caveat emptor.
 */
void timevaladd(struct timeval *t1, const struct timeval *t2)
{
	t1->tv_sec += t2->tv_sec;
	t1->tv_usec += t2->tv_usec;
	timevalfix(t1);
}

void timevalsub(struct timeval *t1, const struct timeval *t2)
{
	t1->tv_sec -= t2->tv_sec;
	t1->tv_usec -= t2->tv_usec;
	timevalfix(t1);
}

void timevalfix(struct timeval *t1)
{
	if (t1->tv_usec < 0) {
		t1->tv_sec--;
		t1->tv_usec += 1000000;
	}
	if (t1->tv_usec >= 1000000) {
		t1->tv_sec++;
		t1->tv_usec -= 1000000;
	}
}

#else

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#include <sys/time.h>

void timevalfix(struct timeval *t1);
void timevaladd(struct timeval *t1, const struct timeval *t2);
void timevalsub(struct timeval *t1, const struct timeval *t2);

int
getitimer(u_int which, struct itimerval *itv)
{
  usetup;

  if (which != ITIMER_REAL || !itv)
    errno_return(EINVAL, -1);

  *itv = u.u_timer[which];

  return 0;
}

int
setitimer(u_int which, const struct itimerval *itv, struct itimerval *oitv)
{
  usetup;
  u_int ticks = 0;
  struct ixnode *node;

  if (which != ITIMER_REAL)
    errno_return(EINVAL, -1);

  if (oitv && getitimer(which, oitv))
    return -1;
  if (itv == 0)
    return 0;

  if (itimerfix((struct timeval *)&itv->it_value) ||
      itimerfix((struct timeval *)&itv->it_interval))
    errno_return(EINVAL, -1);

  u.u_timer[which] = *itv;
  u.u_time.time = itv->it_value.tv_sec * 50 + itv->it_value.tv_usec / timer_resolution;
  Disable();
  for (node = timer_wait_queue.head; node; node = node->next)
  {
    struct utimenode *tnode = (struct utimenode *)node;
    if (tnode->time + ticks <= u.u_time.time)
      ticks += tnode->time;
    else
    {
      u.u_time.time -= ticks;
      tnode->time -= u.u_time.time;
      ixinsert(&timer_wait_queue, &u.u_time.node, node->prev);
      break;
    }
  }
  if (node == NULL)
  {
    u.u_time.time -= ticks;
    ixaddtail(&timer_wait_queue, &u.u_time.node);
  }
  Enable();

  return 0;
}

/*
 * Check that a proposed value to load into the .it_value or
 * .it_interval part of an interval timer is acceptable, and
 * fix it to have at least minimal value (i.e. if it is less
 * than the resolution of the clock, round it up.)
 */
int itimerfix(struct timeval *tv)
{
  usetup;

  if (tv->tv_sec < 0 || tv->tv_sec > 100000000 ||
      tv->tv_usec < 0 || tv->tv_usec >= 1000000)
    errno_return (EINVAL, -1);
  if (tv->tv_sec == 0 && tv->tv_usec != 0 && tv->tv_usec < timer_resolution)
    tv->tv_usec = timer_resolution;
  return 0;
}

/*
 * Decrement an interval timer by a specified number
 * of microseconds, which must be less than a second,
 * i.e. < 1000000.  If the timer expires, then reload
 * it.  In this case, carry over (usec - old value) to
 * reducint the value reloaded into the timer so that
 * the timer does not drift.  This routine assumes
 * that it is called in a context where the timers
 * on which it is operating cannot change in value.
 */

/* on 0 return, send a signal to process */

int
itimerdecr (struct itimerval *itp, int usec)    /* CALLED FROM INTERRUPT !! */
{
  if (itp->it_value.tv_usec < usec)
    {
      if (itp->it_value.tv_sec == 0)
	{
	  /* expired, and already in next interval */
	  usec -= itp->it_value.tv_usec;
	  goto expire;
	}

      itp->it_value.tv_usec += 1000000;
      itp->it_value.tv_sec--;
    }

  itp->it_value.tv_usec -= usec;
  usec = 0;
  if (timerisset(&itp->it_value))
    return (1);

  /* expired, exactly at end of interval */
expire:
  if (timerisset(&itp->it_interval))
    {
      itp->it_value = itp->it_interval;
      itp->it_value.tv_usec -= usec;
      if (itp->it_value.tv_usec < 0)
	{
	  itp->it_value.tv_usec += 1000000;
	  itp->it_value.tv_sec--;
	}
    }
  else
    itp->it_value.tv_usec = 0;          /* sec is already 0 */

  return (0);
}

/*
 * Add and subtract routines for timevals.
 * N.B.: subtract routine doesn't deal with
 * results which are before the beginning,
 * it just gets very confused in this case.
 * Caveat emptor.
 */
void timevaladd(struct timeval *t1, const struct timeval *t2)
{
  t1->tv_sec += t2->tv_sec;
  t1->tv_usec += t2->tv_usec;
  timevalfix(t1);
}

void timevalsub(struct timeval *t1, const struct timeval *t2)
{
  t1->tv_sec -= t2->tv_sec;
  t1->tv_usec -= t2->tv_usec;
  timevalfix(t1);
}

void timevalfix(struct timeval *t1)
{
  if (t1->tv_usec < 0) {
    t1->tv_sec--;
    t1->tv_usec += 1000000;
  }
  if (t1->tv_usec >= 1000000) {
    t1->tv_sec++;
    t1->tv_usec -= 1000000;
  }
}

#endif
