/*
 * Copyright (c) 1988 The Regents of the University of California.
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
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
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
static char sccsid[] = "@(#)getpass.c   5.9 (Berkeley) 5/6/91";
#endif /* LIBC_SCCS and not lint */

#define _KERNEL
#include "ixemul.h"
#include <sys/termios.h>
#include <termios.h>
#include "kprintf.h"
#include "ixprotos.h"

int       tcgetattr __P((int, struct termios *));
int       tcsetattr __P((int, int, const struct termios *));
ssize_t	 write __P((int, const void *, size_t));

char *
getpass(const char *prompt)
{
    struct termios term;
    register int ch;
    register char *s;
    FILE *fp, *outfp;
    long omask;
    int echo;
    static char buf[_PASSWORD_LEN + 1];
    usetup;

    outfp = fp = stderr;

    /*
     * note - blocking signals isn't necessarily the
     * right thing, but we leave it for now.
     */
    omask = sigblock(sigmask(SIGINT)|sigmask(SIGTSTP));
    (void)tcgetattr(fileno(fp), &term);
    if ((echo = term.c_lflag) & ECHO) {
        term.c_lflag &= ~(ECHO|ICANON);
        (void)tcsetattr(fileno(fp), TCSAFLUSH|TCSASOFT, &term);
    }
    (void)fputs(prompt, outfp);
    rewind(outfp);                  /* implied flush */
    for (s = buf; (ch = getc(fp)) != EOF && ch != '\n' && ch != '\r';)
        if (s < buf + _PASSWORD_LEN)
            *s++ = ch;
    *s = '\0';
    write(fileno(outfp), "\n", 1);
    if (echo & ECHO) {
        term.c_lflag = echo;
        tcsetattr(fileno(fp), TCSAFLUSH|TCSASOFT, &term);
    }
    sigsetmask(omask);
    return buf;
}
