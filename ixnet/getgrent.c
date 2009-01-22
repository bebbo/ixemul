/*
 * Copyright (c) 1989 The Regents of the University of California.
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
static char sccsid[] = "@(#)getgrent.c  5.9 (Berkeley) 4/1/91";
#endif /* LIBC_SCCS and not lint */

#define _KERNEL
#include "ixnet.h"
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <grp.h>

/* for AS225 */
#undef _PATH_GROUP
#define _PATH_GROUP "/inet/db/group"

static int grscan(), start_gr();

#define MAXGRP          200
#define MAXLINELENGTH   1024

struct group *
getgrent(void)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;

    if (p->u_networkprotocol == IX_NETWORK_AMITCP) {
	return UG_getgrent();
    }
    if ((!u.u_grp_fp && !start_gr()) || !grscan(0, 0, NULL))
	return(NULL);
    return(&u.u_group);
}

struct group *
getgrnam(const char *name)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;

    if (p->u_networkprotocol == IX_NETWORK_AMITCP) {
	struct group *err = UG_getgrnam(name);

	if (!err) {
	    errno = ug_GetErr();
	}
	return err;
    }
    else {
	int rval;

	if (!start_gr())
	    return(NULL);

	rval = grscan(1, 0, name);
	if (!u.u_grp_stayopen)
	    endgrent();

	return(rval ? &u.u_group : NULL);
    }
}

struct group *
getgrgid(gid_t gid)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;

    if (p->u_networkprotocol == IX_NETWORK_AMITCP) {
	struct group *err;
	err = UG_getgrgid(gid);

	if (!err) {
	    errno = ug_GetErr();
	}
	return err;
    }
    else {
	int rval;

	if (!start_gr())
	    return(NULL);

	rval = grscan(1, gid, NULL);
	if (!u.u_grp_stayopen)
		endgrent();

	return(rval ? &u.u_group : NULL);
    }
}

static int
start_gr(void)
{
    usetup;

    if (u.u_grp_fp) {
	rewind(u.u_grp_fp);
	return(1);
    }
    return((u.u_grp_fp = fopen(_PATH_GROUP, "r")) ? 1 : 0);
}

int
setgrent(void)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;

    if (p->u_networkprotocol == IX_NETWORK_AMITCP) {
	UG_setgrent();
	return 1;
    }
    return setgroupent(0);
}

int
setgroupent(int stayopen)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;

    if (p->u_networkprotocol == IX_NETWORK_AMITCP) {
	UG_setgrent();
	return 1;
    }

    if (!start_gr())
      return(0);
    u.u_grp_stayopen = stayopen;
    return 1;
}

void
endgrent(void)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;

    if (p->u_networkprotocol == IX_NETWORK_AMITCP) {
	UG_endgrent();
    }
    if (u.u_grp_fp) {
	(void)fclose(u.u_grp_fp);
	u.u_grp_fp = NULL;
    }
}

static int
grscan(int search, int gid, char *name)
{
    usetup;
    register char *cp, **m;
    char *bp;
    char *fgets(), *strsep(), *index();

    if (u.u_grp_line == NULL)
      u.u_grp_line = malloc(MAXLINELENGTH + 1);
    if (u.u_members == NULL)
      u.u_members = malloc(MAXGRP * sizeof(char *));
    if (u.u_grp_line == NULL || u.u_members == NULL)
      {
	errno = ENOMEM;
	return 0;
      }
    for (;;) {
	if (!fgets(u.u_grp_line, MAXLINELENGTH, u.u_grp_fp))
	    return(0);
	bp = u.u_grp_line;
	/* skip lines that are too big */
	if (!index(u.u_grp_line, '\n')) {
	    int ch;

	    while ((ch = getc(u.u_grp_fp)) != '\n' && ch != EOF)
		;

	    continue;
	}
	u.u_group.gr_name = strsep(&bp, "|\n");
	if (search && name && strcmp(u.u_group.gr_name, name))
	    continue;

	u.u_group.gr_passwd = strsep(&bp, "|\n");
	if (!(cp = strsep(&bp, "|\n")))
	    continue;

	u.u_group.gr_gid = atoi(cp);
	if (search && name == NULL && u.u_group.gr_gid != gid)
	    continue;

	for (m = u.u_group.gr_mem = u.u_members;; ++m) {
	    if (m == &u.u_members[MAXGRP - 1]) {
		*m = NULL;
		break;
	    }

	    if ((*m = strsep(&bp, ", \n")) == NULL)
		break;
	}
	return(1);
    }
    /* NOTREACHED */
}


int
setgroups (int ngroups, const int *gidset)
{
  usetup;
  struct ixnet *p = u.u_ixnet;

  if (p->u_networkprotocol == IX_NETWORK_AMITCP)
    return UG_setgroups(ngroups,gidset);
  return (ngroups >= 1) ? 0 : -1;
}

