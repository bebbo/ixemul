/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)strftime.c	5.8 (Berkeley) 6/1/90";
#endif /* LIBC_SCCS and not lint */

/*
 * NOTE: I used the older version of this file, since the 4.3bsd-net2
 *	 version uses mktime() from ctime.c, and ctime.c is badly suited
 *	 for inclusion in a shared library (too much static stuff...)
 */


#define _KERNEL
#include "ixemul.h"

#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <tzfile.h>
#include <string.h>

static char *afmt[] = {
	"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat",
};
static char *Afmt[] = {
	"Sunday", "Monday", "Tuesday", "Wednesday", "Thursday", "Friday",
	"Saturday",
};
static char *bfmt[] = {
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep",
	"Oct", "Nov", "Dec",
};
static char *Bfmt[] = {
	"January", "February", "March", "April", "May", "June", "July",
	"August", "September", "October", "November", "December",
};

static size_t _fmt(const char *, const struct tm *);
static int _conv (int, int, char);
static int _add (char *);

size_t strftime(char *s, size_t maxsize, const char *format, const struct tm *t)
{
        usetup;
	size_t res = 0;

	u.u_pt = s;
	if ((u.u_gsize = maxsize) < 1)
	  	return 0;
	if (_fmt(format, t)) {
		*u.u_pt = '\0';
		res = maxsize - u.u_gsize;
	}
	return res;
}

static size_t _fmt(const char *format, const struct tm *t)
{
        usetup;

	for (; *format; ++format) {
		if (*format == '%')
			switch (*++format) {
			case '\0':
				--format;
				break;
			case 'A':
				if (t->tm_wday < 0 || t->tm_wday > 6)
					return(0);
				if (!_add(Afmt[t->tm_wday]))
					return(0);
				continue;
			case 'a':
				if (t->tm_wday < 0 || t->tm_wday > 6)
					return(0);
				if (!_add(afmt[t->tm_wday]))
					return(0);
				continue;
			case 'B':
				if (t->tm_mon < 0 || t->tm_mon > 11)
					return(0);
				if (!_add(Bfmt[t->tm_mon]))
					return(0);
				continue;
			case 'b':
			case 'h':
				if (t->tm_mon < 0 || t->tm_mon > 11)
					return(0);
				if (!_add(bfmt[t->tm_mon]))
					return(0);
				continue;
			case 'C':
				if (!_fmt("%a %b %e %H:%M:%S %Y", t))
					return(0);
				continue;
			case 'c':
				if (!_fmt("%m/%d/%y %H:%M:%S", t))
					return(0);
				continue;
			case 'e':
				if (!_conv(t->tm_mday, 2, ' '))
					return(0);
				continue;
			case 'D':
				if (!_fmt("%m/%d/%y", t))
					return(0);
				continue;
			case 'd':
				if (!_conv(t->tm_mday, 2, '0'))
					return(0);
				continue;
			case 'H':
				if (!_conv(t->tm_hour, 2, '0'))
					return(0);
				continue;
			case 'I':
				if (!_conv(t->tm_hour % 12 ?
				    t->tm_hour % 12 : 12, 2, '0'))
					return(0);
				continue;
			case 'j':
				if (!_conv(t->tm_yday + 1, 3, '0'))
					return(0);
				continue;
			case 'k':
				if (!_conv(t->tm_hour, 2, ' '))
					return(0);
				continue;
			case 'l':
				if (!_conv(t->tm_hour % 12 ?
				    t->tm_hour % 12 : 12, 2, ' '))
					return(0);
				continue;
			case 'M':
				if (!_conv(t->tm_min, 2, '0'))
					return(0);
				continue;
			case 'm':
				if (!_conv(t->tm_mon + 1, 2, '0'))
					return(0);
				continue;
			case 'n':
				if (!_add("\n"))
					return(0);
				continue;
			case 'p':
				if (!_add(t->tm_hour >= 12 ? "PM" : "AM"))
					return(0);
				continue;
			case 'R':
				if (!_fmt("%H:%M", t))
					return(0);
				continue;
			case 'r':
				if (!_fmt("%I:%M:%S %p", t))
					return(0);
				continue;
			case 'S':
				if (!_conv(t->tm_sec, 2, '0'))
					return(0);
				continue;
			case 'T':
			case 'X':
				if (!_fmt("%H:%M:%S", t))
					return(0);
				continue;
			case 't':
				if (!_add("\t"))
					return(0);
				continue;
			case 'U':
				if (!_conv((t->tm_yday + 7 - t->tm_wday) / 7,
				    2, '0'))
					return(0);
				continue;
			case 'W':
				if (!_conv((t->tm_yday + 7 -
				    (t->tm_wday ? (t->tm_wday - 1) : 6))
				    / 7, 2, '0'))
					return(0);
				continue;
			case 'w':
				if (!_conv(t->tm_wday, 1, '0'))
					return(0);
				continue;
			case 'x':
				if (!_fmt("%m/%d/%y", t))
					return(0);
				continue;
			case 'y':
				if (!_conv((t->tm_year + TM_YEAR_BASE)
				    % 100, 2, '0'))
					return(0);
				continue;
			case 'Y':
				if (!_conv(t->tm_year + TM_YEAR_BASE, 4, '0'))
					return(0);
				continue;
			case 'Z':
				if (!t->tm_zone || !_add(t->tm_zone))
					return(0);
				continue;
			case '%':
			/*
			 * X311J/88-090 (4.12.3.5): if conversion char is
			 * undefined, behavior is undefined.  Print out the
			 * character itself as printf(3) does.
			 */
			default:
				break;
		}
		if (!u.u_gsize--)
			return(0);
		*u.u_pt++ = *format;
	}
	return(u.u_gsize);
}

static int _conv(int n, int digits, char pad)
{
	char buf[10];
	register char *p;

	buf[sizeof(buf) - 1] = '\0';
	for (p = buf + sizeof(buf) - 2; n > 0 && p > buf; n /= 10, --digits)
		*p-- = n % 10 + '0';
	while (p > buf && digits-- > 0)
		*p-- = pad;
	return(_add(++p));
}

static int _add(char *str)
{
        usetup;

	for (;; ++u.u_pt, --u.u_gsize) {
		if (!u.u_gsize)
			return(0);
		if (!(*u.u_pt = *str++))
			return(1);
	}
}
