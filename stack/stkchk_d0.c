#include "a4.h"		/* for the A4 macro */

asm("
	.text
	.even
	.globl	___stkchk_d0
	.globl	___stkchk_0

___stkchk_d0:
	negl	d0
	addl	sp,d0
	cmpl	"A4(___stk_limit)",d0
	jcc	stkchk_d0_ret
	jmp	___stkovf
stkchk_d0_ret:
	rts

___stkchk_0:
	cmpl	"A4(___stk_limit)",sp
	jcc	stkchk_0_ret
	jmp	___stkovf
stkchk_0_ret:
	rts
");
