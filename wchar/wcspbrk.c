#include <wchar.h>
#include <stddef.h>

wchar_t *wcspbrk(const wchar_t *p, const wchar_t *q)
{
	wchar_t c;

	while ((c = *p) != 0)
	{
		const wchar_t *s = q;

		while (*s)
		{
			if (*s == c)
				return (wchar_t *)p;
			++s;
		}

		++p;
	}

	return NULL;
}

