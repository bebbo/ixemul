#include <wchar.h>
#include <stddef.h>

wchar_t *wcschr(const wchar_t *p, wchar_t c)
{
	while (*p && *p != c)
		++p;

	return *p == c ? (wchar_t *)p : NULL;
}

