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
 *  machdep.c,v 1.1.1.1 1994/04/04 04:30:40 amiga Exp
 *
 *  machdep.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:40  amiga
 * Initial CVS check in.
 *
 *  Revision 1.5  1993/11/05  21:59:18  mwild
 *  add code to deal with inet.library
 *
 *  Revision 1.4  1992/10/20  16:25:24  mwild
 *  no nasty 'c' polling in DEF-signalhandler...
 *
 *  Revision 1.3  1992/08/09  20:57:59  amiga
 *  add volatile to sysbase access, or the optimizer takes illegal shortcuts...
 *
 *  Revision 1.2  1992/07/04  19:20:20  mwild
 *  add yet another state in which not to force a context switch.
 *  Probably unnecessary paranoia...
 *
 * Revision 1.1  1992/05/14  19:55:40  mwild
 * Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include <sys/wait.h>
#include "kprintf.h"

#include <string.h>
#include <exec/execbase.h>

/* jump to pc in supervisor mode, usp is set to USP before */
extern void volatile supervisor (u_int pc, u_int usp);

/* context restore functions for 68000 and 68020 rsp */

/* takes the sigcontext * from the usp and restores it 
 * Assumes it's called by Supervisor(), ie. with an exception frame
 */
extern void volatile do_sigreturn (void);

/*
 * These two are callable with jsr from supervisor mode, and then
 * set up a fake exception frame and call do_sigreturn().
 */
extern void volatile sup00_do_sigreturn_ssp (u_int ssp);
extern void volatile sup00_do_sigreturn (void);
extern void volatile sup00_do_sigresume (void);
extern void volatile restore_00 ();
extern void volatile sup20_do_sigreturn_ssp (u_int ssp);
extern void volatile sup20_do_sigreturn (void);
extern void volatile sup20_do_sigresume (void);
extern void volatile restore_20 ();
/*
 * Either one of sup{00,20}_do_sigreturn, set by configure_context_switch ();
 */
static void volatile (*sup_do_sigresume) (void);
static void volatile (*sup_do_sigreturn) (void);
static void volatile (*sup_do_sigreturn_ssp) (u_int ssp);

void setrun (struct Task *t);
void sendsig(struct user *p, sig_t catcher, int sig, int mask, unsigned code, void *addr);

struct sigframe {
  int			sf_signum;	/* signo for handler */
  int			sf_code;	/* additional info for handler */
  void			*sf_addr;	/* yet another info for handler ;-)) */
  sig_t			sf_handler;	/* handler addr for u_sigc */
  struct sigcontext 	sf_sc;		/* actual context */
};

void
configure_context_switch (void)
{
  /* The stack frame of the 68010 is identical to the 68020! */
  if (has_68010_or_up)
    {
      sup_do_sigresume = sup20_do_sigresume;
      sup_do_sigreturn = sup20_do_sigreturn;
      sup_do_sigreturn_ssp = sup20_do_sigreturn_ssp;
    }
  else
    {
      sup_do_sigresume = sup00_do_sigresume;
      sup_do_sigreturn = sup00_do_sigreturn;
      sup_do_sigreturn_ssp = sup00_do_sigreturn_ssp;
    }
}

void volatile
sigreturn (struct sigcontext *sc)
{
  supervisor ((u_int) do_sigreturn, (u_int) sc);
}

void volatile
sig_trampoline (struct sigframe sf)
{
  usetup;

  if (u.u_a4)
    asm ("movel %0,a4" : : "g" (u.u_a4));
  ((void (*)())sf.sf_handler) (sf.sf_signum, sf.sf_code, sf.sf_addr, & sf.sf_sc);

  sigreturn (& sf.sf_sc);
}

// Utility function for trap.s
struct Task *get_current_task(void)
{
  return FindTask(0);
}

/*
 * This one is executed in Supervisor mode, just before dispatching this
 * task, so be as quick as possible here !
 */
void
sig_launch (void) 
{
  struct Task 		*me 		= FindTask(0);
  struct user 		*u_ptr 		= getuser(me);
  sigset_t 		sigmsg 		= sigmask (SIGMSG);
  sigset_t		sigint 		= sigmask (SIGINT);
  sigset_t 		newsigs;
  int 			i;
  u_int			usp = 0, orig_usp;
  struct sigcontext 	*sc;
  u_int			ret_pc, ret_ssp;

  /* GCC supports nested functions, and that's just what I need! */

  void setup_sigcontext(void)
  {
    usp = orig_usp = get_usp () + 8; 	/* set up by our glue_launch() stub */
    
    /* the glue passes us the values of the pc and ssp to restore, if we should
     * decide to sup_do_sigreturn_ssp() out of here, instead of leaving harmlessly..
     */
    ret_pc  = ((u_int *)usp)[-2];
    ret_ssp = ((u_int *)usp)[-1];
    
    /* push a sigcontext that will get us back if no other signals
     * were produced */
    usp -= sizeof (struct sigcontext);
    sc = (struct sigcontext *) usp;
    set_usp (usp);
    
    sc->sc_onstack = u.u_onstack;
    sc->sc_mask	 = u.p_sigmask;
    sc->sc_sp	 = orig_usp;

    /* the OS context restore function expects a5 to contain the usp, so
     * we have to obey.. */
    sc->sc_fp	 = orig_usp;
    sc->sc_ap	 = *(u_int *)&me->tc_Flags;
    sc->sc_pc	 = ret_pc;
    sc->sc_ps	 = get_sr();
  
    u.u_regs = NULL;
    u.u_fpregs = NULL;
  };

  if (u.u_mask_state) /* do not handle signals while the stop-handler is running */
    return;

  /* if we're inside ix_sleep, no signal processing is done to break the
     Wait there as soon as possible. Signals resume on return of ix_sleep */
  /* Likewise if the process is stopped for debugging (SSTOP).  */
  if (u.p_stat == SSLEEP || u.p_stat == SSTOP)
    return;

  /* special processing for Wait()ing in Commodore inet.library. They
     do reasonable interrupt checking, but only on SIGBREAKF_CTRL_C. So
     whenever we have a signal to deliver, send ^C.. */
  if (u.p_stat == SWAIT)
    {
      setup_sigcontext();
      if (CURSIG(&u))
        Signal (me, SIGBREAKF_CTRL_C);
      goto out;
    }

  /* smells kludgy I know...... */
  if (me->tc_TDNestCnt >= 0 || me->tc_IDNestCnt >= 0)
    return;

  setup_sigcontext();

  /*
   * first check AmigaOS signals. If SIGMSG is set to SIG_IGN or SIG_DFL, 
   * we do our default mapping of SIGBREAKF_CTRL_C into SIGINT.
   */
  newsigs = me->tc_SigRecvd & ~u.u_lastrcvsig;
  u.u_lastrcvsig = me->tc_SigRecvd;

  if (u.u_ixnetbase)
    netcall(NET__siglaunch, newsigs);

  if (((u.p_sigignore & sigmsg) || !(u.p_sigcatch & sigmsg)) 
      && (newsigs & SIGBREAKF_CTRL_C))
    {
      /* in that case send us a SIGINT, if it's not ignored */
      if (!(u.p_sigignore & sigint))
        {
	  struct Process *proc = (struct Process *)(u.u_session ? u.u_session->pgrp : (int)me);
          _psignalgrp(proc, SIGINT);
        }
        
      /* in this mode we fully handle and use SIGBREAKF_CTRL_C, so remove it
       * from the Exec signal mask */
       
      me->tc_SigRecvd &= ~SIGBREAKF_CTRL_C;
      u.u_lastrcvsig &= ~SIGBREAKF_CTRL_C;
    }
  else if (newsigs && (u.p_sigcatch & sigmsg))
    {
      /* if possible, deliver the signal directly to get a code argument */
      if (!(u.p_flag & STRC) && !(u.p_sigmask & sigmsg))
        {
          u.u_ru.ru_nsignals++;
          sendsig(&u, u.u_signal[SIGMSG], SIGMSG, u.p_sigmask, newsigs, 0);
          u.p_sigmask |= u.u_sigmask[SIGMSG] | sigmsg;
          setrun (me);
        }
      else
        _psignal (me, SIGMSG);
    }

  if ((i = CURSIG(&u)))
    {
      psig(&u, i);
    }

out:
  /* now try to optimize. We could always call sup_do_sigreturn here, but if no
   * signals generated frames, we can just as well simply return, after having
   * restored our usp */
  if (usp == get_usp ())
    {
      /* this is probably not even necessary, since after processing sig_launch
       * the OS reinstalls the usp as me->tc_SPReg, but I guess it's cleaner to
       * do it explicitly here, to show that we reset usp to what it was before
       */
      set_usp (orig_usp);
      return;
    }
  sup_do_sigreturn_ssp (ret_ssp);
}


void
switch_glue (void)
{
}

/*
 * Send an interrupt to process.
 * Called from psig() which is called from sig_launch, thus we are in
 * SUPERVISOR .
 */
void
sendsig (struct user *p, sig_t catcher, int sig, int mask, unsigned code, void *addr)
{
  struct Task		*me = FindTask(0);
  u_int 		usp, orig_usp;
  struct sigframe 	*sf;
  struct sigcontext	*sc;
  int			oonstack;
  int			to_stopped_handler = (catcher == (sig_t)stopped_process_handler);
  int 			*dummy_frame;

  orig_usp = get_usp();	/* get value to restore later */

  oonstack = p->u_onstack;

  if (!p->u_onstack && (p->u_sigonstack & sigmask(sig)))
    {
      p->u_onstack = 1;
      usp = (u_int) p->u_sigsp;
    }
  else
    usp = orig_usp;
  
  /* make room for dummy stack frame (used by GDB) */
  usp -= 8;
  dummy_frame = (int *)usp;
  /* push signal frame */
  usp -= sizeof (struct sigframe);
  sf = (struct sigframe *) usp;
  
  /* fill out the frame */
  sf->sf_signum        = sig;
  sf->sf_code          = code;
  sf->sf_addr	       = addr;
  sf->sf_handler       = catcher;
  sf->sf_sc.sc_onstack = oonstack;
  sf->sf_sc.sc_mask    = mask;
  sf->sf_sc.sc_sp      = (int) orig_usp;	/* previous sigcontext */
  sf->sf_sc.sc_fp      = p->u_regs ? p->u_regs->r_regs[13] : 0;
  sf->sf_sc.sc_ap      = *(u_int *)&me->tc_Flags;
  sf->sf_sc.sc_ps      = get_sr() & ~0x8000;	/* we're in supervisor then */
  /* this pc will restore it */
  sf->sf_sc.sc_pc      = (int)(to_stopped_handler ? sup_do_sigresume : sup_do_sigreturn);

  /* push a signal context to call sig_trampoline */
  usp -= sizeof (struct sigcontext);
  sc = (struct sigcontext *) usp;

  /*
   * NOTE: we set the default of a handler to Permit(), Enable(). I guess this
   *       makes sense, since if either Forbid() or Disable() is active, it
   *	   shouldn't be possible to invoke a signal anyway, EXCEPT if the
   *	   task is Wait()ing, then the OS calls Switch() directly while
   *	   either Disable() or Forbid() is active (depends on OS version).
   */

  sc->sc_onstack = p->u_onstack;
  sc->sc_mask    = p->p_sigmask;
  sc->sc_sp	 = ((int) sf) - 4; /* so that sp@(4) is the argument */
  dummy_frame[0] = (int)(p->u_regs ? p->u_regs->r_regs[13] : 0);
  dummy_frame[1] = (int)(p->u_regs ? p->u_regs->r_pc : 0);
  sc->sc_fp	 = (int)dummy_frame;
  sc->sc_ap	 = (me->tc_Flags << 24) | (me->tc_State << 16) |
  		   ((u_char)(-1) << 8) | (u_char)(-1);
  sc->sc_ps	 = ((to_stopped_handler || !p->u_regs) ? 0 : (p->u_regs->r_sr & ~0x2000));
  sc->sc_pc	 = (int) sig_trampoline;
  
  set_usp (usp);
}


/*
 * called as the default action of a signal that terminates the process
 */
void
sig_exit (unsigned int code)
{
  /* the whole purpose of this code inside is to
   * prettyprint and identify the job that just terminates
   * This stuff should be handled by a shell, but since there's (yet) no
   * shell that knows how to interpret a signal-exit code I have to do it
   * here myself...
   */

  extern char *sys_siglist[NSIG];
  extern void exit2(int);
  struct Process *me = (struct Process *)FindTask(0);
  struct user *u_ptr = getuser(me);
  char err_buf[255];
  struct CommandLineInterface *cli;
  char process_name[255];
  int is_fg;

  /* make sure we're not interrupted in this last step.. */
  u.p_flag &= ~STRC;    /* disable tracing */
  syscall (SYS_sigsetmask, ~0);
  if ((ix.ix_flags & ix_create_enforcer_hit) && has_68020_or_up)
    {
      /* This piece of assembly will skip all the saved registers, signal
	 contexts, etc. on the stack and set the stack pointer accordingly.
	 The number 700 has been determined by experimenting. After setting
	 the SP it will put the exit code into the address 0xDEADDEAD. This
	 creates an Enforcer hit, and Enforcer will show a stack dump starting
	 with the new SP. If we didn't add 700 bytes to the SP, then you would
	 have to configure Enforcer to use a stacktrace of more than 24 lines
	 before you would get to the relevant parts of the trace. After that we
	 add a nop and restore the SP. The nop was needed because due to the
	 pipelining of a 68040 the SP was already updated before Enforcer could
	 read the value of the SP. More nops may be needed for the 68060 CPU. */

      asm ("movel %0,d0
            addw  #700,sp
            movel d0,0xdeaddead
            nop
            addqw #2,sp
            movel d0,0xdeaddead
            nop
            addaw #-702,sp" : /* no output */ : "a" (code));
    }
  
  /* output differs depending on
   *  o  whether we're a CLI or a WB process (stderr or requester)
   *  o  whether this is a foreground or background process
   *  o  whether this is SIGINT or not
   */

  if ((cli = BTOCPTR (me->pr_CLI)))
    {
      if (!GetProgramName(process_name, sizeof(process_name)))
        process_name[0] = 0;

      is_fg = cli->cli_Interactive && !cli->cli_Background;
    }
  else
    {
      process_name[0] = 0;
      if (me->pr_Task.tc_Node.ln_Name)
        strncpy (process_name, me->pr_Task.tc_Node.ln_Name, sizeof (process_name) - 1);
        
      /* no WB process is ever considered fg */
      is_fg = 0;
    }
  /* if is_fg and SIGINT, simulate tty-driver and display ^C */
  if (!is_fg || (code != SIGINT))
    {
      strcpy (err_buf, (code < NSIG) ? sys_siglist[code] : "Unknown signal");

      /* if is_fg, don't display the job */
      if (! is_fg)
        {
          strcat (err_buf, " - ");
	  strcat (err_buf, process_name);
          /* if we're a CLI we have an argument line saved, that we can print
           * as well */
	  if (cli)
      	    {
	      int line_len;
	      char *cp;
	      
	      /* we can display upto column 77, this should be safe on all normal
	       * amiga CLI windows */
	      line_len = 77 - strlen (err_buf) - 1;
	      if (line_len > u.u_arglinelen)
	        line_len = u.u_arglinelen;

	      if (line_len > 0 && u.u_argline)
	        {
	          strcat (err_buf, " ");
		  strncat (err_buf, u.u_argline, line_len);
		}

	      /* now get rid of possible terminating line feeds/cr's */
	      for (cp = err_buf; *cp && *cp != '\n' && *cp != '\r'; cp++) ;
	      *cp = 0;
	    }
	}

      if (cli)
        {
          /* uniformly append ONE line feed */
	  strcat (err_buf, "\n");
          syscall (SYS_write, 2, err_buf, strlen (err_buf));
        }
      else
        ix_panic (err_buf);
    }
  else
    syscall (SYS_write, 2, "^C\n", 3);

  exit2(W_EXITCODE(0, code));
  /* not reached */
}

/* this quite brute-force method, but the only thing I could think of that
 * really guarantees that there was a context switch...
 */

/* But I can think of something better: install a high-priority task when
 * the library is opened. That task always Wait()s on signal 1 << 31. So
 * when we want to make a context switch, we signal that task. Because of
 * the high priority of that task, exec.library switches to that task.
 * That task goes immediately back to Waiting for a signal, so
 * exec.library will go back to another task.
 *
 * Just in case this task has also a high priority, we keep around the
 * old method too.
 */
void force_task_switch(void)
{
  int curr_disp = SysBase->DispCount;
  Signal(ix.ix_task_switcher, 1 << 31);  /* signal the task switcher */
  while (curr_disp == ((volatile struct ExecBase *)SysBase)->DispCount) ;
}

/*
 * This is used to awaken a possibly sleeping sigsuspend()
 * and to force a context switch, if we send a signal to ourselves
 */
void
setrun (struct Task *t)
{
  struct user *p = getuser(t);
  struct Task *me = FindTask(0);
  u_int	sr;

  /* NOTE: the context switch is done to make sure sig_launch() is called as
   *       soon as possible in the respective task. It's not nice if you can
   *       return from a kill() to yourself, before the signal handler had a
   *       chance to react accordingly to the signal..
   */
  asm volatile (" 
    movel a5,a0
    lea	  L_get_sr,a5
    movel 4:w,a6
    jsr	  a6@(-0x1e)
    movel a1,%0
    bra	  L_skip
L_get_sr:
    movew sp@,a1	| get sr register from the calling function
    rte
L_skip:
    movel a0,a5
	" : "=g" (sr) : : "a0", "a1", "a6");

  /* Don't force context switch if:
     o  running in Supervisor mode
     o  we setrun() some other process
     o  running under either Forbid() or Disable() */
  if ((sr & 0x2000)
      || me != t
      || p->p_stat == SSLEEP
      || p->p_stat == SWAIT
      || p->p_stat == SSTOP
      || SysBase->TDNestCnt >= 0
      || SysBase->IDNestCnt >= 0)
    {
      extern int select();

      /* make testing of p_stat and reaction atomic */
      Forbid();

      if (p->p_stat == SWAIT)
        Signal (t, SIGBREAKF_CTRL_C);
      else if (p->p_stat == SSTOP)
        {
          p->p_stat = SRUN;
	  Signal (t, 1 << p->p_zombie_sig);
        }
      else if (p->p_wchan == (caddr_t) p)
	{
	  KPRINTF (("setrun $%lx\n", p));
          ix_wakeup ((u_int)p);
        }
      else if (p->p_stat == SSLEEP || p->p_wchan == (caddr_t) select)
        Signal (t, 1<<p->u_sleep_sig);
        
      Permit();
      return;
    }

  force_task_switch();
}

/*
 * Mapping from vector numbers into signals
 */
const static int hwtraptable[256] = {
  SIGILL, /* Reset initial stack pointer */
  SIGILL, /* Reset initial program counter */
  SIGBUS, /* Bus Error */
  SIGBUS, /* Address Error */
  SIGILL, /* Illegal Instruction */
  SIGFPE, /* Zero Divide */
  SIGFPE, /* CHK, CHK2 Instruction */
  SIGFPE, /* cpTRAPcc, TRAPcc, TRAPV Instruction */
  SIGILL, /* Privilege Violation */
  SIGTRAP,/* Trace */
  SIGEMT, /* Line 1010 Emulator */
  SIGEMT, /* Line 1111 Emulator */
  SIGILL,
  SIGILL, /* Coprocessor Protocol Violation */
  SIGILL, /* Format Error */
  SIGILL, /* Uninitialized Interrupt */
  SIGILL, /* 16 */
  SIGILL, /* 17 */
  SIGILL, /* 18 */
  SIGILL, /* 19 */		/* unimplemented, reserved */
  SIGILL, /* 20 */
  SIGILL, /* 21 */
  SIGILL, /* 22 */
  SIGILL, /* 23 */
  SIGILL, /* spurious Interrupt */
  SIGILL, /* Level 1 Interrupt Autovector */
  SIGILL, /* Level 2 Interrupt Autovector */
  SIGILL, /* Level 3 Interrupt Autovector */
  SIGILL, /* Level 4 Interrupt Autovector */
  SIGILL, /* Level 5 Interrupt Autovector */
  SIGILL, /* Level 6 Interrupt Autovector */
  SIGILL, /* Level 7 Interrupt Autovector */
  SIGTRAP, /* Trap #0 (not available on Unix) */
  SIGTRAP, /* Trap #1 */
  SIGILL, /* Trap #2 */
  SIGILL, /* Trap #3 */
  SIGILL, /* Trap #4 */
  SIGILL, /* Trap #5 */
  SIGILL, /* Trap #6 */
  SIGILL, /* Trap #7 */
  SIGILL, /* Trap #8 */
  SIGILL, /* Trap #9 */
  SIGILL, /* Trap #10 */
  SIGILL, /* Trap #11 */
  SIGILL, /* Trap #12 */
  SIGILL, /* Trap #13 */
  SIGILL, /* Trap #14 */
  SIGILL, /* Trap #15 (not available on Unix) */
  SIGFPE, /* FPCP Branch or Set on Unordererd Condition */
  SIGFPE, /* FPCP Inexact Result */
  SIGFPE, /* FPCP Divide by Zero */
  SIGFPE, /* FPCP Underflow */
  SIGFPE, /* FPCP Operand Error */
  SIGFPE, /* FPCP Overflow */
  SIGFPE, /* FPCP Signaling NAN */
  SIGILL,
  SIGBUS, /* MMU Configuration Error */
  SIGILL, /* MMU Illegal Operation (only 68851) */
  SIGILL, /* MMU Privilege Violation (only 68851) */
  /* rest undefined or free user-settable.. */
};

/*
 * handle traps handed over from the lowlevel trap handlers
 */
void
trap (void)
{
  u_int			format;
  void			*addr;
  struct reg		*regs;
  struct fpreg		*fpregs;
  struct Task 		*me = FindTask(0);
  /* precalculate struct user, so we don't have to go thru SysBase all the time */
  struct user 		*p = getuser(me);
  int 			sig;
  u_int			usp, orig_usp;
  struct sigcontext 	*sc;
  u_int			ret_pc, ret_ssp;
  extern long		vector_old_pc;
  extern long		vector_nop;

  usp = orig_usp = get_usp () + 8;	/* skip argument parameters */
  format = ((u_int *)usp)[0];
  addr = (void *)((u_int *)usp)[1];
  regs = (struct reg *)((u_int *)usp)[2];
  fpregs = (struct fpreg *)((u_int *)usp)[3];

  ret_pc  = ((u_int *)usp)[-2];
  ret_ssp = ((u_int *)usp)[-1];
  
  /* push a sigcontext that will get us back here if no other signals
   * were produced */
  usp -= sizeof (struct sigcontext);
  sc = (struct sigcontext *) usp;
  set_usp (usp);
  sc->sc_onstack = p->u_onstack;
  sc->sc_mask	 = p->p_sigmask;
  sc->sc_sp	 = orig_usp;
  sc->sc_fp	 = regs->r_regs[13];
  sc->sc_ap	 = *(u_int *)&me->tc_Flags;
  sc->sc_pc	 = ret_pc;
  sc->sc_ps	 = get_sr() & ~0x8000;

  if (regs->r_pc == (void *)(&vector_nop) + 2)
    {
      regs->r_pc = (void *)vector_old_pc;
      vector_old_pc = 0;
    }
  p->u_regs = regs;
  p->u_fpregs = fpregs;
  /* format contains the vector * 4, in the lower 12 bits */
  sig = *(int *)((u_char *)hwtraptable + (format & 0x0fff));
  
  if (sig == SIGTRAP)
    regs->r_sr &= ~0x8000;	/* turn off the trace flag */

  trapsignal (me, sig, format, addr);

  if ((sig = CURSIG(p)))
    psig (p, sig);

  /* now try to optimize. We could always call sup_do_sigreturn here, but if no
   * signals generated frames, we can just as well simply return, after having
   * restored our usp */
  if (usp == get_usp ())
    {
      set_usp (orig_usp);
      return;
    }
  sup_do_sigreturn_ssp (ret_ssp);
}

void resume_signal_check(void)
{
  int sig;
  usetup;

  if ((sig = issig(&u)))  /* always go through issig if we restart the process */
    psig (&u, sig);
}
