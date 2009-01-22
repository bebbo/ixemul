/*-
 * Copyright (c) 1995 Leonard Norrgard.  All rights reserved.
 * Copyright (c) 1994 Christopher G. Demetriou.  All rights reserved.
 * Copyright (c) 1982, 1986, 1989, 1993
 *	The Regents of the University of California.  All rights reserved.
 * (c) UNIX System Laboratories, Inc.
 * All or some portions of this file are derived from material licensed
 * to the University of California by American Telephone and Telegraph
 * Co. or Unix System Laboratories, Inc. and are reproduced herein with
 * the permission of UNIX System Laboratories, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)sys_process.c	8.1 (Berkeley) 6/10/93
 */

/*
 * References:
 *	(1) Bach's "The Design of the UNIX Operating System",
 *	(2) sys/miscfs/procfs from UCB's 4.4BSD-Lite distribution,
 *	(3) the "4.4BSD Programmer's Reference Manual" published
 *		by USENIX and O'Reilly & Associates.
 * The 4.4BSD PRM does a reasonably good job of documenting what the various
 * ptrace() requests should actually do, and its text is quoted several times
 * in this file.
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#include <signal.h>
#include <string.h>
#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <exec/execbase.h>


int process_read_regs (struct user *p, struct reg *regs)
{
  usetup;

  if (p->u_regs == NULL)
    {
      bzero(regs, sizeof(struct reg));
      errno = EIO;
      return -1;
    }
  bcopy (p->u_regs, regs, sizeof (struct reg));
  return 0;
}

int process_write_regs (struct user *p, struct reg *regs)
{
  usetup;

  if (p->u_regs == NULL)
    {
      errno = EIO;
      return -1;
    }
  bcopy (regs, p->u_regs, sizeof (struct reg));
  return 0;
}

int process_read_fpregs (struct user *p, struct fpreg *fpregs)
{
  usetup;

  if (p->u_fpregs == NULL)
    {
      bzero(fpregs, sizeof(struct fpreg));
      errno = EIO;
      return -1;
    }
  bcopy (p->u_fpregs, fpregs, sizeof (struct fpreg));
  return 0;
}

int process_write_fpregs (struct user *p, struct fpreg *fpregs)
{
  usetup;

  if (p->u_fpregs == NULL)
    {
      errno = EIO;
      return -1;
    }
  bcopy (fpregs, p->u_fpregs, sizeof (struct fpreg));
  return 0;
}

int process_sstep (struct user *t, int sstep)
{
  usetup;

  if (sstep)
    if (t->u_regs == NULL)
      {
        errno = EIO;
        return -1;
      }
    else
      t->u_regs->r_sr |= 0x8000;
  else if (t->u_regs)
    t->u_regs->r_sr &= ~0x8000;
  return 0;
}

int process_set_pc (struct user *t, caddr_t addr)
{
  usetup;

  if (t->u_regs)
    {
      t->u_regs->r_pc = addr;
      return 0;
    }
  errno = EIO;
  return -1;
}

static void ptrace_install_vector(void)
{
  extern void install_vector(void);  /* trap.s */

  Supervisor((void *)install_vector);
}

static void ptrace_restore_vector(void)
{
  extern void restore_vector(void);  /* trap.s */

  Supervisor((void *)restore_vector);
}

int ptrace (int request, pid_t pid, caddr_t addr, int data)
{
  struct Task *task, *me = FindTask(0);
  struct user *t, *u_ptr = getuser(me);
  int step;
  int error;

  if (request == PT_GETIXINFO)
    {
      struct small_ixnet_base {
	struct Library     ixnet_lib;
	unsigned char      ix_myflags;
	unsigned char      ix_pad;
	BPTR		   ix_seg_list;
      };

      static struct ixinfo info;
      extern void sig_trampoline();
      extern void sig_launch();
        
      info.version = 1;
      info.ixemul_seglist = ix.ix_seg_list;
      info.ixnet_seglist = (u.u_ixnetbase ? ((struct small_ixnet_base *)(u.u_ixnetbase))->ix_seg_list : NULL);
      info.sigtramp_start = (long)sig_trampoline;
      info.sigtramp_end = (long)sig_launch;
      if (has_68020_or_up)
        {
          info.install_vector = ptrace_install_vector;
          info.restore_vector = ptrace_restore_vector;
        }
      else
        {
          info.install_vector = NULL;
          info.restore_vector = NULL;
        }
      return (int)&info;
    }
  else if (request == PT_TRACE_ME)
    task = me;
  else if ((request == PT_ATTACH))
    { 
      /* have to check if the task really exists */
      if (pid == 0 || (task = pfind(pid)) == NULL)
        {
          errno = ESRCH;
          return -1;
        }
    }
  else
    task = (struct Task *) pid;

/* Temporarily until I'm convinced it all works. It makes it also easier
   to debug gdb. */
#if 0
  {
    char *req;

    switch (request)
      {
      case PT_TRACE_ME:   req = "PT_TRACE_ME";  break;
      case PT_READ_I:     req = "PT_READ_I";    break;
      case PT_READ_D:     req = "PT_READ_D";    break;
      case PT_READ_U:     req = "PT_READ_U";    break;
      case PT_WRITE_I:    req = "PT_WRITE_I";   break;
      case PT_WRITE_D:    req = "PT_WRITE_D";   break;
      case PT_WRITE_U:    req = "PT_WRITE_U";   break;
      case PT_CONTINUE:   req = "PT_CONTINUE";  break;
      case PT_KILL:       req = "PT_KILL";      break;
      case PT_STEP:       req = "PT_STEP";      break;
      case PT_GETSEGS:    req = "PT_GETSEGS";   break;
      case PT_GETIXINFO:  req = "PT_GETIXINFO"; break;
      case PT_GETREGS:    req = "PT_GETREGS";   break;
      case PT_SETREGS:    req = "PT_SETREGS";   break;
      case PT_GETEXENAME: req = "PT_GETEXENAME";break;
      case PT_GETA4:      req = "PT_GETA4";     break;
      case PT_GETFPREGS:  req = "PT_GETFPREGS"; break;
      case PT_SETFPREGS:  req = "PT_SETFPREGS"; break;
      case PT_ATTACH:     req = "PT_ATTACH";    break;
      case PT_DETACH:     req = "PT_DETACH";    break;
      default:       req = "*Unknown request*"; break;
      }
    switch (request)
      {
      case PT_WRITE_I:
      case PT_WRITE_D:
      case PT_WRITE_U:
      case PT_KILL:
      case PT_CONTINUE:
      case PT_STEP:
        KPrintF("ptrace (%s, pid=%lx, addr=%lx , data=%lx);\n",
	        req, pid, addr, data);
      }
  }
#endif

  /* sanity check */
  if (task == NULL || (t = getuser(task)) == NULL)
    {
      errno = ESRCH;
      return -1;
    }

  /* Check that the arguments are valid.  */
  switch (request)
    {
    case PT_TRACE_ME:
      /* Saying that you're being traced is always OK.  */
      break;

    case PT_ATTACH:
      /* You can't attach to a process if:
	 (1) it's the process that's doing the attaching or  */
      if (t == &u)
        {
          errno = EPERM;
          return -1;
        }

      /* (2) it's already being traced.  */
      if (t->p_flag & STRC)
        {
          errno = EPERM;
          return -1;
        }
      break;

    case PT_READ_I:
    case PT_READ_D:
    case PT_WRITE_I:
    case PT_WRITE_D:
    case PT_CONTINUE:
    case PT_KILL:
    case PT_DETACH:
    case PT_GETFPREGS:
    case PT_SETFPREGS:

      /* You can't do what you want to the process if:  */

      /* (1) It's not being traced at all,  */
      if (!(t->p_flag & STRC))
        {
          errno = EPERM;
          return -1;
        }

      /* (2) it's not being traced by _you_, or  */
      if (getuser(t->p_pptr) != &u)
        {
          errno = EPERM;
          return -1;
        }

      /* (3) it's not currently stopped.  */
      if (t->p_stat != SSTOP
	  || !(t->p_flag & SWTED))
        {
          errno = EPERM;
          return -1;
        }
      break;

    case PT_STEP:
    case PT_GETREGS:
    case PT_SETREGS:
    case PT_GETSEGS:	/* you can always do this */
    case PT_GETEXENAME:	/* you can always do this */
    case PT_GETA4:	/* you can always do this */
      break;

    default:
      /* It was not a valid request. */
      errno = EIO;
      return -1;
    }

  /* Now actually do the job.  */
  step = 0;

  switch (request)
    {
    case PT_TRACE_ME:
      /* Child declares it's being traced, just set the trace flag.  */
      t->p_flag |= STRC;
      break;

    case PT_READ_I:
    case PT_READ_D:
      /* Check whether this is valid memory */
      if (((int)addr & 1) || addr == 0 || ((TypeOfMem(addr)) == 0))
        {
          errno = EIO;
          return -1;
        }
      return *((int *)addr);

    case PT_WRITE_I:
    case PT_WRITE_D:
      /* Check whether this is valid memory */
      if (((int)addr & 1) || addr == 0 || ((TypeOfMem(addr)) == 0))
        {
          errno = EIO;
          return -1;
        }
      *((int *)addr) = data;
      CacheClearE(addr, 4, CACRF_ClearI | CACRF_ClearD);
      return 0;

    case PT_GETSEGS:
      return (t->u_segs ? (int)t->u_segs->segment : 0);

    case PT_GETEXENAME:
      return (t->u_segs ? (int)t->u_segs->name : 0);

    case PT_GETA4:
      return t->u_a4;

      /* case PT_READ_U: fixme */
      /* case PT_WRITE_U: fixme */

    case PT_STEP:
      /* From the 4.4BSD PRM:
	 "Execution continues as in request PT_CONTINUE; however
	 as soon as possible after execution of at least one
	 instruction, execution stops again. [ ... ]"  */
      step = 1;
      /* fallthrough */

    case PT_CONTINUE:
      /* From the 4.4BSD PRM:
	 "The data argument is taken as a signal number and the
	 child's execution continues at location addr as if it
	 incurred that signal.  Normally the signal number will
	 be either 0 to indicate that the signal that caused the
	 stop should be ignored, or that value fetched out of
	 the process's image indicating which signal caused
	 the stop.  If addr is (int *)1 then execution continues
	 from where it stopped." */
      /* step = 0 done above. */

      /* Check that data is a valid signal number or zero.  */
      if (data < 0 || data >= NSIG)
        {
          errno = EIO;
          return -1;
        }

      /* Arrange for a single-step, if that's requested and possible.  */
      if ((error = process_sstep (t, step)))
	return error;

      /* If the address parameter is not (int *)1, set the pc.  */
      if ((int *)addr != (int *)1)
	if ((error = process_set_pc (t, addr)))
	  return error;

      /* Finally, deliver the requested signal (or none).  */
    sendsig:
      t->p_xstat = data;
      setrun (task);
      return 0;

    case PT_KILL:
      /* not being traced any more */
      t->p_flag &= ~STRC;
      /* Just send the process a KILL signal.  */
      data = SIGKILL;
      goto sendsig;

    case PT_GETREGS:
      return process_read_regs (t, (struct reg *)addr);
      
    case PT_SETREGS:
      return process_write_regs (t, (struct reg *)addr);
      
    case PT_GETFPREGS:
      return process_read_fpregs (t, (struct fpreg *)addr);
      
    case PT_SETFPREGS:
      return process_write_fpregs (t, (struct fpreg *)addr);

    case PT_ATTACH:      
	/*
	 * Go ahead and set the trace flag.
	 * Save the old parent (it's reset in
	 *   _DETACH, and also in vfork.c:wait4()
	 * Reparent the process so that the tracing
	 *   proc gets to see all the action.
	 * Stop the target.
	 */
	t->p_flag |= STRC;
	t->p_xstat = 0;         /* XXX ? */
	if (t->p_pptr != (struct Process *)me) {
	  t->p_opptr = t->p_pptr;
	  proc_reparent((struct Process *)task, (struct Process *)me);
	}
	_psignal(task, SIGSTOP);
	return (0);

    case PT_DETACH:      
	/* not being traced any more */
	t->p_flag &= ~STRC;

	/* give process back to original parent */
	if (t->p_opptr != t->p_pptr)
        {
	  if (t->p_opptr && pfind((pid_t)t->p_opptr))
	    proc_reparent((struct Process *)task, t->p_opptr);
	}

	t->p_opptr = NULL;
	t->p_flag &= ~SWTED;

	/* and deliver any signal requested by tracer. */
	if (t->p_stat == SSTOP)
	  goto sendsig;
	else if (data)
	  _psignal(task, data);

	return (0);

    default:
      /* Unknown request.  */
      errno = EIO;
      return -1;
    }
  return 0;	/* correct return value? */
}
