/*
 * Copyright (c) 1990 Regents of the University of California.
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
static char sccsid[] = "@(#)bsearch.c   5.3 (Berkeley) 5/17/90";
#endif /* LIBC_SCCS and not lint */

#define _KERNEL
#include "ixemul.h"

#include <stddef.h>             /* size_t */
#include <stdlib.h>

#ifdef NATIVE_MORPHOS
#define bsearch   _bsearch
#define FLAGPRM     ,int is68k

/* code for:
	movem.l d0-d1,-(sp)
	jsr     (a0)
	addq.l  #8,sp
	rts
*/
static const UWORD compar_gate[] = {
	0x48E7,0xC000,0x4E90,0x508F,0x4E75
};

static inline int my_compar(const void *p, const void *q, int (*qcmp)(), int is68k) {
	if (is68k) {
		REG_D0 = (ULONG)p;
		REG_D1 = (ULONG)q;
		REG_A0 = (ULONG)qcmp;
		//REG_A4 = REG_A4;
		return MyEmulHandle->EmulCallDirect68k((APTR)compar_gate);
	} else {
		return qcmp(p, q);
	}
}
#define CMP(x, y)   my_compar(x, y, compar, is68k)

#else
#define FLAGPRM
#define CMP(x, y)   compar(x, y)
#endif

/*
 * Perform a binary search.
 *
 * The code below is a bit sneaky.  After a comparison fails, we
 * divide the work in half by moving either left or right. If lim
 * is odd, moving left simply involves halving lim: e.g., when lim
 * is 5 we look at item 2, so we change lim to 2 so that we will
 * look at items 0 & 1.  If lim is even, the same applies.  If lim
 * is odd, moving right again involes halving lim, this time moving
 * the base up one item past p: e.g., when lim is 5 we change base
 * to item 3 and make lim 2 so that we will look at items 3 and 4.
 * If lim is even, however, we have to shrink it by one before
 * halving: e.g., when lim is 4, we still looked at item 2, so we
 * have to make lim 3, then halve, obtaining 1, so that we will only
 * look at item 3.
 */
void *
bsearch(const void *key, const void *base0, 
	size_t nmemb, size_t size, int (*compar)()  FLAGPRM)
{
	register char *base = (char *)base0;
	register int lim, cmp;
	register void *p;

	for (lim = nmemb; lim != 0; lim >>= 1) {
		p = base + (lim >> 1) * size;
		cmp = CMP(key, p);
		if (cmp == 0)
			return (p);
		if (cmp > 0) {  /* key > p: move right */
			base = (char *)p + size;
			lim--;
		} /* else move left */
	}
	return (NULL);
}


#ifdef NATIVE_MORPHOS
#undef bsearch
#undef FLAGPRM
#undef CMP

void *
bsearch(const void *key, const void *base0, 
	size_t nmemb, size_t size, int (*compar)())
{
	return _bsearch(key, base0, nmemb, size, compar, 0);
}

void *
_trampoline_bsearch(void)
{
	int *p = (int *)REG_A7;
	const void *key = (void *)p[1];
	const void *base0 = (const void *)p[2];
	size_t nmemb = p[3];
	size_t size = p[4];
	int (*compar)() = (int(*)())p[5];

	return _bsearch(key, base0, nmemb, size, compar, 1);
}

const struct EmulLibEntry _gate_bsearch = {
	TRAP_LIB, 0, (void(*)())_trampoline_bsearch
};

#endif

