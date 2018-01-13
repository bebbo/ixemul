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
static char sccsid[] = "@(#)isctype.c	5.2 (Berkeley) 6/1/90";
#endif /* LIBC_SCCS and not lint */

#define _KERNEL
#include "ixemul.h"

#define _ANSI_LIBRARY
#include <ctype.h>

#undef isalnum
int isalnum(int c)
{
  return ((_ctype_ + 1)[(unsigned char)c] & (_U|_L|_N));
}

#undef isalpha
int isalpha(int c)
{
  return ((_ctype_ + 1)[(unsigned char)c] & (_U|_L));
}

#undef isblank
int
isblank(c)
	int c;
{
	return(c == ' ' || c == '\t');
}

#undef iscntrl
int iscntrl(int c)
{
  return ((_ctype_ + 1)[(unsigned char)c] & _C);
}

#undef isdigit
int isdigit(int c)
{
  return ((_ctype_ + 1)[(unsigned char)c] & _N);
}

#undef isgraph
int isgraph(int c)
{
  return ((_ctype_ + 1)[(unsigned char)c] & (_P|_U|_L|_N));
}

#undef islower
int islower(int c)
{
  return ((_ctype_ + 1)[(unsigned char)c] & _L);
}

#undef isprint
int isprint(int c)
{
  return ((_ctype_ + 1)[(unsigned char)c] & (_P|_U|_L|_N|_B));
}

#undef ispunct
int ispunct(int c)
{
  return ((_ctype_ + 1)[(unsigned char)c] & _P);
}

#undef isspace
int isspace(int c)
{
  return ((_ctype_ + 1)[(unsigned char)c] & _S);
}

#undef isupper
int isupper(int c)
{
  return ((_ctype_ + 1)[(unsigned char)c] & _U);
}

#undef isxdigit
int isxdigit(int c)
{
  return ((_ctype_ + 1)[(unsigned char)c] & (_N|_X));
}

#undef tolower
int tolower(int c)
{
  return (((_ctype_ + 1)[(unsigned char)c] & _U) ? (c) - 'A' + 'a' : (c));
}

#undef toupper
int toupper(int c)
{
  return (((_ctype_ + 1)[(unsigned char)c] & _L) ? (c) - 'a' + 'A' : (c));
}

#undef isascii
int isascii(int c)
{
  return (c & ~0177) == 0;
}

#undef isiso
int isiso(int c)
{
  return (c & ~0377) == 0;
}

#undef toascii
int toascii(int c)
{
  return ((c) & 0177);
}

#undef toiso
int toiso(int c)
{
  return ((c) & 0377);
}
