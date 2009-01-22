/*
 *  This file is part of ixnet.library for the Amiga.
 *  Copyright (C) 1995 Jeff Shepherd
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
 *  $Id: ixnet_expunge.c,v 1.1.1.1 2005/03/15 15:57:09 laire Exp $
 *
 *  $Log: ixnet_expunge.c,v $
 *  Revision 1.1.1.1  2005/03/15 15:57:09  laire
 *  a new beginning
 *
 *  Revision 1.1.1.1  2000/05/07 19:37:45  emm
 *  Imported sources
 *
 *  Revision 1.1.1.1  2000/04/29 00:45:15  nobody
 *  Initial import
 *
 *
 */

#define _KERNEL
#include "ixnet.h"

extern void close_libraries(void);

void ixnet_expunge (struct ixnet_base *ixbase) {
    close_libraries();
}
