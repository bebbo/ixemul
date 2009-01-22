#define _KERNEL
#include <wchar.h>
#include <stdio.h>

#undef btowc

wint_t btowc(int c)
{
	return c & ~0x7f ? WEOF : c;
}
