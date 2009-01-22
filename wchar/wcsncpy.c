#include <wchar.h>

wchar_t *wcsncpy(wchar_t *p, const wchar_t *q, size_t n)
{
	wchar_t *s = p;

	while (n && (*p = *q) != 0)
	{
		++p;
		++q;
		--n;
	}

	return s;
}
