#include "a4.h"		/* for the A4 macro */

/*
 * Special glue that doesn't clobber any registers.
 */
asm("
  	.globl	___stkovf
___stkovf:
	movel	"A4(_ixemulbase)",sp@-
	addl	#-6*481-24,sp@
	rts

	.globl	___stkext
___stkext:
	movel	"A4(_ixemulbase)",sp@-
	addl	#-6*482-24,sp@
	rts

  	.globl	___stkext_f
___stkext_f:
	movel	"A4(_ixemulbase)",sp@-
	addl	#-6*483-24,sp@
  	rts
  
  	.globl	___stkrst
___stkrst:
	movel	"A4(_ixemulbase)",sp@-
	addl	#-6*484-24,sp@
	rts

	.globl	___stkext_startup
___stkext_startup:
	movel	"A4(_ixemulbase)",sp@-
	addl	#-6*571-24,sp@
	rts
");
