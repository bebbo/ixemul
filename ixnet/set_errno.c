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
 *  $Id: set_errno.c,v 1.2 2005/03/19 21:24:00 piru Exp $
 *
 *  $Log: set_errno.c,v $
 *  Revision 1.2  2005/03/19 21:24:00  piru
 *  2005-03-02  Harry Sintonen <sintonen@iki.fi>
 *  	* ixnet: fixed h_errno variable, it was never set before.
 *  	* Bumped version to 49.18.
 *
 *  Revision 1.1.1.1  2000/05/07 19:37:46  emm
 *  Imported sources
 *
 *  Revision 1.1.1.1  2000/04/29 00:44:40  nobody
 *  Initial import
 *
 *
 */

/* routines that need -ffixed-a4 compiled for them */
#define _KERNEL
#include "ixnet.h"
#include <amitcp/socketbasetags.h>
#include <amitcp/usergroup.h>

void set_errno(int *real_errno, int *real_h_errno)
{
  usetup;
  struct ixnet *p = (struct ixnet *)u.u_ixnet;

  if (p->u_networkprotocol == IX_NETWORK_AMITCP) {
    if (real_errno) {
      struct TagItem ug_list[] = {
        {UGT_INTRMASK, SIGBREAKB_CTRL_C},
        {UGT_ERRNOPTR(sizeof(int)), (ULONG)real_errno},
        {TAG_END}
      };

      TCP_SetErrnoPtr(real_errno,sizeof(*real_errno));
      ug_SetupContextTagList(p->u_progname, ug_list);
    }

    if (real_h_errno) {
      struct TagItem tcp_list[] = {
        { SBTM_SETVAL(SBTC_HERRNOLONGPTR), (ULONG)real_h_errno },
        { TAG_END }
      };

      TCP_SocketBaseTagList(tcp_list);
    }
  }
  else /*if (p->u_networkprotocol == IX_NETWORK_AS225) */ {
    if (real_errno) {
      SOCK_setup_sockets(128,real_errno);
    }
  }
}
