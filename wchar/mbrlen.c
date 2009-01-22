#include <wchar.h>
#include <stddef.h>

#undef mbrlen

size_t mbrlen(const char *s, size_t n, mbstate_t *ps)
{
	return mbrtowc(NULL, s, n, NULL);
}
