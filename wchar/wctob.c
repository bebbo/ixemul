#include <wchar.h>
#include <stdio.h>

#undef wctob

int wctob(wint_t c)
{
	return c & ~0x7f ? EOF : c;
}
