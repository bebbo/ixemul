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
 *  symlink.c,v 1.1.1.1 1994/04/04 04:30:36 amiga Exp
 *
 *  symlink.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:36  amiga
 * Initial CVS check in.
 *
 *  Revision 1.2  1992/07/04  19:21:58  mwild
 *  cut a possibly leading `./' in symlink contents. This is a kludge, but it's
 *  the only way to get `configure' working without changes.
 *
 * Revision 1.1  1992/05/14  19:55:40  mwild
 * Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include <string.h>

#ifndef LINK_SOFT
#define LINK_SOFT 1
#endif

static void u2a(const char *orig, char *new)
{
  if (index(orig, ':'))
  {
    strcpy(new, orig);
    return;
  }
  if (*orig == '/')
  {
    while (*orig == '/')
      orig++;
    while (*orig && *orig != '/')
      *new++ = *orig++;
    *new++ = ':';
    if (*orig)
      orig++;
  }
  while (*orig)
  {
    if (!memcmp(orig, "../", 3))
    {
      orig += 3;
      *new++ = '/';
    }
    else if (!memcmp(orig, "./", 2))     
      orig += 2;
    else if (*orig == '/')
      orig++;
    else if (!strcmp(orig, ".."))
    {
      orig += 2;
      *new++ = '/';
    }
    else if (!strcmp(orig, "."))
      orig++;
    else
    {
      while (*orig && *orig != '/')
        *new++ = *orig++;
      if (*orig == '/')
        *new++ = *orig++;
    }
  }
  *new = '\0';
}

int symlink (const char *old, const char *new)
{
  char *lstr;
  usetup;

  // Sanity check: AFS has problems with symlink("", ""), it creates
  // a 'directory' that can't be deleted.
  //
  // You can also test this with: 'MakeLink("", "", 1);'
  if (!old || !new || !strlen(old) || !strlen(new))
    errno_return(ENOENT, -1);

  /* is long-alignment needed here? This is DOS, isn't it... */
  lstr = alloca (strlen (old) + 4);
  lstr = LONG_ALIGN (lstr);
  u2a(old, lstr);
  if (__make_link ((char *)new, (BPTR)lstr, LINK_SOFT))
    return 0;
  errno = __ioerr_to_errno(IoErr());
  return -1;
}
