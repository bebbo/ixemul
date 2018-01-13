/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1994  Rafael W. Luebbert
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
#include <ixemul.h>
#include <stdarg.h>
#include <proto/intuition.h>

void ix_panic(const char *msg, ...)
{
  struct IntuitionBase *IntuitionBase;
  u_char old_flags;
  struct Task *me = FindTask(0);
  va_list ap;

  /*
   * This function may be called with our signals enabled. So we have to make
   * sure that no signals are processed while in here.
   * Since this function may be called at random points in initialization,
   * it is not safe to call sigsetmask() here, so I have to resort to the
   * more drastic solution of temporarily turning off TF_LAUNCH
   */
  old_flags = me->tc_Flags;
  me->tc_Flags &= ~TF_LAUNCH;

  va_start(ap, msg);

  if ((IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 0)))
    {
      struct EasyStruct panic = {
        sizeof(struct EasyStruct),
        0,
        "ixnet.library message",
        (char *)msg,
        "Abort"
      };
        
      EasyRequestArgs(NULL, &panic, NULL, ap);

      CloseLibrary ((struct Library *) IntuitionBase);
   }

  me->tc_Flags = old_flags;
  va_end(ap);
}
