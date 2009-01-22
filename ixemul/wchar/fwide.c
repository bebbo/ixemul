#include "stdio.h"
#include <wchar.h>


#define _NOT_ORIENTED_STREAM(f) (((f)->_flags & (__SWCH | __SCHR)) == 0)
#define _CHAR_STREAM(f)         ((f)->_flags & __SCHR)
#define _WCHAR_STREAM(f)        ((f)->_flags & __SWCH)
#define _ORIENT_CHAR(f)         do { FILE *s = (f); s->_flags &= ~__SWCH; s->_flags |= __SCHR; } while (0)
#define _ORIENT_WCHAR(f)        do { FILE *s = (f); s->_flags &= ~__SCHR; s->_flags |= __SWCH; } while (0)


int fwide(FILE *f, int mode)
{
	if (_NOT_ORIENTED_STREAM(f))
	{
		if (mode > 0)
			_ORIENT_WCHAR(f);
		else if (mode < 0)
			_ORIENT_CHAR(f);
	}

	return _WCHAR_STREAM(f) ? 1 : (_CHAR_STREAM(f) ? -1 : 0);
}
