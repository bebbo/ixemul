#include <stdlib.h>
#include <wchar.h>

size_t mbstowcs(wchar_t * __restrict p, const char * __restrict s, size_t n)
{
	return mbsrtowcs(p, &s, n, NULL);
}
