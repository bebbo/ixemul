#include "a4.h"         /* for the A4 macro */

/*
 * Special glue that doesn't clobber any registers.
 */
#ifndef __PPC__
asm(" \n\
	.globl  ___stkovf \n\
___stkovf: \n\
	movel   "A4(_ixemulbase)",sp@- \n\
	addl    #-6*481-24,sp@ \n\
	rts \n\
\n\
	.globl  ___stkext \n\
___stkext: \n\
	movel   "A4(_ixemulbase)",sp@- \n\
	addl    #-6*482-24,sp@ \n\
	rts \n\
\n\
	.globl  ___stkext_f \n\
___stkext_f: \n\
	movel   "A4(_ixemulbase)",sp@- \n\
	addl    #-6*483-24,sp@ \n\
	rts \n\
 \n\
	.globl  ___stkrst \n\
___stkrst: \n\
	movel   "A4(_ixemulbase)",sp@- \n\
	addl    #-6*484-24,sp@ \n\
	rts \n\
\n\
	.globl  ___stkext_startup \n\
___stkext_startup: \n\
	movel   "A4(_ixemulbase)",sp@- \n\
	addl    #-6*571-24,sp@ \n\
	rts \n\
");
#else

#if defined(BASEREL)
#   define LDVAR(reg, var) "lwz "#reg","#var"@sdarel(13)"
#elif defined(LBASEREL)
#   define LDVAR(reg, var) "addis "#reg",13,"#var"@drelha; lwz "#reg","#var"@drell("#reg")"
#else
#   define LDVAR(reg, var) "lis "#reg","#var"@ha; lwz "#reg","#var"@l("#reg")"
#endif

asm("
	.section \".text\"
	.align  2
"/*     .globl  __stkovf
	.type   __stkovf,@function
__stkovf:
	"LDVAR(12,_ixbasearray)"
	lwz     12,481*4-4(12)
	mtctr   12
	bctr
*/"
	.globl  __stkext
	.type   __stkext,@function
__stkext:
	"LDVAR(12,_ixbasearray)"
	lwz     12,482*4-4(12)
	mtctr   12
	bctr

	.globl  __stkext_f
	.type   __stkext_f,@function
__stkext_f:
	mflr    0
	"LDVAR(12,_ixbasearray)"
	lwz     12,483*4-4(12)
	mtlr    12
	stw     0,4(1)
	blr
"/*
	.globl  __stkrst
	.type,  __stkrst,@function
__stkrst:
	"LDVAR(12,_ixbasearray)"
	lwz     12,484*4-4(12)
	mtctr   12
	bctr
*/"
	.globl  __stkext_startup
	.type   __stkext_startup,@function
__stkext_startup:
	"LDVAR(12,_ixbasearray)"
	lwz     12,571*4-4(12)
	mtctr   12
	bctr
");


#endif
