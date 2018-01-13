#include <ixemul.h>

#include "common.h"
#include "defs.h"

#include <ixmath.h>

/*
 * double frexp(val, eptr)
 * returns: x s.t. val = x * (2 ** n), with n stored in *eptr
 */
double frexp(double value, int *eptr)
{
  union
  {
    double v;
    struct 
    {
      u_int u_sign  :  1;
      u_int u_exp   : 11;
      u_int u_mant1 : 20;
      u_int u_mant2 : 32;
    } s;
  } d;

  if (value)
    {
      d.v = value;
      *eptr = d.s.u_exp - 1022;
      d.s.u_exp = 1022;
      return d.v;
    }
  *eptr = 0;
  return 0.0;
}

/* fabs(double) */
ENTRY(fabs)
asm("
	movel	sp@(4),d0
	movel	sp@(8),d1
	bclr	#31,d0
	rts
");

/* -double */
ENTRY(__negdf2)
asm("
	movel	sp@(4),d0
	movel	sp@(8),d1
	bchg	#31,d0
	rts
");

/* -single */
ENTRY(__negsf2)
asm("
	movel	sp@(4),d0
	bchg	#31,d0
	rts
");


#if defined(mc68020) || defined(mc68030) || defined(mc68040) || defined(mc68060)

// Let the compiler do the hard work :-)

/* int / int */
int __divsi3(int a, int b)
{
  return a / b;
}

/* int % int */
int __modsi3(int a, int b)
{
  return a % b;
}

/* int * int */
int __mulsi3(int a, int b)
{
  return a * b;
}

/* unsigned / unsigned */
unsigned __udivsi3(unsigned a, unsigned b)
{
  return a / b;
}

/* unsigned % unsigned */
unsigned __umodsi3(unsigned a, int b)
{
  return a % b;
}


#else


SItype __divsi3(SItype a, SItype b)
{
  unsigned SItype q, r;
  int neg = (a < 0) != (b < 0);

  if (a < 0) a = -a;
  if (b < 0) b = -b;

  divmodu(q, r, a, b);

  return neg ? -q : q;
}

SItype __modsi3(SItype a, SItype b)
{
  unsigned SItype q, r;
  int neg = (a < 0);

  if (a < 0) a = -a;
  if (b < 0) b = -b;

  divmodu(q, r, a, b);

  return neg ? -r : r;
}

SItype __mulsi3(SItype a, SItype b)
{
  int neg = (a < 0) != (b < 0);
  SItype res;

  if (a < 0) a = -a;
  if (b < 0) b = -b;
  
  res = mulu(a,b);
  return neg ? -res : res;
}

unsigned SItype __udivsi3(unsigned SItype a, unsigned SItype b)
{
  unsigned SItype q, r;
  divmodu(q, r, a, b);
  return q;
}

unsigned SItype __umodsi3(unsigned SItype a, unsigned SItype b)
{
  unsigned SItype q, r;
  divmodu(q, r, a, b);
  return r;
}

#endif



#ifdef __HAVE_68881__

// Let the compiler to the hard work :-)

int __eqdf2(double a, double b)
{
  if (a == b) return 0; return 1;
}

int __eqsf2(float a, float b)
{
  if (a == b) return 0; return 1;
}

SItype __fixsfsi(float a)
{
  return a;
}

float __floatsisf(SItype a)
{
  return a;
}

int __gedf2(double a, double b)
{
  if (a >= b) return 0; return -1;
}

int __gesf2(float a, float b)
{
  if (a >= b) return 0; return -1;
}

int __gtdf2(double a, double b)
{
  if (a > b) return 1; return 0;
}

int __gtsf2(float a, float b)
{
  if (a > b) return 1; return 0;
}

int __ledf2(double a, double b)
{
  if (a <= b) return 0; return 1;
}

int __lesf2(float a, float b)
{
  if (a <= b) return 0; return 1;
}

int __ltdf2(double a, double b)
{
  if (a < b) return -1; return 0;
}

int __ltsf2(float a, float b)
{
  if (a < b) return -1; return 0;
}

int __nedf2(double a, double b)
{
  if (a != b) return 1; return 0;
}

int __nesf2(float a, float b)
{
  if (a != b) return 1; return 0;
}

/* double + double */
double __adddf3(double a, double b)
{
  return a + b;
}

/* single + single */
float __addsf3(float a, float b)
{
  return a + b;
}

/* double > double: 1 */
/* double < double: -1 */
/* double == double: 0 */
ENTRY(__cmpdf2)
asm("
	fmoved	sp@(4),fp0
	fcmpd	sp@(12),fp0
	fbgt	Lagtb1
	fslt	d0
	extbl	d0
	rts
Lagtb1:
	moveq	#1,d0
	rts
");

/* single > single: 1 */
/* single < single: -1 */
/* single == single: 0 */
ENTRY(__cmpsf2)
asm("
	fmoves	sp@(4),fp0
	fcmps	sp@(8),fp0
	fbgt	Lagtb2
	fslt	d0
	extbl	d0
	rts
Lagtb2:
	moveq	#1,d0
	rts
");

/* double / double */
double __divdf3(double a, double b)
{
  return a / b;
}

/* single / single */
float __divsf3(float a, float b)
{
  return a / b;
}

/* double * double */
double __muldf3(double a, double b)
{
  return a * b;
}

/* single * single */
float __mulsf3(float a, float b)
{
  return a * b;
}

/* double - double */
double __subdf3(double a, double b)
{
  return a - b;
}

/* single - single */
float __subsf3(float a, float b)
{
  return a - b;
}

/* (float) double */
float __truncdfsf2(double a)
{
  return a;
}

/* (double) float */
double __extendsfdf2(float a)
{
  return a;
}

/* (int) double */
SItype __fixdfsi(double a)
{
  return a;
}

/* (double) int */
double __floatsidf(SItype a)
{
  return a;
}

/*
 * double ldexp(val, exp)
 * returns: val * (2**exp), for integer exp
 */
ENTRY(ldexp)
asm("
	fmoved		sp@(4),fp0
	fbeq		Ldone
	ftwotoxl	sp@(12),fp1
	fmulx		fp1,fp0
Ldone:
	fmoved		fp0,sp@-
	movel		sp@+,d0
	movel		sp@+,d1
	rts
");

/*
 * double modf(val, iptr)
 * returns: xxx and n (in *iptr) where val == n.xxx
 */
ENTRY(modf)
asm("
	fmoved	sp@(4),fp0
	movel	sp@(12),a0
	fintrzx	fp0,fp1
	fmoved	fp1,a0@
	fsubx	fp1,fp0
	fmoved	fp0,sp@-
	movel	sp@+,d0
	movel	sp@+,d1
	rts
");



#else /* __HAVE_68881__ */



/*  Special patch to work around bug in IEEEDPCmp:
 *  if the first 32 bits of both doubles are equal, and
 *  both doubles are negative, then the result can no longer
 *  be trusted.
 * 
 *  This is the output of a small test program:
 *
 *    test -2.000001 -2.0000009
 *    a = -2.0000009999999996956888, b = -2.0000008999999998593466
 *    (0xc0000000 0x8637bd05, 0xc0000000 0x78cbc3b8)
 *    cmp(a,b) = 0, cmp(b,a) = 1
 *
 *    test -2.0000001 -2.0000002
 *    a = -2.00000009999999983634211, b = -2.0000001999999996726842
 *    (0xc0000000 0xd6bf94d, 0xc0000000 0x1ad7f29a)
 *    cmp(a,b) = 1, cmp(b,a) = 1
 *
 *  As you can see, the results are wrong.
 *
 *  So, we just make both variables positive and exchange them before
 *  passing them to IEEEDPCmp.
 *
 *  This bug was discovered by Bart Van Assche, thanks!
 */

static int ieeedpcmp(double a, double b)
{
  if (*((char *)&a) & *((char *)&b) & 0x80)  /* both doubles negative? */
    {
      *((char *)&a) &= 0x7f;    /* yes, make positive */
      *((char *)&b) &= 0x7f;
      return IEEEDPCmp(b, a);   /* pass them to IEEEDPCmp the other way round */
    }
  return IEEEDPCmp(a, b);
}


int __eqdf2(double a, double b)
{
  return ieeedpcmp(a, b);
}

int __eqsf2(FLOAT a, FLOAT b)
{
  return IEEESPCmp(a, b);
}

SItype __fixsfsi(FLOAT a)
{
  return IEEESPFix(a);
}

SFVALUE __floatsisf(SItype a)
{
  return IEEESPFlt(a);
}

int __gedf2(double a, double b)
{
  return ieeedpcmp(a, b);
}

int __gesf2(FLOAT a, FLOAT b)
{
  return IEEESPCmp(a, b);
}

int __gtdf2(double a, double b)
{
  return ieeedpcmp(a, b);
}

int __gtsf2(FLOAT a, FLOAT b)
{
  return IEEESPCmp(a, b);
}

int __ledf2(double a, double b)
{
  return ieeedpcmp(a, b);
}

int __lesf2(FLOAT a, FLOAT b)
{
  return IEEESPCmp(a, b);
}

int __ltdf2(double a, double b)
{
  return ieeedpcmp(a, b);
}

int __ltsf2(FLOAT a, FLOAT b)
{
  return IEEESPCmp(a, b);
}

int __nedf2(double a, double b)
{
  return !!ieeedpcmp(a, b);
}

int __nesf2(FLOAT a, FLOAT b)
{
  return !!IEEESPCmp(a, b);
}

/* double + double */
double __adddf3(double a, double b)
{
  return IEEEDPAdd(a, b);
}

/* single + single */
SFVALUE __addsf3(FLOAT a, FLOAT b)
{
  return IEEESPAdd(a, b);
}

/* double > double: 1 */
/* double < double: -1 */
/* double == double: 0 */
int __cmpdf2(double a, double b)
{
  return ieeedpcmp(a, b);
}

/* single > single: 1 */
/* single < single: -1 */
/* single == single: 0 */
int __cmpsf2(FLOAT a, FLOAT b)
{
  return IEEESPCmp(a, b);
}

/* double / double */
double __divdf3(double a, double b)
{
  return IEEEDPDiv(a, b);
}

/* single / single */
SFVALUE __divsf3(FLOAT a, FLOAT b)
{
  return IEEESPDiv(a, b);
}

/* double * double */
double __muldf3(double a, double b)
{
  return IEEEDPMul(a, b);
}

/* single * single */
SFVALUE __mulsf3(FLOAT a, FLOAT b)
{
  return IEEESPMul(a, b);
}

/* double - double */
double __subdf3(double a, double b)
{
  return IEEEDPSub(a, b);
}

/* single - single */
SFVALUE __subsf3(FLOAT a, FLOAT b)
{
  return IEEESPSub(a, b);
}

/* (float) double */
SFVALUE __truncdfsf2(double a)
{
  return IEEEDPTieee(a);
}

/* (double) float */
double __extendsfdf2(FLOAT a)
{
  return IEEEDPFieee(a);
}

/* (int) double */
SItype __fixdfsi(double a)
{
  return IEEEDPFix(a);
}

/* (double) int */
double __floatsidf(SItype a)
{
  return IEEEDPFlt(a);
}

/*
 * ldexp returns the quanity "value" * 2 ^ "exp"
 *
 * For the mc68000 using IEEE format the double precision word format is:
 *
 * WORD N   =>    SEEEEEEEEEEEMMMM
 * WORD N+1 =>    MMMMMMMMMMMMMMMM
 * WORD N+2 =>    MMMMMMMMMMMMMMMM
 * WORD N+3 =>    MMMMMMMMMMMMMMMM
 *
 * Where:          S  =>   Sign bit
 *                 E  =>   Exponent
 *                 X  =>   Ignored (set to 0)
 *                 M  =>   Mantissa bit
 *
 * NOTE:  Beware of 0.0; on some machines which use excess 128 notation for the
 * exponent, if the mantissa is zero the exponent is also.
 *
 */

#define MANT_MASK 0x800FFFFF	/* Mantissa extraction mask     */
#define ZPOS_MASK 0x3FF00000	/* Positive # mask for exp = 0  */
#define ZNEG_MASK 0x3FF00000	/* Negative # mask for exp = 0  */

#define EXP_MASK 0x7FF00000	/* Mask for exponent            */
#define EXP_SHIFTS 20		/* Shifts to get into LSB's     */
#define EXP_BIAS 1023		/* Exponent bias                */

union dtol
{
  double dval;
  int ival[2];
};

double ldexp(double value, int exp)
{
  union dtol number;
  int *iptr, cexp;

  if (value == 0.0)
    return (0.0);
  number.dval = value;
  iptr = &number.ival[0];
  cexp = (((*iptr) & EXP_MASK) >> EXP_SHIFTS) - EXP_BIAS;
  *iptr &= ~EXP_MASK;
  exp += EXP_BIAS;
  *iptr |= ((exp + cexp) << EXP_SHIFTS) & EXP_MASK;
  return (number.dval);
}

/*
 * modf(value, iptr): return fractional part of value, and stores the
 * integral part into iptr (a pointer to double).
 */

double modf(double value, double *iptr)
{
  /* if value negative */
  if (IEEEDPTst(value) < 0)
    {
      /* in that case, the integer part is calculated by ceil() */
      *iptr = IEEEDPCeil(value);
      return IEEEDPSub(*iptr, value);
    }
  else
    {
      /* if positive, we go for the floor() */
      *iptr = IEEEDPFloor(value);
      return IEEEDPSub(value, *iptr);
    }
}

#endif /* __HAVE_68881__ */
