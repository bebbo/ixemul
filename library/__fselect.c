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
 *  __fselect.c,v 1.1.1.1 1994/04/04 04:29:45 amiga Exp
 *
 *  __fselect.c,v
 * Revision 1.1.1.1  1994/04/04  04:29:45  amiga
 * Initial CVS check in.
 *
 *  Revision 1.2  1993/11/05  21:49:09  mw
 *  socket-changes
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#include "select.h"

/* don't use the `normal' packet port for select. We need synchronous
 * notification to be able to Wait() for multiple replies */

/*
 * select operation on a "normal" AmigaOS filehandle. Normal means,
 * we only have the WaitForChar() call available to find out, whether
 * a read would block. Write() cannot be caught, so we always allow it.
 * Request for exceptional data is routed to read.
 */

int
__fselect (struct file *f, int select_cmd, int io_mode,
	   fd_set *ignored, u_long *also_ignored)
{
  usetup;
  int result;

  if (select_cmd == SELCMD_PREPARE)
    {
      /* always possible... perhaps return ~0 in this case ???? */
      if (io_mode != SELMODE_IN) return 0;
      SelLastError(f) = 0;

      /* 10 seconds waittime */
      if (!SELPKT_IN_USE(f))
        SelSendPacket1(f, __selport, ACTION_WAIT_CHAR, 10 * 1000000);
      return 1 << __selport->mp_SigBit;
    }
  else if (select_cmd == SELCMD_CHECK)
    {
      /* only read is supported, other modes default to `ok' */
      if (io_mode != SELMODE_IN) return 1;

      if (SELPKT_IN_USE(f))
        return 0;

      /* there are two possible answers: error (packet not supported) 
       * and the `real' answer.
       * An error is treated as to allow input, so select() won't block
       * indefinitely...
       * & 1 converts dos-true (-1) into normal true (1) ;-)
       */
      result = SelLastError(f) ? 1 : (SelLastResult(f) & 1);
      /* don't make __write() think its last packet failed.. */
      SelLastError(f) = 0;
      return result;
    }
  else if (select_cmd == SELCMD_POLL)
    {
      /* only read is supported, other modes default to `ok' */
      if (io_mode != SELMODE_IN) return 1;

      __get_file (f);

      result = !IsInteractive(CTOBPTR(f->f_fh)) || WaitForChar(CTOBPTR(f->f_fh), 0);
      __release_file (f);
      return result;
    }
  else
    return 0;
}
