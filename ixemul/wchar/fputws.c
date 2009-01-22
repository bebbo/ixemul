#include "stdio.h"
#include <wchar.h>

int fputws(const wchar_t *s, FILE *f)
{
	char buf[256];
	int r = 1;

	if (!(f->_flags & (__SWCH | __SCHR)))
		f->_flags |= __SWCH;

	while (*s)
	{
		size_t l = wcsrtombs(buf, &s, sizeof(buf), NULL);

		if (l == (size_t)-1)
		{
			f->_flags |= __SERR;
			r = EOF;
			break;
		}

		if (fwrite(buf, 1, l, f) != l)
		{
			r = EOF;
			break;
		}
	}

	return r;
}
