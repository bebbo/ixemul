/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)sleep.c	5.5 (Berkeley) 6/1/90";
#endif /* LIBC_SCCS and not lint */

#define _KERNEL
#include "ixemul.h"
#include <sys/time.h>

static void
sleephandler_sleep(void)
{
        usetup;
	u.u_ringring = 1;
}

u_int sleep(u_int seconds)
{
        usetup;
	register struct itimerval *itp;
	struct itimerval itv, oitv;
	struct sigvec vec, ovec;
	long omask;

	itp = &itv;
	if (!seconds)
		return 0;
	timerclear(&itp->it_interval);
	timerclear(&itp->it_value);
	if (syscall (SYS_setitimer, ITIMER_REAL, itp, &oitv) < 0)
		return 0;
	itp->it_value.tv_sec = seconds;
	if (timerisset(&oitv.it_value)) {
		if (timercmp(&oitv.it_value, &itp->it_value, >))
			oitv.it_value.tv_sec -= itp->it_value.tv_sec;
		else {
			itp->it_value = oitv.it_value;
			/*
			 * This is a hack, but we must have time to return
			 * from the setitimer after the alarm or else it'll
			 * be restarted.  And, anyway, sleep never did
			 * anything more than this before.
			 */
			oitv.it_value.tv_sec = 1;
			oitv.it_value.tv_usec = 0;
		}
	}
	vec.sv_handler = sleephandler_sleep;
        vec.sv_mask = vec.sv_onstack = 0;
	syscall (SYS_sigvec, SIGALRM, &vec, &ovec);
	omask = syscall (SYS_sigblock, sigmask(SIGALRM));
	u.u_ringring = 0;
	(void) syscall (SYS_setitimer, ITIMER_REAL, itp, (struct itimerval *)0);
	while (!u.u_ringring)
		syscall (SYS_sigpause, omask & ~sigmask(SIGALRM));
	(void) syscall (SYS_sigvec, SIGALRM, &ovec, (struct sigvec *)0);
	(void) syscall (SYS_sigsetmask, omask);
	(void) syscall (SYS_setitimer, ITIMER_REAL, &oitv, (struct itimerval *)0);
	return 0;
}
