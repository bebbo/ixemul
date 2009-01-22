#define _KERNEL
#include "ixemul.h"
#include <wctype.h>
#include <ctype.h>

#undef iswalnum
int iswalnum(wint_t c)
{
	int r = 0;

	if ((unsigned)c <= 255)
		return ((_ctype_ + 1)[c] & (_U|_L|_N));
	//else if(__localevec[LC_CTYPE-1])
	//	r = IsAlNum(__localevec[LC_CTYPE-1], c);

	return r;
}
#undef iswalpha
int iswalpha(wint_t c)
{
	int r = 0;

	if ((unsigned)c <= 255)
		return ((_ctype_ + 1)[c] & (_U|_L));
	//else if(__localevec[LC_CTYPE-1])
	//	r = IsAlpha(__localevec[LC_CTYPE-1], c);

	return r;
}
#undef iswblank
int iswblank(wint_t c)
{
	return c == L' ' || c == L'\t';
}
#undef iswcntrl
int iswcntrl(wint_t c)
{
	int r = 0;

	if ((unsigned)c <= 255)
		return ((_ctype_ + 1)[c] & _C);
	//else if(__localevec[LC_CTYPE-1])
	//	r = IsCntrl(__localevec[LC_CTYPE-1], c);

	return r;
}
#undef iswdigit
int iswdigit(wint_t c)
{
	return ((_ctype_ + 1)[c] & _N);
}
#undef iswgraph
int iswgraph(wint_t c)
{
	int r = 0;

	if ((unsigned)c <= 255)
		return ((_ctype_ + 1)[c] & (_P|_U|_L|_N));
	//else if(__localevec[LC_CTYPE-1])
	//	r = IsGraph(__localevec[LC_CTYPE-1], c);

	return r;
}
#undef iswlower
int iswlower(wint_t c)
{
	int r = 0;

	if ((unsigned)c <= 255)
		return ((_ctype_ + 1)[c] & _L);
	//else if(__localevec[LC_CTYPE-1])
	//	r = IsLower(__localevec[LC_CTYPE-1], c);

	return r;
}
#undef iswprint
int iswprint(wint_t c)
{
	int r = 0;

	if ((unsigned)c <= 255)
		return ((_ctype_ + 1)[c] & (_P|_U|_L|_N|_B));
	//else if(__localevec[LC_CTYPE-1])
	//	r = IsPrint(__localevec[LC_CTYPE-1], c);

	return r;
}
#undef iswpunct
int iswpunct(wint_t c)
{
	int r = 0;

	if ((unsigned)c <= 255)
		return ((_ctype_ + 1)[c] & _P);
	//else if(__localevec[LC_CTYPE-1])
	//	r = IsPunct(__localevec[LC_CTYPE-1], c);

	return r;
}
#undef iswspace
int iswspace(wint_t c)
{
	int r = 0;

	if ((unsigned)c <= 255)
		return ((_ctype_ + 1)[c] & _S);
	//else if(__localevec[LC_CTYPE-1])
	//	r = IsSpace(__localevec[LC_CTYPE-1], c);

	return r;
}
#undef iswupper
int iswupper(wint_t c)
{
	int r = 0;

	if ((unsigned)c <= 255)
		return ((_ctype_ + 1)[c] & _U);
	//else if(__localevec[LC_CTYPE-1])
	//	r = IsUpper(__localevec[LC_CTYPE-1], c);

	return r;
}
#undef iswxdigit
int iswxdigit(wint_t c)
{
	int r = 0;

	if ((unsigned)c <= 255)
		return ((_ctype_ + 1)[c] & (_N|_X));

	return r;
}
#undef towlower
wint_t towlower(wint_t c)
{
	return (unsigned)c <= 255 && (_ctype_ + 1)[c] & _U ? c + 'a' - 'A' : c;
}
#undef towupper
wint_t towupper(wint_t c)
{
	return (unsigned)c <= 255 && (_ctype_ + 1)[c] & _L ? c + 'A' - 'a' : c;
}

#warning wcsftime not implemented

size_t wcsftime(wchar_t *p, size_t n, const wchar_t *f, const struct tm *t)
{
	int k;
	for (k = 0; *f && k < n - 1; ++k)
		*p++ = f[k];
	*p = 0;
	return k;
}

wctype_t wctype(const char *s)
{
	static const char * const names[] =
	{
		"alnum",
		"alpha",
		"blank",
		"cntrl",
		"digit",
		"graph",
		"lower",
		"print",
		"punct",
		"space",
		"upper",
		"xdigit",
		NULL
	};

	const char * const *p;
	unsigned n = 1;

	for (p = names; *p; ++p, ++n)
	{
		if (strcmp(s, *p) == 0)
		{
			return n;
		}
	}

	return 0;
}

int iswctype(wint_t c, wctype_t d)
{
	int (* const func[])(wint_t) =
	{
		iswalnum,
		iswalpha,
		iswblank,
		iswcntrl,
		iswdigit,
		iswgraph,
		iswlower,
		iswprint,
		iswpunct,
		iswspace,
		iswupper,
		iswxdigit
	};
	const int numfunc = sizeof(func) / sizeof(func[0]);

	int r = 0;

	if (d >= 1 && d <= numfunc)
		r = func[d - 1](c);

	return r;
}

