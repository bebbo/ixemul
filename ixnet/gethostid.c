/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1995 Jeff Shepherd
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
 */
#define _KERNEL
#include "ixnet.h"
#include <netdb.h>
#include <unistd.h>

long gethostid(void)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;

    if (network_protocol == IX_NETWORK_AMITCP)
        return (long)TCP_GetHostId();
    else /* if (network_protocol == IX_NETWORK_AS225) */ {
        char hostname[MAXPATHLEN];
        struct hostent *ht;
        gethostname(hostname,MAXPATHLEN);
        ht = gethostbyname(hostname);
        if (ht) {
            return (int)ht->h_addr;
        }
        return 0;
    }
}

