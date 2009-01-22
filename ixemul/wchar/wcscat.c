#include <wchar.h>

wchar_t *wcscat(wchar_t *p, const wchar_t *q)
{
	wchar_t *s = p;

	while(*s)
		++s;

	while((*s++ = *q++))
		;

	return p;
}
