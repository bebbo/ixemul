#include "ixemul.h"

/* I wish I knew a more elegant way to do this:

   sstr(STACKSIZE) -> str((16384)) -> "(16384)"
*/
#define str(s) #s
#define sstr(s) str(s)

asm("
	.data
	.even
	.globl	___stack
	.ascii	\"StCk\"	| Magic cookie
___stack:
	.long	" sstr(STACKSIZE) "
	.ascii	\"sTcK\"	| Magic cookie
");
