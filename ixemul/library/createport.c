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
 *  $Id: createport.c,v 1.1.1.1 2005/03/15 15:57:09 laire Exp $
 *
 *  $Log: createport.c,v $
 *  Revision 1.1.1.1  2005/03/15 15:57:09  laire
 *  a new beginning
 *
 *  Revision 1.4  2002/06/14 21:51:09  emm
 *  Compiles again, but unstable.
 *
 *  Revision 1.3  2002/06/14 17:23:30  laire
 *  removed __pos__. Needs a test(i couldn^t)
 *
 *  Revision 1.2  2000/06/20 22:17:19  emm
 *  First attempt at a native MorphOS ixemul
 *
 *  Revision 1.1.1.1  2000/05/07 19:38:01  emm
 *  Imported sources
 *
 *  Revision 1.1.1.1  2000/04/29 00:49:02  nobody
 *  Initial import
 *
 *  Revision 1.2  1994/06/19  15:09:29  rluebbert
 *  *** empty log message ***
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#include <exec/ports.h>

#ifdef TRACK_ALLOCS
#undef AllocMem
#define AllocMem(x,y) debug_AllocMem("create_port",x,y)
void *debug_AllocMem(const char *,int, int);
#endif

#if 0 //def NATIVE_MORPHOS

#include <exec/memory.h>

#define NEWLIST(l) ((l)->lh_Head = (struct Node *)&(l)->lh_Tail, \
		    /*(l)->lh_Tail = NULL,*/ \
		    (l)->lh_TailPred = (struct Node *)&(l)->lh_Head)

struct MsgPort *CreatePort(STRPTR name,LONG pri)
{ struct MsgPort *port = NULL;
  UBYTE portsig;

  if ((BYTE)(portsig=AllocSignal(-1)) >= 0)
  { if (!(port=AllocMem(sizeof(struct MsgPort),MEMF_CLEAR|MEMF_PUBLIC)))
      FreeSignal(portsig);
    else
    {
      port->mp_Node.ln_Type=NT_MSGPORT;
      port->mp_Node.ln_Pri=pri;
      port->mp_Node.ln_Name=name;
      port->mp_Flags=PA_SIGNAL;
      port->mp_SigBit=portsig;
      port->mp_SigTask=SysBase->ThisTask;
      NEWLIST(&port->mp_MsgList);
      if (port->mp_Node.ln_Name)
	AddPort(port);
    }
  }
  return port;
}

#endif

struct MsgPort *
ix_create_port(unsigned char *name, long pri)
{
  struct MsgPort *port = CreatePort(name, pri);
  usetup;

  if (!port)
    {
      errno = ENOMEM;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return 0;
    }

  return port;
}
