#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)memcpy.c    1.0 (mw)";
#endif /* LIBC_SCCS and not lint */

#define _KERNEL
#include <ixemul.h>

void *
memcpy(dst0, src0, length)
	void *dst0;
	const void *src0;
	size_t length;
{
	CopyMem ((char *)src0, dst0, length);
	return(dst0);
}
