#include <stdlib.h>
#include <wchar.h>

int mbtowc(wchar_t * __restrict wcp, const char * __restrict s, size_t n)
{
	int r;

	if (s == NULL)
	{
		r = 0;
	}
	else
	{
		r = (int)mbrtowc(wcp, s, n, NULL);
		if (r == -2)
			r = -1;
	}

	return r;
}
