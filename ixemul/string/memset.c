/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
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
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
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
static char sccsid[] = "@(#)memset.c    5.6 (Berkeley) 1/26/91";
#endif /* LIBC_SCCS and not lint */

#include <sys/cdefs.h>
#include <string.h>

#ifdef __PPC__

/* Relatively fast memset for PPC. - Piru */

#undef USE_INDEX
#undef USE_WRITE64
#define USE_INDEX	1
#define USE_WRITE64	0

void *memset(void *b, int c, size_t len)
{
  typedef unsigned char u8;
  typedef unsigned int  u32;
#if USE_WRITE64
  typedef double        LTYPE;
#else
  typedef unsigned int  LTYPE;
#endif
  const size_t LSIZE = sizeof(LTYPE);
  const size_t LMASK = sizeof(LTYPE) - 1;

  void *dst = b;

#if 0
  if ((u8) c == 0)
  {
    bzero(b, len);
    return dst;
  }
#endif

  if (len > LSIZE * 4)
  {
    LTYPE fc;
    size_t n, o;
#if USE_WRITE64
    u32 ft, *fcp = (u32 *) &fc;

    ft = (u8) c;
    ft |= (ft << 24) | (ft << 16) | (ft << 8);
    fcp[0] = ft; fcp[1] = ft;
#else
    fc = (u8) c;
    fc |= (fc << 24) | (fc << 16) | (fc << 8);
#endif

    n = (size_t)b & LMASK;
    if (n)
    {
      len -= n;
      do
        *((u8 *)b)++ = c;
      while (--n);
    }

    n = len / LSIZE;
    len &= LMASK;

    if (n >= 8)
    {
      o = n / 8;
      n &= 7;

#if USE_INDEX

      do
      {
        ((LTYPE *)b)[0] = fc;
        ((LTYPE *)b)[1] = fc;
        ((LTYPE *)b)[2] = fc;
        ((LTYPE *)b)[3] = fc;
        ((LTYPE *)b)[4] = fc;
        ((LTYPE *)b)[5] = fc;
        ((LTYPE *)b)[6] = fc;
        ((LTYPE *)b) += 8;
        ((LTYPE *)b)[-1] = fc;

      } while (--o);

#else

      ((LTYPE *)b)--;
      do
      {
        *++((LTYPE *)b) = fc;
        *++((LTYPE *)b) = fc;
        *++((LTYPE *)b) = fc;
        *++((LTYPE *)b) = fc;

        *++((LTYPE *)b) = fc;
        *++((LTYPE *)b) = fc;
        *++((LTYPE *)b) = fc;
        *++((LTYPE *)b) = fc;

      } while (--o);
      ((LTYPE *)b)++;

#endif
    }

    while (n--)
      *((LTYPE *)b)++ = fc;
  }

  while (len--)
    *((u8 *)b)++ = c;

  return dst;
}

#else

void *
memset(dst, c, n)
	void *dst;
	register int c;
	register size_t n;
{

	if (n != 0) {
		register char *d = dst;

		do
			*d++ = c;
		while (--n != 0);
	}
	return (dst);
}

#endif
