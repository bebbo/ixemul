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
static char sccsid[] = "@(#)getnetent.c 5.8 (Berkeley) 2/24/91";
#endif /* LIBC_SCCS and not lint */

#define _KERNEL
#include "ixnet.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>

#define MAXALIASES      35

static FILE *netf;
static struct netent net;
static char *net_aliases[MAXALIASES];
int _net_stayopen;

void
setnetent(int f)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;

    if (network_protocol == IX_NETWORK_AS225) {
        SOCK_setnetent(f);
        return;
    }
    else /* if (network_protocol == IX_NETWORK_AMITCP) */ {
        if (netf == NULL)
            netf = fopen(_TCP_PATH_NETWORKS, "r" );
        else
            rewind(netf);
        _net_stayopen |= f;
    }
}

void
endnetent(void)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;

    if (p->u_networkprotocol == IX_NETWORK_AS225) {
        SOCK_endnetent();
        return;
    }

    if (netf) {
        fclose(netf);
        netf = NULL;
    }
    _net_stayopen = 0;
}

struct netent *
getnetent(void)
{
    usetup;
    char *s;
    register char *cp, **q;
    static char line[BUFSIZ+1];
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;

    if (network_protocol == IX_NETWORK_AS225) {
        return SOCK_getnetent();
    }
    else /* if (network_protocol == IX_NETWORK_AMITCP) */ {
        if (netf == NULL && (netf = fopen(_TCP_PATH_NETWORKS, "r" )) == NULL)
            return (NULL);
again:
        s = fgets(line, BUFSIZ, netf);
        if (s == NULL)
            return (NULL);
        if (*s == '#')
            goto again;
        cp = strpbrk(s, "#\n");
        if (cp == NULL)
            goto again;
        *cp = '\0';
        net.n_name = s;
        cp = strpbrk(s, " \t");
        if (cp == NULL)
            goto again;
        *cp++ = '\0';
        while (*cp == ' ' || *cp == '\t')
            cp++;
        s = strpbrk(cp, " \t");
        if (s != NULL)
            *s++ = '\0';
        net.n_net = inet_network(cp);
        net.n_addrtype = AF_INET;
        q = net.n_aliases = net_aliases;
        if (s != NULL)
            cp = s;
        while (cp && *cp) {
            if (*cp == ' ' || *cp == '\t') {
                cp++;
                continue;
            }
            if (q < &net_aliases[MAXALIASES - 1])
                *q++ = cp;
            cp = strpbrk(cp, " \t");
            if (cp != NULL)
                *cp++ = '\0';
        }
        *q = NULL;
        return (&net);
    }
}
