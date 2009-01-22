#define _KERNEL
#include "ixemul.h"
#include <stdio.h>
#include <wchar.h>

#undef getwchar

wint_t getwchar(void)
{
	usetup;
	return getwc(stdin);
}
