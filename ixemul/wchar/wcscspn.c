#include <wchar.h>

size_t wcscspn(const wchar_t *p, const wchar_t *q)
{
	const wchar_t *s = p;
	wchar_t c;

	while ((c = *s) != 0)
	{
		const wchar_t *t = q;

		while (*t)
		{
			if (*t == c)
				goto end;
			++t;
		}

		++s;
	}

end:
	return s - p;
}

