/******************************************************************\
*								   *
*  <math-68040.h>		last modified: 16 Nov 1996.	   *
*								   *
*  Based on <math-68881.h> by Matthew Self.			   *
*  Inline versions of the math libraries using a mixture of	   *
*  68040 FPU instructions and calls to Motorola's libFPSP.a.	   *
*  Author: Kriton Kyrimis					   *
*								   *
*  Original Copyright notice for <math-68881.h> follows:	   *
*								   *
********************************************************************
*								   *
*  <math-68881.h>		last modified: 18 May 1989.	   *
*								   *
*  Copyright (C) 1989 by Matthew Self.				   *
*  You may freely distribute verbatim copies of this software	   *
*  provided that this copyright notice is retained in all copies.  *
*  You may distribute modifications to this software under the     *
*  conditions above if you also clearly note such modifications    *
*  with their author and date.			   	     	   *
*								   *
*  Note:  errno is not set to EDOM when domain errors occur for    *
*  most of these functions.  Rather, it is assumed that the	   *
*  68881's OPERR exception will be enabled and handled		   *
*  appropriately by the	operating system.  Similarly, overflow	   *
*  and underflow do not set errno to ERANGE.			   *
*								   *
*  Send bugs to Matthew Self (self@bayes.arc.nasa.gov).		   *
*								   *
\******************************************************************/

/* Dec 1991  - mw - added support for -traditional mode */

#include <errno.h>

#if defined(__STDC__) || defined(__cplusplus)
#define _DEFUN(name, args1, args2) name ( args2 )
#define _AND ,
#define _CONST const
#else
#define _DEFUN(name, args1, args2) name args1 args2;
#define _AND ;
#define _CONST
#endif

extern double fsind	__P((double x));
extern double fcosd	__P((double x));
extern double ftand	__P((double x));
extern double fasind	__P((double x));
extern double facosd	__P((double x));
extern double fatand	__P((double x));
extern double fsinhd	__P((double x));
extern double fcoshd	__P((double x));
extern double ftanhd	__P((double x));
extern double fatanhd	__P((double x));
extern double fetoxd	__P((double x));
extern double fetoxm1d	__P((double x));
extern double flognd	__P((double x));
extern double flognp1d	__P((double x));
extern double flog10d	__P((double x));
extern double fsqrtd	__P((double x));
extern double fintrzd	__P((double x));
extern double fabsd	__P((double x));
extern double fintd	__P((double x));
extern double fintd	__P((double x));
extern double fmodd	__P((double x, double y));
extern double fremd	__P((double x, double y));
extern double fscaled	__P((double x, double y));
extern double fgetexpd	__P((double x));
extern double fgetmand	__P((double x));

#ifndef HUGE_VAL
#define HUGE_VAL							\
({									\
  double huge_val;							\
									\
  __asm ("fmove%.d #0x7ff0000000000000,%0"	/* Infinity */		\
	 : "=f" (huge_val)						\
	 : /* no inputs */);						\
  huge_val;								\
})
#endif

/* Rewritten -- K.K. 16-Nov-96 */
__inline static _CONST double 
_DEFUN(sin, (x),
    double x)
{
  return fsind(x);
}

/* Rewritten -- K.K. 16-Nov-96 */
__inline static _CONST double 
_DEFUN(cos, (x),
    double x)
{
  return fcosd(x);
}

/* Rewritten -- K.K. 16-Nov-96 */
__inline static _CONST double 
_DEFUN(tan, (x),
    double x)
{
  return ftand(x);
}

/* Rewritten -- K.K. 16-Nov-96 */
__inline static _CONST double 
_DEFUN(asin, (x),
    double x)
{
  return fasind(x);
}

/* Rewritten -- K.K. 16-Nov-96 */
__inline static _CONST double 
_DEFUN(acos, (x),
    double x)
{
  return facosd(x);
}

/* Rewritten -- K.K. 16-Nov-96 */
__inline static _CONST double 
_DEFUN(atan, (x),
    double x)
{
  return fatand(x);
}

/* Modified to used M_PI and M_PI_2 from <math.h> instead of calculating pi
   and pi/2 using instructions that must be emulated on the 68040.
   -- K.K. 16-Nov-96
*/
__inline static _CONST double 
_DEFUN(atan2, (y, x),
    double y _AND
    double x)
{
  if (x > 0)
    {
      if (y > 0)
	{
	  if (x > y)
	    return atan (y / x);
	  else
	    return M_PI_2 - atan (x / y);
	}
      else
	{
	  if (x > -y)
	    return atan (y / x);
	  else
	    return - M_PI_2 - atan (x / y);
	}
    }
  else
    {
      if (y > 0)
	{
	  if (-x > y)
	    return M_PI + atan (y / x);
	  else
	    return M_PI_2 - atan (x / y);
	}
      else
	{
	  if (-x > -y)
	    return - M_PI + atan (y / x);
	  else if (y < 0)
	    return - M_PI_2 - atan (x / y);
	  else
	    {
	      double value;

	      errno = EDOM;
	      __asm ("fmove%.d %#0rnan,%0" 	/* quiet NaN */
		     : "=f" (value)
		     : /* no inputs */);
	      return value;
	    }
	}
    }
}

/* Rewritten -- K.K. 16-Nov-96 */
__inline static _CONST double 
_DEFUN(sinh, (x),
    double x)
{
  return fsinhd(x);
}

/* Rewritten -- K.K. 16-Nov-96 */
__inline static _CONST double 
_DEFUN(cosh, (x),
    double x)
{
  return fcoshd(x);
}

/* Rewritten -- K.K. 16-Nov-96 */
__inline static _CONST double 
_DEFUN(tanh, (x),
    double x)
{
  return ftanhd(x);
}

/* Rewritten -- K.K. 16-Nov-96 */
__inline static _CONST double 
_DEFUN(atanh, (x),
    double x)
{
  return fatanhd(x);
}

/* Rewritten -- K.K. 16-Nov-96 */
__inline static _CONST double 
_DEFUN(exp, (x),
    double x)
{
  return fetoxd(x);
}

/* Rewritten -- K.K. 16-Nov-96 */
__inline static _CONST double 
_DEFUN(expm1, (x),
    double x)
{
  return fetoxm1d(x);
}

/* Rewritten -- K.K. 16-Nov-96 */
__inline static _CONST double 
_DEFUN(log, (x),
    double x)
{
  return flognd(x);
}

/* Rewritten -- K.K. 16-Nov-96 */
__inline static _CONST double 
_DEFUN(log1p, (x),
    double x)
{
  return flognp1d(x);
}

/* Rewritten -- K.K. 16-Nov-96 */
__inline static _CONST double 
_DEFUN(log10, (x),
    double x)
{
  return flog10d(x);
}

/* Added ifdef-ed out call to fsqrtd to make it clear that the call
   must not be used. -- K.K. 16-Nov-96
*/
__inline static _CONST double 
_DEFUN(sqrt, (x),
    double x)
{
#if 0	/* fsqrt is supported on the 68040! */
  return fsqrtd(x);
#else
  double value;

  __asm ("fsqrt%.x %1,%0"
	 : "=f" (value)
	 : "f" (x));
  return value;
#endif
}

__inline static _CONST double
hypot (_CONST double x, _CONST double y)
{
  return sqrt (x*x + y*y);
}

/* Replaced fintrz.x instruction with call to fintrd. -- K.K. 16-Nov-96 */
__inline static _CONST double 
_DEFUN(pow, (x, y),
    _CONST double x _AND
    _CONST double y)
{
  if (x > 0)
    return exp (y * log (x));
  else if (x == 0)
    {
      if (y > 0)
	return 0.0;
      else
	{
	  double value;

	  errno = EDOM;
	  __asm ("fmove%.d %#0rnan,%0"		/* quiet NaN */
		 : "=f" (value)
		 : /* no inputs */);
	  return value;
	}
    }
  else	/* x < 0 */
    {
      double temp;

      temp = fintrzd(y);			/* integer-valued float */
      if (y == temp)
        {
	  int i = (int) y;
	  
	  if ((i & 1) == 0)			/* even */
	    return exp (y * log (-x));
	  else
	    return - exp (y * log (-x));
        }
      else
        {
	  double value;

	  errno = EDOM;
	  __asm ("fmove%.d %#0rnan,%0"		/* quiet NaN */
		 : "=f" (value)
		 : /* no inputs */);
	  return value;
        }
    }
}

/* Added ifdef-ed out call to fabsd to make it clear that the call
   must not be used. -- K.K. 16-Nov-96
*/
__inline static _CONST double 
_DEFUN(fabs, (x),
    double x)
{
#if 0	/* fabs supported on the 68040! */
  return fabsd(x);
#else
  double value;

  __asm ("fabs%.x %1,%0"
	 : "=f" (value)
	 : "f" (x));
  return value;
}
#endif

/* Replaced fint.x instruction with call to fintd. -- K.K. 16-Nov-96 */
__inline static _CONST double 
_DEFUN(ceil, (x),
    double x)
{
  int rounding_mode, round_up;
  double value;

  __asm __volatile ("fmove%.l fpcr,%0"
		  : "=dm" (rounding_mode)
		  : /* no inputs */ );
  round_up = rounding_mode | 0x30;
  __asm __volatile ("fmove%.l %0,fpcr"
		  : /* no outputs */
		  : "dmi" (round_up));
  value = fintd(x);
  __asm __volatile ("fmove%.l %0,fpcr"
		  : /* no outputs */
		  : "dmi" (rounding_mode));
  return value;
}

/* Replaced fint.x instruction with call to fintd. -- K.K. 16-Nov-96 */
__inline static _CONST double 
_DEFUN(floor, (x),
    double x)
{
  int rounding_mode, round_down;
  double value;

  __asm __volatile ("fmove%.l fpcr,%0"
		  : "=dm" (rounding_mode)
		  : /* no inputs */ );
  round_down = (rounding_mode & ~0x10)
		| 0x20;
  __asm __volatile ("fmove%.l %0,fpcr"
		  : /* no outputs */
		  : "dmi" (round_down));
  value = fintd(x);
  __asm __volatile ("fmove%.l %0,fpcr"
		  : /* no outputs */
		  : "dmi" (rounding_mode));
  return value;
}

/* Replaced fint.x instruction with call to fintd. -- K.K. 16-Nov-96 */
__inline static _CONST double 
_DEFUN(rint, (x),
    double x)
{
  int rounding_mode, round_nearest;
  double value;

  __asm __volatile ("fmove%.l fpcr,%0"
		  : "=dm" (rounding_mode)
		  : /* no inputs */ );
  round_nearest = rounding_mode & ~0x30;
  __asm __volatile ("fmove%.l %0,fpcr"
		  : /* no outputs */
		  : "dmi" (round_nearest));
  value = fintd(x);
  __asm __volatile ("fmove%.l %0,fpcr"
		  : /* no outputs */
		  : "dmi" (rounding_mode));
  return value;
}

/* Rewritten -- K.K. 16-Nov-96 */
__inline static _CONST double 
_DEFUN(fmod, (x, y),
    double x _AND
    double y)
{
  return fmodd(x, y);
}

/* Rewritten -- K.K. 16-Nov-96 */
__inline static _CONST double 
_DEFUN(drem, (x, y),
    double x _AND
    double y)
{
  return fremd(x, y);
}

/* Rewritten -- K.K. 16-Nov-96 */
__inline static _CONST double 
_DEFUN(scalb, (x, n),
    double x _AND
    int n)
{
  return fscaled(x, (double)n);
}

/* Rewritten -- K.K. 16-Nov-96 */
__inline static double 
_DEFUN(logb, (x),
    double x)
{
  return fgetexpd(x);
}

/* Rewritten -- K.K. 16-Nov-96 */
__inline static _CONST double 
_DEFUN(ldexp, (x, n),
    double x _AND
    int n)
{
  return fscaled(x, (double)n);
}

/* Replaced fgetexp.x, fgetman.x, and fscale.b instructions with calls to
   fgetexpd, fgetmand, and fgscaled, respectively. Removed float_exponent
   temprary variable. -- K.K. 16-Nov-96
*/
__inline static double 
_DEFUN(frexp, (x, exp),
    double x _AND
    int *exp)
{
  int int_exponent;
  double mantissa;

  int_exponent = (int) fgetexpd(x);	/* integer-valued float */
  mantissa = fgetmand(x);		/* 1.0 <= mantissa < 2.0 */
  if (mantissa != 0)
    {
      mantissa = fscaled(mantissa, -1.0);	/* mantissa /= 2.0 */
      int_exponent += 1;
    }
  *exp = int_exponent;
  return mantissa;
}

/* Replaced fintrz.x instruction with call to fintrzd. -- K.K. 16-Nov-96 */
__inline static double 
_DEFUN(modf, (x, ip),
    double x _AND
    double *ip)
{
  double temp;

  temp = fintrzd(x);			/* integer-valued float */
  *ip = temp;
  return x - temp;
}

#undef _DEFUN
#undef _AND
#undef _CONST
