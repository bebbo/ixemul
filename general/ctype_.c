/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)ctype_.c	5.6 (Berkeley) 6/1/90";
#endif /* LIBC_SCCS and not lint */

#define _KERNEL
#include "ixemul.h"

#include <ctype.h>

const char _ctype_[1 + 256] = {
  0,								/* -1      */
  _C,	_C,	_C,	_C,	_C,	_C,	_C,	_C,	/* 00 - 07 */
  _C,	_C|_S,	_C|_S,	_C|_S,	_C|_S,	_C|_S,	_C,	_C,	/* 08 - 0f */
  _C,	_C,	_C,	_C,	_C,	_C,	_C,	_C,	/* 10 - 17 */
  _C,	_C,	_C,	_C,	_C,	_C,	_C,	_C,	/* 18 - 20 */
  _S|_B,_P,	_P,	_P,	_P,	_P,	_P,	_P,	/* 20 - 27 */
  _P,	_P,	_P,	_P,	_P,	_P,	_P,	_P,	/* 28 - 2f */
  _N,	_N,	_N,	_N,	_N,	_N,	_N,	_N,	/* 30 - 37 */
  _N,	_N,	_P,	_P,	_P,	_P,	_P,	_P,	/* 38 - 3f */
  _P,	_U|_X,	_U|_X,	_U|_X,	_U|_X,	_U|_X,	_U|_X,	_U,	/* 40 - 47 */
  _U,	_U,	_U,	_U,	_U,	_U,	_U,	_U,	/* 48 - 4f */
  _U,	_U,	_U,	_U,	_U,	_U,	_U,	_U,	/* 50 - 57 */
  _U,	_U,	_U,	_P,	_P,	_P,	_P,	_P,	/* 58 - 5f */
  _P,	_L|_X,	_L|_X,	_L|_X,	_L|_X,	_L|_X,	_L|_X,	_L,	/* 60 - 67 */
  _L,	_L,	_L,	_L,	_L,	_L,	_L,	_L,	/* 68 - 6f */
  _L,	_L,	_L,	_L,	_L,	_L,	_L,	_L,	/* 70 - 77 */
  _L,	_L,	_L,	_P,	_P,	_P,	_P,	_C,	/* 78 - 7f */

/* Until we support proper Locales, set the remainder of this
   table to 0 */

#if 0
  /* ISO-1 character set */
  _C,	_C,	_C,	_C,	_C,	_C,	_C,	_C,	/* 80 - 87 */
  _C,	_C,	_C,	_C,	_C,	_C,	_C,	_C,	/* 88 - 8f */
  _C,	_C,	_C,	_C,	_C,	_C,	_C,	_C,	/* 90 - 97 */
  _C,	_C,	_C,	_C,	_C,	_C,	_C,	_C,	/* 98 - 9f */
  _S,	_P,	_P,	_P,	_P,	_P,	_P,	_P,	/* a0 - a7 */
  _P,	_P,	_P,	_P,	_P,	_P,	_P,	_P,	/* a8 - af */
  _P,  	_P,	_P,	_P,	_P,	_P,	_P,	_P,	/* b0 - b7 */
  _P,	_P,	_P,	_P,	_P,	_P,	_P,	_P,	/* b8 - bf */
  _U,	_U,	_U,	_U,	_U,	_U,	_U,	_U,	/* c0 - c7 */
  _U,	_U,	_U,	_U,	_U,	_U,	_U,	_U,	/* c8 - cf */
  _U,	_U,	_U,	_U,	_U,	_U,	_U,	_P,	/* d0 - d7 */
  _U,	_U,	_U,	_U,	_U,	_U,	_U,	_L,	/* d8 - df */
  _L,	_L,	_L,	_L,	_L,	_L,	_L,	_L,	/* e0 - e7 */
  _L,	_L,	_L,	_L,	_L,	_L,	_L,	_L,	/* e8 - ef */
  _L,	_L,	_L,	_L,	_L,	_L,	_L,	_P,	/* f0 - f7 */
  _L,	_L,	_L,	_L,	_L,	_L,	_L,	_L,	/* f8 - ff */
#endif
};
