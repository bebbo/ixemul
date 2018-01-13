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
 *  $Id: profil.c,v 1.2 1994/06/19 15:14:32 rluebbert Exp $
 *
 *  $Log: profil.c,v $
 *  Revision 1.2  1994/06/19  15:14:32  rluebbert
 *  *** empty log message ***
 *
 *
 */
#define _KERNEL
#include "ixemul.h"

int
profil (char *buff, int bufsiz, int offset, int scale)
{
  usetup;

  Disable();	/* prevent ix_timer from accessing these variables */
  u.u_prof.pr_base  = (short *)buff;
  u.u_prof.pr_size  = bufsiz;
  u.u_prof.pr_off   = offset;
  u.u_prof.pr_scale = scale;
  Enable();
  
  return (int)&u.u_prof_last_pc;
}
