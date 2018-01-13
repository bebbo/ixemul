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
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#include <hardware/intbits.h>
#include <exec/memory.h>
#include <string.h>

extern void launch_glue (), switch_glue ();
extern void trap_20 ();
extern void trap_00 ();
extern int ix_timer();

static BOOL
__ix_open_muFS (struct user *ix_u)
{
  if (muBase)
    {
      /* set up multiuser data's */
      ix_u->u_UserInfo = muAllocUserInfo ();
      if (NULL == ix_u->u_UserInfo)
        return FALSE;
      ix_u->u_fileUserInfo = muAllocUserInfo ();
      if (NULL == ix_u->u_fileUserInfo)
        return FALSE;

      ix_u->u_GroupInfo = muAllocGroupInfo ();
      if (NULL == ix_u->u_GroupInfo)
        return FALSE;
      ix_u->u_fileGroupInfo = muAllocGroupInfo ();
      if (NULL == ix_u->u_fileGroupInfo)
        return FALSE;
    }

  return TRUE;
}

struct ixemul_base *
ix_open (struct ixemul_base *ixbase)
{
  /* here we must initialize our `user' structure */
  struct user *ix_u;
  /* an errno for those that later don't set it in ix_startup() */
  static int default_errno;
  /* an h_errno for those that later don't set it in ix_startup() */
  static int default_h_errno;
  struct Task *me = FindTask(0);
  char *tmp;
  int a4_size = A4_POINTERS * 4;

  tmp = (char *)kmalloc(sizeof(struct user) + a4_size);
  if (tmp)  
    {
      /* bzero is safe, ie. doesn't need to reference struct user */
      bzero (tmp, sizeof (struct user) + a4_size);
      ix_u = (struct user *)(tmp + a4_size);

      /* remember old state */
      ix_u->u_otask_flags = me->tc_Flags;
      ix_u->u_olaunch     = me->tc_Launch;
      ix_u->u_oswitch     = me->tc_Switch;
      ix_u->u_otrap_code  = me->tc_TrapCode;
      ix_u->u_otrap_data  = getuser(me);
    
      ixnewlist ((struct ixlist *)&ix_u->u_md.md_list);

      ix_u->u_mdp = &ix_u->u_md;
      
      ix_u->u_task = me;

      if (ix.ix_flags & ix_show_stack_usage)      
        {
          tmp = (char *)(get_sp() - 64);  /* 64 bytes safety margin */
          memset(me->tc_SPLower, 0xdb, (u_long)tmp - (u_long)me->tc_SPLower); 
        }

      KPRINTF (("ix_open: ix_u = $%lx, ix_open @$lx\n", ix_u, ix_open));

      getuser(me) = ix_u;

#ifndef NOTRAP
      /* The stackframe of the 68010 is identical to the 68020! */
      if (has_68010_or_up)
	me->tc_TrapCode = trap_20;
      else
	me->tc_TrapCode = trap_00;
#endif

      initstack();

      /* setup the p_sigignore mask correctly */
      siginit (ix_u);
      me->tc_SigRecvd &= 0x0fff;

      /* this library is a replacement for any c-library, thus we should be
       * started at the START of a program, and out of 16 available signals 
       * these calls simply have to succeed... I know I'm a lazy guy ;-) */
      ix_u->u_sleep_sig = AllocSignal (-1);
      ix_u->u_pipe_sig = AllocSignal (-1);

      ix_u->u_ixbase = ixbase;
      ix_u->u_errno = &default_errno;
      ix_u->u_h_errno = &default_h_errno;

      /* ixnet.library is loaded iff ixbase->ix_network_type != IX_NETWORK_NONE */
      if (ixbase->ix_network_type != IX_NETWORK_NONE)
	ix_u->u_ixnetbase = OpenLibrary("ixnet.library", 44);  // Let ixnet check if the ixnet version matches ours

      me->tc_Launch    = launch_glue;
      me->tc_Switch    = switch_glue;
      me->tc_Flags    |= TF_LAUNCH | TF_SWITCH;
      ix_u->u_itimerint.is_Node.ln_Type = NT_INTERRUPT;
      ix_u->u_itimerint.is_Node.ln_Name = me->tc_Node.ln_Name;
      ix_u->u_itimerint.is_Node.ln_Pri  = 1;
      ix_u->u_itimerint.is_Data         = (APTR) me;
      ix_u->u_itimerint.is_Code	        = (APTR) ix_timer;

      AddIntServer (INTB_VERTB, &ix_u->u_itimerint);
      Disable();
      ixaddtail(&timer_task_list, &ix_u->u_user_node);
      Enable();

      ix_u->u_trace_flags = 1;
      ix_u->u_sync_mp = (struct MsgPort *)ix_create_port(0, 0);
      ix_u->u_select_mp = (struct MsgPort *)ix_create_port(0, 0);
      
      /* the CD storage. since 0 is a valid value for a lock, we use -1 */
      ix_u->u_startup_cd = (BPTR)-1;

      /* support for subprocesses a la Unix */

      /* each process starts out to be in its own process group. vfork()
       * scribbles over this to inherit the parents process group instead */
      ix_u->p_pgrp = (int) me;
      ix_u->p_pptr = (struct Process *) 1;		/* hi init ;-)) */
      ix_u->p_cptr =
        ix_u->p_osptr =
          ix_u->p_ysptr = 0;			/* no children to start with */
      ix_u->p_vfork_msg = 0;
      ix_u->p_zombie_sig = AllocSignal (-1);
      ix_u->u_rand_next = 1;
      ixnewlist ((struct ixlist *)&ix_u->p_zombies);
      strcpy(ix_u->u_logname, "unknown");
      ix_u->u_LogFile = -1;
      ix_u->u_LogTag = "syslog";
      ix_u->u_LogFacility = LOG_USER;
      ix_u->u_LogMask = 0xff;
      ix_u->u_a4_pointers_size = A4_POINTERS;

      ix_u->u_cmask = 0022; /* default, see manpage for umask() */

      if (ix_u->u_sync_mp && ix_u->u_select_mp)
        {
          ix_u->u_time_req = (struct timerequest *)
	    ix_create_extio(ix_u->u_sync_mp, sizeof (struct timerequest));
	  
	  if (ix_u->u_time_req)
	    {
	      if (!OpenDevice (TIMERNAME, UNIT_MICROHZ,
	      		       (struct IORequest *) ix_u->u_time_req, 0))
	        {
		  syscall (SYS_gettimeofday, &ix_u->u_start, 0);

		  /* have to mask out ALL signals until ix_startup has had a
		   * chance to setup its exit jmp_buf. If not, _longjmp will
		   * generate a longjmp-botch using a not initialized jmpbuf! */
		  syscall (SYS_sigsetmask, ~0);

                  if (__ix_open_muFS (ix_u))
		    return ixbase;
                  else
                    {
                      /* couldn't allocate muFS stuff */
                      __ix_close_muFS (ix_u);
                    }
		}
	      /* couldn't open the timer device */
	      ix_delete_extio((struct IORequest *)ix_u->u_time_req);
	    }
        }

      if (ix_u->u_select_mp)
        ix_delete_port(ix_u->u_select_mp);

      if (ix_u->u_sync_mp)
        ix_delete_port(ix_u->u_sync_mp);

      Disable();
      ixremove(&timer_task_list, &ix_u->u_user_node);
      Enable();
      RemIntServer (INTB_VERTB, &ix_u->u_itimerint);
      me->tc_Flags    = ix_u->u_otask_flags;
      me->tc_Launch   = ix_u->u_olaunch;
      FreeSignal (ix_u->u_sleep_sig);
      FreeSignal (ix_u->u_pipe_sig);
      FreeSignal (ix_u->p_zombie_sig);

      /* all_free() MUST come before we remove the pointer to u */
      all_free ();
#ifndef NOTRAP
      me->tc_TrapCode = ix_u->u_otrap_code;
      getuser(me) = ix_u->u_otrap_data;
#endif

      kfree (tmp);
    }

  return 0;
}
