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
 *  $Id: ix_expunge.c,v 1.9 1994/07/07 15:08:35 rluebbert Exp $
 *
 *  $Log: ix_expunge.c,v $
 *  Revision 1.9  1994/07/07  15:08:35  rluebbert
 *  10*NOFILE -> NOFILE
 *
 *  Revision 1.8  1994/07/07  15:03:19  rluebbert
 *  Removed ttyport dummy routines
 *
 *  Revision 1.7  1994/07/07  12:22:20  rluebbert
 *  *** empty log message ***
 *
 *  Revision 1.6  1994/07/07  11:44:46  rluebbert
 *  *** empty log message ***
 *
 *  Revision 1.5  1994/06/19  15:12:54  rluebbert
 *  *** empty log message ***
 *
 *  Revision 1.3  1992/05/22  01:43:58  mwild
 *  when debugging, check whether buddy allocator clean
 *
 * Revision 1.2  1992/05/17  21:21:56  mwild
 * changed async mp to be global
 *
 * Revision 1.1  1992/05/14  19:55:40  mwild
 * Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"

void ix_expunge (struct ixemul_base *ixbase)
{
  extern long vector_install_count;	/* from trap.s */
  extern void *restore_vector;		/* from trap.s */

//  RemIntServer(INTB_VERTB, &ixbase->ix_itimerint);
  EndNotify(&ixbase->ix_notify_request);

  Forbid();
  ix_delete_task(ixbase->ix_task_switcher);
  Permit();

  if (ixbase->ix_global_environment)
    {
      char **tmp = ixbase->ix_global_environment;
      
      while (*tmp)
        kfree(*tmp++);
      kfree(ixbase->ix_global_environment);
    }

  close_libraries();

  FreeMem (ixbase->ix_file_tab, NOFILE * sizeof (struct file));

  if (vector_install_count)
    {
      vector_install_count = 1;		/* force restoration original bus error vector */
      Supervisor(restore_vector);
    }
}
