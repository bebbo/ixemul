/*-
 * Copyright (c) 1990 The Regents of the University of California.
 * All rights reserved.
 *
 * This code is derived from software contributed to Berkeley by
 * the Systems Programming Group of the University of Utah Computer
 * Science Department.
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

#ifdef __PPC__
#include <string.h>

char  *index(const char *a, int b)
{
      return (char *)strchr(a,b);
}

char *strchr(const char *s,int c)
{
  while (*s!=(char)c)
    if (!(*s++))
      {
	s = NULL;
	break;
      }
  return (char *)s;
}


#else
#include "defs.h"

ENTRY(index)
ENTRY(strchr)
asm(" \n\
	movl    sp@(4),a0       /* string */ \n\
	movb    sp@(11),d0      /* char to look for */ \n\
ixloop: \n\
	cmpb    a0@,d0          /* found our char? */ \n\
	jeq     ixfound         /* yes, break out */ \n\
	tstb    a0@+            /* null? */ \n\
	jne     ixloop          /* no, keep going */ \n\
 	subal   a0,a0           /* not found, return null */ \n\
ixfound: \n\
	movl    a0,d0           /* found, return pointer */ \n\
	rts \n\
");
#endif
