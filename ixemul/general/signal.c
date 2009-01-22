/*
 * Copyright (c) 1985, 1989 Regents of the University of California.
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
static char sccsid[] = "@(#)signal.c    5.5 (Berkeley) 6/1/90";
#endif /* LIBC_SCCS and not lint */

/*
 * Almost backwards compatible signal.
 */
#define _KERNEL
#include "ixemul.h"

sig_t
signal(int s, sig_t a)
{
	struct sigaction sa, osa;

	sa.sa_handler = a;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sa.sa_flags |= SA_RESTART;
	if (syscall (SYS_sigaction, s, &sa, &osa) < 0)
		return (BADSIG);
	return (osa.sa_handler);
}

#ifdef NATIVE_MORPHOS

sig_t
_trampoline_signal(void)
{
	int *p = (int *)REG_A7;
	int s = p[1];
	sig_t a = (sig_t)p[2];

	if (a != SIG_DFL && a != SIG_IGN && a != SIG_ERR)
		a = (sig_t)((int)a ^ 1);

	a = signal(s, a);

	if (a != SIG_DFL && a != SIG_IGN && a != SIG_ERR)
		a = (sig_t)((int)a ^ 1);

	return a;
}

struct EmulLibEntry _gate_signal = {
  TRAP_LIB, 0, (void(*)())_trampoline_signal
};

#endif

