#include <stdlib.h>
#include <wchar.h>

size_t wcstombs(char * __restrict p, const wchar_t * __restrict s, size_t n)
{
	return wcsrtombs(p, &s, n, NULL);
}

