#include <wchar.h>

size_t wcslen(const wchar_t *s)
{
	const wchar_t *p = s;
	while (*s)
		++s;
	return s - p;
}
