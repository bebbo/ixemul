#include <wchar.h>

#undef putwc

wint_t putwc(wchar_t c, FILE *f)
{
	return fputwc(c, f);
}

