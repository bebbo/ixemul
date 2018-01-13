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
 *  ix_get_vars2.c,v 1.1.1.1 1994/04/04 04:30:55 amiga Exp
 *
 *  ix_get_vars2.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:55  amiga
 * Initial CVS check in.
 *
 *  Revision 1.4  1992/08/09  20:49:16  amiga
 *  add cast
 *
 *  Revision 1.3  1992/07/04  19:14:44  mwild
 *  add new variable, environ-in, primarly support for fork-emulation in ksh
 *
 * Revision 1.2  1992/05/20  01:32:00  mwild
 * do atexit(_cleanup) here, ix_open is the wrong place
 *
 * Revision 1.1  1992/05/14  19:55:40  mwild
 * Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#include <stdio.h>
#include <stdlib.h>

extern char _ctype_[];
extern int sys_nerr;
extern void _cleanup();

void
ix_get_vars (int argc, char **ctype, int *_sys_nerr, 
	     struct Library **sysbase, struct Library **dosbase,
	     FILE ***fpp, char ***environ_out, char ***environ_in,
	     int *real_errno, int *real_h_errno, struct __res_state *_res,
	     int *_res_socket, int *execlib)
{
  usetup;

  switch (argc)
    {
    default:
    case 12:
      if (execlib)
        *execlib = 0;

    case 11:
      if (_res_socket)
        u.u_res_socket = _res_socket;

    case 10:
      if (_res)
        u.u_res = _res;

    case 9:
      if (real_h_errno)
        {
          u.u_h_errno = real_h_errno;
        }
        
    case 8:
      /* Needed in pdksh: after the ix_resident call the child needs to
         tell ixemul.library what the new address of the errno variable is. */
      if (real_errno)
	{
	  u.u_errno = real_errno;
	  /* assume when h_errno is set, errno is set also */
          if (u.u_ixnetbase)
	    netcall(NET_set_errno, real_errno, u.u_h_errno);
	}

    case 7:
      /* a `bit' kludgy.. */
      if (environ_in) *environ_in = *u.u_environ;

    case 6:
      if (environ_out) u.u_environ = environ_out;

    case 5:
      if (fpp)
        {
	  __init_stdinouterr ();
          *fpp = (FILE **) &u.u_sF[0];
	  /* make sure all stdio buffers are flushed on exit() */
	  atexit (_cleanup);
        }

    case 4:
      if (dosbase) *dosbase = (struct Library *)DOSBase;
      
    case 3:
      if (sysbase) *sysbase =  (struct Library *)SysBase;
      
    case 2:
      if (_sys_nerr) *_sys_nerr = sys_nerr;
      
    case 1:
      if (ctype) *ctype = _ctype_;

    case 0:
      break;
    }
   
  /* now that system start is over, free the tracer */
  u.u_trace_flags = 0;
}
