#include "ixemul.h"

/* I wish I knew a more elegant way to do this:

   sstr(STACKSIZE) -> str((16384)) -> "(16384)"
*/
#define str(s) #s
#define sstr(s) str(s)

#ifndef __PPC__
asm(" \n\
	.data \n\
	.even \n\
	.globl  ___stack \n\
	.ascii  \"StCk\"        | Magic cookie \n\
___stack: \n\
	.long   " sstr(STACKSIZE) " \n\
	.ascii  \"sTcK\"        | Magic cookie \n\
");
#else
asm("
	.section \".data\"
	.align  2
	.globl  __stack
	.ascii  \"StCk\"        /* Magic cookie */
__stack:
	.long   " sstr(STACKSIZE * 2) "
	.ascii  \"sTcK\"        /* Magic cookie */
");

#endif
