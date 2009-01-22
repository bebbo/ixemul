#define _KERNEL
#include "ixemul.h"
#include <stdio.h>
#include <wchar.h>

#undef putwchar

wint_t putwchar(wchar_t c)
{
	usetup;
	register FILE *so = stdout;

	return putwc(c, so);
}
