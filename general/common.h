#ifndef _COMMON_H_
#define _COMMON_H_

#include <exec/types.h>
#undef FLOAT

#include "types.h"
#define FLOAT SFVALUE

#define lowpart(a) ((unsigned short)a)
#define highpart(a) ((unsigned short)((a)>>16))

static inline unsigned long
mulu (unsigned long U, unsigned long V)
{
  register unsigned long result asm("d0");

  asm volatile ("
    movel	%1,d0
    movel	%2,d1
    movel	d0,d2
    movel	d1,d3
    swap	d2
    swap	d3
    mulu	d1,d2
    mulu	d0,d3
    mulu	d1,d0
    addw	d3,d2
    swap	d2
    clrw	d2
    addl	d2,d0"
    : "=r" (result)
    : "g" (U), "g" (V)
    : "d0", "d1", "d2", "d3");
  return result;
}

#define divmodu(q, r, n, d) 				\
({ register unsigned long rq asm("d0"), rr asm("d1");	\
							\
   asm volatile ("					\
   	movel	%2,d0;					\
   	movel	%3,d1;					\
	cmpl	#0xffff,d1;				\
	bhi	2f;					\
	movel	d1,d3;					\
	swap	d0;                                     \
	movew	d0,d3;                                  \
	beq	1f;					\
	divu	d1,d3;                                  \
	movew	d3,d0;                                  \
   1:                                               	\
	swap	d0;                                     \
	movew	d0,d3;                                  \
	divu	d1,d3;                                  \
	movew	d3,d0;                                  \
	swap	d3;                                     \
	movew	d3,d1;                                  \
	bra	5f;                              	\
                                                        \
   2:                                       		\
	movel	d1,d3;                                  \
	movel	d0,d1;                                  \
	clrw	d1;                                     \
	swap	d1;                                     \
	swap	d0;                                     \
	clrw	d0;                                     \
	moveq	#15,d2;                                 \
   3:                                             	\
	addl	d0,d0;                                  \
	addxl	d1,d1;                                  \
	cmpl	d1,d3;                                  \
	bhi	4f;                                  	\
	subl	d3,d1;                                  \
	addqw	#1,d0;                                  \
   4:                                               	\
	dbra	d2,3b;                              	\
                                                        \
   5:"                                         		\
   : "=r" (rq), "=r" (rr)                               \
   : "g" (n), "g" (d)                                   \
   : "d0", "d1", "d2", "d3");                           \
   q = rq; r = rr;})

#endif
