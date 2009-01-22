#ifndef __IXMATH_H__
#define __IXMATH_H__

#ifndef __HAVE_68881__

#ifdef __PPC__
#define _NO_PPCINLINE
#endif

#ifndef __MORPHOS__
#include <proto/mathieeedoubbas.h>
#include <proto/mathieeedoubtrans.h>
#include <proto/mathieeesingbas.h>
#endif

#ifdef __PPC__
#undef _NO_PPCINLINE
#endif

#endif /* __HAVE_68881__ */

#endif /* __IXMATH_H__ */
