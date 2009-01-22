#include <wchar.h>

#undef getwc

wint_t getwc(FILE *f)
{
	return fgetwc(f);
}

