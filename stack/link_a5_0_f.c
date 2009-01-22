#include "a4.h"         /* for the A4 macro */

#ifndef __PPC__
asm(" \n\
	.text \n\
	.even \n\
	.globl  ___link_a5_0_f \n\
	.globl  ___sub_0_sp_f \n\
\n\
___link_a5_0_f: \n\
	movel   sp@+,a0 \n\
	cmpl    "A4(___stk_limit)",sp \n\
	jcc     l0 \n\
	jbsr    l2 \n\
l0:     link    a5,#0:W \n\
	jmp     a0@ \n\
\n\
___sub_0_sp_f: \n\
	movel   sp@+,a0 \n\
	cmpl    "A4(___stk_limit)",sp \n\
	jcc     l1 \n\
	jbsr    l2 \n\
l1:     jmp     a0@ \n\
\n\
l2:     moveq   #0,d0 \n\
	moveq   #0,d1 \n\
	jra     ___stkext_f \n\
");
#endif
