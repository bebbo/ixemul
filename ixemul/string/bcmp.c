/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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

/* bcmp(s1, s2, n) */

#include "defs.h"

#ifndef mc68000
//#include <sys/syscall.h>
#include <string.h>

int   bcmp(const void *a, const void *b, size_t size)
{
      return (int)memcmp(a,b,size);
//    return (int)_syscall3(SYS_bcmp,a,b,size);
}
#else
/*
 * This is probably not the best we can do, but it is still 2-10 times
 * faster than the C version in the portable gen directory.
 *
 * Things that might help:
 *      - longword align when possible (only on the 68020)
 *      - use nested DBcc instructions or use one and limit size to 64K
 */
ENTRY(bcmp)
asm(" \n\
	movl    sp@(4),a0       /* string 1 */ \n\
	movl    sp@(8),a1       /* string 2 */ \n\
	movl    sp@(12),d0      /* length */ \n\
	jeq     bcdone_bcmp     /* if zero, nothing to do */ \n\
	movl    a0,d1 \n\
	btst    #0,d1           /* string 1 address odd? */ \n\
	jeq     bceven          /* no, skip alignment */ \n\
	cmpmb   a0@+,a1@+       /* yes, compare a byte */ \n\
	jne     bcnoteq         /* not equal, return non-zero */ \n\
	subql   #1,d0           /* adjust count */ \n\
	jeq     bcdone_bcmp     /* count 0, return zero */ \n\
bceven: \n\
	movl    a1,d1 \n\
	btst    #0,d1           /* string 2 address odd? */ \n\
	jne     bcbloop         /* yes, no hope for alignment, compare bytes */ \n\
	movl    d0,d1           /* no, both even */ \n\
	lsrl    #2,d1           /* convert count to longword count */ \n\
	jeq     bcbloop         /* count 0, skip longword loop */ \n\
bclloop: \n\
	cmpml   a0@+,a1@+       /* compare a longword */ \n\
	jne     bcnoteq         /* not equal, return non-zero */ \n\
	subql   #1,d1           /* adjust count */ \n\
	jne     bclloop         /* still more, keep comparing */ \n\
	andl    #3,d0           /* what remains */ \n\
	jeq     bcdone_bcmp     /* nothing, all done */ \n\
bcbloop: \n\
	cmpmb   a0@+,a1@+       /* compare a byte */ \n\
	jne     bcnoteq         /* not equal, return non-zero */ \n\
	subql   #1,d0           /* adjust count */ \n\
	jne     bcbloop         /* still more, keep going */ \n\
	rts \n\
bcnoteq: \n\
	moveq   #1,d0 \n\
bcdone_bcmp: \n\
	rts \n\
");
#endif
