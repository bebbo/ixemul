#include <wchar.h>

#undef mbsinit

int mbsinit(const mbstate_t *mbs)
{
	return 1;
}
