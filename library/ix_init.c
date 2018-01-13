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

#include <exec/memory.h>
#include <dos/var.h>

#include <string.h>
#include <unistd.h>

/* not changed after the library is initialized */
struct ixemul_base		*ixemulbase = NULL;

struct ExecBase			*SysBase = NULL;
struct DosLibrary		*DOSBase = NULL;

#ifndef __HAVE_68881__
struct MathIeeeSingBasBase	*MathIeeeSingBasBase = NULL;
struct MathIeeeDoubBasBase	*MathIeeeDoubBasBase = NULL;
struct MathIeeeDoubTransBase	*MathIeeeDoubTransBase = NULL;
#endif

struct Library                  *muBase = NULL;

static struct
{
  void		**base;
  char		*name;
  ULONG         minver;  /* minimal version          */
  BOOL          opt;     /* no fail if not available */
}
ix_libs[] =
{
  { (void **)&DOSBase, "dos.library", 37 },
#ifndef __HAVE_68881__
  { (void **)&MathIeeeSingBasBase, "mathieeesingbas.library" },
  { (void **)&MathIeeeDoubBasBase, "mathieeedoubbas.library" },
  { (void **)&MathIeeeDoubTransBase, "mathieeedoubtrans.library" },
#endif
  { NULL, NULL }
};


struct ixlist timer_wait_queue;
struct ixlist timer_ready_queue;
struct ixlist timer_task_list;
ULONG timer_resolution;

static void ix_task_switcher(void)
{
  unsigned long sigs = 0;

  AllocSignal(29);
  AllocSignal(30);
  AllocSignal(31);

  // Signals:
  //
  // 1 << 29: Timer finished
  // 1 << 30: Global environment has changed
  // 1 << 31: Task switch

  while (!(sigs & (1 << 29)))
  {
    sigs = Wait(0x7 << 29);
    if (sigs & (1 << 30))
      ix.ix_env_has_changed = 1;
    if (sigs & (1 << 28))
    {
    }
  }
}

int open_libraries(void)
{
  int i;

  for (i = 0; ix_libs[i].base; i++)
    if (!(*(ix_libs[i].base) = (void *)OpenLibrary(ix_libs[i].name, ix_libs[i].minver))
        && !ix_libs[i].opt)
      {
	ix_panic("%s required!", ix_libs[i].name);
	return 0;
      }
  return 1;
}

void close_libraries(void)
{
  int i;
  
  for (i = 0; ix_libs[i].base; i++)
    if (*ix_libs[i].base)
      CloseLibrary(*ix_libs[i].base);
}

struct ixemul_base *ix_init (struct ixemul_base *ixbase)
{
  int i;
  char buf[256];
  extern char hostname[];  /* in getcrap.c */

  if (!open_libraries())
    {
      close_libraries();
      return 0;
    }

  ixbase->ix_file_tab = (struct file *)AllocMem (NOFILE * sizeof(struct file), MEMF_PUBLIC | MEMF_CLEAR);
  ixbase->ix_fileNFILE = ixbase->ix_file_tab + NOFILE;
  ixbase->ix_lastf = ixbase->ix_file_tab;
  
  memset(&ixbase->ix_notify_request, 0, sizeof(ixbase->ix_notify_request));
  memset(&ixbase->ix_ptys, 0, sizeof(ixbase->ix_ptys));
  
  ixnewlist(&timer_wait_queue);
  ixnewlist(&timer_ready_queue);
  ixnewlist(&timer_task_list);
  ixnewlist(&ixbase->ix_detached_processes);

  timer_resolution = 1000000 / SysBase->VBlankFrequency;

  /* Read the GMT offset. This environment variable is 5 bytes long. The
     first 4 form a long that contains the offset in seconds and the fifth
     byte is ignored. */
  if (GetVar("IXGMTOFFSET", buf, 6, GVF_BINARY_VAR) == 5 && IoErr() == 5)
    ix_set_gmt_offset(*((long *)buf));
  else
    ix_set_gmt_offset(0);

  if (GetVar(IX_ENV_SETTINGS, buf, sizeof(struct ix_settings) + 1, GVF_BINARY_VAR) == sizeof(struct ix_settings))
    ix_set_settings((struct ix_settings *)buf);
  else
    ix_set_settings(ix_get_default_settings());

  /* Set the hostname if the environment variable HOSTNAME exists. */
  if (GetVar("HOSTNAME", buf, 64, 0) > 0)
    strcpy(hostname, buf);

  if (ixbase->ix_flags & ix_support_mufs)
    muBase = OpenLibrary("multiuser.library", 39);

  /* initialize the list structures for the allocator */
  init_buddy ();

  ixbase->ix_task_switcher = (struct Task *)ix_create_task("ixemul task switcher", 9, ix_task_switcher, 2048);

  ixbase->ix_notify_request.nr_stuff.nr_Signal.nr_Task = ixbase->ix_task_switcher;
  ixbase->ix_notify_request.nr_stuff.nr_Signal.nr_SignalNum = 30;
  ixbase->ix_notify_request.nr_Name = "ENV:";
  ixbase->ix_notify_request.nr_Flags = NRF_SEND_SIGNAL;

  /* initialize port number for AF_UNIX(localhost) sockets */
  ixbase->ix_next_free_port = 1024;

#if 0 
/* TODO */
  if (ixbase->ix_task_switcher)
    {
      extern int ix_timer();

      ixbase->ix_itimerint.is_Node.ln_Type = NT_INTERRUPT;
      ixbase->ix_itimerint.is_Node.ln_Name = "ixemul timer interrupt";
      ixbase->ix_itimerint.is_Node.ln_Pri = 1;
      ixbase->ix_itimerint.is_Data = (APTR)0;
      ixbase->ix_itimerint.is_Code = (APTR)ix_timer;

      AddIntServer(INTB_VERTB, &ixbase->ix_itimerint);
    }
#endif

  if (ixbase->ix_file_tab)
    {
      configure_context_switch ();

      for (i = 0; i < IX_NUM_SLEEP_QUEUES; i++)
        ixnewlist ((struct ixlist *)&ixbase->ix_sleep_queues[i]);

      ixbase->ix_global_environment = NULL;
      ixbase->ix_env_has_changed = 0;

      StartNotify(&ixbase->ix_notify_request);
      seminit();

      return ixbase;
    }

  if (ixbase->ix_task_switcher)
    {
#if 0
      RemIntServer(INTB_VERTB, &ixbase->ix_itimerint);
#endif

      Forbid();
      ix_delete_task(ixbase->ix_task_switcher);
      Permit();
    }

  if (ixbase->ix_file_tab)
    FreeMem (ixbase->ix_file_tab, NOFILE * sizeof(struct file));
  else
    ix_panic ("out of memory");

  close_libraries();
  
  return 0;
}      

