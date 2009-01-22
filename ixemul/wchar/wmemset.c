#include <wchar.h>

wchar_t *wmemset(wchar_t *p, wchar_t c, size_t n)
{
	wchar_t *q = p;

	while (n)
	{
		*q++ = c;
		--n;
	}

	return p;
}
