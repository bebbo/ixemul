#include <wchar.h>

int wmemcmp(const wchar_t *p, const wchar_t *q, size_t n)
{
	while (n && *p == *q)
	{
		++p;
		++q;
		--n;
	}

	return n ? *p - *q : 0;
}

