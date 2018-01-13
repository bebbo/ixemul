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
 */

#define _KERNEL
#include "ixemul.h"

int __unlock (BPTR lock)
{
  UnLock(lock);
  return 0;
}

/* ATTENTION: name-clash between sysio-__close and DOS-__Close !! */
void __Close (BPTR fh)
{
  struct FileHandle *fhp = BTOCPTR (fh);
  
  /* only do this if not closing a dummy NIL: filehandle */
  usetup;
  struct StandardPacket *sp;

  if (fhp->fh_Type)
    {
      sp = alloca(sizeof(*sp)+2);
      sp = LONG_ALIGN (sp);
      __init_std_packet(sp);

      sp->sp_Pkt.dp_Port = u.u_sync_mp;
      sp->sp_Pkt.dp_Type = ACTION_END;
      sp->sp_Pkt.dp_Arg1 = fhp->fh_Arg1;

      PutPacket (fhp->fh_Type, sp);
      __wait_sync_packet (sp);

      SetIoErr(sp->sp_Pkt.dp_Res2);
    }
  else
    SetIoErr(0);
  FreeDosObject (DOS_FILEHANDLE, fhp);
}
