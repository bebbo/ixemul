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
#include "kprintf.h"

extern struct ExecBase	*SysBase;

void
ixnet_close (struct ixnet_base *ixbase)
{
    usetup;
    struct ixnet *p = (struct ixnet *)u.u_ixnet;

    if (p->u_SockBase) {
        SOCK_cleanup_sockets();
	FreeSignal(p->u_sigurg);
	FreeSignal(p->u_sigio);
	CloseLibrary(p->u_SockBase);
    }

    if (p->u_UserGroupBase) {
	CloseLibrary(p->u_UserGroupBase);
    }

    if (p->u_TCPBase) {
	FreeSignal(p->u_sigurg);
	FreeSignal(p->u_sigio);
	CloseLibrary(p->u_TCPBase);
    }

    FreeMem(p, sizeof(struct ixnet));
}
