#include <wchar.h>
#include <stddef.h>

wchar_t *wmemchr(const wchar_t *p, wchar_t c, size_t n)
{
	while (n && *p != c)
	{
		++p;
		--n;
	}

	return n == 0 ? NULL : (wchar_t *)p;
}

