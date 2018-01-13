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
 *  tracecntl.c,v 1.1.1.1 1994/04/04 04:29:49 amiga Exp
 *
 *  tracecntl.c,v
 * Revision 1.1.1.1  1994/04/04  04:29:49  amiga
 * Initial CVS check in.
 *
 *  Revision 1.6  1993/11/05  22:03:32  mwild
 *  treat vfork2 same as vfork
 *
 *  Revision 1.5  1992/08/09  21:01:09  amiga
 *  import sysbase
 *
 *  Revision 1.4  1992/07/04  19:23:16  mwild
 *  double the number of passed parameters. Probably still not enough for weird
 *  cases, but I can't do much about that...
 *
 * Revision 1.3  1992/05/22  01:51:07  mwild
 * all common double returning functions are _JMP or they won't work
 *
 * Revision 1.2  1992/05/18  12:24:20  mwild
 * new way of getting at the result of a function. Do the call from
 * inside the handler and tell the library call hook to not call the
 * function again (TRACE_ACTION_RTS). Removed trace_exit() handler.
 *
 * Revision 1.1  1992/05/14  19:55:40  mwild
 * Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#include <sys/tracecntl.h>

#ifdef TRACE_LIBRARY

static struct List packets = {
  (struct Node *) &packets.lh_Tail, 0, (struct Node *) &packets.lh_Head,
};

static struct ix_mutex psem;

/* for each function traced, the trace_entry() function decides whether
   the trace_exit() function is invoked or not.
   If trace_entry() returns false, trace_exit() is not invoked.

   NOTE: having setjmp, vfork and the like invoke trace_exit will *NOT*
         work and will cause crashes!!

   Since I consider placing break points just to get the return value
   a bit overkill, I'll take a less optimal solution: I copy 16 args, this
   will do for 99% of all cases, and some nasty printf() style call will
   probably fail if tracing is enabled, so what ;-)) */

int
trace_entry (int scall, int (*func)(int, ...),
	     int t1, int t2, int t3, int t4, void *ret_addr,
	     int a1, int a2, int a3, int a4, int a5, int a6, int a7, int a8,
	     int a9, int aa, int ab, int ac, int ad, int ae, int af)
{
  struct trace_packet *tp, *ntp;
  struct Task *me = FindTask (0);
  struct user *u_ptr = getuser(me);
  int te_action, handler_active;

  /* for safety, don't do anything if running in Forbid() or Disable() */
  if (SysBase->TDNestCnt >= 0 || SysBase->IDNestCnt >= 0)
    return TRACE_ACTION_JMP;

  /* have to do this here, or the handler may get into a deadlock
     situation when trying to obtain the semaphore */
  if (u.u_trace_flags)
    return TRACE_ACTION_JMP;

  /* get default action value */  
  switch (scall)
    {
      case SYS_abort:
      case SYS_exit:
      case SYS_longjmp:
      case SYS_setjmp:		/* can return twice ! */
      case SYS_siglongjmp:
      case SYS_sigreturn:
      case SYS_sigsetjmp:	/* "" */
      case SYS__exit:
      case SYS__longjmp:
      case SYS__setjmp:		/* "" */
      case SYS_vfork:		/* "" */
      case SYS_ix_vfork:	/* "" */
      case SYS_ix_vfork_resume:	/* does longjmp-y thing... */
      case SYS_execve:
      case SYS_ix_geta4:	/* special, result is in A4, not D0 ;-)) */
      case SYS_ix_check_cpu:	/* may not return (but isn't used currently ;-)) */
      case SYS_ix_startup:	/* those two call longjmp thru _exit */
      case SYS_ix_exec_entry:
      case SYS_fork:
      case SYS_floor:		/* have all functions returning more than */
      case SYS_ceil:		/* 4 bytes be called JMP'y */
      case SYS_atof:
      case SYS_frexp:
      case SYS_modf:      
      case SYS_ldexp:
      case SYS_ldiv:		/* functions returning a pointer to a struct in a1 */
      case SYS_div:
      case SYS_inet_makeaddr:
      case SYS_atan ... SYS_fabs:	/* all the trigo stuff from *transbase.. */
      case SYS_strtod:
	te_action = TRACE_ACTION_JMP;
	break;

      default:
	te_action = TRACE_ACTION_JSR;
	break;
    }

  /* don't use syscall() here.. */
  ix_mutex_lock(&psem);
  handler_active = 0;
  for (tp  = (struct trace_packet *) packets.lh_Head;
       (ntp = (struct trace_packet *) tp->tp_message.mn_Node.ln_Succ);
       tp  = ntp)
    {
      if ((!tp->tp_pid || tp->tp_pid == (pid_t) me) &&
      	  (!tp->tp_syscall || tp->tp_syscall == scall))
	{
	  Remove ((struct Node *) tp);

	  tp->tp_is_entry = 1;
	  tp->tp_argv = &scall;
	  tp->tp_errno = u.u_errno;
	  /* provide the default for the handler to (possibly) override */
	  tp->tp_action = te_action;
	  /* wanted to use u.u_sync_mp here, but this leads to some
	     deadlock situations when the port is used for packets.. */
	  tp->tp_message.mn_ReplyPort = (struct MsgPort *) me;
	  SetSignal (0, SIGBREAKF_CTRL_E);
          PutMsg (tp->tp_tracer_port, (struct Message *) tp);
	  Wait (SIGBREAKF_CTRL_E);
	  /* the last handler wins ;-)) */
	  te_action = tp->tp_action;
	  handler_active = 1;

	  /* should be safe.. */
	  AddHead (&packets, (struct Node *) tp);
	}
    }
  ix_mutex_unlock(&psem);

  if (! handler_active)
    te_action = TRACE_ACTION_JMP;

  switch (te_action)      
    {
    case TRACE_ACTION_ABORT:
      abort();

    default:
    case TRACE_ACTION_JMP:
      return TRACE_ACTION_JMP;
      
    case TRACE_ACTION_JSR:
      {
        int result, error;

        /* we now know that there is at least one trace handler
           interested in this result, so do the extra overhead of
           calling with (excess) argument copying */
	result = func (a1, a2, a3, a4, a5, a6, a7, a8, a9, aa, ab, ac, ad, ae, af);
	error  = errno;

	/* replace the function address with the result */
	*(int *)&func = result;
	
	/* and repeat the process of calling the trace handler(s) */
        ix_mutex_lock(&psem);
        for (tp  = (struct trace_packet *) packets.lh_Head;
             (ntp = (struct trace_packet *) tp->tp_message.mn_Node.ln_Succ);
             tp  = ntp)
          {
            if ((!tp->tp_pid || tp->tp_pid == (pid_t) me) &&
      	        (!tp->tp_syscall || tp->tp_syscall == scall))
	      {
	        Remove ((struct Node *) tp);

		tp->tp_is_entry = 0;
		tp->tp_argv = &scall;
		errno = error;
		KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
		tp->tp_errno = u.u_errno;
		tp->tp_message.mn_ReplyPort = (struct MsgPort *) me;
		SetSignal (0, SIGBREAKF_CTRL_E);
		PutMsg (tp->tp_tracer_port, (struct Message *) tp);
		Wait (SIGBREAKF_CTRL_E);

		error = errno;
		/* should be safe.. */
		AddHead (&packets, (struct Node *) tp);
	      }
	  }
	ix_mutex_unlock(&psem);
	errno = error;
	KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      }
      /* fall into */

    case TRACE_ACTION_RTS:
      return TRACE_ACTION_RTS;
    }
}

#endif /* TRACE_LIBRARY */


int
ix_tracecntl (enum trace_cmd cmd, struct trace_packet *tp)
{
  usetup;

#ifndef TRACE_LIBRARY
  errno = ENOSYS;
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
  return -1;
#else
  switch (cmd)
    {
    case TRACE_INSTALL_HANDLER:
      tp->tp_message.mn_Node.ln_Type = NT_MESSAGE;
      tp->tp_message.mn_Length = sizeof (struct trace_packet);
      ix_mutex_lock(&psem);
      AddTail(&packets, (struct Node *)tp);
      ix_mutex_unlock(&psem);
      u.u_trace_flags = 1;
      return 0;

    case TRACE_REMOVE_HANDLER:
      ix_mutex_lock(&psem);
      Remove((struct Node *)tp);
      ix_mutex_unlock(&psem);
      u.u_trace_flags = 0;
      return 0;
      
    default:
      errno = EINVAL;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return -1;
    }
#endif
}
