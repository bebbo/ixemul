#include "a4.h"         /* for the A4 macro */

#ifndef __PPC__
asm(" \n\
	.text \n\
	.even \n\
	.globl  ___sub_d0_sp \n\
	.globl  ___move_d0_sp \n\
	.globl  ___unlk_a5_rts \n\
\n\
___sub_d0_sp: \n\
	movel   sp@+,a0 \n\
	movel   sp,d1 \n\
	subl    d0,d1 \n\
	cmpl    "A4(___stk_limit)",d1 \n\
	jcc     l0 \n\
	jbsr    ___stkext \n\
l0:     subl    d0,sp \n\
	jmp     a0@ \n\
\n\
___move_d0_sp: \n\
	jra     ___stkrst \n\
\n\
___unlk_a5_rts: \n\
	movel   d0,a0 \n\
	movel   a5,d0 \n\
	jbsr    ___stkrst \n\
	movel   a0,d0 \n\
	movel   sp@+,a5 \n\
	rts \n\
");
#else
asm("
	.section \".text\"
	.align  2
	.type   __alloc_stk_ext,@function
	.type   __alloc_stk_chk,@function
	.globl  __alloc_stk_ext
	.globl  __alloc_stk_chk

/*  r3 = size */
__alloc_stk_chk: /* fix me */
__alloc_stk_ext:
"
#if defined(LBASEREL)
"
	addis   12,13,__stk_limit@drelha
	lwz     12,__stk_limit@drell(12)
"
#elif defined(BASEREL)
"
	lwz     12,__stk_limit@sdarel(13)
"
#else
"
	lis     12,__stk_limit@ha
	lwz     12,__stk_limit@l(12)
"
#endif
"
	sub     11,1,3
	lwz     0,0(1)
	cmp     0,12,11
	neg     4,3
	bge-    l1
	stwux   0,1,4
	blr
l1:     b       __stkext
__end__alloc_stk_ext:
	.size	__alloc_stk_ext,__end__alloc_stk_ext-__alloc_stk_ext
");

#endif

