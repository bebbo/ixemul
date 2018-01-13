/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
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
static char sccsid[] = "@(#)atexit.c	5.1 (Berkeley) 5/15/90";
#endif /* LIBC_SCCS and not lint */

#define _KERNEL
#include "ixemul.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "atexit.h"

#define __atexit (u.u_atexit)

/*
 * Register a function to be performed at exit.
 */
int
atexit(void (*fn)())
{
        usetup;
	register struct atexit *p;

	if ((p = __atexit) == 0)
	  {
	    /* don't need a static table anymore, since OpenLibrary() comes
	     * before ix_startup() */
	    __atexit = p = (struct atexit *) syscall (SYS_malloc, sizeof (*p));
	    bzero (p, sizeof (*p));
	  }

	if (p->ind >= ATEXIT_SIZE) {
		if ((p = (struct atexit *) syscall (SYS_malloc, sizeof(*p))) == NULL)
			return (-1);
#if 0
		/* this was in the original source */
		__atexit->next = p;
		__atexit = p;
#else
		p->next = __atexit;
		__atexit = p;
		p->ind = 0;
#endif
	}
	p->fns[p->ind++] = fn;
	return (0);
}
