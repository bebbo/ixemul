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

/* bit = ffs(value) */

#ifdef __PPC__

asm("
	.text
	.globl  ffs
ffs:
	addi    4,3,-1
	xor     3,4,3
	cntlzw  4,3
	li      3,32
	subf    3,4,3
	andi.   3,3,31
	blr
");

#else
#include "defs.h"

ENTRY(ffs)
asm(" \n\
	moveq   #-1,d0 \n\
	movl    sp@(4),d1 \n\
	beq     done \n\
again: \n\
	addql   #1,d0 \n\
	btst    d0,d1 \n\
	beq     again \n\
done: \n\
	addql   #1,d0 \n\
	rts \n\
");
#endif
