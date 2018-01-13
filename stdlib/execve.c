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
 */

#define _KERNEL
#include <string.h>
#include "ixemul.h"
#include "kprintf.h"
#include <hardware/intbits.h>
#include <ctype.h>
#include <sys/wait.h>
#include <stdio.h>

#include <sys/exec.h>

#include "atexit.h"
#define __atexit (u.u_atexit)

#define JMP_MAGIC_16(code) ((code[0] & 0xffff0000) == 0x4efa0000)
#define JMP_MAGIC_32(code) ((code[0] & 0xffff0000) == 0x4efb0000)
/* The following define tests for the 'bra' instruction which the
   current assembler generates at the start of a program that is
   compiled with crt0.o. The previous defines are for backwards
   compatibility with older assemblers. */
#define JRA_MAGIC_16(code) ((code[0] & 0xffff0000) == 0x60000000)

#define MAGIC_16(code) \
  ((JRA_MAGIC_16 (code) || JMP_MAGIC_16 (code)) && \
   (code[1] & 0xffff) == OMAGIC)

#define MAGIC_32(code) \
  (JMP_MAGIC_32 (code) && (code[2] & 0xffff) == OMAGIC)

static int compatible_startup (void *code, int argc, char **argv);
static char *quote (char *orig);
static void volatile on_real_stack (BPTR *segs, char **argv, char **environ, int omask);

BPTR dup2_BPTR (int);
void readargs_kludge (BPTR);

int
execve (const char *path, char * const *argv, char * const *environ)
{
  BPTR *segs;
  u_int omask, err;
  char *extra_args = 0;
  struct Process *me = (struct Process *) FindTask (0);
  struct user *u_ptr = getuser(me);

  KPRINTF (("execve (%s,...)\n", path));
  KPRINTF_ARGV ("argv", argv);
  KPRINTF_ARGV ("environ", environ);

  omask = syscall (SYS_sigsetmask, ~0);

  segs = __load_seg ((char *)path, &extra_args);

  if (segs)
    {
      /* Now it gets somewhat nasty... since I have to revert to the `real'
         stack (since the parent will want its sp back ;-)), I have to save
         the values of this stack frame into registers, or I'll never be
         able to access them afterwards again.. */
      register BPTR *_segs asm ("d2");
      register char **_argv asm ("d3");
      register char **_environ asm ("d4");

      /* if we got extra arguments, split them into a 2 el argument vector, and join
       * `argv' */
      if (extra_args && *extra_args)
	{
	  char **ap;
          char **nargv;
          int size;

          for (size = 0, ap = (char **)argv; *ap; size++, ap++) ;
          nargv = (char **) syscall(SYS_malloc, (size + 4) * sizeof(char *));
          ap = nargv;
          argv++;				/* discard the program name */
          *ap = extra_args;			/* new program name */

          for(;;)
	    {
	      ap[1] = index(*ap, '\001');
              ap++;
	      if (*ap)
		{
		  **ap = 0;
                  (*ap)++;
		}
	      else break;
	    }

          while ((*ap++ = *argv++)) ;
          argv = (char * const *)nargv;
	}

      _segs = segs;
      _argv = (char **)argv;
      _environ = (char **)environ;

      KPRINTF (("execve: about to call on_real_stack ()\n"));
      if (u.p_vfork_msg)
        {
          me->pr_Task.tc_SPLower = (void *)u.p_vfork_msg->vm_clower;
          me->pr_Task.tc_SPUpper = (void *)u.p_vfork_msg->vm_cupper;
          set_sp ((u_int) u.u_save_sp);
          /* fool the optimizer... */
          asm volatile ("" : "=g" (_segs), "=g" (_argv), "=g" (_environ) : "0" (_segs), "1" (_argv), "2" (_environ));
          KPRINTF (("execve () restored native sp\n"));
	}
      on_real_stack (_segs, _argv, _environ, omask);
      /* never reached */
    }

  err = errno;

  syscall (SYS_sigsetmask, omask);

  errno = err;
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
  return -1;
}


char **
dupvec (char **vec)
{
  int n;
  char **vp;
  char **res;
  static char *empty[] = { NULL };
  
  if (! vec)
    return empty;

  for (n = 0, vp = vec; *vp; n++, vp++) ;

  /* contrary to `real' vfork(), malloc() works in the child on its own
     data, that is it won't clobber anything in the parent  */
  
  res = (char **) syscall (SYS_malloc, (n + 1) * 4);
  if (res)
    {
      for (vp = res; n-- > 0; vp++, vec++)
        *vp = (char *) syscall (SYS_strdup, *vec);

      *vp = 0;
    }

  return res;
}

static void volatile 
on_real_stack (BPTR *segs, char **argv, char **environ, int omask)
{
  int private_startup;
  u_int *code;
  int (*entry) (struct ixemul_base *, int, char **, char **, int) = NULL;
  struct exec *hdr = NULL;
  int f;
  int sg;
  jmp_buf old_exit;
  u_int old_a4 = 0;
  usetup;

  KPRINTF (("entered on_real_stack()\n"));
  /* first make sure that we're later passing on `safe' data to our child, ie.
     copy it from wherever the data is currently stored into child malloc space */
  vfork_own_malloc ();
  if (environ)
    *u.u_environ = dupvec(environ);
  environ = *u.u_environ;
  argv = dupvec(argv);
  KPRINTF_ARGV ("copy of argv", argv);
    
  u.u_segs = (struct my_seg *)segs;

  sg = (long)*segs;
  sg <<= 2;
  u.u_start_pc = sg + 4;
  u.u_end_pc = sg + *(long *)(sg - 4) - 8;
  code = (void *)u.u_start_pc;
  
  /* Check whether this program has our magic header.  See crt0.c for details. */

  if (code && MAGIC_16 (code))
    {
      private_startup = 1;
      hdr = (struct exec *) &code[1];
    }
  else if (code && MAGIC_32 (code))
    {
      private_startup = 1;
      hdr = (struct exec *) &code[2];
    }
  else
    {
      private_startup = 0;
    }

  KPRINTF (("magic header %s\n", private_startup ? "found" : "NOT found"));
  KPRINTF (("code[0] = %lx; code[1] = %lx; code[2] = %lx\n", code[0], code[1], code[2]));

#if 0
  {
    char **cp;
    KPRINTF (("execve ["));
    for (cp = argv; *cp; cp++) KPRINTF (("%s%s", *cp, cp[1] ? ", " : "], ["));
    for (cp = environ; *cp; cp++) KPRINTF (("%s%s", *cp, cp[1] ? ", " : "]\n"));
  }
#endif

  if (private_startup)
    {
      entry = (void *) hdr->a_entry;
      
      if (! entry) private_startup = 0;
    }
      
  /* okay, get ready to turn us into a new process, as much as
     we can actually.. */

  /* close all files with the close-on-exec flag set */
  for (f = 0; f < NOFILE; f++)
    {
      if (u.u_ofile[f] && (u.u_pofile[f] & UF_EXCLOSE))
      syscall (SYS_close, f);
    }

  /* BIG question what to do with registered atexit() handlers before
     an exec.. Unix for sure does nothing, since the process space is
     physically written over. In the AmigaOS I could (!) imagine
     cases where calling some atexit() handlers (mostly in the case
     of destructors for C++ objects) would result in erronous
     behaving of the program. However, since atexit() handlers also
     serve to get rid of acquired Amiga resources, I morally feel
     obliged to call the handlers.. lets see if this results in
     incompatibilities with programs that expect Unix behavior. (Note
     that I don't call exit() after exeve() returns, I call _exit(),
     and _exit() does not walk the atexit() list).

     There is one special case that I catch here, this is stdio. No
     Unix program would ever expect stdio buffers to be flushed by
     an execve() call. So since stdio is in the library I know the
     address of the handler to skip ;-)) */
     
  while (__atexit)
    {
      while (__atexit->ind --)
	{
	  /* this is the stdio function to flush all buffers */
	  extern void _cleanup();
	
	  if (__atexit->fns[__atexit->ind] != _cleanup)
	    {
	      if (u.u_a4)
	        asm volatile ("movel %0, a4" : : "g" (u.u_a4));
	      __atexit->fns[__atexit->ind] ();
	    }
	}
      __atexit = __atexit->next;
    }

  /* `ignored signals remain ignored across an execve, but
      signals that are caught are reset to their default values.
      Blocked signals remain blocked regardless of changes to
      the signal action. The signal stack is reset to be
      undefined'. */

  u.u_sigonstack = 0;	/* default = on normal stack */
  u.u_sigintr    = 0;	/* default = don't interrupt syscalls */
  u.p_sigcatch   = 0;	/* no signals caught by user -> SIG_DFL */
  for (f = 0; f < NSIG; f++)  /* reset handlers to SIG_DFL, except for SIG_IGN */
    if (u.u_signal[f] != SIG_IGN)
      u.u_signal[f] = SIG_DFL;

  /* what happens when we execute execve() from a signal handler
     that executes on the signal stack? Better don't do this... */

  /* deinstall our sigwinch input-handler */
  __ix_remove_sigwinch ();

  /* clear the a4 pointers */
  bzero((char *)&u - u.u_a4_pointers_size * 4, u.u_a4_pointers_size * 4);
	      
  /* save the original exit-jmpbuf, as ix_exec_entry() will destroy
   * it later */
  bcopy (u.u_jmp_buf, old_exit, sizeof (old_exit));
  if (u.p_flag & SFREEA4)
    {
      old_a4 = u.u_a4;
      u.p_flag &= ~SFREEA4;
    }
	      
  /* count the arguments */
  for (f = 0; argv[f]; f++) ;
  KPRINTF (("found %ld args\n", f));

  KPRINTF (("execve() having parent resume\n"));
  if (u.p_vfork_msg)
    {
      /* make the parent runable again */
      ReplyMsg ((struct Message *) u.p_vfork_msg);
      u.p_vfork_msg = 0;
    }

  KPRINTF (("execve() calling entry()\n"));
  {
    BPTR origprogdir = 0;
    extern void ix_stack_usage(void);
	
    /* to run as a `true' AmigaOS program, ProgramName should
     * only be that: the program name with no path. An ixemul
     * program retrieves its name from argv[0] anyway.
     */
    char *programname = basename (argv[0]);
    SetProgramName(programname);
    if (u.u_segs->programdir)
      origprogdir = SetProgramDir(u.u_segs->programdir);

    u.u_oldmask = omask;
    u.u_a4 = 0; /* assume it's not baserelative */

    if (private_startup)
      {
	u.p_xstat = entry(ixemulbase, f, argv, environ, 0);
	if (u.p_flag & SFREEA4) /* free data segment allocated by a pure executable */
	  {
	    kfree ((void *)(u.u_a4 - 0x7ffe));
	    u.p_flag &= ~SFREEA4;
	  }
      }
    else
      {
	/*  Disable ctrl-C handling, otherwise it would be possible that
	 *  this process would be killed, while the child was still running.
	 */
        omask = syscall(SYS_sigsetmask, ~0);
        compatible_startup (code, f, argv);
        syscall(SYS_sigsetmask, omask);
      }

    ix_stack_usage();
    u.p_flag |= SUSAGE;

    if (u.u_segs->programdir)
      SetProgramDir (origprogdir);
  }

  __free_seg (segs);

  if (old_a4)
    {
      u.u_a4 = old_a4; 
      u.p_flag |= SFREEA4;
    }
  KPRINTF (("old program doing _exit(%ld)\n", f));
  /* and fake an _exit */
  _clean_longjmp (old_exit, 1);
}  


struct user *safe_getuser(struct Process *task)
{
  struct user *u_ptr = NULL;
  struct ixnode *node;

  /* Check if the process has been 'detached' from ixemul first.
     If it is, getuser(task) is invalid. It is usually NULL, but it
     may be non-NULL too, if the non-ixemul task uses tc_TrapData. */

  Forbid();
  for (node = ixemulbase->ix_detached_processes.head; node; node = node->next)
    {
      struct user *p = (struct user *)((char *)node - offsetof(struct user, u_detached_node));
      if (p->u_task == task)
	{
	  u_ptr = p;
	  break;
	}
    }
  if (!u_ptr)
    u_ptr = getuser(task);
  Permit();

  return u_ptr;
}


/* some rather rude support to start programs that don't have a struct exec
 * information at the beginning.
 * 1.3 NOTE: This will only start plain C programs, nothing that smells like
 *           BCPL. Limited support for ReadArgs() style parsing is here, but not
 *	     everything is set up that would have to be set up for BCPL programs
 *	     to feel at home. Also don't use Exit() in those programs, it wouldn't
 *	     find what it expects on the stack....
 */
static int
compatible_startup (void *code, int argc, char **argv)
{
  char *al;
  int max, res;
  u_int oldsigalloc;
  struct Process *me = (struct Process *)FindTask(0);
  struct user *u_ptr = getuser(me);
  
  KPRINTF (("entered compatible_startup()\n"));
  KPRINTF (("argc = %ld\n", argc));
  KPRINTF_ARGV ("argv", argv);

  /* ignore the command name ;-) */
  argv++;

  max = 1024;
  al = (char *) kmalloc (max);
  res = -1;
  if (al)
    {
      char *cp;
      BPTR old_cis, old_cos, old_ces;
      BPTR dup_cis, dup_cos, dup_ces;
      void *old_trapdata, *old_trapcode;
      int old_flags;
      void *old_launch, *old_switch;
      struct file *f;

      for (cp = al; *argv; )
        {
	  char *newel = quote (*argv);
          int elsize = strlen (newel ? newel : *argv) + 2;
	  KPRINTF (("arg [%s] quoted as [%s]\n", *argv, newel ? newel : *argv));
          
          if (cp + elsize >= al + max)
            {
	      char *nal;
              max <<= 1;
              nal = (char *) krealloc (al, max);
              if (! nal) break;
              cp = nal + (cp-al);
              al = nal;
	    }

	  strcpy (cp, newel ? newel : *argv);
	  cp += elsize - 2;
	  *cp++ = ' ';
	  *cp = 0;
	  if (newel) kfree (newel);
	  ++argv;
        }
      
      /* BCPL weirdness ... */
      *cp++ = '\n';
      *cp = 0;

      KPRINTF (("BCPL cmd line = [%s]\n", al));

      /* problem with RunCommand: the allocated signal mask is not reset
	 for the new process, thus if several RunCommands are nested, a
	 late started process might run out of signals. This behavior makes
	 no sense, since the starting process is *suspended* while the `child'
	 is running, thus it doesn't need its signals in the meantime ! */

      oldsigalloc = me->pr_Task.tc_SigAlloc & 0xffff0000;	/* hacky...*/
      me->pr_Task.tc_SigAlloc &= 0xffff;

      /* cleanup as much of ixemul.library as possible, so that the started
         process can take over */
      old_flags             = me->pr_Task.tc_Flags;
      me->pr_Task.tc_Flags  = u.u_otask_flags;
      old_launch            = me->pr_Task.tc_Launch;
      me->pr_Task.tc_Launch = u.u_olaunch; /* restoring this disables our signals */
      old_switch            = me->pr_Task.tc_Switch;
      me->pr_Task.tc_Switch = u.u_oswitch;
      RemIntServer (INTB_VERTB, & u.u_itimerint);
      Disable();
      ixremove(&timer_task_list, &u.u_user_node);
      Enable();

      /* limited support (part 2 ;-)) for I/O redirection on old programs
         If we're redirecting to a plain file, don't go thru a IXPIPE, 
         temporarily use our DOS files in that case. Any other file type
         is routed thru an IXPIPE though. */
      
      f = u.u_ofile[0];
      old_cis = dup_cis = 0;
      
      /* I do wish I knew why pty's need to go though ixpipe:. But if I don't
         do this, the output simply disappears :-( */

      if (f && f->f_type == DTYPE_FILE && memcmp(f->f_name, "/fifo/pty", 9))
        {
          old_cis = SelectInput (CTOBPTR (f->f_fh));
          readargs_kludge (CTOBPTR (f->f_fh));
        }
      else if (!f)
        {
          int fd = syscall(SYS_open, "/dev/null", 0);
    
          dup_cis = dup2_BPTR (fd);
          syscall(SYS_close, fd);
        }
      else
        dup_cis = dup2_BPTR (0);
      if (dup_cis)
        {
          old_cis = SelectInput (dup_cis);
          readargs_kludge (dup_cis);
        }

      f = u.u_ofile[1];
      old_cos = dup_cos = 0;
      if (f && f->f_type == DTYPE_FILE && memcmp(f->f_name, "/fifo/pty", 9))
        {
          old_cos = SelectOutput (CTOBPTR (f->f_fh));
        }
      else if (!f)
        {
          int fd = syscall(SYS_open, "/dev/null", 1);
    
          dup_cos = dup2_BPTR (fd);
          syscall(SYS_close, fd);
        }
      else
        dup_cos = dup2_BPTR (1);
      if (dup_cos)
        old_cos = SelectOutput (dup_cos);

      old_ces = me->pr_CES;
      dup_ces = 0;
      f = u.u_ofile[2];
      if (f && f->f_type == DTYPE_FILE && memcmp(f->f_name, "/fifo/pty", 9))
        {
	  me->pr_CES = CTOBPTR (f->f_fh);
        }
      else if (!f)
        {
          int fd = syscall(SYS_open, "/dev/null", 2);
    
          dup_ces = dup2_BPTR (fd);
          syscall(SYS_close, fd);
        }
      else
        dup_ces = dup2_BPTR (2);
      me->pr_CES = dup_ces ? : old_ces;

      /* BEWARE that after this reset no library functions can be
         called any longer until the moment where trapdata is 
         reinstalled !! */

      Forbid();
      ixaddtail(&ixemulbase->ix_detached_processes, &u.u_detached_node);
#ifndef NOTRAP
      old_trapcode = me->pr_Task.tc_TrapCode;
      me->pr_Task.tc_TrapCode = u.u_otrap_code;
#endif
      old_trapdata = getuser(me);
      getuser(me) = 0;
      Permit();

      {
	struct CommandLineInterface *CLI = BTOCPTR (me->pr_CLI);
	u_int stack_size = CLI ? CLI->cli_DefaultStack * 4 : me->pr_StackSize;

	/* Note: The use of RunCommand() here means, that we *waste* the
	         entire stack space allocated for this process! If someone
	         comes up with a clever trick (probably involving StackSwap ())
	         where the stack of this process can be freed before calling
	         RunCommand (), lots of users with memory problems would be
	         thankful! */
	res = RunCommand (CTOBPTR (code) - 1, stack_size, al, cp - al);
      }
      /* reinstall enough of ixemul to be able to finish cleanly 
         (the recent addition of an ix_sleep() at the end of a vfork'd
          process makes it necessary to reinstall the signalling facilities!) */
      Forbid();
      getuser(me) = old_trapdata;
#ifndef NOTRAP
      me->pr_Task.tc_TrapCode = old_trapcode;
#endif
      ixremove(&ixemulbase->ix_detached_processes, &u.u_detached_node);
      Permit();

      /* have to do this, or ix_close() is not able to RemoveIntServer .. */
      AddIntServer (INTB_VERTB, & u.u_itimerint);
      Disable();
      ixaddhead(&timer_task_list, &u.u_user_node);
      Enable();
      me->pr_Task.tc_Launch = old_launch;
      me->pr_Task.tc_Switch = old_switch;
      me->pr_Task.tc_Flags  = old_flags;

      /* Some programs can set ixemul Signals. This can happen because
         ixemul resets the allocated signal mask in the Task structure.
         Make very sure they're off, or Enforcer hits will be the result
         because the 'deathmessage-handshake' returns too early. */
      SetSignal(0, ~0);

      kfree (al);

      if (old_cis)
	SelectInput (old_cis);
      if (old_cos)
	Flush (SelectOutput (old_cos));
      me->pr_CES = old_ces;

      if (dup_cis)
        Close (dup_cis);
      if (dup_cos)
        Close (dup_cos);
      if (dup_ces)
        Close (dup_ces);

      me->pr_Task.tc_SigAlloc |= oldsigalloc;
    }

  u.p_xstat = W_EXITCODE(res, 0);
  return res;
}

/* try to obtain a DOS filehandle on the specified descriptor. This only
   works, if the user has mounted IXPIPE: */
BPTR
dup2_BPTR (int fd)
{
  long id;
  char name[20];
  
  id = syscall(SYS_fcntl, fd, F_EXTERNALIZE, 0);
  if (id >= 0)
    {
      sprintf (name, "IXPIPE:%x", (unsigned int)id);
      /* 0x4242 is a magic packet understood by IXPIPE: to F_INTERNALIZE id */
      return Open (name, 0x4242);
    }

  return 0;
}


//* the mysteries of DOS seem to never want to take an end... */
void
readargs_kludge (BPTR bp)
{
  int ch;
  static const int EOS_CHAR = -1;

#if 0
  /* the autodocs say this bug is fixed after v37, well, perhaps that was a
     very deep wish, nevertheless unheard by dos...
     Without this kludge, you have to actually press return if stdin is not
     redirected...
     Thanks mbs: without your shell code I would never have guessed that 
                 something that weird could be possible....
   */
  if (ix.ix_dos_base->lib_Version <= 37)
#endif
    {
      ch = UnGetC (bp, EOS_CHAR) ? 0 : '\n';
      while ((ch != '\n') && (ch != EOS_CHAR))
        ch = FGetC (bp);
      Flush (bp);
    }
}


static char *
quote (char *orig)
{
  int i;
  char *new, *cp;
  
  i = strlen (orig);
  
  if (strpbrk (orig, "\"\'\\ \t\n"))
    {
      /* worst case, each character needs quoting plus starting and ending " */
      new = (char *) kmalloc (i * 2 + 3);
      if (! new) return 0;

      cp = new;
      *cp++ = '"';
      while (*orig)
        {
          if (index ("\"\\", *orig))
            *cp++ = '\\';
	  *cp++ = *orig++;
	}
      *cp++ = '"';
      *cp = 0;
      
      return new;
    }
  else
    return 0;	/* means `just use the original string' */
}   

