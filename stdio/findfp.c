/*	$NetBSD: findfp.c,v 1.6 1995/02/02 02:09:17 jtc Exp $	*/

/*-
 * Copyright (c) 1990, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * Chris Torek.
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
#if 0
static char sccsid[] = "@(#)findfp.c	8.2 (Berkeley) 1/4/94";
#endif
static char rcsid[] = "$NetBSD: findfp.c,v 1.6 1995/02/02 02:09:17 jtc Exp $";
#endif /* LIBC_SCCS and not lint */

#define _KERNEL
#include "ixemul.h"

#include <sys/param.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include "local.h"

#define	NDYNAMIC 10		/* add ten more whenever necessary */

void
__init_stdinouterr (void)
{
  FILE *fp;
  usetup;

  fp = u.u_sF[0] = __sfp();
  if (fp)
    {
      fp->_flags  = __SRD;
      fp->_file   = STDIN_FILENO;
      fp->_cookie = fp;
      fp->_close  = __sclose;
      fp->_read   = __sread;
      fp->_seek   = __sseek;
      fp->_write  = __swrite;
    }

  fp = u.u_sF[1] = __sfp();
  if (fp)
    {
      fp->_flags  = __SWR;
      fp->_file   = STDOUT_FILENO;
      fp->_cookie = fp;
      fp->_close  = __sclose;
      fp->_read   = __sread;
      fp->_seek   = __sseek;
      fp->_write  = __swrite;
    }

  fp = u.u_sF[2] = __sfp();
  if (fp)
    {
      fp->_flags  = __SRW|__SNBF;
      fp->_file   = STDERR_FILENO;
      fp->_cookie = fp;
      fp->_close  = __sclose;
      fp->_read   = __sread;
      fp->_seek   = __sseek;
      fp->_write  = __swrite;
    }
}

static struct glue *
moreglue(n)
	register int n;
{
	register struct glue *g;
	register FILE *p;
	static FILE empty;

	g = (struct glue *)malloc(sizeof(*g) + ALIGNBYTES + n * sizeof(FILE));
	if (g == NULL)
		return (NULL);
	p = (FILE *)ALIGN(g + 1);
	g->next = NULL;
	g->niobs = n;
	g->iobs = p;
	while (--n >= 0)
		*p++ = empty;
	return (g);
}

/*
 * Find a free FILE for fopen et al.
 */
FILE *
__sfp()
{
	usetup;
	register FILE *fp;
	register int n;
	register struct glue *g;

	for (g = &__sglue;; g = g->next) {
		for (fp = g->iobs, n = g->niobs; --n >= 0; fp++)
			if (fp->_flags == 0)
				goto found;
		if (g->next == NULL && (g->next = moreglue(NDYNAMIC)) == NULL)
			break;
	}
	return (NULL);
found:
	fp->_flags = 1;		/* reserve this slot; caller sets real flags */
	fp->_p = NULL;		/* no current pointer */
	fp->_w = 0;		/* nothing to read or write */
	fp->_r = 0;
	fp->_bf._base = NULL;	/* no buffer */
	fp->_bf._size = 0;
	fp->_lbfsize = 0;	/* not line buffered */
	fp->_file = -1;		/* no file */
/*	fp->_cookie = <any>; */	/* caller sets cookie, _read/_write etc */
	fp->_ub._base = NULL;	/* no ungetc buffer */
	fp->_ub._size = 0;
	fp->_lb._base = NULL;	/* no line buffer */
	fp->_lb._size = 0;
	return (fp);
}

/*
 * XXX.  Force immediate allocation of internal memory.  Not used by stdio,
 * but documented historically for certain applications.  Bad applications.
 */
void
f_prealloc()
{
	usetup;
	register struct glue *g;
	int n;

	n = getdtablesize() - FOPEN_MAX + 20;		/* 20 for slop. */
	for (g = &__sglue; (n -= g->niobs) > 0 && g->next; g = g->next)
		/* void */;
	if (n > 0)
		g->next = moreglue(n);
}

/*
 * exit() calls _cleanup() through *__cleanup, set whenever we
 * open or buffer a file.  This chicanery is done so that programs
 * that do not use stdio need not link it all in.
 *
 * The name `_cleanup' is, alas, fairly well known outside stdio.
 */
void
_cleanup()
{
	/* (void) _fwalk(fclose); */
	(void) _fwalk(__sflush);		/* `cheating' */
}
