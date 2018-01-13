/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1991, 1992  Markus M. Wild
 *  Portions Copyright (C) 1995 Jeff Shepherd
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
 *  ix_exec_entry.c,v 1.1.1.1 1994/04/04 04:30:55 amiga Exp
 *
 *  ix_exec_entry.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:55  amiga
 * Initial CVS check in.
 *
 *  Revision 1.3  1992/08/09  20:47:19  amiga
 *  call main thru exit, or no atexit handlers will be called!
 *
 *  Revision 1.2  1992/07/04  19:13:42  mwild
 *  add SIGWINCH handler installation/removal
 *
 * Revision 1.1  1992/05/14  19:55:40  mwild
 * Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"
#include <sys/wait.h>
#include <string.h>

int
ix_exec_entry (int argc, char **argv, char **environ, int *real_errno, 
	       int (*main)(int, char **, char **))
{
  struct Process *me = (struct Process *)FindTask(0);
  struct user *u_ptr = getuser(me);

  /* we're coming from another process here, either after vfork() or from a
   * process that wants to `replace' itself with this program. Thus the fpu
   * is probably already initialized to some bogus state. That's why we
   * HAVE to restore it into a default state here. */

  if (has_fpu)
    resetfpu();

  if (is_ixconfig(argv[0]))
    {
      u.p_xstat = W_EXITCODE(20, 0);
      return u.p_xstat;
    }

  /* a program started by ix_exec_entry() must have some ix'like shell
     around, that takes care of printing exit-states, so don't print them
     again in sig_exit() */
  u.u_argline = 0;
  u.u_arglinelen = 0;

  /* put some AmiTCP and AS225 stuff in here since it can't go into ix_open
   * I need a pointer to the REAL errno
   */
  if (real_errno)
    {
      u.u_errno = real_errno;
      if (u.u_ixnetbase)
	netcall(NET_set_errno, real_errno, NULL);
    }

  KPRINTF (("&errno = %lx\n", real_errno));

  if (!_setjmp (u.u_jmp_buf))
    {
      /* install the signal mask that was active before execve() was called */
      syscall (SYS_sigsetmask, u.u_oldmask);

      /* this is not really the right thing to do, the user should call
         ix_get_vars2 () to initialize environ to the address of the variable
         in the calling program. However, this setting guarantees that 
         the user area entry is valid for getenv() calls. */
      u.u_environ = &environ;

      __ix_install_sigwinch ();

      /* If this process is traced (under debugger control)
	 then cause a sigtrap.  */
      if (u.p_flag & STRC)
	{
	  KPRINTF(("ix_exec_entry(): STRC: _psignal (me=%08lx, SIGTRAP);\n", (long)me));

	  _psignal ((struct Task *) me, SIGTRAP);

	  /* A context switch is guaranteed to have taken place at this time,
	     since we signalled ourselves.  As we are continuing now, it means
	     the debugger process has told us to continue.  */
	}
      /* the first time thru call the program */
      exit (main (argc, argv, environ));
      /* not reached! */
    }
  /* we came from a longjmp-call */

  __ix_remove_sigwinch ();

  return u.p_xstat;
}

/* ixconfig is no longer supported and can actually crash ixemul, so
   abort if the user tries to start ixconfig. */
int is_ixconfig(char *prog)
{
  int len;

  if ((len = strlen(prog)) <= 8 || strchr("/:", prog[len - 9]))
    if (len >= 8 && !stricmp(&prog[len - 8], "ixconfig"))
      {
        ix_panic("Please use ixprefs instead of ixconfig!");
        return 1;
      }
  return 0;
}
