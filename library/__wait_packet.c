/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1991, 1992  Markus M. Wild
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
 *  __wait_packet.c,v 1.1.1.1 1994/04/04 04:30:14 amiga Exp
 *
 *  __wait_packet.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:14  amiga
 * Initial CVS check in.
 *
 *  Revision 1.3  1992/07/04  19:08:41  mwild
 *  change to new ix_sleep format, add wmesg-string
 *
 * Revision 1.2  1992/05/18  11:58:15  mwild
 * use dp_Port field, NT_REPLYMSG is not reliable
 *
 * Revision 1.1  1992/05/14  19:55:40  mwild
 * Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"
#include <signal.h>


/* I'm using dp_Port as an indicator whether this packet is
   free. If in use, this field can't be 0. */
#define PACKET_IN_USE(sp) (sp->sp_Pkt.dp_Port)

void
__wait_sync_packet(struct StandardPacket *sp)
{
  struct StandardPacket *prw;
  int omask;
  usetup;

  /* this is the synchronous way of dealing with packets that may
   * arrive at a port. */

  if (! PACKET_IN_USE (sp)) return;

  omask = syscall (SYS_sigsetmask, ~0);

  for (;;)
    {
      if ((prw = GetPacket(u.u_sync_mp)))
        {
	  PACKET_IN_USE (prw) = 0;
          if (prw == sp) break;
        }
      else
        {
	  Wait(1<<u.u_sync_mp->mp_SigBit);
	}
    }
  
  syscall (SYS_sigsetmask, omask);
}

void
__wait_select_packet(struct StandardPacket *sp)
{
  struct StandardPacket *prw;
  int omask;
  usetup;

  /* this is the synchronous way of dealing with packets that may
   * arrive at a port. */

  if (! PACKET_IN_USE (sp)) return;

  omask = syscall (SYS_sigsetmask, ~0);

  for (;;)
    {
      if ((prw = GetPacket(u.u_select_mp)))
        {
	  PACKET_IN_USE (prw) = 0;
          if (prw == sp) break;
        }
      else
        {
	  Wait(1<<u.u_select_mp->mp_SigBit);
	}
    }
  
  syscall (SYS_sigsetmask, omask);
}

