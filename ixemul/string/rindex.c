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

#ifndef mc68000
#include <string.h>

char *rindex(const char *a, int b)
{
  return (char *)strrchr(a,b);
}

char *strrchr(const char *s, int c)
{
  char *c1 = NULL;
  do
    if (*s == (char)c)
      c1 = (char *)s;
  while (*s++ != '\0');
  return c1;
}

#else
#include "defs.h"

ENTRY(rindex)
ENTRY(strrchr)
asm(" \n\
	movl    sp@(4),a1       /* string */ \n\
	movb    sp@(11),d1      /* char to look for */ \n\
	moveq   #0,d0           /* clear rindex pointer */ \n\
rixloop: \n\
	cmpb    a1@,d1          /* found our char? */ \n\
	jne     rixnope         /* no, check for null */ \n\
	movl    a1,d0           /* yes, remember location */ \n\
rixnope: \n\
	tstb    a1@+            /* null? */ \n\
	jne     rixloop         /* no, keep going */ \n\
	rts \n\
");
#endif
