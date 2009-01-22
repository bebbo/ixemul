#include <wchar.h>
#include <stddef.h>

size_t mbsrtowcs(wchar_t * __restrict p, const char ** __restrict sp, size_t n, mbstate_t * __restrict ps)
{
	const char* s = *sp;
	size_t r = 0;

	if (p == NULL)
		n = (size_t)-1;

	while (r < n)
	{
		wchar_t c;
		size_t t = mbrtowc(&c, s, (size_t)-1, NULL);

		if (t == (size_t)-1)
		{
			r = t;
			break;
		}


		if (p)
			*p++ = c;

		if (c == 0)
		{
			if (p)
				s = NULL;
			break;
		}

		++r;
		s += t;
	}

	*sp = s;

	return r;
}

