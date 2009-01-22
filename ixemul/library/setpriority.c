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
 *  setpriority.c,v 1.1.1.1 1994/04/04 04:30:34 amiga Exp
 *
 *  setpriority.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:34  amiga
 * Initial CVS check in.
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

/* REMARK: all priorities are negated, since on the Amiga, lower
 * priorities mean less power, not more power.. */

int
setpriority(int which, int who, int prio)
{
  usetup;

  if (prio < PRIO_MIN) prio = PRIO_MIN;
  else if (prio > PRIO_MAX) prio = PRIO_MAX;

  if (which < PRIO_PROCESS || which > PRIO_USER)
    {
      errno = EINVAL;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return -1;
    }

  if (which == PRIO_USER || who == 0)
    {
      struct Task *myself = (struct Task *)SysBase->ThisTask;
      /* in this case I ignore the 'which' argument, and just set
       * my own Priority */
      SetTaskPri(myself, -prio);
      return 0;
    }

  /* so we look for processes. I ignore a difference between processes
   * and process-groups.. */

  /* try to validate, that the given pid is really a task-pointer */
  /* a pointer has to be word-aligned */
  if (!(who & 1))
    {
      struct Task *task = (struct Task *)who;
      /* it must have a node-type of either NT_PROCESS or NT_TASK */
      if (task->tc_Node.ln_Type == NT_PROCESS ||
	  task->tc_Node.ln_Type == NT_TASK)
	{
	  /* so we have to believe, this is really a task */
	  SetTaskPri(task, -prio);
	  return 0;
	}
    }
  errno = ESRCH;
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
  return -1;
}
