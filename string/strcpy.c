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

char *strcpy(char *s1,const char *s2)
{ char *s=s1;
  do
    *s++=*s2;
  while(*s2++!='\0');
  return s1;
}
#else
#include "defs.h"

ENTRY(strcpy)
asm(" \n\
	movl    sp@(8),a0               /* a0 = fromaddr */ \n\
	movl    sp@(4),d0               /* return value is toaddr */ \n\
	movl    d0,a1                   /* a1 = toaddr */ \n\
strcpyloop: \n\
	movb    a0@+,a1@+               /* copy a byte */ \n\
	jne     strcpyloop              /* copied non-null, keep going */ \n\
	rts \n\
");
#endif
