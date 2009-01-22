#define _KERNEL
#include "ixemul.h"
#include <wchar.h>
#include <stddef.h>
#include <errno.h>


size_t wcsrtombs(char *p, const wchar_t **sp, size_t n, mbstate_t *ps)
{
	usetup;
	const wchar_t* s = *sp;
	size_t r = 0;

	while (n)
	{
		wchar_t c = *s;

		if (c & ~0x1fffff)
		{
			r = -1;
			errno = EILSEQ;
			break;
		}
		else if (c & ~0xffff)
		{
			if (n < 4)
				break;
			*p++ = 0xf0 | ((c >> 18) & 0x07);
			*p++ = 0x80 | ((c >> 12) & 0x3f);
			*p++ = 0x80 | ((c >> 6) & 0x3f);
			*p++ = 0x80 | (c & 0x3f);
			n -= 4;
			r += 4;
		}
		else if (c & ~0x7ff)
		{
			if (n < 3)
				break;
			*p++ = 0xe0 | ((c >> 12) & 0x0f);
			*p++ = 0x80 | ((c >> 6) & 0x3f);
			*p++ = 0x80 | (c & 0x3f);
			n -= 3;
			r += 3;
		}
		else if (c & ~0x7f)
		{
			if (n < 2)
				break;
			*p++ = 0xc0 | ((c >> 6) & 0x1f);
			*p++ = 0x80 | (c & 0x3f);
			n -= 2;
			r += 2;
		}
		else
		{
			*p++ = c;
			--n;
			++r;
		}

		++s;

		if (c == 0)
		{
			--r;
			if (p)
				s = NULL;
			break;
		}
	}

	*sp = s;

	return r;
}
