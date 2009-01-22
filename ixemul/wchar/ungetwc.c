#include "stdio.h"
#include <stdlib.h>
#include <wchar.h>
#include <errno.h>

wint_t ungetwc(wint_t c, FILE *f)
{
	char buf[MB_CUR_MAX];
	size_t n = wctomb(buf, c);

	if (n == (size_t)-1)
		return WEOF;

	do
	{
		if (ungetc(buf[--n], f) == EOF)
			return WEOF;
	}
	while (n != 0);

	return c;
}
