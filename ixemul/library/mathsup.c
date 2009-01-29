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
 *  $Id: mathsup.c,v 1.1.1.1 2005/03/15 15:57:09 laire Exp $
 *
 *  $Log: mathsup.c,v $
 *  Revision 1.1.1.1  2005/03/15 15:57:09  laire
 *  a new beginning
 *
 *  Revision 1.3  2003/12/12 14:21:41  piru
 *  No longer relies on shared math libraries under MorphOS.
 *
 *  Revision 1.2  2001/03/28 20:37:15  emm
 *  Fixed math functions.
 *
 *  Revision 1.1.1.1  2000/05/07 19:38:21  emm
 *  Imported sources
 *
 *  Revision 1.1.1.1  2000/04/29 00:47:24  nobody
 *  Initial import
 *
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

//#ifdef HACK_FPU /* Bernd should know what he is doing.... */
#undef __HAVE_68881__
//#endif

#ifdef __MORPHOS__

#include <ixemul.h>
#include <ixmath.h>

#if 1

#include <math.h>

/* NOTE: This is different than the GNU extension - Piru */

double sincos(double* pf2, double parm)
{
    *pf2 = cos(parm);
    return sin(parm);
}

#else

double sincos(double* pf2, double parm)
{
    *(double*)&REG_D0 = parm;
    REG_A0 = (LONG)pf2;
    REG_A6 = (LONG)MathIeeeDoubTransBase;
    REG_D0 = MyEmulHandle->EmulCallDirectOS(-0x36);
    return *(double*)&REG_D0;
}

/* GRRRR Commodore does it the other way round... */
double const pow(double arg, double exp)
{
    *(double*)&REG_D0 = arg;
    *(double*)&REG_D2 = exp;
    REG_A6 = (LONG)MathIeeeDoubTransBase;
    REG_D0 = MyEmulHandle->EmulCallDirectOS(-0x5a);
    return *(double*)&REG_D0;
}

double const atan(double parm)
{
    *(double*)&REG_D0 = parm;
    REG_A6 = (LONG)MathIeeeDoubTransBase;
    REG_D0 = MyEmulHandle->EmulCallDirectOS(-0x1e);
    return *(double*)&REG_D0;
}

double const sin(double parm)
{
    *(double*)&REG_D0 = parm;
    REG_A6 = (LONG)MathIeeeDoubTransBase;
    REG_D0 = MyEmulHandle->EmulCallDirectOS(-0x24);
    return *(double*)&REG_D0;
}

double const cos(double parm)
{
    *(double*)&REG_D0 = parm;
    REG_A6 = (LONG)MathIeeeDoubTransBase;
    REG_D0 = MyEmulHandle->EmulCallDirectOS(-0x2a);
    return *(double*)&REG_D0;
}

double const tan(double parm)
{
    *(double*)&REG_D0 = parm;
    REG_A6 = (LONG)MathIeeeDoubTransBase;
    REG_D0 = MyEmulHandle->EmulCallDirectOS(-0x30);
    return *(double*)&REG_D0;
}

double const sinh(double parm)
{
    *(double*)&REG_D0 = parm;
    REG_A6 = (LONG)MathIeeeDoubTransBase;
    REG_D0 = MyEmulHandle->EmulCallDirectOS(-0x3c);
    return *(double*)&REG_D0;
}

double const cosh(double parm)
{
    *(double*)&REG_D0 = parm;
    REG_A6 = (LONG)MathIeeeDoubTransBase;
    REG_D0 = MyEmulHandle->EmulCallDirectOS(-0x42);
    return *(double*)&REG_D0;
}

double const tanh(double parm)
{
    *(double*)&REG_D0 = parm;
    REG_A6 = (LONG)MathIeeeDoubTransBase;
    REG_D0 = MyEmulHandle->EmulCallDirectOS(-0x48);
    return *(double*)&REG_D0;
}

double const exp(double parm)
{
    *(double*)&REG_D0 = parm;
    REG_A6 = (LONG)MathIeeeDoubTransBase;
    REG_D0 = MyEmulHandle->EmulCallDirectOS(-0x4e);
    return *(double*)&REG_D0;
}

double const log(double parm)
{
    *(double*)&REG_D0 = parm;
    REG_A6 = (LONG)MathIeeeDoubTransBase;
    REG_D0 = MyEmulHandle->EmulCallDirectOS(-0x54);
    return *(double*)&REG_D0;
}

double const sqrt(double parm)
{
    *(double*)&REG_D0 = parm;
    REG_A6 = (LONG)MathIeeeDoubTransBase;
    REG_D0 = MyEmulHandle->EmulCallDirectOS(-0x60);
    return *(double*)&REG_D0;
}

double const asin(double parm)
{
    *(double*)&REG_D0 = parm;
    REG_A6 = (LONG)MathIeeeDoubTransBase;
    REG_D0 = MyEmulHandle->EmulCallDirectOS(-0x72);
    return *(double*)&REG_D0;
}

double const acos(double parm)
{
    *(double*)&REG_D0 = parm;
    REG_A6 = (LONG)MathIeeeDoubTransBase;
    REG_D0 = MyEmulHandle->EmulCallDirectOS(-0x78);
    return *(double*)&REG_D0;
}

double const log10(double parm)
{
    *(double*)&REG_D0 = parm;
    REG_A6 = (LONG)MathIeeeDoubTransBase;
    REG_D0 = MyEmulHandle->EmulCallDirectOS(-0x7e);
    return *(double*)&REG_D0;
}

double const floor(double parm)
{
    *(double*)&REG_D0 = parm;
    REG_A6 = (LONG)MathIeeeDoubBasBase;
    REG_D0 = MyEmulHandle->EmulCallDirectOS(-0x5a);
    return *(double*)&REG_D0;
}

double const ceil(double parm)
{
    *(double*)&REG_D0 = parm;
    REG_A6 = (LONG)MathIeeeDoubBasBase;
    REG_D0 = MyEmulHandle->EmulCallDirectOS(-0x60);
    return *(double*)&REG_D0;
}

#endif

#elif !defined(__HAVE_68881__)

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

#define PI 3.14159265358979323846
 
double atan2(double y,double x)
{ return x>=y?(x>=-y?      IEEEDPAtan(y/x):     -PI/2-IEEEDPAtan(x/y)):
              (x>=-y? PI/2-IEEEDPAtan(x/y):y>=0? PI  +IEEEDPAtan(y/x):
                                          -PI  +IEEEDPAtan(y/x));
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
# define __math_decl const
# include <math-68881.h>
#endif /* __HAVE_68881__ */
