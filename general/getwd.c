/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1991, 1992  Ray Burr
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

/* Written and Copyright by Ray Burr. 
 * Put under the GNU Library General Public License.
 * Thanks Ray !
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"
#include <string.h>

#ifndef MAXPATHLEN
#define MAXPATHLEN 1024
#endif

/* _GetPathFromLock - Given a pointer to an AmigaOS Lock structure, build
   a rooted path string in a buffer of a given size.  The buffer should be
   at least 'buffer_length' bytes.  Returns a pointer to the result
   or NULL on an error.  'errno' will be set in the case of an error.  This
   function returns the path string based at 'buffer[0]' but it can alter
   any part of the buffer.  */

static char *
_GetPathFromLock (BPTR lock, char *buffer, int buffer_length)
{
  usetup;
  char *p;
  BPTR fl, next_fl;
  int length;
  struct FileInfoBlock *fib;
  int omask;
  char *result = 0;

  /* Allocate space on stack for fib. */

  BYTE fib_Block[sizeof (struct FileInfoBlock) + 2];

  /* Make sure fib is longword aligned. */

  fib = (struct FileInfoBlock *) fib_Block;
  if (((ULONG) fib & 0x02) != 0)
    fib = (struct FileInfoBlock *) ((ULONG) fib + 2);

  p = 0L;

  /* Duplicate the lock so that the directory structure can't change
     while we're doing this. */

  omask = syscall (SYS_sigsetmask, ~0);
  fl = DupLock (lock);

  /* Follow the chain of directories and build the name in 'buffer' */

  while (fl != 0L)
    {
      if (Examine (fl, fib) == DOSFALSE)
	{
	  errno = __ioerr_to_errno(IoErr());
	  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
	  UnLock (fl);
	  goto ret;
	}
      next_fl = ParentDir (fl);
      UnLock (fl);
      if (p != 0L)
	{
	  if (next_fl != 0L)
	    *--p = '/';
	  else
	    *--p = ':';
	}
      else
	{
	  p = buffer + buffer_length - 1;	/* fix 20-jan-92 ## mw */
	  *p = '\0';
	  if (next_fl == 0L)
	    *--p = ':';
	}
      length = strlen (fib->fib_FileName);
      p -= length;
      if (p <= buffer)
	{
	  if (next_fl != 0L)
	    UnLock (next_fl);
	  goto ret;
	}
      bcopy (fib->fib_FileName, p, length);
      fl = next_fl;
    }

  /* Move the pathname so that it starts at buffer[0]. */

  bcopy (p, buffer, strlen (p) + 1);

  result = buffer;

ret:
  syscall (SYS_sigsetmask, omask);
  return result;
}


static char *
_get_pwd (char *buffer, int buffer_length)
{
  usetup;
  struct Process *proc;
  char *result, *colon;
  extern char *index (const char *, int);

  if (u.u_is_root)
    {
      strcpy(buffer, "/");
      return buffer;
    }

  proc = (struct Process *) FindTask (0L);

  /* Just return an empty string if this is not a process. */

  if (proc == 0L || proc->pr_Task.tc_Node.ln_Type != NT_PROCESS)
    {
      buffer[0] = '\0';
      return buffer;
    }

  /* make room for slash */
  if (ix.ix_flags & ix_translate_slash)
    buffer++;

  if (GetCurrentDirName (buffer, buffer_length) ||
      NameFromLock((BPTR)proc->pr_CurrentDir, buffer, buffer_length))
    {
      result = buffer;
      goto returnit;
    }
  /* and as the last chance resort to the 1.3 algorithm */

  result = _GetPathFromLock((BPTR)proc->pr_CurrentDir, buffer, buffer_length);

returnit:
  if ((ix.ix_flags & ix_translate_slash) && result)
    {
      colon = index (result, ':');
      if (colon)
        {
	  *colon = '/';
          result--;
          result[0] = '/';
	  return result;
        }

       bcopy (result, result - 1, strlen (result) + 1);
       return result - 1;
    }

  return result;
}


char *
getwd (char *buffer)
{
  usetup;
  char *path;

  path = _get_pwd (buffer, MAXPATHLEN);
  if (path == 0L)
    {
      strcpy (buffer, "getwd - ");
      strcat (buffer, strerror (errno));
      return 0L;
    }

  return path;
}


char *
getcwd (char *buffer, size_t buffer_length)
{
  usetup;

  if (buffer == 0L)
    {
      buffer = (char *) syscall (SYS_malloc, buffer_length + 2);
      if (buffer == 0L)
	{
	  errno = ENOMEM;
	  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
	  return 0L;
	}
    }

  return _get_pwd (buffer, buffer_length);
}
