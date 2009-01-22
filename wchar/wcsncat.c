#include <wchar.h>

wchar_t *wcsncat(wchar_t *p, const wchar_t *q, size_t n)
{
	if (n != 0)
	{
		wchar_t *s = p;

		while(*s)
			++s;

		do
		{
			*s++ = *q++;
		}
		while (--n != 0);

		*s = 0;
	}

	return p;
}


