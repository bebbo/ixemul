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
 *  $Id: vfork.c,v 1.2 2005/03/31 20:47:00 piru Exp $
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#include <sys/syscall.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <stddef.h>
#include <setjmp.h>
#include <string.h>
#include <stdlib.h>

#include <utility/tagitem.h>
#include <dos/dostags.h>

#ifdef MORPHOS
#include <emul/emulinterface.h>
#endif

#include "version.h"

#ifdef NATIVE_MORPHOS
void set_68k_sp(u_int sp) { REG_A7 = sp; }
u_int get_68k_sp(void) { return REG_A7; }
#endif

void vfork_own_malloc ();
void volatile vfork_longjmp (jmp_buf, int);
void ruadd(struct rusage *ru, struct rusage *ru2);

struct death_msg {
  struct ixnode         dm_node;
  struct Process        *dm_child;
  int                   dm_pgrp;
  int                   dm_status;
  struct rusage         dm_rusage;
};

/* this is the new process generated by vfork () ! */
static void
launcher (void)
{
  void *ixb;
  struct Process *me = (struct Process *) SysBase->ThisTask;
  struct user *u_ptr;
  struct vfork_msg *vm = NULL;
  int omask;

  KPRINTF(("vforked: opening ixemul\n"));
  //kprintf("%ld\n",me->pr_Task.tc_UserData);
  ixb = OpenLibrary ("ixemul.library", IX_VERSION);

  u_ptr = getuser(me);
  //kprintf("%ld\n",me->pr_Task.tc_UserData);
  KPRINTF(("vforked: waiting for startup msg\n"));

/*{int* p=(int*)get_sp();
KPRINTF(("r2=%lx ret=%lx\n",MyEmulHandle,(*(int**)p)[1]));
KPRINTF(("r1=%lx: %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	 p, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]));
p=*(int **)p;
KPRINTF(("nxt=%lx: %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	 p, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]));
p=(int*)get_68k_sp();
KPRINTF(("a7=%lx: %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	 p, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]));
}*/
  
  do
    {
      if (!vm)
	WaitPort (& me->pr_MsgPort);

      vm = (struct vfork_msg *) GetMsg (&me->pr_MsgPort);
    }
  while (!vm || vm->vm_self != me);

  KPRINTF(("vforked: initializing\n"));

  if (ixb)
    {
      /* get parents user area */
      volatile struct user *pu = getuser(vm->vm_pptr);
      /* `my' user area. This way we don't have to recalculate it too often */
      volatile struct user *mu = &u;
      /* reaping the dup function of execve() ;-)) */
      int fd;
      int a4_size = pu->u_a4_pointers_size * 4;

#ifdef NATIVE_MORPHOS
      mu->u_is_ppc = pu->u_is_ppc;
      KPRINTF(("is_ppc = %ld, u = %lx, pu = %lx, u->r13 = %lx, pu->r13 = %lx\n", mu->u_is_ppc, mu, pu, mu->u_a4, pu->u_a4));
#endif

      /* link ourselves into the parents process lists. Guarantee single
       * threaded access to those lists by locking out any other users of
       * the library (nicer than to just call Forbid()) */
      ix_lock_base ();

#ifdef NATIVE_MORPHOS
      vm->vm_clowerppc = (int)me->pr_Task.tc_ETask->PPCSPLower;
      vm->vm_cupperppc = (int)me->pr_Task.tc_ETask->PPCSPUpper;
#endif
      vm->vm_clower = (int)me->pr_Task.tc_SPLower;
      vm->vm_cupper = (int)me->pr_Task.tc_SPUpper;

      /* our older sybling is the last recently created child of the parent */
      mu->p_osptr = pu->p_cptr;
	  mu->u_parent_userdata = 0;
      /* we have no younger sybling */
      mu->p_ysptr = 0;
      /* if we have an older sybling, point its `younger sybling' field at us */
      if (mu->p_osptr)
	safe_getuser(mu->p_osptr)->p_ysptr = me;
      /* set the parents `last recently created child' field at us */
      pu->p_cptr = me;

      /* inherit the session of our parent */
      mu->u_session = pu->u_session;
      if (mu->u_session)
	mu->u_session->s_count++; /* and increase use count */

      /* inherit the process group of our parent */
      mu->p_pgrp = pu->p_pgrp;
      mu->p_pptr = vm->vm_pptr;

      /* inherit the uid/gid information from parent */
      mu->u_ruid = pu->u_ruid;
      mu->u_euid = pu->u_euid;
      mu->u_rgid = pu->u_rgid;
      mu->u_egid = pu->u_egid;

      if ((mu->u_ngroups = pu->u_ngroups))
	bcopy((char *)pu->u_grouplist, (char *)mu->u_grouplist, pu->u_ngroups * sizeof(int));

      if ((mu->u_logname_valid = pu->u_logname_valid))
	strcpy((char *)mu->u_logname, (char *)pu->u_logname);

      shmfork((struct user *)pu, (struct user *)mu);

      /* if we got our own malloc list already, it is safe to call malloc here.
	 If not, the stuff done here is postponed to either vfork_resume, or
	 execve */
      if (vm->vm_own_malloc)
	{
	  /* inherit these global variables. */
	  mu->u_environ = (char ***) malloc (4);
	  * mu->u_environ = dupvec (* pu->u_environ);
	  mu->u_errno = (int *) malloc (4);
	  *mu->u_errno = 0;
	  mu->u_h_errno = (int *) malloc (4);
	  *mu->u_h_errno = 0;
	}
      else
	{
	  /* borrow the variables of the parent */
	  mu->u_environ = pu->u_environ;
	  mu->u_errno = pu->u_errno;

	  /* tell malloc to use the parents malloc lists */
	  mu->u_mdp = pu->u_mdp;
	}

      /* and inherit several other things as well, upto not including u_md */
      bcopy ((void *)&pu->u_a4_pointers_size, (void *)&mu->u_a4_pointers_size,
	     offsetof (struct user, u_md) - offsetof (struct user, u_a4_pointers_size));
      bcopy ((char *)pu - a4_size, (char *)mu - a4_size, a4_size);

      /* some things have been copied that should be reset */
      mu->p_flag &= ~(SFREEA4 | STRC);
      mu->p_xstat = 0;
      bzero ((void *)&mu->u_ru, sizeof (struct rusage));
      bzero ((void *)&mu->u_prof, sizeof (struct uprof));
      mu->u_prof_last_pc = 0;
      bzero ((void *)&mu->u_cru, sizeof (struct rusage));
      bzero ((void *)&mu->u_timer[0], sizeof (struct itimerval)); /* just the REAL timer! */
      syscall (SYS_gettimeofday, & mu->u_start, 0);
      omask = vm->vm_rc;        /* signal mask to restore at the end */

      /* and adjust the open count of each of the copied filedescriptors */
      for (fd = 0; fd < NOFILE; fd++)
	if (mu->u_ofile[fd])
	{
	  /* obtain (and create a new fd) for INET sockets */
	  if (mu->u_ofile[fd]->f_type == DTYPE_SOCKET)
	  {
	    /* Was this socket released? */
	    if (mu->u_ofile[fd]->f_socket_id)
	    {
	      /* Yes, it was. So we now obtain a new socket from the underlying TCP/IP stack */
	      int newfd = syscall(SYS_ix_obtain_socket, mu->u_ofile[fd]->f_socket_id,
		  mu->u_ofile[fd]->f_socket_domain,
		  mu->u_ofile[fd]->f_socket_type,
		  mu->u_ofile[fd]->f_socket_protocol);

	      mu->u_ofile[fd]->f_socket_id = 0;
	      /* move the newly created fd to the old one */
	      if (newfd != -1)
		{
		  /* Tricky bit: we have to remember that for dupped sockets all file descriptors
		     point to the same file structure: obtaining one released file descriptor will
		     obtain them all. So we have to scan for file descriptors that point to the
		     same file structure for which we just obtained the socket and copy the new
		     file structure into the dupped file descriptors.

		     Note that the newly created file structure has the field f_socket_id set to 0,
		     so we won't come here again because of the if-statement above that tests whether
		     the socket was released. */
		  int fd2;

		  /* scan the file descriptors */
		  for (fd2 = fd + 1; fd2 < NOFILE; fd2++)
		    {
		      /* do they point to the same file structure? */
		      if (mu->u_ofile[fd] == mu->u_ofile[fd2])
		      {
			/* in that case copy the newly obtained socket into this file descriptor
			   and increase the open count */
			mu->u_ofile[fd2] = mu->u_ofile[newfd];
			mu->u_ofile[newfd]->f_count++;
		      }
		    }
		  /* and finally do the same for fd */
		  mu->u_ofile[fd] = mu->u_ofile[newfd];
		  mu->u_ofile[newfd] = 0;
		}
	    }
	  }
	  else
	    mu->u_ofile[fd]->f_count++;
	}
      /* also copy u_segs, after all, the child will run initially in the
	 same SegList as the parent. */
      mu->u_segs = pu->u_segs;
      mu->u_start_pc = pu->u_start_pc;
      mu->u_end_pc = pu->u_end_pc;

      mu->u_is_root = pu->u_is_root;
      mu->u_a4 = pu->u_a4;

      /* copying finished, allow other processes to vfork() as well ;-)) */
      ix_unlock_base ();
 
      /* remember the message we have to reply when either _exit() or
       * execve() is called */
      mu->p_vfork_msg = vm;

      vm->vm_rc = 0;

#ifdef NATIVE_MORPHOS
      mu->u_save_sp = mu->u_is_ppc ? (void *) get_sp () : (void *) get_68k_sp();
#else
      mu->u_save_sp = (void *) get_sp ();
#endif
      KPRINTF(("sp = %lx\n", mu->u_save_sp));
/*#ifdef NATIVE_MORPHOS
      mu->u_save_68k_sp = (void *) get_68k_sp ();
      KPRINTF(("r1 = %lx, a7 = %lx\n", mu->u_save_sp, mu->u_save_68k_sp));
#endif*/

/*{int* p=(int*)get_sp();
KPRINTF(("r1=%lx: %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	 p, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]));
p=(int*)get_68k_sp();
KPRINTF(("a7=%lx: %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	 p, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]));
}*/
      /* we get here when the user does an _exit()
       * (so as well after execve() terminates !) */
      if (_setjmp ((void *)mu->u_jmp_buf))
	{
	  int i;

/*{int* p=(int*)get_sp();
KPRINTF(("r1=%lx: %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	 p, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]));
p=(int*)get_68k_sp();
KPRINTF(("a7=%lx: %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	 p, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]));
}*/
	  /* reset `mu' in here, setjmp() might have clobbered it */
	  me = (struct Process *)SysBase->ThisTask;
	  u_ptr = getuser(me);
	  mu = &u;

	  if (mu->p_vfork_msg)
	    {
#ifdef NATIVE_MORPHOS
	      me->pr_Task.tc_ETask->PPCSPLower = (void *)mu->p_vfork_msg->vm_clowerppc;
	      me->pr_Task.tc_ETask->PPCSPUpper = (void *)mu->p_vfork_msg->vm_cupperppc;
#endif
	      me->pr_Task.tc_SPLower = (void *)mu->p_vfork_msg->vm_clower;
	      me->pr_Task.tc_SPUpper = (void *)mu->p_vfork_msg->vm_cupper;
	    }

	  /* overkill? */
	  vfork_own_malloc ();

	  /* although this is done in CloseLibrary(), files should
	     really be closed *before* a death-message is sent to
	     the parent. */
	  for (i = 0; i < NOFILE; i++)
	    if (u.u_ofile[i])
	      syscall (SYS_close, i);

	  /* free memory (look that most, best all memory is freed here, as
	     long as we're not inside Forbid. If memory is freed in CloseLibrary,
	     it may potentially have to wait for the memory semaphore in buddy-alloc.c,
	     thus breaking the Forbid! */
	  all_free ();

	  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));

	  /* this whole thing only happens if our parent is still alive ! */
	  Forbid ();
	  KPRINTF(("p_ptr = %lx\n", mu->p_pptr));
	  if (mu->p_pptr && mu->p_pptr != (struct Process *) 1)
	    send_death_msg((struct user *)mu);
	  else
	    {
	      KPRINTF (("vforked: couldn't send death_msg\n"));
	    }
	  KPRINTF (("vforked: now closing library\n"));
	  CloseLibrary (ixb);

/*{int* p=(int*)get_sp();
KPRINTF(("r2=%lx ret=%lx\n",MyEmulHandle,(*(int**)p)[1]));
KPRINTF(("r1=%lx: %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	 p, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]));
p=*(int **)p;
KPRINTF(("nxt=%lx: %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	 p, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]));
p=(int*)get_68k_sp();
KPRINTF(("a7=%lx: %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	 p, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]));
}*/
	  KPRINTF (("vforked: falling off the edge of the world.\n"));
	  /* just fall off the edge of the world, this is a process */
	  return;
	}

      syscall (SYS_sigsetmask, omask);

      KPRINTF (("vforked: jumping back\n"));

      vm = *&vm; /* uh ? */

#ifdef NATIVE_MORPHOS
      /* don't change r2 */
      vm->vm_regs->jb[3] = (LONG)MyEmulHandle;

      if (mu->u_is_ppc)
	{
	  /* do a ppc longjmp() without restoring the 68k context */
	  me->pr_Task.tc_ETask->PPCSPLower = (void *)vm->vm_plower;
	  me->pr_Task.tc_ETask->PPCSPUpper = (void *)vm->vm_pupper;
	  /* don't change a7 */
	  vm->vm_regs->jb[75] = get_68k_sp();
	  /* jump into nevereverland ;-) */
	  vfork_longjmp (vm->vm_regs->jb, 0);
	  /* NOTREACHED */
	}
      else
	{
	  /* do a 68k longjmp() without restoring the ppc context */
	  /* binary code for:
		move.l  a0,a7
		rts
	  */
	  static const UWORD gate[] = { 0x2E48, 0x4E75 };
	  me->pr_Task.tc_SPLower = (void *)vm->vm_plower;
	  me->pr_Task.tc_SPUpper = (void *)vm->vm_pupper;
	  REG_D0 = 0;
	  REG_A0 = vm->vm_regs->jb[75]; /* a7 */
	  memcpy(&REG_D2, &vm->vm_regs->jb[64], 6*4);
	  memcpy(&REG_A2, &vm->vm_regs->jb[70], 5*4);
KPRINTF(("r1 = %lx, a7 = %lx\n", get_sp(), REG_A0));
{int* p=REG_A0;
KPRINTF(("%08lx %08lx %08lx %08lx %08lx %08lx\n",p[0],p[1],p[2],p[3],p[4],p[5]));}
	  (void)MyEmulHandle->EmulCallDirect68k((APTR)gate);
	  /* NOTREACHED */
	}
#else
      me->pr_Task.tc_SPLower = (void *)vm->vm_plower;
      me->pr_Task.tc_SPUpper = (void *)vm->vm_pupper;
      /* jump into nevereverland ;-) */
      vfork_longjmp (vm->vm_regs->jb, 0);
      /* NOTREACHED */
#endif
    }

  Forbid();
  vm->vm_rc = ENOMEM; /* can't imagine any other reason why the OpenLib should fail */
  ReplyMsg ((struct Message *) vm);
  /* fall off the edge of the world ;-) */
}

#ifdef NATIVE_MORPHOS
struct EmulLibEntry _gate_launcher = {
  TRAP_LIBNR, 0, (void(*)())launcher
};
#endif

void
send_death_msg(struct user *mu)
{
  struct death_msg *dm = 0;
  struct user *pu = safe_getuser(mu->p_pptr);

  /* We are in forbid state */

  /* KPRINTF (("vforked: parent alive, zombie-sig = %ld, vfork_msg = $%lx.\n",
		 pu->p_zombie_sig, mu->p_vfork_msg));*/

  /* send the parent a death message with our return code */
  dm = (struct death_msg *) kmalloc (sizeof (struct death_msg));

  if (dm)
    {
      dm->dm_status = mu->p_xstat;
      dm->dm_rusage = mu->u_ru;
      ruadd (&dm->dm_rusage, (struct rusage *)&mu->u_cru);
      dm->dm_child = (struct Process *) SysBase->ThisTask;
      dm->dm_pgrp  = mu->p_pgrp;
      KPRINTF (("vfork-exit: Adding child $%lx to $%lx\n", dm->dm_child, mu->p_pptr));
      ixaddtail ((struct ixlist *) &pu->p_zombies, (struct ixnode *) dm);
    }

  _psignal ((struct Task *)mu->p_pptr, SIGCHLD);

  /* have to wakeup the parent `by hand' to make sure it gets
     out of its sleep, since it might have SIGCHLD masked out or
     ignored at the moment */
  if (pu->p_stat == SSLEEP && pu->p_wchan == (caddr_t) pu)
{KPRINTF(("ix_wakeup()\n"));
    ix_wakeup ((u_int)pu);}

  if (mu->p_vfork_msg)
{KPRINTF(("replymsg()\n"));
    ReplyMsg ((struct Message *) mu->p_vfork_msg);}

  /* this is necessary for process synchronisation, this process
     will be unlinked from the process chain by wait4(), which will
     also take care of reparenting the process if it was PT_ATTACHed
      by a debugger */
  if (dm)
    ix_sleep ((caddr_t)dm, "vfork-dm");
KPRINTF(("end send_death_msg()\n"));
}

/* This function is used by vfork_resume and execve. Perhaps it should be made
   externally available? It causes the process to switch to its own malloc
   list, and copies errno and environ into private space. */
void
vfork_own_malloc (void)
{
  usetup;
  /* use volatile here, or the compiler might do wrong `optimization' .. */
  volatile struct user *p = &u;

  if (p->u_mdp != &p->u_md)
    {
      char **parent_environ = *p->u_environ;

      /* switch to our memory list (which is initialized by OpenLibrary) */
      p->u_mdp = (void *)&p->u_md;
      /* dupvec now uses malloc() on our list */
      p->u_environ = (char ***) malloc (4);
      *p->u_environ = dupvec (parent_environ);
      p->u_errno = (int *) malloc (4);
      *p->u_errno = 0;
      p->u_h_errno = (int *) malloc (4);
      *p->u_h_errno = 0;
    }
}

#define _MAKESTR(o) #o
#define MAKESTR(o) _MAKESTR(o)

#ifdef NATIVE_MORPHOS

asm("
	.globl vfork
	.globl ix_vfork
	.globl ix_vfork_resume

vfork:
	/* store a setjmp () compatible frame on the stack to pass to _vfork () */
	stwu    1,-384(1)               /* _JBLEN longs + prolog on the stack */
	mflr    0
	addi    3,1,8
	stw     0,388(1)
	bl      setjmp
	/* now patch sp and pc, since they differ */
	lwz     0,388(1)
	addi    4,1,384                 /* account for buffer space */
	stw     0,28(1)                 /* insert real PC */
	stw     4,16(1)
	/* tell _vfork *not* yet to switch to own malloc-list */
	li      3,0
	addi    4,1,8
	b       _vfork
/*        bl      _vfork
	lwz     0,388(1)
	mtlr    0
	addi    1,1,384
	stw     3,0(2)
	blr*/

/*_trampoline_vfork:
	stwu    1,-80(1)
	mflr    0
	stmw    16,8(1)
	stw     0,84(1)
	lmw     16,0(2)
	bl      vfork
	lwz     31,60(2)
	mr      16,3
	lwz     0,84(1)
	stmw    16,0(2)
	mtlr    0
	lmw     16,8(1)
	addi    1,1,80
	blr */

ix_vfork:
	/* this is the vfork used in older versions of the library */
	stwu    1,-384(1)               /* _JBLEN longs + prolog on the stack */
	mflr    0
	addi    3,1,8
	stw     0,388(1)
	bl      setjmp
	/* now patch sp and pc, since they differ */
	lwz     0,388(1)
	addi    4,1,384                 /* account for buffer space */
	stw     0,28(1)                 /* insert real PC */
	stw     4,16(1)
	/* tell _vfork to have the child run with own malloc-list. */
	li      3,1
	addi    4,1,8
	b       _vfork
/*        bl      _vfork
	lwz     0,388(1)
	mtlr    0
	addi    1,1,384
	stw     3,0(2)
	blr*/

/*_trampoline_ix_vfork:
	stwu    1,-80(1)
	mflr    0
	stmw    16,8(1)
	stw     0,84(1)
	lmw     16,0(2)
	bl      ix_vfork
	lwz     31,60(2)
	mr      16,3
	lwz     0,84(1)
	stmw    16,0(2)
	mtlr    0
	lmw     16,8(1)
	addi    1,1,80
	blr  */

	/* the following is longjmp(), with the subtle difference that this */
	/* thing doesn't insist in returning something non-zero... */
vfork_longjmp:
	lwz     5,8(3)
	cmpi    0,5,0           /* ensure non-zero SP */
	beq-    Lbotch          /* oops! */
	lwz     2,12(3)
	lmw     14,304(3)       /* fp2-fp7 */
	lwz     6,252(3)        /* *a7 */
	stmw    14,0x88(2)
	lmw     26,280(3)       /* a2-a7 */
	lwz     5,248(3)        /* pc */
	stmw    26,40(2)
	stw     6,0(31)
	lmw     26,256(3)       /* d2-d7 */
	stmw    26,8(2)
	lmw     12,24(3)
	lfd     14,104(3)
	lfd     15,112(3)
	lfd     16,120(3)
	lfd     17,128(3)
	lfd     18,136(3)
	lfd     19,144(3)
	lfd     20,152(3)
	lfd     21,160(3)
	lfd     22,168(3)
	lfd     23,176(3)
	lfd     24,184(3)
	lfd     25,192(3)
	lfd     26,200(3)
	lfd     27,208(3)
	lfd     28,216(3)
	lfd     29,224(3)
	lfd     30,232(3)
	lfd     31,240(3)
	mtcr    12
	b       sigreturn

Lbotch:
	b       longjmperror

ix_vfork_resume:
	/* pass the stack pointer */
	lwz     3,0(1) /* lr will be in the next frame. r1+8 would probably be enough */
	addi    3,3,12
	b       _vfork_resume

_trampoline_ix_vfork_resume:
	stwu    1,-64(1)
	mflr    0
	stmw	18,8(1)
	lwz     3,60(2)         /* r3 = a7 */
	lmw     18,8(2)		/* save 68k regs */
	stw     0,68(1)
	addi    3,3,128         /* add some space to be a little safer */
	bl      _vfork_resume
	lwz	31,60(2)	/* r31 = a7. It was changed, don't restore it. */
        lwz     0,68(1)
	stmw	18,8(2)		/* restore 68k regs */
	mtlr    0
	lmw	18,8(1)
	addi    1,1,64
	blr
");

pid_t vfork(void);
pid_t ix_vfork(void);
void ix_vfork_resume(void);
/*void _trampoline_vfork(void);
void _trampoline_ix_vfork(void);*/
void _trampoline_ix_vfork_resume(void);

struct EmulLibEntry _gate_vfork = {
  TRAP_LIBRESTORE, 0, (void(*)())/*_trampoline_*/vfork
};

struct EmulLibEntry _gate_ix_vfork = {
  TRAP_LIBRESTORE, 0, (void(*)())/*_trampoline_*/ix_vfork
};

struct EmulLibEntry _gate_ix_vfork_resume = {
  TRAP_LIBRESTORE, 0, (void(*)())_trampoline_ix_vfork_resume
};

#else

asm (" \n\
	.globl _vfork \n\
	.globl _ix_vfork \n\
	.globl _ix_vfork_resume \n\
_vfork: \n\
	| store a setjmp () compatible frame on the stack to pass to _vfork () \n\
	lea     sp@(-18*4),sp           | _JBLEN (17) longs on the stack \n\
	pea     sp@ \n\
	jbsr    _setjmp \n\
	addqw   #4,sp \n\
	| now patch sp and pc, since they differ \n\
	addl    #20*4,sp@(8)            | account for buffer space \n\
	movel   sp@(18*4),sp@(20)       | insert real PC (return addr on stack) \n\
	| tell _vfork *not* yet to switch to own malloc-list \n\
	pea     0:W \n\
	bsr     __vfork \n\
	lea     sp@(18*4 + 4),sp \n\
	rts \n\
\n\
_ix_vfork: \n\
	| this is the vfork used in older versions of the library \n\
	lea     sp@(-18*4),sp           | _JBLEN (17) longs on the stack \n\
	pea     sp@ \n\
	jbsr    _setjmp \n\
	addqw   #4,sp \n\
	| now patch sp and pc, since they differ \n\
	addl    #20*4,sp@(8)            | account for buffer space \n\
	movel   sp@(18*4),sp@(20)       | insert real PC (return addr on stack) \n\
	| tell _vfork to have the child run with own malloc-list. \n\
	pea     1:W \n\
	bsr     __vfork \n\
	lea     sp@(18*4 + 4),sp \n\
	rts \n\
\n\
	| the following is longjmp(), with the subtle difference that this \n\
	| thing doesn't insist in returning something non-zero... \n\
_vfork_longjmp: \n\
	movel   sp@(4),a0       /* save area pointer */ \n\
	tstl    a0@(8)          /* ensure non-zero SP */ \n\
	jeq     Lbotch          /* oops! */ \n\
	movel   sp@(8),d0       /* grab return value */ \n\
	moveml  a0@(28),d2-d7/a2-a4/a6  /* restore non-scratch regs */ \n\
	movel   a0,sp@-         /* let sigreturn */ \n\
	jbsr    _sigreturn      /*   finish for us */ \n\
\n\
Lbotch: \n\
	jbsr    _longjmperror \n\
	stop    #0 \n\
\n\
_ix_vfork_resume: \n\
	pea     sp@             | pass the sp containing the return address \n\
	bsr     __vfork_resume \n\
	addqw   #4,sp \n\
	rts \n\
"); 
#endif

/*
 * this is an implementation extension to the `real' vfork2(). Normally you
 * can only cause the parent to resume by calling _exit() or execve() from
 * the child. Since I can't provide a real fork() on the Amiga, this function
 * is a third possibility to make the parent resume. You have then two
 * concurrent processes sharing the same frame and global data... Please be
 * EXTREMELY careful what you may do and what not. vfork2() itself is a hack,
 * this is an even greater one...
 *
 * DO NOT use this function in combination with vfork(), or you'll get a big
 * memory leak. Only use it with vfork2().
 */

/*
 * don't make this function static, gcc doesn't notice that it's used from the
 * assembly code above, so make sure it isn't optimized away...
 */
#ifdef NATIVE_MORPHOS
static void _reply_msg(struct Message *msg)
{
    ReplyMsg(msg);
}
#endif

void
_vfork_resume (u_int *copy_from_sp)
{
  struct Process *me = (struct Process *) SysBase->ThisTask;
  struct user *u_ptr = getuser(me);
  /* vm is used after stack-switching, put it in a register */
#ifdef NATIVE_MORPHOS
  register struct vfork_msg **vm asm("r14") = &u.p_vfork_msg;
  ULONG save[13];
  int k;

  for (k = 2; k < 15; ++k)
    save[k - 2] = ((ULONG *)MyEmulHandle)[k];
#else
  register struct vfork_msg **vm asm("a2") = &u.p_vfork_msg;
#endif

/*  KPRINTF(("resume: me = %lx, vm = %lx, u_ptr = %lx\n", me, vm, u_ptr));
{int* p=(int*)get_sp();
KPRINTF(("r1=%lx: %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	 p, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]));
p=(int*)get_68k_sp();
KPRINTF(("a7=%lx: %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	 p, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]));
p=(int*)u.u_save_sp;
KPRINTF(("orig a7=%lx: %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	 p, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]));
}*/

  if (*vm)
    {
      u_int *sp = (u_int *)u.u_save_sp;
      u_int *copy_till_sp;
      u_int *p;
#ifdef NATIVE_MORPHOS
/*      u_int *sp_68k = (u_int *)u.u_save_68k_sp;
      u_int *copy_till_68k_sp = (u_int *)get_68k_sp ();
      u_int *copy_from_68k_sp = copy_from_sp;*/
      int offset;
/*      copy_from_sp = (u_int *)getuser((*vm)->vm_pptr)->u_vfork_frame[2] + 4;
KPRINTF(("r1 = %lx, save_r1 = %lx, a7 = %lx, save_a7 = %lx\n", copy_till_sp, sp, copy_till_68k_sp, sp_68k));*/
      offset = (int)sp - (int)copy_from_sp - 4;
      if (u.u_is_ppc)
	{
	  copy_till_sp = (u_int *)get_sp ();
	}
      else
	{
	  copy_till_sp = (u_int *)get_68k_sp();
	}
#else 
      copy_till_sp = (u_int *)get_sp ();
#endif
  
      /* be sure to switch to our memory list */
      vfork_own_malloc ();
KPRINTF(("memlist switched\n"));

      /* copy the stack frame */
KPRINTF(("copy stack %lx-%lx to %lx\n", copy_till_sp, copy_from_sp, sp));
      p = copy_from_sp + 1;
      do
	*--sp = *--p;
      while (p > copy_till_sp);
#ifdef NATIVE_MORPHOS
      if (u.u_is_ppc)
	{
	  /* Major hack below. If that works, we're lucky ! */
	  /* adjust the frame pointers */
	  p = sp;
	  while (*p >= (u_int)copy_till_sp && *p < (u_int)copy_from_sp)
	    {
	      KPRINTF(("adjusting frame %lx -> %lx\n", *p, *p+offset));
	      *p += offset;
	      p = (u_int *)*p;
	    }

	  /* if the EmulHandle was in the copied part of the stack, adjust the pointer */
	  if ((u_int *)MyEmulHandle < copy_from_sp &&
	      (u_int *)MyEmulHandle > copy_till_sp)
{           MyEmulHandle = (void *)((int)MyEmulHandle + offset);
KPRINTF(("EmulHandle adjusted: %lx\n", MyEmulHandle));}

/*KPRINTF(("ppc stack:\n"));
p = sp;
while (p < sp + 256)
  {
    KPRINTF(("%08lx: %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	     p,p[0],p[1],p[2],p[3],p[4],p[5],p[6],p[7]));
    p+=8;
  }*/
	}

KPRINTF(("state: r1=%lx, lr=%lx\n", get_sp(), __builtin_return_address(0)));
#endif

      me->pr_Task.tc_SPLower = (void *)(*vm)->vm_clower;
      me->pr_Task.tc_SPUpper = (void *)(*vm)->vm_cupper;
#ifdef NATIVE_MORPHOS
      me->pr_Task.tc_ETask->PPCSPUpper = (void *)(*vm)->vm_cupperppc;
      me->pr_Task.tc_ETask->PPCSPLower = (void *)(*vm)->vm_clowerppc;
      if (u.u_is_ppc)
	{
	  set_sp ((u_int) sp);
	}
      else
	{
	  set_68k_sp ((u_int) sp);
	}
KPRINTF(("stack switched: r1=%lx, lr=%lx, a7=%lx\n", get_sp(), __builtin_return_address(0), get_68k_sp()));
#else
      set_sp ((u_int) sp);
#endif
/*{int* p=(int*)get_sp();
KPRINTF(("r1=%lx: %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	 p, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]));
p=(int*)get_68k_sp();
KPRINTF(("a7=%lx: %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	 p, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]));
}*/

#ifdef NATIVE_MORPHOS
      _reply_msg ((struct Message *) *vm);
#else
      ReplyMsg ((struct Message *) *vm);
#endif
      *vm = 0;
    }
/*{int* p=(int*)get_sp();
KPRINTF(("r1=%lx: %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	 p, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]));
p=(int*)get_68k_sp();
KPRINTF(("a7=%lx: %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	 p, p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]));
}*/
#ifdef NATIVE_MORPHOS
    for (k = 2; k < 15; ++k)
      ((ULONG *)MyEmulHandle)[k] = save[k - 2];
#endif
KPRINTF(("done\n"));
}

#define STACKNAME ("IXSTACK")   /* Environment variable for stack size */

static int get_stack_size(struct Process *proc)
{
  int stack_size,stack_size2=512000;
  char *tmp;
  struct CommandLineInterface *CLI = BTOCPTR (proc->pr_CLI);
  if ((tmp = getenv (STACKNAME)))stack_size2 = atoi (tmp);
    stack_size = CLI ? CLI->cli_DefaultStack * 4 : proc->pr_StackSize;
	if (stack_size2 > stack_size)stack_size = stack_size2;
  if (stack_size < STACKSIZE)
    return STACKSIZE;
  return stack_size; 
  
}

/*
 * don't make this function static, gcc doesn't notice that it's used from the
 * assembly code above, so make sure it isn't optimized away...
 */
int
_vfork (int own_malloc, struct reg_parms rp)
{
  struct Process *me = (struct Process *) SysBase->ThisTask;
  struct user *u_ptr = getuser(me);
  struct CommandLineInterface *CLI = (void *)(me->pr_CLI);
  u_int stack_size = get_stack_size(me);
  u_int plower, pupper;
/*#ifdef NATIVE_MORPHOS
  u_int plowerppc, pupperppc;
#endif*/
  /* those *have* to be in registers to survive the stack deallocation */
#ifndef __PPC__
  register struct vfork_msg *vm asm ("a2");
#else
  register struct vfork_msg *vm asm ("r14");
  register u_int return_addr_68k asm ("r15");
#endif
  struct Process *child;
  int i;
  
  struct TagItem tags [] = {
#ifdef NATIVE_MORPHOS
    { NP_Entry, (ULONG) &_gate_launcher },
#else
    { NP_Entry, (ULONG) launcher },
#endif
    { NP_Name, (ULONG) "vfork()'d process" },   /* to be overridden by execve() */
    { NP_StackSize, stack_size },               /* same size we use */
    { NP_Cli, TRUE },         /* same thing we are */
	//{ NP_Cli, (ULONG) (CLI ? -1 : 0) },         /* same thing we are */
    { TAG_END, 0 }
  };

KPRINTF(("vfork: user=%lx, a4_size=%ld, a4=%lx\n",u_ptr,u_ptr->u_a4_pointers_size*4,u_ptr->u_a4));
  KPRINTF (("vfork: creating child with stacksize = $%lx[$%lx,$%lx]\n", stack_size, tags[3]));
  vm = (struct vfork_msg *) kmalloc (sizeof (struct vfork_msg));
  if (!vm)
    {
      errno = ENOMEM;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      vfork_longjmp (u.u_vfork_frame, -1);
      return -1; /* not reached */
    }

  bzero(vm, sizeof(*vm));
  vm->vm_msg.mn_ReplyPort = u.u_sync_mp;
  vm->vm_msg.mn_Node.ln_Type = NT_MESSAGE;
  vm->vm_msg.mn_Length = sizeof (struct vfork_msg);
  vm->vm_pptr = me;
  vm->vm_own_malloc = own_malloc;
  vm->vm_regs = &rp;
#ifdef NATIVE_MORPHOS
  if (u.u_is_ppc)
    {
      vm->vm_plower = (int)me->pr_Task.tc_ETask->PPCSPLower;
      vm->vm_pupper = (int)me->pr_Task.tc_ETask->PPCSPUpper;
    }
  else
    {
      vm->vm_plower = (int)me->pr_Task.tc_SPLower;
      vm->vm_pupper = (int)me->pr_Task.tc_SPUpper;
      return_addr_68k = *(u_int *)get_68k_sp();
    }
#else
  vm->vm_plower = (int)me->pr_Task.tc_SPLower;
  vm->vm_pupper = (int)me->pr_Task.tc_SPUpper;
#endif
  /* we have to block all signals as long as the child uses our resources.
   * but since the child needs to start with the signal mask BEFORE this
   * general blocking, we have to pass it the old signal mask. This is a
   * way to do it */
  vm->vm_rc = vm->vm_omask = syscall (SYS_sigsetmask, ~0);

  /* save the passed frame in our user structure, since the child will
     deallocate it from the stack when it `returns' to user code */
  bcopy (&rp, &u.u_vfork_frame, sizeof (rp));

  /* if some fd's are sockets, release them so the child can obtain them */
  for (i = 0; i < NOFILE; i++)
    {
      if (u.u_ofile[i] && u.u_ofile[i]->f_type == DTYPE_SOCKET)
	u.u_ofile[i]->f_socket_id = syscall(SYS_ix_release_socket, i);
    }
 
  KPRINTF (("vfork: creating child with stacksize = $%lx[$%lx,$%lx]\n", stack_size, tags[3]));
  vm->vm_self = CreateNewProc (tags);

  if (!vm->vm_self)
    {
      /* do I have to close input/output here? Or does the startup close
	 them no matter whether it succeeds or not ? */
      syscall (SYS_sigsetmask, vm->vm_omask);
      kfree (vm);
      errno = EPROCLIM;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      vfork_longjmp (u.u_vfork_frame, -1);
      return -1; /* not reached */
    }

  /* As soon as this message is dispatched, the child will `return' and
     deallocate the stack we're running on. So afterwards, *only* use
     register variables and then longjmp () back.
     Since we don't have a stack until after the longjmp(), temporarily
     switch to our mini-stack */
#ifdef NATIVE_MORPHOS
  if (u.u_is_ppc)
    {
      me->pr_Task.tc_ETask->PPCSPLower = (void *)(((int)u.u_mini_stack));
      me->pr_Task.tc_ETask->PPCSPUpper = (void *)(((int)u.u_mini_stack) + sizeof(u.u_mini_stack));
      set_sp(((u_int)me->pr_Task.tc_ETask->PPCSPUpper & ~15) - 512);
    }
  else
    {
	
      me->pr_Task.tc_SPLower = (void *)u.u_mini_stack;
      me->pr_Task.tc_SPUpper = (void *)(((int)u.u_mini_stack) + sizeof(u.u_mini_stack));
      set_68k_sp((u_int)me->pr_Task.tc_SPUpper);
    }
#else
  	
  me->pr_Task.tc_SPLower = (void *)u.u_mini_stack;
  me->pr_Task.tc_SPUpper = (void *)(((int)u.u_mini_stack) + sizeof(u.u_mini_stack));
  set_sp((u_int)me->pr_Task.tc_SPUpper);  // need to be inline, so crash when not optimize
#endif
 
  PutMsg (&vm->vm_self->pr_MsgPort, (struct Message *) vm);

  /* wait until the child does execve() or _exit() */
  KPRINTF (("waiting for child\n"));
  WaitPort (getuser(vm->vm_pptr)->u_sync_mp);
  GetMsg (getuser(vm->vm_pptr)->u_sync_mp);
  
  KPRINTF (("child returned or called execve()\n"));
  syscall (SYS_sigsetmask, vm->vm_omask);

  plower = vm->vm_plower;
  pupper = vm->vm_pupper;
  me = vm->vm_pptr;

  child = vm->vm_self;
  // restore u_ptr
  u_ptr = getuser(me);

  if (vm->vm_rc)
    {
      errno = (int) vm->vm_rc;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      vm->vm_self = (struct Process *) -1;
    }

  /* this is the parent return, so we pass the id of the new child */
  kfree (vm);

#ifdef NATIVE_MORPHOS
  if (u.u_is_ppc)
    {
      me->pr_Task.tc_ETask->PPCSPLower = (void *)plower;
      me->pr_Task.tc_ETask->PPCSPUpper = (void *)pupper;
    }
  else
    {
      me->pr_Task.tc_SPLower = (void *)plower;
      me->pr_Task.tc_SPUpper = (void *)pupper;
      *(u_int *)u.u_vfork_frame[75] = return_addr_68k;
    }
#else
  me->pr_Task.tc_SPLower = (void *)plower;
  me->pr_Task.tc_SPUpper = (void *)pupper;
#endif

  KPRINTF(("returning\n"));
  /* could use longjmp() here, but since we already *have* the local one.. */
  vfork_longjmp (u.u_vfork_frame, (int) child);
  return 0;  /* not reached */
}

void ruadd(struct rusage *ru, struct rusage *ru2)
{
	register long *ip, *ip2;
	register int i;

	timevaladd(&ru->ru_utime, &ru2->ru_utime);
	timevaladd(&ru->ru_stime, &ru2->ru_stime);
	if (ru->ru_maxrss < ru2->ru_maxrss)
		ru->ru_maxrss = ru2->ru_maxrss;
	ip = &ru->ru_first; ip2 = &ru2->ru_first;
	for (i = &ru->ru_last - &ru->ru_first; i > 0; i--)
		*ip++ += *ip2++;
}

/*
 * make process 'parent' the new parent of process 'child'.
 */
void
proc_reparent(struct Process *child, struct Process *parent)
{
	register struct Process *o;
	register struct Process *y;
	struct user *cu = safe_getuser(child);
	struct user *pu = (parent != (struct Process *)1 ? safe_getuser(parent) : 0);

	if (cu->p_pptr == parent)
	  return;

	KPRINTF(("proc_reparent(%lx, %lx)\n", child, parent));

	/* fix up the child linkage for the old parent */
	o = cu->p_osptr;
	y = cu->p_ysptr;
	if (y)
	  safe_getuser(y)->p_osptr = o;
	if (o)
	  safe_getuser(o)->p_ysptr = y;
	if (cu->p_pptr && cu->p_pptr != (struct Process *)1)
	  {
	    struct user *tu = safe_getuser(cu->p_pptr);
	    if (tu->p_cptr == child)
	      tu->p_cptr = o;
	  }

	if (pu)
	  {
	    /* fix up child linkage for new parent, if there is one */
	    o = pu->p_cptr;
	    if (o)
	      safe_getuser(o)->p_ysptr = child;
	    cu->p_osptr = o;
	    cu->p_ysptr = NULL;
	    pu->p_cptr = child;
	    cu->p_pptr = parent;
	  }
}

static int
possible_childs(int pid, struct Process *cptr)
{
  usetup;

  while (cptr)
  {
/*
 * Seems things are getting filled in too slowly, so we will wait till they do
 * get filled. [this solves a NULL pointer problem] -dz-
 */
/*	  while (!getuser(cptr))
		{
			Delay(5);
		}*/
    struct user *u_ptr = safe_getuser(cptr);

    if (u_ptr->p_stat != SZOMB)
      if (pid == -1 ||
	  (pid == 0 && u_ptr->p_pgrp == u.p_pgrp) ||
	  (pid < -1 && u_ptr->p_pgrp == -pid) ||
	  (pid == (int)cptr))
	return TRUE;
    cptr = u_ptr->p_osptr;
  }
  return FALSE;
}

/* This function is in desperate need of redesign !!!! */

int
wait4 (int pid, int *status, int options, struct rusage *rusage)
{
  struct Process *me = (struct Process *) SysBase->ThisTask;
  struct user *u_ptr = getuser(me);

  KPRINTF(("wait4(%lx, %lx)\n", pid, options));

  for (;;)
    {
      int err = 0;
      struct death_msg *dm;
      int got_node = 0;

      Forbid ();

      for (dm = (struct death_msg *)u.p_zombies.head;
           dm;
           dm = (struct death_msg *)dm->dm_node.next)
        {
          if (pid == -1 ||
	      (pid == 0 && dm->dm_pgrp == u.p_pgrp) ||
	      (pid < -1 && dm->dm_pgrp == - pid) ||
	      (pid == (int) dm->dm_child))
            {
	      got_node = 1;
	      break;
            }
        }

      if (!got_node && !possible_childs(pid, u.p_cptr))
	err = ECHILD;

      if (got_node)
	/* Handle exited children first.  */
	{
	  struct Process *child = dm->dm_child;
	  struct user *cu = getuser(child);
	  struct Task *t;

if(!cu){ cu=safe_getuser(child);
KPRINTF(("********** cu==0 !! real = %lx\n", cu));}
	  /*
	   * If we got the child through a ptrace 'attach',
	   * we need to give it back to the old parent.
	   */
	  if (cu->p_opptr && (t = pfind((pid_t)cu->p_opptr)))
	    {
	      proc_reparent(child, (struct Process *)t);
	      cu->p_opptr = 0;
	      _psignal(t, SIGCHLD);
	      ix_wakeup((u_int)t);
	      Permit();
	      return (int)child;
	    }
	  KPRINTF (("wait4: unlinking child $%lx\n", dm->dm_child));
	  ixremove ((struct ixlist *)&u.p_zombies, (struct ixnode *) dm);

	  cu->p_stat = SZOMB;

	  ix_wakeup ((u_int)dm);
	  Permit ();

	  if (status)
	  {
	    *status = dm->dm_status;
	  }
	  if (rusage)
	    *rusage = dm->dm_rusage;

	  kfree (dm);

	  return (int) child;
	}
      else
	/* No child processes have died for now.
	   Do we have a traced child process to handle?  */
	{
	  struct Process *p;
	  struct user *pu;

	  KPRINTF(("wait4: checking traced children, pid=%lx, u.p_cptr=%lx\n",
		   pid, u.p_cptr));

	  for (p = u.p_cptr; p; p = pu->p_osptr)
	    {
	      KPRINTF(("wait4: checking pid p=%lx\n", p));
	      pu = safe_getuser(p);
	      if (pid == -1
		  || ((int) p) == pid
		  || pu->p_pgrp == -pid
		  || (pid == 0
		      && u.p_pgrp == pu->p_pgrp))
		{
		  KPRINTF(("wait4: pu->p_stat=%lx, pu->flag=%lx\n",
		      pu->p_stat, pu->p_flag));
		  if (pu->p_stat == SSTOP
		      && (pu->p_flag & SWTED) == 0
		      && (pu->p_flag & STRC || options & WUNTRACED))
		    {
		      KPRINTF(("wait4: SSTOPed; p_xstat=0x%lx, W_STOPCODEd=0x%lx\n",
			  pu->p_xstat, W_STOPCODE (pu->p_xstat)));
		      pu->p_flag |= SWTED;
		      if (status)
			{
			  *status = W_STOPCODE (pu->p_xstat);
			  KPRINTF(("wait4: status was set to %ld\n", *status));
			}
		      Permit();
		      return (int)p;
		    }
		}
	    }
	}

      if (!got_node && err)
	{
	  Permit();
	  errno = err;
	  KPRINTF(("&errno = %lx, errno = %ld\n", &errno, errno));
	  return -1;
	}

      if (options & WNOHANG)
	{
	  Permit();
	  return 0;
	}

      ix_sleep((caddr_t)&u, "wait4");
      if (u.p_sig)
	err = EINTR;

      Permit();
      if (CURSIG(&u))
	setrun((struct Task *)me);
    }
}