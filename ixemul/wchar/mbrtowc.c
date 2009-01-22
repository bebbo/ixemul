#define _KERNEL
#include "ixemul.h"
#include <wchar.h>
#include <stddef.h>
#include <errno.h>

size_t mbrtowc(wchar_t *wcp, const char *s, size_t n, mbstate_t *ps)
{
	usetup;

	int r = -1;
	wchar_t c = 0;

	if (s == NULL)
	{
		wcp = NULL;
		s = "";
		n = 1;
	}

	if (n == 0)
	{
		r = -2;
	}
	else
	{
		int c1 = *s++;

		if ((c1 & 0x80) == 0)
		{
			c = c1;
			r = 1;
		}
		else if ((c1 & 0xc0) != 0xc0)
		{
			// Invalid
		}
		else if (n == 1)
		{
			r = -2;
		}
		else
		{
			int c2 = *s++;

			if ((c2 & 0xc0) != 0x80)
			{
				// Invalid
			}
			else if ((c1 & 0x20) == 0)
			{
				c = ((c1 & 0x1f) << 6) | (c2 & 0x3f);
				r = 2;
			}
			else if (n == 2)
			{
				r = -2;
			}
			else
			{
				int c3 = *s++;

				if ((c3 & 0xc0) != 0x80)
				{
					// Invalid
				}
				else if ((c1 & 0x10) == 0)
				{
					c = ((c1 & 0x0f) << 12) | ((c2 & 0x3f) << 6) | (c3 & 0x3f);
					r = 3;
				}
				else if (n == 3)
				{
					r = -2;
				}
				else
				{
					int c4 = *s;

					if ((c3 & 0xc0) != 0x80)
					{
						// Invalid
					}
					else if ((c1 & 0x08) == 0)
					{
						c = ((c1 & 0x07) << 18) | ((c2 & 0x3f) << 12) |
							((c3 & 0x3f) << 6) | (c4 & 0x3f);
						r = 4;
					}
					else
					{
						// Invalid
					}
				}
			}
		}
	}

	if (r >= 0)
	{
		if (r == 0)
			c = 0;

		if (c == 0)
			r = 0;

		if (wcp)
			*wcp = c;
	}
	else if (r == -1)
	{
		errno = EILSEQ;
	}

	return (size_t)r;
}
