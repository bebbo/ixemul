/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1991, 1992  Markus M. Wild
 *  Portions Copyright (C) 1994 Rafael W. Luebbert
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  $Id: mathsup.c,v 1.6 1994/06/25 12:15:02 rluebbert Exp $
 *
 *  $Log: mathsup.c,v $
 *  Revision 1.6  1994/06/25  12:15:02  rluebbert
 *  removed errno from pow. Probably not a good idea,
 *  but I'm tired of patching stuff right now.
 *
 *  Revision 1.5  1994/06/25  12:05:50  rluebbert
 *  bugfix
 *
 *  Revision 1.4  1994/06/25  11:53:41  rluebbert
 *  Put in 68881 pow and moved Commodore's to ifndef __HAVE_68881__
 *
 *  Revision 1.3  1994/06/19  15:14:01  rluebbert
 *  *** empty log message ***
 *
 *
 */

#ifndef __HAVE_68881__

#include <ixemul.h>
#include <ixmath.h>

double sincos(double* pf2, double parm)
{
  return IEEEDPSincos(pf2, parm);
}

/* GRRRR Commodore does it the other way round... */
double const pow(double arg, double exp)
{
  return IEEEDPPow(exp, arg);
}

double const atan(double parm)
{
  return IEEEDPAtan(parm);
}

double const sin(double parm)
{
  return IEEEDPSin(parm);
}

double const cos(double parm)
{
  return IEEEDPCos(parm);
}

double const tan(double parm)
{
  return IEEEDPTan(parm);
}

double const sinh(double parm)
{
  return IEEEDPSinh(parm);
}

double const cosh(double parm)
{
  return IEEEDPCosh(parm);
}

double const tanh(double parm)
{
  return IEEEDPTanh(parm);
}

double const exp(double parm)
{
  return IEEEDPExp(parm);
}

double const log(double parm)
{
  return IEEEDPLog(parm);
}

double const sqrt(double parm)
{
  return IEEEDPSqrt(parm);
}

double const asin(double parm)
{
  return IEEEDPAsin(parm);
}

double const acos(double parm)
{
  return IEEEDPAcos(parm);
}

double const log10(double parm)
{
  return IEEEDPLog10(parm);
}

double const floor(double parm)
{
  return IEEEDPFloor(parm);
}

double const ceil(double parm)
{
  return IEEEDPCeil(parm);
}

#else /* There is a 68881 or 68882 (__HAVE_68881__ is defined) */

const double sin(double x)
{
  double value;

  __asm ("fsin%.x %1,%0"
	 : "=f" (value)
	 : "f" (x));
  return value;
}

const double cos(double x)
{
  double value;

  __asm ("fcos%.x %1,%0"
	 : "=f" (value)
	 : "f" (x));
  return value;
}

double sincos(double* pf2, double parm)
{
  *pf2 = cos(parm);
  return sin(parm);
}

const double tan(double x)
{
  double value;

  __asm ("ftan%.x %1,%0"
	 : "=f" (value)
	 : "f" (x));
  return value;
}
const double asin(double x)
{
  double value;

  __asm ("fasin%.x %1,%0"
	 : "=f" (value)
	 : "f" (x));
  return value;
}

const double acos(double x)
{
  double value;

  __asm ("facos%.x %1,%0"
	 : "=f" (value)
	 : "f" (x));
  return value;
}

const double atan(double x)
{
  double value;

  __asm ("fatan%.x %1,%0"
	 : "=f" (value)
	 : "f" (x));
  return value;
}
const double sinh(double x)
{
  double value;

  __asm ("fsinh%.x %1,%0"
	 : "=f" (value)
	 : "f" (x));
  return value;
}

const double cosh(double x)
{
  double value;

  __asm ("fcosh%.x %1,%0"
	 : "=f" (value)
	 : "f" (x));
  return value;
}

const double tanh(double x)
{
  double value;

  __asm ("ftanh%.x %1,%0"
	 : "=f" (value)
	 : "f" (x));
  return value;
}
const double exp(double x)
{
  double value;

  __asm ("fetox%.x %1,%0"
	 : "=f" (value)
	 : "f" (x));
  return value;
}
const double log(double x)
{
  double value;

  __asm ("flogn%.x %1,%0"
	 : "=f" (value)
	 : "f" (x));
  return value;
}
const double log10(double x)
{
  double value;

  __asm ("flog10%.x %1,%0"
	 : "=f" (value)
	 : "f" (x));
  return value;
}

const double sqrt(double x)
{
  double value;

  __asm ("fsqrt%.x %1,%0"
	 : "=f" (value)
	 : "f" (x));
  return value;
}
const double ceil(double x)
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
  __asm __volatile ("fint%.x %1,%0"
		  : "=f" (value)
		  : "f" (x));
  __asm __volatile ("fmove%.l %0,fpcr"
		  : /* no outputs */
		  : "dmi" (rounding_mode));
  return value;
}

const double floor(double x)
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
  __asm __volatile ("fint%.x %1,%0"
		  : "=f" (value)
		  : "f" (x));
  __asm __volatile ("fmove%.l %0,fpcr"
		  : /* no outputs */
		  : "dmi" (rounding_mode));
  return value;
}

#define NAN(value)   __asm ("fmoved %#0rnan,%0" : "=f" (value): )
const double pow(double x, double y)
{
  register double value;

  if (x > 0)
    return exp(y * log(x));
  
  if (x == 0)
    {
      if (y == 0)
	{
	  NAN(value);
	  return value;
	}

      return 0.0;
    }

   __asm ("fintrz%.x %1,%0"
	   : "=f" (value)			/* integer-valued float */
	   : "f" (y));

   if (y != value)
     {
       NAN(value);
       return value;
     }
   x = y * log(-x);
   return (((int)y & 1) == 0) ? exp(x) : -exp(x);
}

#endif /* __HAVE_68881__ */
