#include "stdio.h"
#include <wchar.h>
#include <stdlib.h>

wint_t fgetwc(FILE *f)
{
	char buf[MB_CUR_MAX];
	size_t n, m;
	wchar_t c;

	if (!(f->_flags & (__SWCH | __SCHR)))
		f->_flags |= __SWCH;

	if (f->_r > 0)
	{
		n = mbrtowc(&c, f->_p, f->_r, NULL);

		if (n == -1)
		{
			f->_flags |= __SERR;
			return WEOF;
		}
		else if (n == (size_t)-2) /* incomplete mbs */
		{
			n = f->_r;
			memcpy(buf, f->_p, n);
			f->_p += n;
			f->_r = 0;
		}
		else
		{
			f->_p += n;
			f->_r -= n;
			return c;
		}
	}
	else
	{
		n = 0;
	}

	do
	{
		int t = fgetc(f);

		if (t == EOF)
		{
			return WEOF;
		}

		buf[n++] = (char)t;

		m = mbrtowc(&c, buf, n, NULL);
	}
	while (m == (size_t)-2);

	if (m == (size_t)-1)
	{
		f->_flags |= __SERR;
		return WEOF;
	}

	return c;
}

