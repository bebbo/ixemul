#include "a4.h"         /* for the A4 macro */

#ifndef __PPC__
asm(" \n\
	.text \n\
	.even \n\
	.globl  ___link_a5_d0_f \n\
	.globl  ___sub_d0_sp_f \n\
\n\
___link_a5_d0_f: \n\
	movel   sp@+,a0 \n\
	movel   sp,d1 \n\
	subl    d0,d1 \n\
	cmpl    "A4(___stk_limit)",d1 \n\
	jcc     l0 \n\
	jbsr    l2 \n\
l0:     link    a5,#0:W \n\
	subl    d0,sp \n\
	jmp     a0@ \n\
\n\
___sub_d0_sp_f: \n\
	movel   sp@+,a0 \n\
	movel   sp,d1 \n\
	subl    d0,d1 \n\
	cmpl    "A4(___stk_limit)",d1 \n\
	jcc     l1 \n\
	jbsr    l2 \n\
l1:     subl    d0,sp \n\
	jmp     a0@ \n\
\n\
l2:     moveq   #0,d1 \n\
	jra     ___stkext_f \n\
");
#endif
