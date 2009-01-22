#include <wchar.h>

int wcscmp(const wchar_t *p, const wchar_t *q)
{
	int r = 0;

	while (*p && *p == *q)
	{
		++p;
		++q;
	}

	if (*p)
	{
		if (*q)
			r = *p - *q;
		else
			r = 1;
	}
	else if (*q)
	{
		r = -1;
	}

	return r;
}

