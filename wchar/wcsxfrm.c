#include <wchar.h>
#include <stddef.h>

size_t wcsxfrm(wchar_t * __restrict p, const wchar_t * __restrict q, size_t n)
{
	//TODO:

	size_t r = wcslen(q);

	if (p != NULL && n > r)
	{
		wcscpy(p, q);
	}

	return r;
}
