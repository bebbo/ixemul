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
#include "ixnet.h"
#include "ixprotos.h"
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <exec/execbase.h>

extern struct ExecBase *SysBase;

/* called from ixemul.library's siglaunch() */
void _siglaunch(sigset_t newsigs)
{
    struct Task *me = FindTask(0);
    struct user *u_ptr = getuser(me);
    struct ixnet *p = u.u_ixnet;

    int urgmask = 1 << p->u_sigurg;
    int iomask	= 1 << p->u_sigio;

    if (newsigs & urgmask) {
	if (! (u.p_sigignore & sigmask (SIGURG)))
	    _psignal (me, SIGURG);

	me->tc_SigRecvd &= ~urgmask;
	u.u_lastrcvsig &= ~urgmask;
    }

    if (newsigs & iomask) {
	if (! (u.p_sigignore & sigmask (SIGIO)))
	    _psignal (me, SIGIO);

	me->tc_SigRecvd &= ~iomask;
	u.u_lastrcvsig &= ~iomask;
    }
}

/* I found this code from the AmiTCP sdk. It *should* work with AS225 */
int
_fstat(struct file *fp, struct stat *sb)
{
    long value;
    int size = sizeof(value);

    bzero(sb, sizeof(*sb));

    /* st->st_dev = ??? */
    sb->st_mode = S_IFSOCK | S_IRUSR | S_IWUSR;
    sb->st_uid = geteuid();
    sb->st_gid = getegid();

    if (_getsockopt(fp, SOL_SOCKET, SO_SNDBUF,(void *)&value,&size) == 0)
	sb->st_blksize = value;
    return 0;
}
