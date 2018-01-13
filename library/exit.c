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
 *  exit.c,v 1.1.1.1 1994/04/04 04:30:48 amiga Exp
 *
 *  exit.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:48  amiga
 * Initial CVS check in.
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#include <stdlib.h>
#include <sys/wait.h>

#include "../stdlib/atexit.h"

#define __atexit (u.u_atexit)  /* points to head of LIFO stack */

void exit2(int retval)
{
  usetup;

  /* ignore all signals from now on, as I don't think allowing signalling
     while running an exit-handler is such a good idea */
  u.p_sigignore = u.p_sigmask = ~0;

  while (__atexit)
    {
      while (__atexit->ind)
	{
	   if (u.u_a4)
	     asm volatile ("movel %0, a4" : : "g" (u.u_a4));
	   __atexit->fns[--__atexit->ind] ();
	}
      __atexit = __atexit->next;
    }

  u.p_xstat = retval;
  _longjmp (u.u_jmp_buf, 1);
}

void exit (int retval)
{
  usetup;

  if (u.u_ixnetbase)
    netcall(NET_shutdown_inet_daemon);
  exit2(W_EXITCODE(retval, 0));
}

void
_exit (int retcode)
{
  usetup;

  /* ignore all signals from now on */
  u.p_sigignore = u.p_sigmask = ~0;
  u.p_xstat = W_EXITCODE(retcode, 0);
  _longjmp (u.u_jmp_buf, 1);
}
