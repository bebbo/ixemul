/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1995 Lars G. Hecking
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
 *  12/6/97 :	Add OS release check routine
 *             Add machine check routine
 *             David Zaroski <cz253@cleveland.freenet.edu>
 *
 *
 */


#include <sys/utsname.h>

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static char sysname[] = "AmigaOS";
static char version[__SYS_NMLN] = "1";

int
uname (struct utsname *name)
{
  usetup;
  char host[__SYS_NMLN];
  char release[__SYS_NMLN];
  char machine[__SYS_NMLN] = "";
  int res = -1, err = EFAULT;
  UWORD attnflags;

  if (name)
    {

      if (gethostname(&host[0],__SYS_NMLN) == 0)
	{
	  sprintf(version, "%d.%d", SysBase->LibNode.lib_Version, SysBase->SoftVer);
	  attnflags = SysBase->AttnFlags;
/*
 * Check to see if a "m68k" is present
 */
	  if (attnflags & 255) sprintf(machine, "%s", "m68k");

	  switch (SysBase->LibNode.lib_Version)
	  {
		case 34:
			sprintf(release, "%s","1.3");
			break;
		case 35:
			sprintf(release,"%s","1.3");
			break;
		case 36:
			sprintf(release,"%s","2.0");
			break;
		case 37:
			sprintf(release,"%s","2.04");
			break;
		case 38:
			sprintf(release,"%s","2.1");
			break;
		case 39:
			sprintf(release,"%s","3.0");
			break;
		case 40:
			sprintf(release,"%s","3.1");
			break;

		default:
			sprintf(release,"%s","unknown");
			break;
	  }
	  strncpy (name->sysname, sysname,__SYS_NMLN);
	  strncpy (name->nodename, &host[0],__SYS_NMLN);
	  strncpy (name->release, release,__SYS_NMLN);
	  strncpy (name->version, version,__SYS_NMLN);
	  strncpy (name->machine, machine,__SYS_NMLN);
	  res = err = 0;
	}
    }

  errno = err;
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
  return res;
}
