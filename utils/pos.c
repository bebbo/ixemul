/*
    Ixprefs v.2.7--ixemul.library configuration program
    Copyright © 1995,1996 Kriton Kyrimis

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef NO_POS_SUPPORT

#include "ixprefs.h"

int
OpenPOSLibraries(void)
{
  return 0;
}

void
ClosePOSLibraries(void)
{
}

void
POSCleanup(void)
{
}

int
POSGUI(void)
{
  return FAILURE;
}

#endif /* NO_POS_SUPPORT */
