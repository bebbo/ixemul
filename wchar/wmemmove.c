#include <wchar.h>
#include <string.h>

wchar_t *wmemmove(wchar_t *p, const wchar_t *q, size_t n)
{
	return memmove(p, q, n * sizeof(wchar_t));
}
