/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1996 Jeff Shepherd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  $Id:$
 *
 *  $Log:$
 */


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

/* interface with ixnet.library - just call the library (if its open) */
/* sometimes do some local things if ixnet.library isn't open */
#define _KERNEL
#include "ixemul.h"
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

struct hostent *
gethostbyname(const char *name)
{
  usetup;

  if (u.u_ixnetbase)
    return (struct hostent *)netcall(NET_gethostbyname, name);
  return NULL;
}

struct hostent *
gethostbyaddr(const char *addr, int len, int type)
{
  usetup;

  if (u.u_ixnetbase)
    return (struct hostent *)netcall(NET_gethostbyaddr, addr, len, type);
  return NULL;
}


void
sethostent(int stayopen)
{
  usetup;

  if (u.u_ixnetbase)
    netcall(NET_sethostent, stayopen);
}

void
endhostent(void)
{
  usetup;

  if (u.u_ixnetbase)
    netcall(NET_endhostent);
}

struct netent *
getnetbyname(const char *name)
{
  usetup;

  if (u.u_ixnetbase)
    return (struct netent *)netcall(NET_getnetbyname, name);
  return NULL;
}

struct netent *
getnetbyaddr(long net, int type)
{
  usetup;

  if (u.u_ixnetbase)
    return (struct netent *)netcall(NET_getnetbyaddr, net, type);
  return NULL;
}

void
setnetent(int f)
{
  usetup;

  if (u.u_ixnetbase)
    netcall(NET_setnetent, f);
}


void
endnetent(void)
{
  usetup;

  if (u.u_ixnetbase)
    netcall(NET_endnetent);
}

struct netent *
getnetent(void)
{
  usetup;

  if (u.u_ixnetbase)
    return (struct netent *)netcall(NET_getnetent);
  return NULL;
}

#define MAXALIASES 35
#include <stdio.h>
#include <netdb.h>

void
setservent(int f)
{
  usetup;

  if (u.u_ixnetbase) {
    netcall(NET_setservent, f);
  }
  else {
    if (u.u_serv_fp == NULL) {
      u.u_serv_fp = fopen(_PATH_SERVICES, "r" );
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

  if (u.u_ixnetbase) {
    netcall(NET_endservent);
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

  if (u.u_ixnetbase) {
    return (struct servent *)netcall(NET_getservent);
  }
  else {
    char *p;
    register char *cp, **q;

    if (u.u_serv_line == NULL)
      u.u_serv_line = malloc(BUFSIZ + 1);
    if (u.u_serv_aliases == NULL)
      u.u_serv_aliases = malloc(MAXALIASES * sizeof(char *));
    if (u.u_serv_line == NULL || u.u_serv_aliases == NULL)
      {
        errno = ENOMEM;
        return NULL;
      }
    if (u.u_serv_fp == NULL && (u.u_serv_fp = fopen(_PATH_SERVICES, "r" )) == NULL) {
      return NULL;
    }
again1:
    if ((p = fgets(u.u_serv_line, BUFSIZ, u.u_serv_fp)) == NULL)
      return NULL;
    if (*p == '#')
      goto again1;

    cp = strpbrk(p, "#\n");
    if (cp == NULL)
      goto again1;

    *cp = '\0';
    u.u_serv.s_name = p;
    p = strpbrk(p, " \t");
    if (p == NULL)
      goto again1;

    *p++ = '\0';
    while (*p == ' ' || *p == '\t')
      p++;

    cp = strpbrk(p, ",/");
    if (cp == NULL)
      goto again1;

    *cp++ = '\0';
    u.u_serv.s_port = htons((u_short)atoi(p));
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

struct servent *
getservbyname(const char *name, const char *proto)
{
  usetup;

  if (u.u_ixnetbase) {
    return (struct servent *)netcall(NET_getservbyname, name, proto);
  }
  else {
    register struct servent *p;
    register char **cp;

    setservent(u.u_serv_stayopen);
    while ((p = getservent())) {
      if (strcmp(name, p->s_name) == 0)
        goto gotservname;

      for (cp = p->s_aliases; *cp; cp++)
        if (strcmp(name, *cp) == 0)
          goto gotservname;

        continue;
gotservname:
      if (proto == 0 || strcmp(p->s_proto, proto) == 0)
        break;
    }
    if (!u.u_serv_stayopen)
    endservent();

    return (p);
  }
}

struct servent *
getservbyport(int port, const char * proto)
{
    usetup;

    if (u.u_ixnetbase) {
	return (struct servent *)netcall(NET_getservbyport, port,proto);
    }
    else {
	register struct servent *p;

	setservent(u.u_serv_stayopen);
	while ((p = getservent())) {
	    if (p->s_port != port)
		continue;

	    if (proto == 0 || strcmp(p->s_proto, proto) == 0)
		break;
	}
	if (!u.u_serv_stayopen)
	    endservent();

	return (p);
    }
}

long gethostid(void)
{
  usetup;

  if (u.u_ixnetbase)
    return netcall(NET_gethostid);
  return 0xaabbccdd;
}

void
herror(const char *s)
{
  usetup;

  if (u.u_ixnetbase)
    netcall(NET_herror, s);
}

char *
hstrerror(int err)
{
  usetup;

  if (u.u_ixnetbase)
    return (char *)netcall(NET_hstrerror, err);
  return ("Unknown resolver error");
}


struct protoent *
getprotobynumber(int proto)
{
    usetup;

    if (u.u_ixnetbase) {
	return (struct protoent *)netcall(NET_getprotobynumber, proto);
    }
    else {
	register struct protoent *p;

	setprotoent(u.u_proto_stayopen);
	while ((p = getprotoent()))
	    if (p->p_proto == proto)
		break;

	if (!u.u_proto_stayopen)
	    endprotoent();
	return (p);
    }
}

struct protoent *
getprotobyname(const char *name)
{
    usetup;

    if (u.u_ixnetbase) {
        return (struct protoent *)netcall(NET_getprotobyname, name);
    }
    else {
	register struct protoent *p;
	register char **cp;

	setprotoent(u.u_proto_stayopen);
	while ((p = getprotoent())) {
	    if (strcasecmp(p->p_name, name) == 0)
		break;
	    for (cp = p->p_aliases; *cp != 0; cp++)
		if (strcasecmp(*cp, name) == 0)
		    goto found;
	}
found:
	if (!u.u_proto_stayopen)
	    endprotoent();
	return (p);
    }
}

void
setprotoent(int f)
{
    usetup;

    if (u.u_ixnetbase) {
	netcall(NET_setprotoent, f);
    }
    else {
	if (u.u_proto_fp == NULL)
	    u.u_proto_fp = fopen(_PATH_PROTOCOLS, "r" );
	else
	    rewind(u.u_proto_fp);
	u.u_proto_stayopen |= f;
    }
}

void
endprotoent(void)
{
    usetup;

    if (u.u_ixnetbase) {
	netcall(NET_endprotoent);
    }
    else {
	if (u.u_proto_fp) {
	    fclose(u.u_proto_fp);
	    u.u_proto_fp = NULL;
	}
	u.u_proto_stayopen = 0;
    }
}

struct protoent *
getprotoent(void)
{
    usetup;

    if (u.u_ixnetbase) {
	return (struct protoent *)netcall(NET_getprotoent);
    }
    else {
	char *p;
	register char *cp, **q;

        if (u.u_proto_line == NULL)
          u.u_proto_line = malloc(BUFSIZ + 1);
        if (u.u_proto_aliases == NULL)
          u.u_proto_aliases = malloc(MAXALIASES * sizeof(char *));
        if (u.u_proto_line == NULL || u.u_proto_aliases == NULL)
          {
            errno = ENOMEM;
            return NULL;
          }
	if (u.u_proto_fp == NULL && (u.u_proto_fp = fopen(_PATH_PROTOCOLS, "r" )) == NULL) {
		return (NULL);
	}
again3:

	if ((p = fgets(u.u_proto_line, BUFSIZ, u.u_proto_fp)) == NULL)
	    return (NULL);

	if (*p == '#')
	    goto again3;

	cp = strpbrk(p, "#\n");
	if (cp == NULL)
	    goto again3;

	*cp = '\0';
	u.u_proto.p_name = p;
	cp = strpbrk(p, " \t");
	if (cp == NULL)
	    goto again3;

	*cp++ = '\0';
	while (*cp == ' ' || *cp == '\t')
	    cp++;

	p = strpbrk(cp, " \t");
	if (p != NULL)
	    *p++ = '\0';

	u.u_proto.p_proto = atoi(cp);
	q = u.u_proto.p_aliases = u.u_proto_aliases;
	if (p != NULL) {
	    cp = p;
	    while (cp && *cp) {
		if (*cp == ' ' || *cp == '\t') {
		    cp++;
		    continue;
		}

		if (q < &u.u_proto_aliases[MAXALIASES - 1])
		    *q++ = cp;

		cp = strpbrk(cp, " \t");
		if (cp != NULL)
		    *cp++ = '\0';
	    }
	}
	*q = NULL;
	return (&u.u_proto);
    }
}

int
getdomainname (char *name, int namelen)
{
  usetup;

  if (u.u_ixnetbase)
    return netcall(NET_getdomainname, name, namelen);

  name[0] = 0;  /* anyone have a better suggestion? */
  return 0;
}
