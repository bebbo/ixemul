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
 *  createtask.c,v 1.1.1.1 1994/04/04 04:30:45 amiga Exp
 *
 *  createtask.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:45  amiga
 * Initial CVS check in.
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 *
 *  Code is based on the example given in the RKM Libraries & Devices.
 */

#define _KERNEL
#include "ixemul.h"

#include <exec/tasks.h>

#if 0 //def NATIVE_MORPHOS

#include <exec/memory.h>

struct newMemList
{
  struct Node nml_Node;
  UWORD nme_NumEntries;
  struct MemEntry nml_ME[2];
};

const struct newMemList MemTemplate =
{ {0,},
  2,
  { {MEMF_CLEAR|MEMF_PUBLIC, sizeof(struct Task)},
    {MEMF_CLEAR, 0} }
};

void NewList(struct List *list)
{
   LONG *p;

   list->lh_TailPred=(struct Node*)list;
   ((LONG *)list)++;
   p=(LONG *)list; *--p=(LONG)list;
}

struct Task *CreateTask(STRPTR name, LONG pri, APTR initpc, ULONG stacksize)
{
  struct Task *newtask,*task2;
  struct newMemList nml;
  struct MemList *ml;

  stacksize=(stacksize+3)&~3;
  {
    long *p1,*p2;
    int i;

    for (p1=(long *)&nml,p2=(long*)&MemTemplate,i=7; i; *p1++=*p2++,i--) ;
    *p1=stacksize;
  }
  if (!(((unsigned int)ml=AllocEntry((struct MemList *)&nml)) & (1<<31)))
  {
    newtask=ml->ml_ME[0].me_Addr;
    newtask->tc_Node.ln_Type=NT_TASK;
    newtask->tc_Node.ln_Pri=pri;
    newtask->tc_Node.ln_Name=name;
    newtask->tc_SPReg=(APTR)((ULONG)ml->ml_ME[1].me_Addr+stacksize);
    newtask->tc_SPLower=ml->ml_ME[1].me_Addr;
    newtask->tc_SPUpper=newtask->tc_SPReg;
    NewList(&newtask->tc_MemEntry);
    AddHead(&newtask->tc_MemEntry,(struct Node *)ml);
    task2=(struct Task *)AddTask(newtask,&gate,0);
    if (SysBase->LibNode.lib_Version>36 && !task2)
    {
      FreeEntry(ml); newtask=NULL;
    }
  }
  else
    newtask=NULL;

  return newtask;
}

#endif

struct Task *
ix_create_task (unsigned char *name, long pri, void *initPC, u_long stackSize)
{
#ifdef NATIVE_MORPHOS
  return NewCreateTask(
      TASKTAG_NAME, name,
      TASKTAG_PRI, pri,
      TASKTAG_PC, initPC,
      TASKTAG_CODETYPE, CODETYPE_PPC,
      TASKTAG_STACKSIZE, stackSize,
      TAG_END);
#else
  /* round the stack up to longwords... */
  return CreateTask(name, pri, initPC, (stackSize + 3) & ~3);
#endif
}
