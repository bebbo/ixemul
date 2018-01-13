#include "a4.h"		/* for the A4 macro */

asm("
	.text
	.even
	.globl	___sub_d0_sp
	.globl	___move_d0_sp
	.globl	___unlk_a5_rts

___sub_d0_sp:
	movel	sp@+,a0
	movel	sp,d1
	subl	d0,d1
	cmpl	"A4(___stk_limit)",d1
	jcc	l0
	jbsr	___stkext
l0:	subl	d0,sp
	jmp	a0@

___move_d0_sp:
	jra	___stkrst

___unlk_a5_rts:
	movel	d0,a0
	movel	a5,d0
	jbsr	___stkrst
	movel	a0,d0
	movel	sp@+,a5
	rts
");
