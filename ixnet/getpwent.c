/*
 *  This file is part of ixnet.library for the Amiga.
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
 *
 */

#define _KERNEL
#include <pwd.h>
#include "ixnet.h"

#include <fcntl.h>
#include <utmp.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <time.h>

/* prototypes */
static struct passwd *__TCP2InetPwd(struct TCP_passwd *pwd);
static struct passwd *__AS225InetPwd(struct AS225_passwd *pwd);

#undef errno
int errno;

struct passwd *
getpwent(void)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;

    if (network_protocol == IX_NETWORK_AMITCP) {
	struct TCP_passwd *err = UG_getpwent();

	if (err == NULL) {
	    *u.u_errno = ug_GetErr();
	    return NULL;
	}
	else {
	    return __TCP2InetPwd(err);
	}
    }
    else /*if (network_protocol == IX_NETWORK_AS225)*/ {
	struct AS225_passwd *pwd = SOCK_getpwent();

	return (pwd ? __AS225InetPwd(pwd) : NULL);
    }
}

struct passwd *
getpwnam(const char *name)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;

    if (network_protocol == IX_NETWORK_AMITCP) {
	struct TCP_passwd *err = UG_getpwnam(name);

	if (err == NULL) {
	    *u.u_errno = ug_GetErr();
	    return NULL;
	}
	return __TCP2InetPwd(err);
    }
    else /* if (network_protocol == IX_NETWORK_AS225)*/ {
	struct AS225_passwd *pwd = SOCK_getpwnam((char *)name);

	return (pwd ? __AS225InetPwd(pwd) : NULL);
    }
}

struct passwd *
getpwuid(uid_t uid)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;

    /* Don't do this if uid == -2 (nobody2) */
    /* This happens when someone doesn't use AmiTCP's login */
    if (network_protocol == IX_NETWORK_AMITCP) {
	if (uid != (uid_t)-2) {
	    struct TCP_passwd *err = UG_getpwuid(uid);

	    if (err == NULL) {
		*u.u_errno = ug_GetErr();
		return NULL;
	    }
	    return __TCP2InetPwd(err);
	}
	else {
	    return getpwnam(getenv("USER"));
	}
    }
    else /*if (network_protocol == IX_NETWORK_AS225)*/  {
        struct AS225_passwd *pwd = SOCK_getpwuid(uid);

	return (pwd ? __AS225InetPwd(pwd) : NULL);
    }
}

int
setpassent(int stayopen)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;

    if (network_protocol == IX_NETWORK_AMITCP) {
	UG_setpwent();
    }
    else /* if (network_protocol == IX_NETWORK_AS225) */ {
	SOCK_setpwent(0);
    }
    return 1;
}

int
setpwent(void)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;

    if (network_protocol == IX_NETWORK_AMITCP) {
	UG_setpwent();
    }
    else /* if (network_protocol == IX_NETWORK_AS225) */ {
	SOCK_setpwent(0);
    }
    return 1;
}

void
endpwent(void)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;

    if (network_protocol == IX_NETWORK_AMITCP) {
	UG_endpwent();
    }
    else /* if (network_protocol == IX_NETWORK_AS225)*/ {
	SOCK_endpwent();
    }
}

/* change the AmiTCP password structure to the global format */
static struct passwd *__TCP2InetPwd(struct TCP_passwd *pwd)
{
    usetup;
    u.u_passwd.pw_name = pwd->pw_name;
    u.u_passwd.pw_passwd = pwd->pw_passwd;
    u.u_passwd.pw_uid = pwd->pw_uid;
    u.u_passwd.pw_gid = pwd->pw_gid;
    u.u_passwd.pw_change = time((time_t *)NULL);
    u.u_passwd.pw_class = NULL;
    u.u_passwd.pw_gecos = pwd->pw_gecos;
    u.u_passwd.pw_dir = pwd->pw_dir;
    u.u_passwd.pw_shell = pwd->pw_shell;
    u.u_passwd.pw_expire = (time_t)-1;
    return &u.u_passwd;
}

/* change the AS225/INet225's password structure to the global format */
static struct passwd *__AS225InetPwd(struct AS225_passwd *pwd)
{
    usetup;
    u.u_passwd.pw_name = pwd->pw_name;
    u.u_passwd.pw_passwd = pwd->pw_passwd;
    u.u_passwd.pw_uid = pwd->pw_uid;
    u.u_passwd.pw_gid = pwd->pw_gid;
    u.u_passwd.pw_change = time((time_t *)NULL);
    u.u_passwd.pw_class = NULL;
    u.u_passwd.pw_gecos = pwd->pw_gecos;
    u.u_passwd.pw_dir = pwd->pw_dir;
    u.u_passwd.pw_shell = pwd->pw_shell;
    u.u_passwd.pw_expire = (time_t)-1;
    u.u_passwd.pw_change = (time_t)-1;
    return &u.u_passwd;
}

#define errno (* u.u_errno)
