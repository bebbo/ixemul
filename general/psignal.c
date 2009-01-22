/*
 * Copyright (c) 1983 Regents of the University of California.
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
static char sccsid[] = "@(#)psignal.c   5.4 (Berkeley) 6/1/90";
#endif /* LIBC_SCCS and not lint */

#define _KERNEL
#include "ixemul.h"

/*
 * Print the name of the signal indicated
 * along with the supplied message.
 */

void
psignal(u_int sig, const char *s)
{
	register char *c;
	register int n;

	if (sig < NSIG)
		c = sys_siglist[sig];
	else
		c = "Unknown signal";
	n = strlen(s);
	if (n) {
		(void) syscall (SYS_write, 2, s, n);
		(void) syscall (SYS_write, 2, ": ", 2);
	}
	(void) syscall (SYS_write, 2, c, strlen(c));
	(void) syscall (SYS_write, 2, "\n", 1);
}
