/*
 * Copyright (c) 1983 Regents of the University of California.
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
static char sccsid[] = "@(#)getservent.c        5.9 (Berkeley) 2/24/91";
#endif /* LIBC_SCCS and not lint */

#define _KERNEL
#include "ixnet.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAXALIASES      35

void
setservent(int f) 
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;

    if (network_protocol == IX_NETWORK_AS225) {
	SOCK_setservent(f);
    }
    else /* if (network_protocol == IX_NETWORK_AMITCP) */ {
	if (u.u_serv_fp == NULL) {
	    u.u_serv_fp = fopen(_TCP_PATH_SERVICES, "r" );
	    if (!u.u_serv_fp)
		return;
	}
	else
	    rewind(u.u_serv_fp);
	u.u_serv_stayopen |= f;
    }
}

void
endservent(void)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    if (p->u_networkprotocol == IX_NETWORK_AS225) {
	SOCK_endservent();
    }
    else {
	if (u.u_serv_fp) {
	    fclose(u.u_serv_fp);
	    u.u_serv_fp = NULL;
	}
	u.u_serv_stayopen = 0;
    }
}

struct servent *
getservent(void)
{
	usetup;
	char *s;
	register char *cp, **q;
	register struct ixnet *p = (struct ixnet *)u.u_ixnet;
	register int network_protocol = p->u_networkprotocol;

	if (network_protocol == IX_NETWORK_AS225) {
	    return SOCK_getservent();
	}
	else /*if (network_protocol == IX_NETWORK_AMITCP)*/ {
	    if (u.u_serv_line == NULL)
	      u.u_serv_line = malloc(BUFSIZ + 1);
	    if (u.u_serv_aliases == NULL)
	      u.u_serv_aliases = malloc(MAXALIASES * sizeof(char *));
	    if (u.u_serv_line == NULL || u.u_serv_aliases == NULL)
	      {
		errno = ENOMEM;
		return NULL;
	      }
	    if (u.u_serv_fp == NULL && (u.u_serv_fp = fopen(_TCP_PATH_SERVICES, "r" )) == NULL)
		return (NULL);
again:
	    if ((s = fgets(u.u_serv_line, BUFSIZ, u.u_serv_fp)) == NULL)
		return (NULL);

	    if (*s == '#')
		goto again;

	    cp = strpbrk(s, "#\n");
	    if (cp == NULL)
		goto again;

	    *cp = '\0';
	    u.u_serv.s_name = s;
	    s = strpbrk(s, " \t");
	    if (s == NULL)
		goto again;

	    *s++ = '\0';
	    while (*s == ' ' || *s == '\t')
		s++;

	    cp = strpbrk(s, ",/");
	    if (cp == NULL)
		goto again;

	    *cp++ = '\0';
	    u.u_serv.s_port = htons((u_short)atoi(s));
	    u.u_serv.s_proto = cp;
	    q = u.u_serv.s_aliases = u.u_serv_aliases;
	    cp = strpbrk(cp, " \t");
	    if (cp != NULL)
		*cp++ = '\0';

	    while (cp && *cp) {
		if (*cp == ' ' || *cp == '\t') {
		    cp++;
		continue;
	    }
	    if (q < &u.u_serv_aliases[MAXALIASES - 1])
		*q++ = cp;

	    cp = strpbrk(cp, " \t");
	    if (cp != NULL)
		*cp++ = '\0';
	}
	*q = NULL;
	return (&u.u_serv);
    }
}
