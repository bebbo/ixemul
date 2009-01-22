/*-
 * Copyright (c) 1980, 1983 The Regents of the University of California.
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

/* Modified by PerOla Valfridsson 910215 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)qsort.c     5.7 (Berkeley) 5/17/90";
#endif /* LIBC_SCCS and not lint */

#define _KERNEL
#include "ixemul.h"

#include <stdlib.h>

#ifdef NATIVE_MORPHOS
#define qsort   _qsort
#define FLAGPRM     ,int is68k
#define FLAGARG     , is68k

/* code for:
	movem.l d0-d1,-(sp)
	jsr     (a0)
	addq.l  #8,sp
	rts
*/
static const UWORD cmp_gate[] = {
	0x48E7,0xC000,0x4E90,0x508F,0x4E75
};

static inline int my_cmp(const void *p, const void *q, int (*qcmp)(), int is68k) {
	if (is68k) {
		REG_D0 = (ULONG)p;
		REG_D1 = (ULONG)q;
		REG_A0 = (ULONG)qcmp;
		//REG_A4 = REG_A4;
		return MyEmulHandle->EmulCallDirect68k((APTR)cmp_gate);
	} else {
		return qcmp(p, q);
	}
}
#define CMP(x, y)   my_cmp(x, y, qcmp, is68k)

#else
#define FLAGPRM
#define FLAGARG
#define CMP(x, y)   qcmp(x, y)
#endif

static void qst(char *base, char *max, int (*qcmp)(), int qsz  FLAGPRM);

/*
 * qsort.c:
 * Our own version of the system qsort routine which is faster by an average
 * of 25%, with lows and highs of 10% and 50%.
 * The THRESHold below is the insertion sort threshold, and has been adjusted
 * for records of size 48 bytes.
 * The MTHREShold is where we stop finding a better median.
 */

#define         THRESH          4               /* threshold for insertion */
#define         MTHRESH         6               /* threshold for median */

/*
 * qsort:
 * First, set up some global parameters for qst to share.  Then, quicksort
 * with qst(), and then a cleanup insertion sort ourselves.  Sound simple?
 * It's not...
 */

void
qsort(void *base, size_t n, size_t qsz, int (*qcmp)()  FLAGPRM)
{
	register char c, *i, *j, *lo, *hi;
	char *min, *max;

	if (n <= 1)
		return;

	max = base + n * qsz;
	if (n >= THRESH) {
		qst(base, max, qcmp, qsz  FLAGARG);
		hi = base + qsz * THRESH;
	} else {
		hi = max;
	}
	/*
	 * First put smallest element, which must be in the first THRESH, in
	 * the first position as a sentinel.  This is done just by searching
	 * the first THRESH elements (or the first n if n < THRESH), finding
	 * the min, and swapping it into the first position.
	 */
	for (j = lo = base; (lo += qsz) < hi; )
		if (CMP(j, lo) > 0)
			j = lo;
	if (j != base) {
		/* swap j into place */
		for (i = base, hi = base + qsz; i < hi; ) {
			c = *j;
			*j++ = *i;
			*i++ = c;
		}
	}
	/*
	 * With our sentinel in place, we now run the following hyper-fast
	 * insertion sort.  For each remaining element, min, from [1] to [n-1],
	 * set hi to the index of the element AFTER which this one goes.
	 * Then, do the standard insertion sort shift on a character at a time
	 * basis for each element in the frob.
	 */
	for (min = base; (hi = (min += qsz)) < max; ) {
		while (CMP((hi -= qsz), min) > 0)
			/* void */;
		if ((hi += qsz) != min) {
			for (lo = (min + qsz); --lo >= min; ) {
				c = *lo;
				for (i = j = lo; (j -= qsz) >= hi; i = j)
					*i = *j;
				*i = c;
			}
		}
	}
}

/*
 * qst:
 * Do a quicksort
 * First, find the median element, and put that one in the first place as the
 * discriminator.  (This "median" is just the median of the first, last and
 * middle elements).  (Using this median instead of the first element is a big
 * win).  Then, the usual partitioning/swapping, followed by moving the
 * discriminator into the right place.  Then, figure out the sizes of the two
 * partions, do the smaller one recursively and the larger one via a repeat of
 * this code.  Stopping when there are less than THRESH elements in a partition
 * and cleaning up with an insertion sort (in our caller) is a huge win.
 * All data swaps are done in-line, which is space-losing but time-saving.
 * (And there are only three places where this is done).
 */

static void qst(char *base, char *max, int (*qcmp)(), int qsz  FLAGPRM)
{
	register char c, *i, *j, *jj;
	register int ii;
	char *mid, *tmp;
	int lo, hi;

	/*
	 * At the top here, lo is the number of characters of elements in the
	 * current partition.  (Which should be max - base).
	 * Find the median of the first, last, and middle element and make
	 * that the middle element.  Set j to largest of first and middle.
	 * If max is larger than that guy, then it's that guy, else compare
	 * max with loser of first and take larger.  Things are set up to
	 * prefer the middle, then the first in case of ties.
	 */
	lo = max - base;                /* number of elements as chars */
	do      {
		mid = i = base + qsz * ((lo / qsz) >> 1);
		if (lo >= qsz * MTHRESH) {
			j = (CMP((jj = base), i) > 0 ? jj : i);
			if (CMP(j, (tmp = max - qsz)) > 0) {
				/* switch to first loser */
				j = (j == jj ? i : jj);
				if (CMP(j, tmp) < 0)
					j = tmp;
			}
			if (j != i) {
				ii = qsz;
				do      {
					c = *i;
					*i++ = *j;
					*j++ = c;
				} while (--ii);
			}
		}
		/*
		 * Semi-standard quicksort partitioning/swapping
		 */
		for (i = base, j = max - qsz; ; ) {
			while (i < mid && CMP(i, mid) <= 0)
				i += qsz;
			while (j > mid) {
				if (CMP(mid, j) <= 0) {
					j -= qsz;
					continue;
				}
				tmp = i + qsz;  /* value of i after swap */
				if (i == mid) {
					/* j <-> mid, new mid is j */
					mid = jj = j;
				} else {
					/* i <-> j */
					jj = j;
					j -= qsz;
				}
				goto swap;
			}
			if (i == mid) {
				break;
			} else {
				/* i <-> mid, new mid is i */
				jj = mid;
				tmp = mid = i;  /* value of i after swap */
				j -= qsz;
			}
		swap:
			ii = qsz;
			do      {
				c = *i;
				*i++ = *jj;
				*jj++ = c;
			} while (--ii);
			i = tmp;
		}
		/*
		 * Look at sizes of the two partitions, do the smaller
		 * one first by recursion, then do the larger one by
		 * making sure lo is its size, base and max are update
		 * correctly, and branching back.  But only repeat
		 * (recursively or by branching) if the partition is
		 * of at least size THRESH.
		 */
		i = (j = mid) + qsz;
		if ((lo = j - base) <= (hi = max - i)) {
			if (lo >= qsz * THRESH)
				qst(base, j, qcmp, qsz  FLAGARG);
			base = i;
			lo = hi;
		} else {
			if (hi >= qsz * THRESH)
				qst(i, max, qcmp, qsz  FLAGARG);
			max = j;
		}
	} while (lo >= qsz * THRESH);
}

#ifdef NATIVE_MORPHOS
#undef qsort
#undef CMP
#undef FLAGPRM
#undef FLAGARG

void
qsort(void *base, size_t n, size_t qsz, int (*qcmp)())
{
	_qsort(base, n, qsz, qcmp, 0);
}

void
_trampoline_qsort(void)
{
	int *p = (int *)REG_A7;
	void *base = (void *)p[1];
	size_t n = p[2];
	size_t qsz = p[3];
	int (*qcmp)() = (int(*)())p[4];

	_qsort(base, n, qsz, qcmp, 1);
}

const struct EmulLibEntry _gate_qsort = {
	TRAP_LIBNR, 0, (void(*)())_trampoline_qsort
};

#endif

