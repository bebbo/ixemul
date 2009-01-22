#include "a4.h"         /* for the A4 macro */

#ifndef __PPC__
asm(" \n\
	.text \n\
	.even \n\
	.globl  ___stkchk_d0 \n\
	.globl  ___stkchk_0 \n\
\n\
___stkchk_d0: \n\
	negl    d0 \n\
	addl    sp,d0 \n\
	cmpl    "A4(___stk_limit)",d0 \n\
	jcc     stkchk_d0_ret \n\
	jmp     ___stkovf \n\
stkchk_d0_ret: \n\
	rts \n\
\n\
___stkchk_0: \n\
	cmpl    "A4(___stk_limit)",sp \n\
	jcc     stkchk_0_ret \n\
	jmp     ___stkovf \n\
stkchk_0_ret: \n\
	rts \n\
");
#endif
