/*      $NetBSD: fprintf.c,v 1.5 1995/02/02 02:09:26 jtc Exp $  */

/*-
 * Copyright (c) 1990, 1993
 *      The Regents of the University of California.  All rights reserved.
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
#if 0
static char sccsid[] = "@(#)fprintf.c   8.1 (Berkeley) 6/4/93";
#endif
static char rcsid[] = "$NetBSD: fprintf.c,v 1.5 1995/02/02 02:09:26 jtc Exp $";
#endif /* LIBC_SCCS and not lint */

#define _KERNEL
#include "ixemul.h"
#include "my_varargs.h"

#include <stdio.h>

#ifdef NATIVE_MORPHOS
#define vfprintf my_vfprintf
int vfprintf(FILE *, const char *, my_va_list);
#endif


int
fprintf(FILE *fp, const char *fmt, ...)
{
	int ret;
	my_va_list ap;

	my_va_start(ap, fmt);
	ret = vfprintf(fp, fmt, ap);
	my_va_end(ap);
	return (ret);
}

#ifdef NATIVE_MORPHOS

int
_varargs68k_fprintf(FILE *fp, const char *fmt, char *ap1)
{
	my_va_list ap;
	my_va_init_68k(ap, ap1);
	return vfprintf(fp, fmt, ap);
}

asm("	.section \".text\"
	.type	_stk_fprintf,@function
	.globl	_stk_fprintf
_stk_fprintf:
	andi.	11,1,15
	mr	12,1
	bne-	.align_fprintf
	b	fprintf
.align_fprintf:
	addi	11,11,128
	mflr	0
	neg	11,11
	stw	0,4(1)
	stwux	1,1,11

	stw	5,16(1)
	stw	6,20(1)
	stw	7,24(1)
	stw	8,28(1)
	stw	9,32(1)
	stw	10,36(1)
	bc	4,6,.nofloat_fprintf
	stfd	1,40(1)
	stfd	2,48(1)
	stfd	3,56(1)
	stfd	4,64(1)
	stfd	5,72(1)
	stfd	6,80(1)
	stfd	7,88(1)
	stfd	8,96(1)
.nofloat_fprintf:

	addi	5,1,104
	lis	0,0x200
	addi	12,12,8
	addi	11,1,8
	stw	0,0(5)
	stw	12,4(5)
	stw	11,8(5)
	bl	vfprintf
	lwz	1,0(1)
	lwz	0,4(1)
	mtlr	0
	blr
");
#endif

