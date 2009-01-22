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
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

/* this one is for Mike B. Smith ;-) */

void set_dir_name_from_lock(BPTR lock)
{
  char *buf = (char *) kmalloc (MAXPATHLEN);

  if (buf)
    {
      /* NOTE: This shortcuts any symlinks. But then, Unix does the
       *       same, and a shell that wants to be smart about symlinks,
       *       has to track chdir()s itself as well */
      if (NameFromLock (lock, buf, MAXPATHLEN))
	SetCurrentDirName (buf);

      kfree (buf);
    }
}

/*
 * Checks if pathname "name1" is above
 * "name" in directory structure.
 * Lock()/Unlock() are used instead of __lock/__unlock
 * because this routine seems to loop if name2 is a subdir of name1
 */
static short
dirisparent (char *name1, char *name2)
{
    short ret = 0;
    BPTR lock1;

    lock1 = Lock (name1, SHARED_LOCK);
    if (lock1) {
	BPTR lock2;

	lock2 = Lock (name2, SHARED_LOCK);
	if (lock2) {
	    switch (SameLock (lock1, lock2)) {

		case LOCK_DIFFERENT:
		break;

		case LOCK_SAME:
		    ret = 2;
		break;

		case LOCK_SAME_VOLUME:
		{
		    BPTR l;

		    while (lock2) {
			l = lock2;
			lock2 = ParentDir (l);
			UnLock (l);
			if (SameLock (lock1, lock2) == LOCK_SAME) {
			    ret = 1;
			    break;
			}
		    }
		    break;
		}
	    }
	    UnLock (lock2);
	}
	UnLock (lock1);
    }
    return ret;
}

/* if we change our directory, we have to remember the original cd, when
 * the process was started, because we're not allowed to unlock this
 * lock, since we didn't obtain it. */

int chdir (char *path)
{
  BPTR oldlock, newlock;
  int error = 0;
  int omask;
  struct stat stb;
  usetup;

  /* Sigh... CurrentDir() is a DOS-library function, it would probably be
   * ok to just use pr_CurrentDir, but alas, this way we're conformant to
   * programming style guidelines, but we pay the overhead of locking dosbase
   */

  /* chdir("/") with u.u_root_directory set, chdir's to root directory */
  if (!strcmp("/",path) && *u.u_root_directory)
    path = u.u_root_directory;

  if (syscall (SYS_stat, path, &stb) == 0 && !(stb.st_mode & S_IFDIR))
  {
    errno = ENOTDIR;
    KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
    return -1;
  }

  omask = syscall (SYS_sigsetmask, ~0);

  newlock = __lock (path, ACCESS_READ);

  if (newlock == NULL && IoErr() == 6262)
    {
      u.u_is_root = 1;
      SetCurrentDirName("/");
      syscall (SYS_sigsetmask, omask);
      return 0;
    }
  else if (newlock)
    {
      /* chroot() support - don't do a chdir if the path
       * is a parent directory of the root directory
       */
      if (*u.u_root_directory) {
	char dir[MAXPATHLEN];

	if (NameFromLock (newlock, dir, MAXPATHLEN)) {
	  if (dirisparent(dir,u.u_root_directory) == 1) {
	    errno = EACCES;
	    goto chdirerr;
	  }
	}
	else {
	  errno = __ioerr_to_errno (IoErr ());
	  goto chdirerr;
	}
      }

      u.u_is_root = 0;
      oldlock = CurrentDir (newlock);

      if (u.u_startup_cd == (BPTR)-1)
	u.u_startup_cd = oldlock;
      else
	__unlock (oldlock);

      set_dir_name_from_lock(newlock);

      syscall (SYS_sigsetmask, omask);
      return 0;
    }
  error = __ioerr_to_errno (IoErr ());

chdirerr:
  syscall (SYS_sigsetmask, omask);
  errno = error;
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
  return -1;
}

/* change the "root" directory to dir */
int chroot(char *dir)
{
    int retval;
    usetup;

    retval = chdir(dir);

    if (retval == 0) {
	BPTR rootlock = __lock(dir,ACCESS_READ);
	if (rootlock)
	    NameFromLock(rootlock,u.u_root_directory,MAXPATHLEN);
	else
	    retval = -1;
	__unlock(rootlock);
    }
    return retval;
}
