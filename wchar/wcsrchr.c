#include <wchar.h>
#include <stddef.h>

wchar_t *wcsrchr(const wchar_t *p, wchar_t c)
{
	const wchar_t *r = NULL;

	for (;;)
	{
		if (*p == c)
			r = p;
		if (!*p)
			break;
		++p;
	}

	return (wchar_t *)r;
}


