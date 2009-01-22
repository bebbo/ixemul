/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1991, 1992  Markus M. Wild
 *  Portions Copyright (C) 1994 Rafael W. Luebbert
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
 *  $Id: ix_resident.c,v 1.5 1994/06/19 19:30:31 rluebbert Exp $
 *
 *  $Log: ix_resident.c,v $
 *  Revision 1.5  1994/06/19  19:30:31  rluebbert
 *  Optimization of subroutines
 *
 *  Revision 1.4  1994/06/19  15:13:14  rluebbert
 *  *** empty log message ***
 *
 *  Revision 1.2  1992/09/14  01:42:52  mwild
 *  if out of memory, exit. There's no reason to continue, the programs would
 *  just plain crash...
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"
#include <sys/exec.h>


void
ix_resident (int numpar, int a4, int databss_size, long *relocs)
{
  usetup;

  if ((numpar == 3 || numpar == 4) && databss_size)
    {
      int mem = (int)kmalloc (databss_size);
      if (! mem)
        {
	  ix_panic ("Out of memory.");
	  _exit (20);
	}
      else
	{
	  int origmem = a4 - 0x7ffe;

	  memcpy((void *)mem, (void *)origmem, databss_size);

	  if (numpar == 4 && relocs[0] > 0)
	    {
	      int i, num_rel = relocs[0];

	      for (i = 0, relocs++; i < num_rel; i++, relocs++)
	          *(long *)(mem + *relocs) -= origmem - mem;
	    }

	  if (u.p_flag & SFREEA4)
	    kfree ((void *)origmem);
	  else
	    u.p_flag |= SFREEA4;

	  a4 = mem + 0x7ffe;
	}
    }

  
  if (numpar >= 2 && numpar <= 4)
    {
      /* need output parameter, or the asm is optimized away */
      asm volatile ("movel %0, a4" : "=g" (a4) : "0" (a4));
      
      u.u_a4 = a4;
    }
  else
    ix_warning("Unsupported ix_resident call.");
}

void ix_geta4 (void)
{
  usetup;

  if (u.u_a4) {
    asm volatile ("movel %0, a4" : "=g" (u.u_a4) : "0" (u.u_a4));
  }
}

/* This function isn't used at the moment, but it is meant to be called
 * at the start of an executable. This function should check if the
 * executable can run on the CPU of this computer.
 */
void ix_check_cpu (int machtype)
{
#if notyet
  if (machtype == MID_SUN020)
    {
      if (!(SysBase->AttnFlags & AFF_68020))
	{
	  ix_panic ("This program requires at least a 68020 cpu.");
	  syscall (SYS_exit, 20);
	}
    }
#endif
}
