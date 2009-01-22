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
 *  deleteport.c,v 1.1.1.1 1994/04/04 04:30:46 amiga Exp
 *
 *  deleteport.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:46  amiga
 * Initial CVS check in.
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include <exec/ports.h>

#ifdef TRACK_MALLOCS
#undef FreeMem
#define FreeMem(x,y) debug_FreeMem(x,y)
void debug_FreeMem(int,int);
#endif

#if 0 //def NATIVE_MORPHOS

void DeletePort(struct MsgPort *port)
{ int i;

  if (port->mp_Node.ln_Name != NULL)
    RemPort(port);
  i=-1;
  port->mp_Node.ln_Type=i;
  port->mp_MsgList.lh_Head=(struct Node *)i;
  FreeSignal(port->mp_SigBit);
  FreeMem(port,sizeof(struct MsgPort));
}

#endif

void
ix_delete_port (struct MsgPort *port)
{
  DeletePort(port);
}
