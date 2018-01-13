/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
	.asciz "@(#)setjmp.s	5.1 (Berkeley) 5/12/90"
#endif /* LIBC_SCCS and not lint */

/*
 * C library -- setjmp, longjmp
 *
 *	longjmp(a,v)
 * will generate a "return(v)" from
 * the last call to
 *	setjmp(a)
 * by restoring registers from the stack,
 * and a struct sigcontext, see <signal.h>
 */

#include "defs.h"
#include "ix_internals.h"

ENTRY(sigsetjmp)
asm("
	movl	sp@(4),a0
	movl	sp@(8),a0@
	addl	#4,a0
	movl	a0,sp@(4)
	tstl	a0@(-4)
	jne	_setjmp
");

ENTRY(_setjmp)
asm("
	moveq	#0,d0		/* dummy signal mask */
	movel	d0,d1		/*  and onstack */
	jra	setup_setjmp
");

asm("
	/* routine to obtain current values of onstack/sigmask */
get_onstack_sigmask:
	subql	#8,sp		/* space for sigstack args/rvals */
	clrl	sp@		/* don't change it... */
	movl	sp,sp@(4)	/* ...but return the current val */
	jsr	_sigstack	/* note: onstack returned in sp@(4) */
	clrl	sp@		/* don't change mask, just return */
	jsr	_sigblock	/*   old value */
	movl	sp@(4),d1	/* old onstack value */
	addql	#8,sp
	rts			/* d0 = sigmask, d1 = onstack */
");

#define _MAKESTR(o) #o
#define MAKESTR(o) _MAKESTR(o)

ENTRY(setjmp)
asm("
	jsr	get_onstack_sigmask

setup_setjmp:
	movl	sp@(4),a0	/* save area pointer */
	movl	d1,a0@+		/* save old onstack value */
	movl	d0,a0@+		/* save old signal mask */
	lea	sp@(4),a1	/* adjust saved SP since we won't rts */
	movl	a1,a0@+		/* save old SP */
	movl	a5,a0@+		/* save old FP */

	movel	4:w,a1
	movew	a1@(0x126),a0@(2) /* use TDNestCnt and IDNestCnt from Sysbase ! */
	lea	a0@(4),a0	/* skip 4 bytes (the first two bytes used to */
				/* 	contain task specific flags) */
	movl	sp@,a0@+	/* save old PC */
	clrl	a0@+		/* clean PS */
	moveml	d2-d7/a2-a4/a6,a0@	/* save remaining non-scratch regs */
	clrl	d0		/* return 0 */
	rts
");

/* _clean_longjmp, do NOT change onstack/sigmask, do NOT change stackframe */
ENTRY(_clean_longjmp)
asm("
	jsr	get_onstack_sigmask
	movl	sp@(4),a0	/* get area pointer */
	movl	d1,a0@+		/* save current onstack value */
	movl	d0,a0@		/* save current signal mask */

	movl	sp@(4),a0	/* save area pointer */
	movel	a0@(8),d0	/* ensure non-zero SP */
	jeq	botch		/* oops! */
	movl	sp@(8),d1	/* grab return value */
	jne	ok2		/* non-zero ok */
	moveq	#1,d1		/* else make non-zero */
	jra	ok2
");

ENTRY(siglongjmp)
asm("
	movl	sp@(4),a0
	addl	#4,a0
	movl	a0,sp@(4)
	tstl	a0@(-4)
	jne	_longjmp
");

/* _longjmp, do NOT change onstack/sigmask */
ENTRY(_longjmp)
asm("
	jsr	get_onstack_sigmask
	movl	sp@(4),a0	/* get area pointer */
	movl	d1,a0@+		/* save current onstack value */
	movl	d0,a0@		/* save current signal mask */
	/* fall through */
");

ENTRY(longjmp)
asm("
	movl	sp@(4),a0	/* save area pointer */
	movl	a0@(8),d0	/* ensure non-zero SP */
	jeq	botch		/* oops! */
	movl	sp@(8),d1	/* grab return value */
	jne	ok1		/* non-zero ok */
	moveq	#1,d1		/* else make non-zero */
ok1:
	jbsr	___stkrst	/* Go to correct stackframe, d0 should */
				/* contain the SP */
ok2:
	movl	d1,d0
	moveml	a0@(28),d2-d7/a2-a4/a6	/* restore non-scratch regs */
	movl	a0,sp@-		/* let sigreturn */
	jsr	_sigreturn	/*   finish for us */

botch:
	jsr	_longjmperror
	stop	#0
");

/*
 * This routine is called from longjmp() when an error occurs.
 * Programs that wish to exit gracefully from this error may
 * write their own versions.
 */

void
longjmperror()
{
#define	ERRMSG	"longjmp botch\n"
	syscall (SYS_write, 2, ERRMSG, sizeof(ERRMSG) - 1);
	syscall (SYS_abort);
}
