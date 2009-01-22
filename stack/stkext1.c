
#ifdef __PPC__

/* Called with the return address of the caller in r0.
 * Can't change any register besides r0, r11-r12, ctr, ccr0.
 * Puts r0 back into lr before returning.
 */

asm("
	.section \".text\"
	.align  2
	.globl  __stkext1
	.type   __stkext1,@function

__stkext1:
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
	b       __stkext_f
__end_stkext1:
	.size   __stkext1,__end_stkext1-__stkext1

");

#endif

