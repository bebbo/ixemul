/*	$NetBSD: stdlib.h,v 1.24 1995/03/22 01:08:31 jtc Exp $	*/

/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
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
 *	@(#)stdlib.h	5.13 (Berkeley) 6/4/91
 */

#ifndef _STDLIB_H_
#define _STDLIB_H_
#include <machine/ansi.h>

#if !defined(_ANSI_SOURCE)	/* for quad_t, etc. */
#include <sys/types.h>
#endif

#ifdef	_BSD_SIZE_T_
typedef	_BSD_SIZE_T_	size_t;
#undef	_BSD_SIZE_T_
#endif

#ifndef	__cplusplus
#ifndef	_WCHAR_T_
#define	_WCHAR_T_
typedef	__WCHAR_TYPE__	wchar_t;
#endif
#undef	_BSD_WCHAR_T_
#endif

typedef struct {
	int quot;		/* quotient */
	int rem;		/* remainder */
} div_t;

typedef struct {
	long quot;		/* quotient */
	long rem;		/* remainder */
} ldiv_t;

#if !defined(_ANSI_SOURCE)
typedef struct {
	quad_t quot;		/* quotient */
	quad_t rem;		/* remainder */
} qdiv_t;
#endif


#ifndef	NULL
#define	NULL	0
#endif

#define	EXIT_FAILURE	1
#define	EXIT_SUCCESS	0

#define	RAND_MAX	0x7fffffff

#define	MB_CUR_MAX	1	/* XXX */

#include <sys/cdefs.h>

#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L)
#define __MY_INLINE__ inline
#else
#define __MY_INLINE__ extern inline
#endif

__BEGIN_DECLS
#ifdef __NO_INLINE__
__stdargs int	 abs __P((int));
#else
__MY_INLINE__ __stdargs int abs(int j) { return j>=0?j:-j; }
#endif
__stdargs void	 abort __P((void));
__stdargs int	 atexit __P((__stdargs void (*)(void)));
__stdargs double	 atof __P((const char *));
__stdargs int	 atoi __P((const char *));
__stdargs long	 atol __P((const char *));
__stdargs void	*bsearch __P((const void *, const void *, size_t,
	    size_t, __stdargs int (*)(const void *, const void *)));
__stdargs void	*calloc __P((size_t, size_t));
__stdargs div_t	 div __P((int, int));
__stdargs void	 exit __P((int));
__stdargs void	 free __P((void *));
__stdargs char	*getenv __P((const char *));
__stdargs long	 labs __P((long));
__stdargs ldiv_t	 ldiv __P((long, long));
__stdargs void	*malloc __P((size_t));
__stdargs void	 qsort __P((void *, size_t, size_t,
    __stdargs int (*)(const void *, const void *)));
__stdargs int	 rand __P((void));
__stdargs void	*realloc __P((void *, size_t));
__stdargs void	 srand __P((unsigned));
__stdargs double	 strtod __P((const char *, char **));
__stdargs long	 strtol __P((const char *, char **, int));
__stdargs unsigned long
	 strtoul __P((const char *, char **, int));
__stdargs int	 system __P((const char *));

/* these are currently just stubs */
__stdargs int	 mblen __P((const char *, size_t));
__stdargs size_t	 mbstowcs __P((wchar_t *, const char *, size_t));
__stdargs int	 wctomb __P((char *, wchar_t));
__stdargs int	 mbtowc __P((wchar_t *, const char *, size_t));
__stdargs size_t	 wcstombs __P((char *, const wchar_t *, size_t));

#if !defined(_ANSI_SOURCE) && !defined(_POSIX_SOURCE)
#if defined(alloca) && (alloca == __builtin_alloca) && (__GNUC__ < 2)
__stdargs void  *alloca __P((int));     /* built-in for gcc */
#else 
__stdargs void  *alloca __P((size_t));
#endif /* __GNUC__ */ 

__stdargs char	*getbsize __P((int *, long *));
__stdargs char	*cgetcap __P((char *, char *, int));
__stdargs int	 cgetclose __P((void));
__stdargs int	 cgetent __P((char **, char **, char *));
__stdargs int	 cgetfirst __P((char **, char **));
__stdargs int	 cgetmatch __P((char *, char *));
__stdargs int	 cgetnext __P((char **, char **));
__stdargs int	 cgetnum __P((char *, char *, long *));
__stdargs int	 cgetset __P((char *));
__stdargs int	 cgetstr __P((char *, char *, char **));
__stdargs int	 cgetustr __P((char *, char *, char **));

__stdargs int	 daemon __P((int, int));
__stdargs char	*devname __P((int, int));
__stdargs int	 getloadavg __P((double [], int));

__stdargs long	 a64l __P((const char *));
__stdargs char	*l64a __P((long));

__stdargs void	 cfree __P((void *));

__stdargs int	 getopt __P((int, char * const *, const char *));
extern	 char *optarg;			/* getopt(3) external variables */
extern	 int opterr;
extern	 int optind;
extern	 int optopt;
extern	 int optreset;
__stdargs int	 getsubopt __P((char **, char * const *, char **));
extern	 char *suboptarg;		/* getsubopt(3) external variable */

__stdargs int	 heapsort __P((void *, size_t, size_t,
    __stdargs int (*)(const void *, const void *)));
__stdargs int	 mergesort __P((void *, size_t, size_t,
    __stdargs int (*)(const void *, const void *)));
__stdargs int	 radixsort __P((const unsigned char **, int, const unsigned char *,
	    unsigned));
__stdargs int	 sradixsort __P((const unsigned char **, int, const unsigned char *,
	    unsigned));

__stdargs char	*initstate __P((unsigned, char *, int));
__stdargs long	 random __P((void));
__stdargs char	*realpath __P((const char *, char *));
__stdargs char	*setstate __P((char *));
__stdargs void	 srandom __P((unsigned));

__stdargs int	 putenv __P((const char *));
__stdargs int	 setenv __P((const char *, const char *, int));
__stdargs void	 unsetenv __P((const char *));
__stdargs void	 setproctitle __P((const char *, ...));

__stdargs quad_t	 qabs __P((quad_t));
__stdargs qdiv_t	 qdiv __P((quad_t, quad_t));
__stdargs quad_t	 strtoq __P((const char *, char **, int));
__stdargs u_quad_t strtouq __P((const char *, char **, int));

__stdargs double	 drand48 __P((void));
__stdargs double	 erand48 __P((unsigned short[3]));
__stdargs long	 jrand48 __P((unsigned short[3]));
__stdargs void	 lcong48 __P((unsigned short[7]));
__stdargs long	 lrand48 __P((void));
__stdargs long	 mrand48 __P((void));
__stdargs long	 nrand48 __P((unsigned short[3]));
__stdargs unsigned short *seed48 __P((unsigned short[3]));
__stdargs void	 srand48 __P((long));
#endif /* !_ANSI_SOURCE && !_POSIX_SOURCE */

__END_DECLS

#endif /* _STDLIB_H_ */
