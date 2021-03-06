/*	$NetBSD: signal.h,v 1.7 1995/05/28 03:10:06 jtc Exp $	*/

/*-
 * Copyright (c) 1991, 1993
 *	The Regents of the University of California.  All rights reserved.
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
 *
 *	@(#)signal.h	8.3 (Berkeley) 3/30/94
 */

#ifndef _USER_SIGNAL_H
#define _USER_SIGNAL_H

#include <sys/cdefs.h>
#include <sys/signal.h>

#if !defined(_ANSI_SOURCE)
#include <sys/types.h>
#endif

#if !defined(_ANSI_SOURCE) && !defined(_POSIX_SOURCE)
extern __const char *__const sys_signame[];
#ifdef _KERNEL
extern char *sys_siglist[];
#else
extern const char *const sys_siglist[];
#endif
#endif

__BEGIN_DECLS
__stdargs int	raise __P((int));
#ifndef	_ANSI_SOURCE
__stdargs int	kill __P((pid_t, int));
__stdargs int	sigaction __P((int, const struct sigaction *, struct sigaction *));
#if defined(__GNUC__) && defined(__STDC__)
#ifdef __EXTERN_INLINE__
__stdargs int	sigaddset __P((sigset_t *, int));
__stdargs int	sigdelset __P((sigset_t *, int));
__stdargs int	sigismember __P((const sigset_t *, int));
#endif
#endif
__stdargs int	sigemptyset __P((sigset_t *));
__stdargs int	sigfillset __P((sigset_t *));
__stdargs int	sigpending __P((sigset_t *));
__stdargs int	sigprocmask __P((int, const sigset_t *, sigset_t *));
__stdargs int	sigsuspend __P((const sigset_t *));

#if defined(__GNUC__) && defined(__STDC__)
#ifndef __EXTERN_INLINE__
#define __EXTERN_INLINE__ static inline
#endif
__stdargs __EXTERN_INLINE__ int sigaddset(sigset_t *set, int signo) {
	extern int errno;

	if (signo <= 0 || signo >= _NSIG) {
		errno = 22;			/* EINVAL */
		return -1;
	}
	*set |= (1 << ((signo)-1));		/* sigmask(signo) */
	return (0);
}

__stdargs __EXTERN_INLINE__ int sigdelset(sigset_t *set, int signo) {
	extern int errno;

	if (signo <= 0 || signo >= _NSIG) {
		errno = 22;			/* EINVAL */
		return -1;
	}
	*set &= ~(1 << ((signo)-1));		/* sigmask(signo) */
	return (0);
}

__stdargs __EXTERN_INLINE__ int sigismember(const sigset_t *set, int signo) {
	extern int errno;

	if (signo <= 0 || signo >= _NSIG) {
		errno = 22;			/* EINVAL */
		return -1;
	}
	return ((*set & (1 << ((signo)-1))) != 0);
}
#endif

/* List definitions after function declarations, or Reiser cpp gets upset. */
#define	sigemptyset(set)	(*(set) = 0, 0)
#define	sigfillset(set)		(*(set) = ~(sigset_t)0, 0)

#ifndef _POSIX_SOURCE
__stdargs int	killpg __P((pid_t, int));
__stdargs int	sigblock __P((int));
__stdargs int	siginterrupt __P((int, int));
__stdargs int	sigpause __P((int));
__stdargs void volatile	sigreturn __P((struct sigcontext *));
__stdargs int	sigsetmask __P((int));
__stdargs int	sigstack __P((const struct sigstack *, struct sigstack *));
__stdargs int	sigvec __P((int, struct sigvec *, struct sigvec *));
__stdargs void	psignal __P((unsigned int, const char *));
#endif	/* !_POSIX_SOURCE */
#endif	/* !_ANSI_SOURCE */
__END_DECLS

#endif	/* !_USER_SIGNAL_H */
