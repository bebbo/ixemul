
#ifdef __PPC__

/* Called with the return address of the caller in r0.
 * Can't change any register besides r0, r11-r12, ctr, ccr0.
 * Puts r0 back into lr before returning.
 */

asm("
	.section \".text\"
	.align  2
	.globl  __stkchk1
	.type   __stkchk1,@function

__stkchk1:
"
#if defined(LBASEREL)
"
	addis   12,13,__stk_limit@drelha
	mflr    11
	lwz     12,__stk_limit@drell(12)
"
#elif defined(BASEREL)
"
	mflr    11
	lwz     12,__stk_limit@sdarel(13)
"
#else
"
	lis     12,__stk_limit@ha
	mflr    11
	lwz     12,__stk_limit@l(12)
"
#endif
"
	mtctr   11
	cmp     0,12,1
	mtlr    0
	bltctr+
	b       __stkovf
__end_stkchk1:
	.size   __stkchk1,__end_stkchk1-__stkchk1

");

#endif

