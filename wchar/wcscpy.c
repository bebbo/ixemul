#include <wchar.h>

wchar_t *wcscpy(wchar_t *p, const wchar_t *q)
{
	wchar_t *r = p;

	while ((*p++ = *q++) != 0);

	return r;
}

