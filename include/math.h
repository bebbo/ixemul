/*
 * Copyright (c) 1985, 1990 The Regents of the University of California.
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
 *	@(#)math.h	5.8 (Berkeley) 4/2/91
 */

#ifndef	_MATH_H_
#define	_MATH_H_

#include <float.h>

#ifndef BITS
#define BITS(type)	(8 * (int)sizeof(type))
#endif

#define	M_E		2.7182818284590452354	/* e */
#define	M_LOG2E		1.4426950408889634074	/* log 2e */
#define	M_LOG10E	0.43429448190325182765	/* log 10e */
#define	M_LN2		0.69314718055994530942	/* log e2 */
#define	M_LN10		2.30258509299404568402	/* log e10 */
#define	M_PI		3.14159265358979323846	/* pi */
#define	M_PI_2		1.57079632679489661923	/* pi/2 */
#define	M_PI_4		0.78539816339744830962	/* pi/4 */
#define	M_1_PI		0.31830988618379067154	/* 1/pi */
#define	M_2_PI		0.63661977236758134308	/* 2/pi */
#define	M_2_SQRTPI	1.12837916709551257390	/* 2/sqrt(pi) */
#define	M_SQRT2		1.41421356237309504880	/* sqrt(2) */
#define	M_SQRT1_2	0.70710678118654752440	/* 1/sqrt(2) */

#include <sys/cdefs.h>

#if (defined(__GNUC__) || defined(__cplusplus)) && defined(__HAVE_68881__)
#include <math-68881.h>
#else

#define	HUGE_VAL	1e500			/* IEEE: positive infinity */

#if !defined(_ANSI_SOURCE) && !defined(_POSIX_SOURCE)
#define HUGE		DBL_MAX
#endif

__BEGIN_DECLS
double	acos __P((double));
double	asin __P((double));
double	atan __P((double));
double	atan2 __P((double, double));
double	ceil __P((double));
double	cos __P((double));
double	cosh __P((double));
double	exp __P((double));
double	fabs __P((double));
double	floor __P((double));
double	fmod __P((double, double));
double	frexp __P((double, int *));
double	ldexp __P((double, int));
double	log __P((double));
double	log10 __P((double));
double	modf __P((double, double *));
double	pow __P((double, double));
double	sin __P((double));
double	sinh __P((double));
double	sqrt __P((double));
double	tan __P((double));
double	tanh __P((double));

#if !defined(_ANSI_SOURCE) && !defined(_POSIX_SOURCE)
double	acosh __P((double));
double	asinh __P((double));
double	atanh __P((double));
double	cabs();		/* we can't describe cabs()'s argument properly */
double	cbrt __P((double));
double	copysign __P((double, double));
double	drem __P((double, double));
double	erf __P((double));
double	erfc __P((double));
double	expm1 __P((double));
int	finite __P((double));
double	hypot __P((double, double));
#if defined(vax) || defined(tahoe)
double	infnan __P((int));
#endif
double	j0 __P((double));
double	j1 __P((double));
double	jn __P((int, double));
double	lgamma __P((double));
double	log1p __P((double));
double	logb __P((double));
double	rint __P((double));
double	scalb __P((double, int));
double	y0 __P((double));
double	y1 __P((double));
double	yn __P((int, double));
#endif

__END_DECLS

#endif /* __HAVE_68881__ */

#define HAVE_FUNC_ISINF 1
#define HAVE_FUNC_ISNAN 1
#define rint(x) (floor(x + 0.5))
#define rintf(x) ((float)(floor(x + 0.5)))
#define signbit(x) (x < 0)
#define lroundf(x) ((int)(x > 0 ? x + 0.5 : x - 0.5))
#define HAVE_CEILF
#define ceilf(x) ((float)(ceil(x)))
#define HAVE_FLOORF
#define floorf(x) ((float)(floor(x)))
#define HAVE_LROUND
//#define lround(x) ((int)(x > 0 ? x + 0.5 : x - 0.5))
#define HAVE_ROUNDF
//#define roundf(x) ((int)(x > 0 ? x + 0.5 : x - 0.5))
#define roundf(x) ((int)(x > 0) ? floor(x + 0.5) : ceil(x - 0.5)) 
#define HAVE_ROUND
#define round(x) ((int)(x > 0) ? floor(x + 0.5) : ceil(x - 0.5)) 
//#define round(x) ((int)(x > 0 ? x + 0.5 : x - 0.5))
#define HAVE_frexpf
#define frexpf(x,exp) ((float) frexp(x, exp)) 
#define HAVE_LDEXPF 
#define ldexpf(x,exp) ((float) ldexp(x, exp)) 
#define powf(x,y) ((float) pow(x, y)) 
#define HAVE_SINF
#define sinf(x) ((float)sin(x))
#define HAVE_COSF
#define cosf(x) ((float)cos(x))
#define HAVE_FMODF
#define fmodf(x, y) ((float)fmod(x, y))
#define HAVE_ATAN2F
#define atan2f(x, y) ((float)atan2(x, y))
#define HAVE_SQRTF
#define sqrtf(x) ((float)sqrt(x))
#define trunc(x) floor(x)
#define truncf(x) ((float)floor(x))
#define  cbrt(x)   (pow((x),1./3.))
#define  cbrtf(x)  ((float) (pow((x),1./3.)))
#define isfinite(val) (!isnan(val) && !isinf(val))
//#define isfinite(val) (!isinf(val))
#define NAN (0.0/0.0)
#define INFINITY (1.0/0.0)




__BEGIN_DECLS
int	isinf __P((double));
int	isnan __P((double));
__END_DECLS

#endif /* _MATH_H_ */