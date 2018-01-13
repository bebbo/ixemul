/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1991, 1992  Markus M. Wild
 *  Portions Copyright (C) 1994 Rafael W. Luebbert
 *  Portions Copyright (C) 1996 Jeff Shepherd
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
#include <stdlib.h>

UWORD __amiga2unixid(UWORD id)
{
  switch(id) {
    case (UWORD)(-1):
      return(0);
    case (UWORD)(-2):
      return((UWORD)(-1));
    case 0:
      return((UWORD)(-2));
    default:
      return(id);
  }
}

UWORD __unix2amigaid(UWORD id)
{
  switch(id) {
    case (UWORD)(-1):
      return((UWORD)(-2));
    case (UWORD)(-2):
      return(0);
    case 0:
      return((UWORD)(-1));
    default:
      return(id);
  }
}
