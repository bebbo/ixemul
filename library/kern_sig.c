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
 *  $Id: kern_sig.c,v 1.6 1994/07/11 00:32:56 rluebbert Exp $
 *
 *  $Log: kern_sig.c,v $
 *  Revision 1.6  1994/07/11  00:32:56  rluebbert
 *  Put issig back in.
 *
 *  Revision 1.5  1994/07/11  00:27:37  rluebbert
 *  Commented out unused issig
 *
 *  Revision 1.4  1994/06/19  15:13:35  rluebbert
 *  *** empty log message ***
 *
 *  Revision 1.2  1992/07/04  19:19:51  mwild
 *  change to new ix_sleep() semantics
 *
 * Revision 1.1  1992/05/14  19:55:40  mwild
 * Initial revision
 *
 *
 *  Since the code originated from Berkeley, the following copyright
 *  header applies as well. The code has been changed, it's not the
 *  original Berkeley code!
 */

/*
 * Copyright (c) 1982, 1986, 1989 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution is only permitted until one year after the first shipment
 * of 4.4BSD by the Regents.  Otherwise, redistribution and use in source and
 * binary forms are permitted provided that: (1) source distributions retain
 * this entire copyright notice and comment, and (2) distributions including
 * binaries display the following acknowledgement:  This product includes
 * software developed by the University of California, Berkeley and its
 * contributors'' in the documentation or other materials provided with the
 * distribution and in all advertising materials mentioning features or use
 * of this software.  Neither the name of the University nor the names of
 * its contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)kern_sig.c	7.23 (Berkeley) 6/28/90
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#include <stdio.h>
#include <string.h>

#include <wait.h>

#define	ttystopsigmask	(sigmask(SIGTSTP)|sigmask(SIGTTIN)|sigmask(SIGTTOU))
#define	stopsigmask	(sigmask(SIGSTOP)|ttystopsigmask)
#define defaultignmask	(sigmask(SIGCONT)|sigmask(SIGIO)|sigmask(SIGURG)| \
			sigmask(SIGCHLD)|sigmask(SIGWINCH)|sigmask(SIGINFO)|sigmask(SIGMSG))

void setsigvec (int sig, struct sigaction *sa);
void sig_exit (unsigned int code);
void stop (struct user *p);


/*
 * Can process p send the signal signo to process q?
 */
#define CANSIGNAL(p, q, signo) (1)

int
sigaction (int sig, const struct sigaction *nsa, struct sigaction *osa)
{
  struct sigaction vec;
  register struct sigaction *sa;
  int bit;
  usetup;

  if (sig <= 0 || sig >= NSIG || sig == SIGKILL || sig == SIGSTOP)
    {
      errno = EINVAL;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return -1;
    }

  sa = &vec;
  if (osa)
    {
      sa->sa_handler = u.u_signal[sig];
      sa->sa_mask = u.u_sigmask[sig];
      bit = sigmask(sig);
      sa->sa_flags = 0;
      if ((u.u_sigonstack & bit) != 0)
	sa->sa_flags |= SA_ONSTACK;

      if ((u.u_sigintr & bit) == 0)
	sa->sa_flags |= SA_RESTART;

      if (u.p_flag & SNOCLDSTOP)
	sa->sa_flags |= SA_NOCLDSTOP;

      *osa = *sa;
    }

  if (nsa)
    {
      *sa = *nsa;
      setsigvec(sig, sa);
    }
  
  return (0);
}

void
setsigvec (int sig, struct sigaction *sa)
{
  register int bit;
  usetup;

  bit = sigmask(sig);
  /*
   * Change setting atomically.
   */
  Forbid();

  u.u_signal[sig] = sa->sa_handler;
  u.u_sigmask[sig] = sa->sa_mask &~ sigcantmask;

  if ((sa->sa_flags & SA_RESTART) == 0)
    u.u_sigintr |= bit;
  else
    u.u_sigintr &= ~bit;

  if (sa->sa_flags & SA_ONSTACK)
    u.u_sigonstack |= bit;
  else
    u.u_sigonstack &= ~bit;

  if (sig == SIGCHLD) 
    {
      if (sa->sa_flags & SA_NOCLDSTOP)
	u.p_flag |= SNOCLDSTOP;
      else
	u.p_flag &= ~SNOCLDSTOP;
    }

  /*
   * Set bit in p_sigignore for signals that are set to SIG_IGN,
   * and for signals set to SIG_DFL where the default is to ignore.
   * However, don't put SIGCONT in p_sigignore,
   * as we have to restart the process.
   */
  if (sa->sa_handler == SIG_IGN ||
      (bit & defaultignmask && sa->sa_handler == SIG_DFL)) 
    {
      u.p_sig &= ~bit;		/* never to be seen again */
      if (sig != SIGCONT)
	u.p_sigignore |= bit;	/* easier in _psignal */
      u.p_sigcatch &= ~bit;
    }
  else 
    {
      u.p_sigignore &= ~bit;
      if (sa->sa_handler == SIG_DFL)
	u.p_sigcatch &= ~bit;
      else
	u.p_sigcatch |= bit;
    }

  Permit();
}


/*
 * Manipulate signal mask.
 */

int
sigprocmask (int how, const sigset_t *mask, sigset_t *omask)
{
  usetup;

  if (omask)
    *omask = u.p_sigmask;

  if (mask)
    {
      Forbid();

      switch (how) 
        {
        case SIG_BLOCK:
	  u.p_sigmask |= *mask &~ sigcantmask;
	  break;

        case SIG_UNBLOCK:
	  u.p_sigmask &= ~*mask;
	  break;

        case SIG_SETMASK:
	  u.p_sigmask = *mask &~ sigcantmask;
	  break;
	
        default:
	  errno = EINVAL;
	  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
	  goto err_ret;
        }

      Permit();
    }

  if (CURSIG (&u))
    setrun (FindTask (0));

  return 0;

err_ret:
  Permit();
  return -1;
}

int
sigpending (sigset_t *sigs)
{
  usetup;

  *sigs = u.p_sig;
  return 0;
}

/*
 * Generalized interface signal handler, 4.3-compatible.
 * (included in amiga version, because I want to reduce the static part of the
 *  library to a minimum)
 */
/* ARGSUSED */
int
sigvec(int sig, struct sigvec *nsv, struct sigvec *osv)
{
  usetup;
  struct sigvec vec;
  register struct sigvec *sv;
  struct user *p = &u;
  int bit;

  if (sig <= 0 || sig >= NSIG || sig == SIGKILL || sig == SIGSTOP)
    {
      *p->u_errno = EINVAL;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return -1;
    }

  sv = &vec;
  if (osv) 
    {
      *(sig_t *)&sv->sv_handler = p->u_signal[sig];
      sv->sv_mask = p->u_sigmask[sig];
      bit = sigmask(sig);
      sv->sv_flags = 0;
      if ((p->u_sigonstack & bit) != 0)
	sv->sv_flags |= SV_ONSTACK;
      if ((p->u_sigintr & bit) != 0)
	sv->sv_flags |= SV_INTERRUPT;
      if (p->p_flag & SNOCLDSTOP)
	sv->sv_flags |= SA_NOCLDSTOP;
      *osv = *sv;
    }

  if (nsv) 
    {
      *sv = *nsv;
      sv->sv_flags ^= SA_RESTART;	/* opposite of SV_INTERRUPT */
      setsigvec(sig, (struct sigaction *)sv);
    }

  return (0);
}

int
sigblock (int mask)
{
  int result;
  usetup;

  Forbid();
  result = u.p_sigmask;
  u.p_sigmask |= mask &~ sigcantmask;
  Permit();

  return result;
}

int
sigsetmask(int mask)
{
  int result;
  usetup;
  struct user *p = &u;

  Forbid();
  result = u.p_sigmask;
  u.p_sigmask = mask &~ sigcantmask;
  Permit();

  if (CURSIG (p))
    setrun (FindTask (0));

  return result;
}

/*
 * Suspend process until signal, providing mask to be set
 * in the meantime. 
 */
/* ARGSUSED */
int
sigsuspend (const sigset_t *mask)
{
  usetup;
  struct user *p = &u;

  /*
   * When returning from sigpause, we want
   * the old mask to be restored after the
   * signal handler has finished.  Thus, we
   * save it here and mark the proc structure
   * to indicate this (should be in u.).
   */

  Forbid();
  p->u_oldmask = p->p_sigmask;
  p->p_flag |= SOMASK;
  p->p_sigmask = *mask &~ sigcantmask;


  /* NOTE: we have to specify SIGBREAKF_CTRL_C here, as the OS doesn't seem
   *       to reschedule our task, if it receives a signal it isn't waiting
   *       for. If SIGINT is ignored, then this will jump back into the Wait,
   *	   if not, we're leaving correctly, since we waited for a signal
   *	   that now occured (lucky we, the OS tests the Recvd-field before
   *	   tc_Launch has a chance to reset it ;-))
   */

  while (ix_sleep ((caddr_t)p, "sigsuspend") == 0);
  Permit();

  setrun (FindTask (0));

  p->p_sigmask = p->u_oldmask;

  if (CURSIG (p))
    setrun (FindTask (0));

  /* always return EINTR rather than ERESTART... */
  errno = EINTR;
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
  return -1;
}


int
sigpause (int mask)
{
  return sigsuspend (&mask);
}


/* ARGSUSED */
int
sigstack(const struct sigstack *nss, struct sigstack *oss)
{
  usetup;

  if (oss) *oss = u.u_sigstack;
  if (nss) u.u_sigstack = *nss;

  return 0;
}


/*
 * Initialize signal state for process 0;
 * set to ignore signals that are ignored by default.
 */
void
siginit(struct user *p)
{
  p->p_sigignore = defaultignmask &~ sigmask(SIGCONT);
}

/*
 * This looks for the process p, validates it, and checks, whether the process
 * is currently using our signal mechanism (checks magic cookie in struct user)
 */
struct Task *
pfind (pid_t p)
{
  struct Task *t;
  usetup;
  
  if (p && !(p & 1))
    {
      /* have to check if the task really exists */

      struct List *exectasklist;
      struct Node * execnode;

      Disable();
      exectasklist = &(SysBase->TaskWait);
      for (execnode = exectasklist->lh_Head; execnode->ln_Succ;
           execnode = execnode->ln_Succ)
        {
          if ((pid_t)execnode == p)
            break;
        }
      if (execnode == NULL)
        {
          exectasklist = &(SysBase->TaskReady);
          for (execnode = exectasklist->lh_Head; execnode->ln_Succ;
               execnode = execnode->ln_Succ)
            {
              if ((pid_t)execnode == p)
                break;
            }
        }
      Enable();
      if (execnode == NULL)
        return 0;
      t = (struct Task *) p;
      if (t->tc_Node.ln_Type == NT_TASK ||
          t->tc_Node.ln_Type == NT_PROCESS)
        {
          struct user *tu = getuser(t);

          if (tu && !((int)getuser(t) & 1) && tu->u_ixbase == u.u_ixbase)
	    return t;
	}
    }
  else if (! p)
    return FindTask (0);

  return 0;
}

/* ARGSUSED */
int
kill(pid_t pid, int signo)
{
  register struct Task *t;
  usetup;

  if ((unsigned) signo >= NSIG)
    {
      errno = EINVAL;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return -1;
    }

  if (pid >= 0)
    {
      /* kill single process */
      t = pfind(pid);

      if (t == 0)
        {
	  /* there is a small chance, if pid == 0, that we may send the signal
	   * as well. If signo == SIGINT, and pid refers to a valid Task, we send
	   * it a SIGBREAKF_CTRL_C */
	  if ((signo == SIGINT) && pid && !(pid & 1))
	    {
	      t = (struct Task *) pid;
	      if (t->tc_Node.ln_Type == NT_TASK ||
	          t->tc_Node.ln_Type == NT_PROCESS)
	        {
		  Signal (t, SIGBREAKF_CTRL_C);
		  return 0;
		}
	    }

          errno = ESRCH;
	  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
          return -1;
        }

      if (signo)
	_psignal(t, signo);

      return (0);
    }

  /* signalling process groups is not (yet) implemented */  
  errno = ESRCH;
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
  return -1;
}

/* ARGSUSED */
int
killpg(int pgid, int signo)
{
  usetup;

  if ((unsigned) signo >= NSIG)
    errno = EINVAL;
  else
    errno = ESRCH;
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));

  /* signalling process groups is not (yet) implemented */  
  return -1;
}

/*
 * Send a signal caused by a trap to the current process.
 * If it will be caught immediately, deliver it with correct code.
 * Otherwise, post it normally.
 */
void trapsignal(struct Task *t, int sig, unsigned code, void *addr)
{
  int mask;
  usetup;

  mask = sigmask(sig);
  if ((u.p_flag & STRC) == 0 && (u.p_sigcatch & mask) != 0 &&
      (u.p_sigmask & mask) == 0)
    {
      u.u_ru.ru_nsignals++;
      sendsig(getuser(t), u.u_signal[sig], sig, u.p_sigmask, code, addr);
      u.p_sigmask |= u.u_sigmask[sig] | mask;
      setrun (t);
    }
  else
    {
      u.u_code = code;	/* XXX for core dump/debugger */
      _psignal(t, sig);
    }
}

/*
 * Create a core image on the file "core".
 * It writes UPAGES block of the
 * user.h area followed by the entire
 * data+stack segments.
 */
int core(void)
{
  return -1;
}

/*
 * Send the specified signal to the specified process.
 * Most signals do not do anything directly to a process;
 * they set a flag that asks the process to do something to itself.
 * Exceptions:
 *   o When a stop signal is sent to a sleeping process that takes the default
 *     action, the process is stopped without awakening it.
 *   o SIGCONT restarts stopped processes (or puts them back to sleep)
 *     regardless of the signal action (eg, blocked or ignored).
 * Other ignored signals are discarded immediately.
 */
void
_psignal(struct Task *t, int sig)	/* MAY be called in Supervisor/Interrupt  !*/
{
  register sig_t action;
  /* may be another process, so don't use u. here ! */
  struct user *p = getuser(t);
  int mask;


  mask = sigmask(sig);

  /*
   * If proc is traced, always give parent a chance.
   */
  if (p->p_flag & STRC)
    action = SIG_DFL;
  else 
    {

      /* NOTE AMIGA !
       * I can't allow for trap signals to be either ignored or masked out.
       * This would cause the trap to reoccur immediately again, resulting
       * in a deadly loop. So if such a signal gets here, it is converted
       * in a SIGILL, masked in, not ignored, not caught, that's it.
       */
      if (((mask & p->p_sigignore) || (mask & p->p_sigmask))
	  && (sig == SIGILL  || sig == SIGBUS || sig == SIGFPE || 
	      sig == SIGTRAP || sig == SIGEMT))
	{
	  sig = SIGILL;
	  mask = sigmask (sig);
	  p->p_sigignore &= ~mask;
	  p->p_sigmask   &= ~mask;
	  p->p_sigcatch  &= ~mask;
	  /* that's it, SIGILL is now reset to SIG_DFL, which will exit() */
	}

      /*
       * If the signal is being ignored,
       * then we forget about it immediately.
       * (Note: we don't set SIGCONT in p_sigignore,
       * and if it is set to SIG_IGN,
       * action will be SIG_DFL here.)
       */
     if (p->p_sigignore & mask)
	return;

     if (p->p_sigmask & mask)
	action = SIG_HOLD;
     else if (p->p_sigcatch & mask)
	action = SIG_CATCH;
     else
	action = SIG_DFL;
    }

  switch (sig) 
    {
    case SIGTERM:
      if ((p->p_flag&STRC) || action != SIG_DFL)
	break;
	/* FALLTHROUGH */

    case SIGKILL:
      break;

    case SIGCONT:
      p->p_sig &= ~stopsigmask;
      break;

    case SIGTSTP:
    case SIGTTIN:
    case SIGTTOU:
    case SIGSTOP:
      p->p_sig &= ~sigmask(SIGCONT);
      break;
    }
  p->p_sig |= mask;

  /*
   * Defer further processing for signals which are held,
   * except that stopped processes must be continued by SIGCONT.
   */
  if (action == SIG_HOLD && (sig != SIGCONT || p->p_stat != SSTOP))
    return;

  /*
   * If traced process is already stopped,
   * then no further action is necessary.
   */
  if (p->p_stat == SSTOP && (p->p_flag & STRC))
    return;

  setrun(t);
}

void stopped_process_handler (void)
{
  /* We are running in user mode in the context of the exec Task that is
     (in the view of other ixemul.library processes) a stopped process.
     We got here from the stop_process_glue routine.  */

  struct Task *task = FindTask(0);
  struct user *p = getuser(task);

  Forbid();
  _psignal((struct Task *)p->p_pptr, SIGCHLD);
  stop (p);

  while (p->p_stat != SRUN)
    {
      KPRINTF(("SSTOP: Wait (1<<p->p_zombie_sig);\n"));
      Wait (1 << p->p_zombie_sig);
      KPRINTF(("SSTOP: done, p->p_stat=%lx, p_xstat: %lx\n", p->p_stat, p->p_xstat));
    }
  Permit();
}

/*
 * If the current process has a signal to process (should be caught
 * or cause termination, should interrupt current syscall),
 * return the signal number.  Stop signals with default action
 * are processed immediately, then cleared; they aren't returned.
 * This is asked at least once each time a process enters the
 * system (though this can usually be done without actually
 * calling issig by checking the pending signal masks.)
 */
int
issig(struct user *p)	/* called in SUPERVISOR */
{
  register int sig, mask;
  u_int sr;
  usetup;

  KPRINTF(("issig(task=%lx)\n", FindTask(0)));

  asm volatile (" 
    movel a5,a0
    lea	  Lget_sr,a5
    movel 4:w,a6
    jsr	  a6@(-0x1e)
    movel a1,%0
    bra	  Lskip
Lget_sr:
    movew sp@,a1	| get sr register from the calling function
    rte
Lskip:
    movel a0,a5
	" : "=g" (sr) : : "a0", "a1", "a6");

  if (p->u_mask_state)
    {
      mask = p->u_mask_state;
      p->u_mask_state = 0;
      goto restart;
    }
  for (;;)
    {
      mask = p->p_sig & ~p->p_sigmask;
      if (p->p_flag & SVFORK)
	mask &= ~stopsigmask;

      if (mask == 0)	 	/* no signal to send */
	return 0;

      sig = ffs((long)mask);
      mask = sigmask(sig);
      /*
       * We should see pending but ignored signals
       * only if STRC was on when they were posted.
       */
      if ((mask & p->p_sigignore) && (p->p_flag & STRC) == 0) 
        {
          p->p_sig &= ~mask;
	  continue;
	}

      /* Don't do this while waiting in inet.library. */
      if (p->p_stat != SWAIT && (p->p_flag & STRC)
	  && (p->p_flag & SVFORK) == 0) 
        {
	  /*
	   * If traced, always stop, and stay
	   * stopped until released by the parent.
	   */
	  KPRINTF(("issig(task=%lx): ...stopping... Setting p->p_xstat = %ld\n", FindTask(0), sig));
	  p->p_xstat = sig;

	  KPRINTF(("issig(task=%lx): ...stopping... Sending SIGCHLD to parent %lx\n", FindTask(0), p->p_pptr));

	  if (sr & 0x2000)
	    {
	      KPRINTF(("issig(task=%lx): ...stopping... while in supervisor\n", FindTask(0)));
	      p->u_mask_state = mask;
	      sendsig(p, (sig_t)stopped_process_handler, 0, 0, 0, 0);
	      return -1;  /* Drop into the context of the task. */
	    }
	  else
	    {
	      p->u_regs = NULL;
	      p->u_fpregs = NULL;
	      KPRINTF(("issig(task=%lx): ...stopping... while NOT in supervisor\n", FindTask(0)));
	      stopped_process_handler ();
	    }
restart:
	  /*
	   * If the traced bit got turned off,
	   * go back up to the top to rescan signals.
	   * This ensures that p_sig* and u_signal are consistent.
	   *
	   * (That may be, but it fails to work with ptrace(PT_DETACH):
	   * the old SIGTRAP is caught again, but without tracing that
	   * results in a call to exit(). So we comment it out.)
	   *
	   *if ((p->p_flag & STRC) == 0)
	   *  continue;
	   */

	  /*
	   * If parent wants us to take the signal,
	   * then it will leave it in p->p_xstat;
	   * otherwise we just look for signals again.
	   */
	  p->p_sig &= ~mask;	/* clear the old signal */
	  sig = p->p_xstat;
	  if (sig == 0)
	    continue;
	  /*
	   * Put the new signal into p_sig.
	   * If signal is being masked,
	   * look for other signals.
	   */
	  mask = sigmask(sig);
	  p->p_sig |= mask;
	  if (p->p_sigmask & mask)
	    continue;
	}

      /*
       * Decide whether the signal should be returned.
       * Return the signal's number, or fall through
       * to clear it from the pending mask.
       */
      switch ((int)p->u_signal[sig]) 
        {
	case SIG_DFL:
#if notyet
	  /*
	   * Don't take default actions on system processes.
	   */
	  if (p->p_ppid == 0)
	    break;		/* == ignore */
#endif
	  /*
	   * If there is a pending stop signal to process
	   * with default action, stop here,
	   * then clear the signal.  However,
	   * if process is member of an orphaned
	   * process group, ignore tty stop signals.
	   */
	  if (mask & stopsigmask) 
	    {
#if notyet
	      if (p->p_flag&STRC ||
		  (p->p_pgru.pg_jobc == 0 && mask & ttystopsigmask))
		break;	/* == ignore */
	      u.p_xstat = sig;
	      stop(p);
	      if ((u.p_pptr->p_flag & SNOCLDSTOP) == 0)
		_psignal(u.p_pptr, SIGCHLD);
	      swtch();
#endif
	      break;
	    } 
          else if (mask & defaultignmask)
	    {
	      /*
	       * Except for SIGCONT, shouldn't get here.
	       * Default action is to ignore; drop it.
	       */
	      break;		/* == ignore */
	    }
	  else
	    return (sig);
	  /*NOTREACHED*/

	case SIG_IGN:
	  /*
	   * Masking above should prevent us ever trying
	   * to take action on an ignored signal other
	   * than SIGCONT, unless process is traced.
	   */
#if 0
	  if (sig != SIGCONT && (u.p_flag&STRC) == 0)
	    printf("issig\n");
#endif
	  break;		/* == ignore */

	default:
	  /*
	   * This signal has an action, let
	   * psig process it.
	   */
	  return (sig);
	}
      u.p_sig &= ~mask;		/* take the signal! */
    }
  /* NOTREACHED */
}

/*
 * Put the argument process into the stopped
 * state and notify the parent via wakeup.
 * Signals are handled elsewhere.
 * The process must not be on the run queue.
 */
void stop(struct user *p)
{
  p->p_stat = SSTOP;
  p->p_flag &= ~SWTED;
  ix_wakeup((u_int)getuser(p->p_pptr));
}

/*
 * Perform the action specified by the current signal.
 * The usual sequence is:
 *	if (sig = CURSIG(p))
 *		psig(p, sig);
 */
void
psig(struct user *p, int sig)	/* called in SUPERVISOR */
{
  int code, mask, returnmask;
  register sig_t action; 

  do 
    {
      if (sig == -1)
        return;
      mask = sigmask(sig);
      p->p_sig &= ~mask;
      action = p->u_signal[sig];
      if (action != SIG_DFL) 
        {
	   /*
	    * Set the new mask value and also defer further
	    * occurences of this signal.
	    *
	    * Special case: user has done a sigpause.  Here the
	    * current mask is not of interest, but rather the
 	    * mask from before the sigpause is what we want
	    * restored after the signal processing is completed.
	    */
	  if (p->p_flag & SOMASK)
	    {
	      returnmask = p->u_oldmask;
	      p->p_flag &= ~SOMASK;
	    }
	  else
	    returnmask = p->p_sigmask;
	  p->p_sigmask |= p->u_sigmask[sig] | mask;

	  p->u_ru.ru_nsignals++;
	  if (p->u_sig != sig)
	    {
	      KPRINTF(("psig(): code = 0;\n"));
	      code = 0;
	    }
	  else
	    {
	      KPRINTF(("psig(): code = u.u_code;\n"));
	      code = p->u_code;
	      p->u_code = 0;
	    }
	  KPRINTF(("kern_sig.c:psig(): doing sendsig(p, action, sig, returnmask, code, 0);\n"));
	  sendsig(p, action, sig, returnmask, code, 0);
	  continue;
	}

#if whatdoesthisdo
      p->u_acflag |= AXSIG;
#endif

      switch (sig) 
        {
	case SIGILL:
	case SIGIOT:
	case SIGBUS:
	case SIGQUIT:
	case SIGTRAP:
	case SIGEMT:
	case SIGFPE:
	case SIGSEGV:
	case SIGSYS:
	  p->u_sig = sig;
	  if (core() == 0)
	    sig |= WCOREFLAG;
	}
      /* we can't call exit() when in supervisor mode, have to do this just like
       * it was a signal passed on its own frame */
      sendsig(p, (sig_t)sig_exit, sig, 0, 0, 0);
  } while ((sig = CURSIG(p)));
}

static int sigprocessgrp(struct Process *proc, int pgrp, int signal, int test)
{
  struct Process *p;

  if (test)
    {
      if (getuser(proc) == NULL)
        return 0;
      for (p = getuser(proc)->p_cptr; p; p = getuser(p)->p_osptr)
        {
          if (getuser(p) == NULL)
            return 0;
          if (sigprocessgrp(p, pgrp, signal, test) == 0)
            return 0;
        }
      return 1;
    }
  for (p = getuser(proc)->p_cptr; p; p = getuser(p)->p_osptr)
    {
      sigprocessgrp(p, pgrp, signal, test);
    }
  if (getuser(proc)->p_pgrp == pgrp)
    _psignal((struct Task *)proc, signal);
  return 1;
}

void _psignalgrp(struct Process *proc, int signal)
{
  struct Process *p;
  struct Process *ok = proc;

  if (proc == NULL || getuser(proc) == NULL)
    return;
  p = getuser(proc)->p_pptr;

  /* traverse to the top of the process-tree */
  while (p && p != (struct Process *)1)
    {
      ok = p;
      if (getuser(p) == NULL)
        return;
      p = getuser(p)->p_pptr;
    }
  if (sigprocessgrp(ok, getuser(proc)->p_pgrp, signal, 1))
    sigprocessgrp(ok, getuser(proc)->p_pgrp, signal, 0);
}
