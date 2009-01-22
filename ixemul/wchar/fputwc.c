#include "stdio.h"
#include <wchar.h>
#include <stdlib.h>

wint_t fputwc(wchar_t c, FILE *f)
{
	char buf[MB_CUR_MAX];
	size_t n;

	if (!(f->_flags & (__SWCH | __SCHR)))
		f->_flags |= __SWCH;

	n = wcrtomb(buf, c, NULL);

	if (n != (size_t)-1)
	{
		if (fwrite(buf, 1, n, f) != n)
			n = (size_t)-1;
	}

	return n == (size_t)-1 ? WEOF : c;
}

