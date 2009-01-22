#include <wchar.h>

int wcsncmp(const wchar_t *p, const wchar_t *q, size_t n)
{
	int r = 0;

	while (n && *p && *p == *q)
	{
		++p;
		++q;
		--n;
	}

	if (n != 0 && *p != *q)
	{
		if (*p)
			r = 1;
		else
			r = -1;
	}

	return r;
}

