#ifndef _WCTYPE_H
#define _WCTYPE_H

#include <wchar.h>
#include <sys/cdefs.h>

typedef int wctype_t;

typedef int wctrans_t;

#undef WEOF
#define WEOF	(-1)


__BEGIN_DECLS

int iswalnum(wint_t);
int iswalpha(wint_t);
int iswblank(wint_t);
int iswcntrl(wint_t);
int iswdigit(wint_t);
int iswgraph(wint_t);
int iswlower(wint_t);
int iswprint(wint_t);
int iswpunct(wint_t);
int iswspace(wint_t);
int iswupper(wint_t);
int iswxdigit(wint_t);
int iswctype(wint_t, wctype_t);

wctype_t wctype(const char *);

wint_t towlower(wint_t);
wint_t towupper(wint_t);

wint_t towctrans(wint_t, wctrans_t);
wctrans_t wctrans(const char *);

__END_DECLS

#endif
