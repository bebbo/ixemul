#include "stdio.h"
#include <wchar.h>
#include <errno.h>

wchar_t *fgetws(wchar_t *p, int n, FILE *f)
{
	wchar_t *r = p;

	if (n <= 0)
		return NULL;

	while (--n != 0)
	{
		wchar_t c = getwc(f);

		if (c == WEOF)
		{
			if (feof(f) && !ferror(f) && r != p)
			{
				*p = 0;
				return r;
			}
			else
				return NULL;
		}

		*p = c;

		if (c == L'\n')
			break;
	}

	*p = 0;

	return r;
}
