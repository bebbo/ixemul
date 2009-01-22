/*
 * J.T. Conklin, December 12, 1994
 * Public Domain
 */

/*#ifdef __PPC__

__dead void
#ifdef __STDC__
_err(int eval, const char *fmt, ...)
#else
_err(va_alist)
	va_dcl
#endif
{
	va_list ap;
#if __STDC__
	va_start(ap, fmt);
#else
	int eval;
	const char *fmt;

	va_start(ap);
	eval = va_arg(ap, int);
	fmt = va_arg(ap, const char *);
#endif
	verr(eval, fmt, ap);
	va_end(ap);
}

#else

#include <sys/cdefs.h>

#ifdef __indr_reference
__indr_reference(_err, err);
#else*/

#define _err    err
#define _errx   errx
#define _warn   warn
#define _warnx  warnx
#define _verr   verr
#define _verrx  verrx
#define _vwarn  vwarn
#define _vwarnx vwarnx
#define rcsid   _rcsid
#include "err.c"

/*#endif
#endif*/
