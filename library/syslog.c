/*
 * Copyright (c) 1983, 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)syslog.c    5.34 (Berkeley) 6/26/91";
#endif /* LIBC_SCCS and not lint */

#define _KERNEL
#include "ixemul.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/syslog.h>
#include <sys/uio.h>
#include <sys/errno.h>
#include <netdb.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <unistd.h>
#include <paths.h>
#include <stdio.h>

#define LogFile (u.u_LogFile)
#define LogStat (u.u_LogStat)
#define LogTag (u.u_LogTag)
#define LogFacility (u.u_LogFacility)
#define LogMask (u.u_LogMask)

/*
 * syslog, vsyslog --
 *	print message on log file; output is intended for syslogd(8).
 */

void
vsyslog(int pri, const char *fmt, va_list ap)
{
    usetup;
    register int cnt;
    register char *p;
    /* time_t now, time(); */
    int fd, saved_errno;
    char tbuf[2048], fmt_cpy[1024], *stdp = NULL, *ctime();
    register struct user *usr = &u;

    if (usr->u_ixnetbase) {
	netcall(NET_vsyslog, pri, fmt, ap);
    }
    else {
	/* check for invalid bits or no priority set */
	if (!LOG_PRI(pri) || (pri &~ (LOG_PRIMASK|LOG_FACMASK)) ||
	    !(LOG_MASK(pri) & LogMask))
	    return;

	saved_errno = errno;

	/* set default facility if none specified */
	if ((pri & LOG_FACMASK) == 0)
	    pri |= LogFacility;

	/* build the message */
	/* (void)syscall(SYS_time,&now); */
	/*(void)sprintf(tbuf, "<%d>%.15s ", pri, ctime(&now) + 4); */
	(void)sprintf(tbuf, "<%d> ", pri);
	for (p = tbuf; *p; ++p);

	if (LogStat & LOG_PERROR)
	    stdp = p;

	if (LogTag) {
	    (void)strcpy(p, LogTag);
	    for (; *p; ++p);
	}
	if (LogStat & LOG_PID) {
	    (void)sprintf(p, "[%d]", getpid());
	    for (; *p; ++p)
		;
	}
	if (LogTag) {
	    *p++ = ':';
	    *p++ = ' ';
	}

	/* substitute error message for %m */
	{
	    register char ch, *t1, *t2;
	    char *strerror();

            for (t1 = fmt_cpy; (ch = *fmt); ++fmt)
		if (ch == '%' && fmt[1] == 'm') {
		    ++fmt;
		    for (t2 = strerror(saved_errno);
                        (*t1 = *t2++); ++t1);
		}
		else
		    *t1++ = ch;
	    *t1 = '\0';
	}

	(void)vsprintf(p, fmt_cpy, ap);

	cnt = strlen(tbuf);

	/* output to stderr if requested */
	if (LogStat & LOG_PERROR) {
	    struct iovec iov[2];
	    register struct iovec *v = iov;

	    v->iov_base = stdp;
	    v->iov_len = cnt - (stdp - tbuf);
	    ++v;
	    v->iov_base = "\n";
	    v->iov_len = 1;
	    (void)syscall(SYS_writev,STDERR_FILENO, iov, 2);
	}
	/* see if should attempt the console */
	if (!(LogStat&LOG_CONS))
	    return;

	/*
	 * Output the message to the console; don't worry about blocking,
	 * if console blocks everything will.  Make sure the error reported
	 * is the one from the syslogd failure.
	 */
	if ((fd = syscall(SYS_open,_PATH_CONSOLE, O_WRONLY, 0)) >= 0) {
	    (void)strcat(tbuf, "\r\n");
	    cnt += 2;
	    p = index(tbuf, '>') + 1;
	    (void)syscall(SYS_write,fd, p, cnt - (p - tbuf));
	    (void)syscall(SYS_close,fd);
	}
    }
}

void
syslog(int pri, const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vsyslog(pri, fmt, ap);
	va_end(ap);
}

void
openlog(const char *ident, int logstat, int logfac)
{
    usetup;

    if (u.u_ixnetbase) {
	netcall(NET_openlog, ident, logstat, logfac);
    }
    else {
	if (ident != NULL)
	    LogTag = (char *)ident;
	LogStat = logstat;
	if (logfac != 0 && (logfac &~ LOG_FACMASK) == 0)
	    LogFacility = logfac;
    }
}

void
closelog(void)
{
    usetup;

    if (u.u_ixnetbase) {
	netcall(NET_closelog);
    }
    else {
	syscall(SYS_close,LogFile);
	LogFile = -1;
    }
}

/* setlogmask -- set the log mask level */
int
setlogmask(int pmask)
{
  int omask;
  usetup;

  if (u.u_ixnetbase)
    return netcall(NET_setlogmask, pmask);
  omask = LogMask;
  if (pmask != 0)
    LogMask = pmask;
  return omask;
}
