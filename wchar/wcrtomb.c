#define _KERNEL
#include "ixemul.h"
#include <wchar.h>
#include <stdlib.h>
#include <errno.h>

size_t wcrtomb(char * __restrict p, wchar_t c, mbstate_t * __restrict ps)
{
	usetup;
	int r;

	if (p == NULL)
	{
		r = 1;
	}
	else if (c & ~0x1fffff)
	{
		r = -1;
		errno = EILSEQ;
	}
	else if (c & ~0xffff)
	{
		r = 4;
		p[0] = 0xf0 | ((c >> 18) & 0x07);
		p[1] = 0x80 | ((c >> 12) & 0x3f);
		p[2] = 0x80 | ((c >> 6) & 0x3f);
		p[3] = 0x80 | (c & 0x3f);
	}
	else if (c & ~0x7ff)
	{
		r = 3;
		p[0] = 0xe0 | ((c >> 12) & 0x0f);
		p[1] = 0x80 | ((c >> 6) & 0x3f);
		p[2] = 0x80 | (c & 0x3f);
	}
	else if (c & ~0x7f)
	{
		r = 2;
		p[0] = 0xc0 | ((c >> 6) & 0x1f);
		p[1] = 0x80 | (c & 0x3f);
	}
	else
	{
		r = 1;
		p[0] = c;
	}

	return r;
}

