/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1991, 1992  Markus M. Wild
 *  Portions Copyright (C) 1994 Rafael W. Luebbert
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
 *  $Id: ix_startup.c,v 1.5 1994/06/19 15:13:22 rluebbert Exp $
 *
 *  $Log: ix_startup.c,v $
 *  Revision 1.5  1994/06/19  15:13:22  rluebbert
 *  *** empty log message ***
 *
 *  Revision 1.3  1992/08/09  20:55:51  amiga
 *  import sysbase
 *
 *  Revision 1.2  1992/07/04  19:18:21  mwild
 *  remove SIGWINCH handler before returning
 *
 * Revision 1.1  1992/05/14  19:55:40  mwild
 * Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include <sys/wait.h>
#include "kprintf.h"

int _main();

/*
 * Note: I kept the partition into startup and _main(), although in this
 *       case, both functions could be done in one function, since this is
 *       a library, and the user can't override _main anyway but globally...
 */

int
ix_startup (char *aline, int alen,
	    int expand, char *wb_default_window, u_int main, int *real_errno)
{
  struct Process *proc = (struct Process *)FindTask(0);
  struct user *u_ptr = getuser(proc);
  int exit_val;
  struct WBStartup *wb_msg = NULL;
  int fd;
  struct my_seg mySeg = { 0 };

  /*
   * The following code to reset the fpu might not be necessary, BUT since
   * a CLI shell doesn't spawn a new process when executing a command - it 
   * insteads calls the command like a subroutine - it depends on the Shell
   * whether the fpu is setup correctly. And I don't like to depend on any
   * thing ;-)
   */
  if (has_fpu)
    resetfpu();
  /* first deal with WB messages, since those HAVE to be answered properly,
   * even if we should fail later (memory, whatever..) */

  if (!proc->pr_CLI)
    {
      /* we have been started by Workbench. Get the StartupMsg */
      WaitPort (&proc->pr_MsgPort);
      wb_msg = (struct WBStartup *) GetMsg (&proc->pr_MsgPort);
      /* further processing in _main () */
    }
  else
    {
      struct CommandLineInterface *cli = (void *)BADDR(proc->pr_CLI);
      long segs;

      /* for usage by sys_exit() for example */
      KPRINTF (("CLI command line '%s'\n", aline));
      u.u_argline = aline;
      u.u_arglinelen = alen;
      u.u_segs = &mySeg;
      u.u_segs->name = NULL;
      segs = cli->cli_Module;
      u.u_segs->segment = segs;
      segs <<= 2;
      u.u_start_pc = segs + 4;
      u.u_end_pc = segs + *(long *)(segs - 4) - 8;
    }

  u.u_expand_cmd_line = expand;

  /* put some AmiTCP and AS225 in here since it can't go into ix_open
   * I need a pointer to the REAL errno
   */
  if (real_errno)
   {
      u.u_errno = real_errno;
      if (u.u_ixnetbase)
	netcall(NET_set_errno, real_errno, NULL);
   }

  KPRINTF (("&errno = %lx\n", real_errno));
  exit_val = _setjmp (u.u_jmp_buf);

  if (! exit_val)
    {
      /* from now on it's safe to allow signals */
      syscall (SYS_sigsetmask, 0);
      /* the first time thru call the program */
      KPRINTF (("calling __main()\n"));
      if (proc->pr_CLI)
        _main(aline, alen, main);
      else
	_main(wb_msg, wb_default_window, main);
      /* NORETURN */
    }
  /* in this case we came from a longjmp-call */
  exit_val = u.p_xstat;

  __ix_remove_sigwinch ();

  /* had to move the closing of files out of ix_close(), as close() may 
     actually wait for the last packet to arrive, and inside ix_close() we're
     inside Forbid, and may thus never wait! */

  /* close all files */
  for (fd = 0; fd < NOFILE; fd++) 
    if (u.u_ofile[fd]) syscall (SYS_close, fd);

  /* if at all possible, free memory before entering Forbid ! (Semaphore
     problems..) */
  all_free ();

  /*
   * If we were traced by a debugger who got us through a ptrace 'attach',
   * we need to unlink us from our parent and to give it a signal telling
   * it that we've died.
   */
   Forbid();
   if (u.p_pptr && u.p_pptr != (struct Process *) 1)
    send_death_msg(&u);
   Permit();

  /* if started from workbench, Forbid(), since on reply WB will deallocate
   * our task... */
  if (!proc->pr_CLI)
    {
      Forbid ();
      wb_msg = *&wb_msg;  // Work around compiler warning
      ReplyMsg ((struct Message *) wb_msg);
    }

  return WEXITSTATUS(exit_val);
}
