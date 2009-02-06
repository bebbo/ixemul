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

#define _KERNEL
#include "ixemul.h"
#include "defs.h"
//#include "ix_internals.h"

ENTRY(sigsetjmp)
asm(" \n\
	movl	sp@(4),a0 \n\
	movl	sp@(8),a0@ \n\
	addl	#4,a0 \n\
	movl	a0,sp@(4) \n\
	tstl	a0@(-4) \n\
	jne	_setjmp \n\
");

ENTRY(_setjmp)
asm(" \n\
	moveq	#0,d0		/* dummy signal mask */ \n\
	movel	d0,d1		/*  and onstack */ \n\
	jra	setup_setjmp \n\
");

asm(" \n\
	/* routine to obtain current values of onstack/sigmask */ \n\
get_onstack_sigmask: \n\
	subql	#8,sp		/* space for sigstack args/rvals */ \n\
	clrl	sp@		/* don't change it... */ \n\
	movl	sp,sp@(4)	/* ...but return the current val */ \n\
	jsr	_sigstack	/* note: onstack returned in sp@(4) */ \n\
	clrl	sp@		/* don't change mask, just return */ \n\
	jsr	_sigblock	/*   old value */ \n\
	movl	sp@(4),d1	/* old onstack value */ \n\
	addql	#8,sp \n\
	rts			/* d0 = sigmask, d1 = onstack */ \n\
");

#define _MAKESTR(o) #o
#define MAKESTR(o) _MAKESTR(o)

ENTRY(setjmp)
asm(" \n\
	jsr	get_onstack_sigmask \n\
\n\
setup_setjmp: \n\
	movl	sp@(4),a0	/* save area pointer */ \n\
	movl	d1,a0@+		/* save old onstack value */ \n\
	movl	d0,a0@+		/* save old signal mask */ \n\
	lea	sp@(4),a1	/* adjust saved SP since we won't rts */ \n\
	movl	a1,a0@+		/* save old SP */ \n\
	movl	a5,a0@+		/* save old FP */ \n\
\n\
	movel	4:w,a1 \n\
	movew	a1@(0x126),a0@(2) /* use TDNestCnt and IDNestCnt from Sysbase ! */ \n\
	lea	a0@(4),a0	/* skip 4 bytes (the first two bytes used to */ \n\
				/* 	contain task specific flags) */ \n\
	movl	sp@,a0@+	/* save old PC */ \n\
	clrl	a0@+		/* clean PS */ \n\
	moveml	d2-d7/a2-a4/a6,a0@	/* save remaining non-scratch regs */ \n\
	clrl	d0		/* return 0 */ \n\
	rts \n\
");

/* _clean_longjmp, do NOT change onstack/sigmask, do NOT change stackframe */
ENTRY(_clean_longjmp)
asm(" \n\
	jsr	get_onstack_sigmask \n\
	movl	sp@(4),a0	/* get area pointer */ \n\
	movl	d1,a0@+		/* save current onstack value */ \n\
	movl	d0,a0@		/* save current signal mask */ \n\
\n\
	movl	sp@(4),a0	/* save area pointer */ \n\
	movel	a0@(8),d0	/* ensure non-zero SP */ \n\
	jeq	botch		/* oops! */ \n\
	movl	sp@(8),d1	/* grab return value */ \n\
	jne	ok2		/* non-zero ok */ \n\
	moveq	#1,d1		/* else make non-zero */ \n\
	jra	ok2 \n\
");

ENTRY(siglongjmp)
asm(" \n\
	movl	sp@(4),a0 \n\
 	addl	#4,a0 \n\
	movl	a0,sp@(4) \n\
	tstl	a0@(-4) \n\
	jne	_longjmp \n\
");

/* _longjmp, do NOT change onstack/sigmask */
ENTRY(_longjmp)
asm(" \n\
	jsr	get_onstack_sigmask \n\
	movl	sp@(4),a0	/* get area pointer */ \n\
	movl	d1,a0@+		/* save current onstack value */ \n\
	movl	d0,a0@		/* save current signal mask */ \n\
	/* fall through */ \n\
");

ENTRY(longjmp)
asm(" \n\
	movl	sp@(4),a0	/* save area pointer */ \n\
	movl	a0@(8),d0	/* ensure non-zero SP */ \n\
	jeq	botch		/* oops! */ \n\
	movl	sp@(8),d1	/* grab return value */ \n\
	jne	ok1		/* non-zero ok */ \n\
	moveq	#1,d1		/* else make non-zero */ \n\
ok1: \n\
	jbsr	___stkrst	/* Go to correct stackframe, d0 should */ \n\
				/* contain the SP */ \n\
ok2: \n\
	movl	d1,d0 \n\
	moveml	a0@(28),d2-d7/a2-a4/a6	/* restore non-scratch regs */ \n\
	movl	a0,sp@-		/* let sigreturn */ \n\
	jsr	_sigreturn	/*   finish for us */ \n\
\n\
botch: \n\
	jsr	_longjmperror \n\
	stop	#0 \n\
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
