#include <stdlib.h>
#include <wchar.h>

int wctomb(char *s, wchar_t c)
{
	int r;

	if (s == NULL)
	{
		r = 0;
	}
	else
	{
		r = (int)wcrtomb(s, c, NULL);
	}

	return r;
}
