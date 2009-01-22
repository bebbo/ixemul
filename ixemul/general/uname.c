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
 *  12/6/97 :   Add OS release check routine
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
#include <exec/resident.h>

#if defined(__MORPHOS__)
static char sysname[] = "MorphOS";
#else
static char sysname[] = "AmigaOS";
#endif
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
  struct Resident *m_res;

  if (name)
    {

      if (gethostname(&host[0],__SYS_NMLN) == 0)
	{
	  
	  if ((m_res = FindResident("MorphOS")))
	    {
	      strncpy (name->sysname, "MorphOS", __SYS_NMLN);
#if 1
	      /* Proper code to get release & version strings - Piru
	       */
#ifdef __MORPHOS__
	      /* old includes lack RTF_EXTENDED and rt_Revision */
	      if (m_res->rt_Flags & RTF_EXTENDED)
	      {
	        sprintf(release, "%d.%d", m_res->rt_Version, m_res->rt_Revision);
	      }
	      else
#endif
	      {
	        /* Anchient MorphOS: parse string (example): MorphOS ID 0.4 (dd.mm.yyyy) */
	        CONST_STRPTR ps = m_res->rt_IdString;
	        CONST_STRPTR p = ps;
	        STRPTR d = release;
	        if (p && *p)
	        {
	          while (*p && *p != '.') p++;
	          if (*p)
	          {
	            while (p > ps && p[-1] != ' ') p--;

	            while (*p && *p != ' ')
	            {
	              *d++ = *p++;
	            }
	            *d = '\0';
	          }
	        }
	        if ((char *) d == release)
	        {
	          /* oops, no clue of revision */
	          sprintf(release, "%d.?", m_res->rt_Version);
	        }
	      }
	      sprintf(version, "%d.%d", SysBase->LibNode.lib_Version, SysBase->SoftVer);
#else
	      strcpy(release, "0.1"); /* shrug */
	      sprintf(version, "%hd", m_res->rt_Version);
#endif
	      strcpy(machine, "powerpc");
	    }
	  else
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
	    }
	  strncpy (name->nodename, &host[0],__SYS_NMLN);
	  strncpy (name->version, version,__SYS_NMLN);
	  strncpy (name->machine, machine,__SYS_NMLN);
	  strncpy (name->release, release,__SYS_NMLN);
	  res = err = 0;
	}
    }

  errno = err;
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
  return res;
}
