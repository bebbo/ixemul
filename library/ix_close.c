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
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"
#include <hardware/intbits.h>
#include "multiuser.h"
#include <string.h>

void
__ix_close_muFS( struct user *ix_u )
{
  if (muBase)
    {
      if (ix_u->u_UserInfo)
        {
          muFreeUserInfo(ix_u->u_UserInfo);
          ix_u->u_UserInfo = NULL;
        }

      if (ix_u->u_fileUserInfo)
        {
          muFreeUserInfo(ix_u->u_fileUserInfo);
          ix_u->u_fileUserInfo = NULL;
        }

      if (ix_u->u_GroupInfo)
        {
          muFreeGroupInfo(ix_u->u_GroupInfo);
         ix_u->u_GroupInfo = NULL;
        }

      if (ix_u->u_fileGroupInfo)
        {
          muFreeGroupInfo(ix_u->u_fileGroupInfo);
          ix_u->u_fileGroupInfo = NULL;
        }

      /* log me out */
      while (ix_u->u_setuid--)
        muLogout(muT_Quiet, TRUE, TAG_DONE);
    }
}

void ix_stack_usage(void)
{
  if (ix.ix_flags & ix_show_stack_usage)
    {
      struct Task *me = FindTask(0);
      BPTR lock = GetProgramDir();
      struct SUMessage sum;
      struct MsgPort *port, *reply;
      u_char *tmp;

      if ((reply = (struct MsgPort *)ix_create_port(0, 0)))
        {
          bzero(&sum, sizeof(sum));
          if (lock)
            NameFromLock(lock, sum.name, sizeof(sum.name) - 40);
          tmp = me->tc_SPLower;
          while (*++tmp == 0xdb);
          if (lock)
            strcat(sum.name, "/");
          GetProgramName(sum.name + strlen(sum.name), 39);
          sum.stack_size = (u_long)me->tc_SPUpper - (u_long)me->tc_SPLower;
          sum.stack_usage = (u_long)me->tc_SPUpper - (u_long)tmp;
          sum.msg.mn_Node.ln_Type = NT_MESSAGE;
          sum.msg.mn_Length = sizeof(sum);
          sum.msg.mn_ReplyPort = reply;
          Forbid();
          port = FindPort("ixstack port");
          if (port)
            PutMsg(port, (struct Message *)&sum);
          Permit();
          if (port)
            WaitPort(reply);
          ix_delete_port(reply);
        }
    }
}

void
ix_close (struct ixemul_base *ixbase)
{
  struct Task           *me = FindTask(0);
  struct user           *u_ptr = getuser(me);
  struct user 		*ix_u =	&u;
  struct Process	*child;
  struct user		*cu;
  struct ixnode		*dm;	/* really struct death_msg * */

  Disable();
  ixremove(&timer_task_list, &ix_u->u_user_node);
  Enable();
  RemIntServer (INTB_VERTB, &ix_u->u_itimerint);

  Forbid();
  semexit(me);
  Permit();

#ifndef NOTRAP
  /* already reset the trap vector here. It's better to get an alert than
   * to loop infinitely if one of the following functions should crash */
  me->tc_TrapCode = ix_u->u_otrap_code;
#endif
  
  freestack();
  shmexit(ix_u);

  /* had to move this block after the SYS_close's, since close() might have
     to wait for a packet, and then it's essential that our switch/launch
     handlers are still active */
  me->tc_Flags    = ix_u->u_otask_flags;
  me->tc_Launch	  = ix_u->u_olaunch;
  me->tc_Switch   = ix_u->u_oswitch;
  FreeSignal (ix_u->u_sleep_sig);
  FreeSignal (ix_u->u_pipe_sig);

  if (ix_u->u_ixnetbase)
    CloseLibrary (ix_u->u_ixnetbase);

  CloseDevice ((struct IORequest *) ix_u->u_time_req);
  ix_delete_extio((struct IORequest *)ix_u->u_time_req);
  
  if (ix_u->u_startup_cd != (BPTR) -1)
    {
      __unlock (CurrentDir (ix_u->u_startup_cd));
      set_dir_name_from_lock(ix_u->u_startup_cd);
    }

  ix_u->u_trace_flags = 1;
  ix_delete_port(ix_u->u_select_mp);
  ix_delete_port(ix_u->u_sync_mp);

  /* try to free it here */
  __ix_close_muFS(ix_u);

  Forbid();
  for ((child = ix_u->p_cptr); child; (child = cu->p_osptr))
    {
      cu = safe_getuser(child);
      cu->p_pptr = (struct Process *) 1;
    }

  ix_u->p_cptr = 0;

  /* decrease session count and possibly free the session structure */
  if (ix_u->u_session)
    {
      if (ix_u->u_session->pgrp == (int)me)
        ix_u->u_session->pgrp = 0;
      if (ix_u->u_session->s_count-- <= 1)
        kfree(ix_u->u_session);
    }
  
  if (ix_u->p_ysptr)
    safe_getuser(ix_u->p_ysptr)->p_osptr = ix_u->p_osptr;

  if (ix_u->p_osptr)
    safe_getuser(ix_u->p_osptr)->p_ysptr = ix_u->p_ysptr;

  if (ix_u->p_pptr && ix_u->p_pptr != (struct Process *)1)
    {
      struct user *u_ptr = safe_getuser(ix_u->p_pptr);
      if (u_ptr->p_cptr == (struct Process *)me)
	u_ptr->p_cptr = ix_u->p_osptr;
    }
  Permit();

  if (ix_u->p_flag & SFREEA4)
    kfree ((void *)(ix_u->u_a4 - 0x7ffe));

  for (dm = (struct ixnode *)ix_u->p_zombies.head; dm;)
    {
      struct ixnode *tmp = dm;
      dm = dm->next;
      /* there might be children sleeping on this, so wake them up now.. */
      ix_wakeup((u_int)tmp);
      kfree(tmp);
    }

  FreeSignal (ix_u->p_zombie_sig);

  if ((ix_u->p_flag & SUSAGE) == 0)
    ix_stack_usage();

  all_free ();

#ifndef NOTRAP
  /* delay this until here, since the above called functions need access
   * to the user area. */
  getuser(me) = ix_u->u_otrap_data;
#endif

  kfree (((char *)ix_u) - ix_u->u_a4_pointers_size * 4);
}
