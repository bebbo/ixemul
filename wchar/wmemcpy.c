#include <wchar.h>

wchar_t *wmemcpy(wchar_t *p, const wchar_t *q, size_t n)
{
	return memcpy(p, q, n * sizeof(wchar_t));
}
