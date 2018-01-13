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
 *  stat.c,v 1.1.1.1 1994/04/04 04:30:35 amiga Exp
 *
 *  stat.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:35  amiga
 * Initial CVS check in.
 *
 *  Revision 1.2  1993/11/05  22:03:10  mwild
 *  grp/oth support, plus new feature
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"
#include <stdio.h>
#include <string.h>
#include "multiuser.h"

/* currently, links are quite buggy.. hope this get cleaned up RSN ;-) */

long fill_stat_mode(struct stat *stb, struct FileInfoBlock *fib)
{
  long mode;

  /* If this is a directory, and Size is not 0, then this is a volume.
     Since the Protection field seems to be random for volumes, we use
     our own flags (= everything readable & writable) */
  /* Unfortunately, not all filesystem actually set size !=.
     So we use some more elaborate heuristics ... NP */ 

  mode = ((long)(fib->fib_OwnerUID)) << 16 | (u_long)(fib->fib_OwnerGID);

  if (fib->fib_DirEntryType > 0 && 
      (fib->fib_Size != 0 || 
       (mode == fib->fib_DiskKey + 1 && fib->fib_OwnerGID > 880)))
    {
      fib->fib_DirEntryType = ST_ROOT;
      fib->fib_Protection   = 0x0000ff00;
      fib->fib_OwnerUID     = 0xffff;
      fib->fib_OwnerGID     = 0xffff;
    }

  mode = 0L;

  stb->st_amode = fib->fib_Protection;
  fib->fib_Protection ^= 0xf;
  if (fib->fib_Protection & (FIBF_EXECUTE|FIBF_SCRIPT))
    mode |= S_IXUSR;
  if (fib->fib_Protection & (FIBF_WRITE|FIBF_DELETE))
    mode |= S_IWUSR;
  if (fib->fib_Protection & FIBF_READ)
    mode |= S_IRUSR;
#ifdef FIBF_GRP_EXECUTE
  /* FIBF_GRP_EXECUTE requires at least OS3 headers */
  if (fib->fib_Protection & FIBF_GRP_EXECUTE)
    mode |= S_IXGRP;
  if ((fib->fib_Protection & (FIBF_GRP_WRITE|FIBF_GRP_DELETE)) == (FIBF_GRP_WRITE|FIBF_GRP_DELETE))
    mode |= S_IWGRP;
  if (fib->fib_Protection & FIBF_GRP_READ)
    mode |= S_IRGRP;
  if (fib->fib_Protection & FIBF_OTR_EXECUTE)
    mode |= S_IXOTH;
  if ((fib->fib_Protection & (FIBF_OTR_WRITE|FIBF_OTR_DELETE)) == (FIBF_OTR_WRITE|FIBF_OTR_DELETE))
    mode |= S_IWOTH;
  if (fib->fib_Protection & FIBF_OTR_READ)
    mode |= S_IROTH;
  if (fib->fib_Protection & FIBF_HOLD)
    mode |= S_ISTXT;
  if (fib->fib_Protection & muFIBF_SET_UID)
    mode |= S_ISUID;
  if (fib->fib_Protection & muFIBF_SET_GID)
    mode |= S_ISGID;
#endif

  switch (fib->fib_DirEntryType)
    {
    case ST_LINKDIR:
      stb->st_nlink = 3;
      mode |= S_IFDIR;
      break;
    case ST_ROOT:
    case ST_USERDIR:
      stb->st_nlink = 2;
      mode |= S_IFDIR;
      break;

    /* at the moment, we NEVER get this entry, since we can't get a lock
     * on a symlink */
    case ST_SOFTLINK:
      stb->st_nlink = 1;
      mode |= S_IFLNK;
      break;

    case ST_PIPEFILE:
      /* don't use S_IFIFO, we don't have a mkfifo() call ! */
      mode |= S_IFCHR;
      break;

    case ST_LINKFILE:
      stb->st_nlink = 2;
      mode |= S_IFREG;
      break;

    case ST_FILE:
    default:
      stb->st_nlink = 1;
      mode |= S_IFREG;
    }

  /* If a directory is readable, then also allow scanning. */
  if (mode & S_IFDIR)
    mode |= (mode & (S_IRUSR | S_IRGRP | S_IROTH)) >> 2;
  return mode;
}

static int
__stat(const char *fname, struct stat *stb, BPTR (*lock_func)())
{
  BPTR lock;
  struct FileInfoBlock *fib;
  struct InfoData *info;
  int omask, err = 0;
  usetup;

  bzero (stb, sizeof(*stb));

  omask = syscall (SYS_sigsetmask, ~0);

  if (!(lock = (*lock_func) (fname, ACCESS_READ)))
    {
      err = IoErr ();

      /* take special care of NIL:, /dev/null and friends ;-) */
      if (err == 4242)
        {
          stb->st_uid = stb->st_gid = 0;
          stb->st_mode = S_IFCHR | 0777;
          stb->st_nlink = 1;
          stb->st_blksize = ix.ix_fs_buf_factor * 512;
          stb->st_blocks = 0;
          goto rest_sigmask;
        }

      /* take special care of /dev/ptyXX and /dev/ttyXX */
      if (err == 5252)
        {
          stb->st_uid = (uid_t)syscall(SYS_geteuid);
          stb->st_gid = (gid_t)syscall(SYS_getegid);
          stb->st_mode = S_IFCHR | 0777;
          stb->st_nlink = 1;
          stb->st_blksize = ix.ix_fs_buf_factor * 512;
          stb->st_blocks = 0;
          goto rest_sigmask;
        }

      /* root directory */
      if (err == 6262)
        {
          stb->st_uid = stb->st_gid = 0;
          stb->st_mode = S_IFDIR | 0777;
          stb->st_nlink = 3;
          stb->st_size = 1024;
          stb->st_blksize = ix.ix_fs_buf_factor * 512;
          stb->st_blocks = 0;
          goto rest_sigmask;
        }

      KPRINTF (("__stat: lock %s failed, err = %ld.\n", fname, err));
      if (err == ERROR_IS_SOFT_LINK)
	{
	  /* provide some default stb.. we can't get anymore info than
	   * that. Symlinks should work with Lock(), but till now they
	   * don't ;-( */
	  stb->st_handler = (int) DeviceProc ((UBYTE *)fname);
	  stb->st_dev = (dev_t) stb->st_handler;
	  /* HELP! no way to reach the fib of this entry except when
	   * scanning the parent directory, but this is NOT acceptable ! */
	  stb->st_ino = 123;
          stb->st_uid = stb->st_gid = -2;
	  /* this is the most important entry ;-) */
	  stb->st_mode = S_IFLNK | 0777;
	  stb->st_size = 1024; /* again, this should be available... */
	  stb->st_nlink = 1;
	  stb->st_rdev = 0;
	  /* again, these values ARE accessible, but only by ExNext */
	  stb->st_atime = stb->st_mtime = stb->st_ctime = 0;
	  stb->st_blksize = stb->st_blocks = 0;
	  goto rest_sigmask;
        }
error:
      syscall (SYS_sigsetmask, omask);
      errno = __ioerr_to_errno (err);
      KPRINTF (("ioerr = %ld, &errno = %lx, errno = %ld\n", err, &errno, errno));
      return -1;
    }
  KPRINTF (("__stat: lock %s ok.\n",fname));

  /* alloca() returns stack memory, so it's guaranteed to be word 
   * aligned (anything else would be deadly for the sp...) Since DOS needs
   * long aligned data, we'll allocate 1 word more and adjust as needed */
  fib = alloca (sizeof(*fib) + 2);
  /* DON'T use LONG_ALIGN(alloca(..)), the argument is evaluated more than once! */
  fib = LONG_ALIGN (fib);
  fib->fib_OwnerUID = fib->fib_OwnerGID = 0;

  info = alloca (sizeof(*info) + 2);
  info = LONG_ALIGN (info);

  if (!(Examine (lock, fib)))
    {
      __unlock (lock);
      goto error;
    }

  stb->st_mode = fill_stat_mode(stb, fib);

  /* read the uid/gid data */
  stb->st_uid = __amiga2unixid(fib->fib_OwnerUID);
  stb->st_gid = __amiga2unixid(fib->fib_OwnerGID);

  /* some kind of a default-size for directories... */
  stb->st_size = fib->fib_DirEntryType < 0 ? fib->fib_Size : 1024; 
  stb->st_handler = (long)((struct FileLock *)((long)lock << 2))->fl_Task;
  stb->st_dev = (dev_t)stb->st_handler; /* trunc to 16 bit */
  stb->st_ino = get_unique_id(lock, NULL);
  stb->st_atime =
  stb->st_ctime =
  stb->st_mtime = OFFSET_FROM_1970 * 24 * 3600 +
                  ix_get_gmt_offset() +
		  fib->fib_Date.ds_Days * 24 * 60 * 60 +
		  fib->fib_Date.ds_Minute * 60 +
		  fib->fib_Date.ds_Tick/TICKS_PER_SECOND;
  /* in a try to count the blocks used for filemanagement, we add one for
   * the fileheader. Note, that this is wrong for large files, where there
   * are some extension-blocks as well */
  stb->st_blocks = fib->fib_NumBlocks + 1;
  
  bzero (info, sizeof (*info));
  stb->st_blksize = 0;
  if (S_ISREG(stb->st_mode) && Info(lock, (void *)info))
    {
      int bytesperblock = 0;

      /* optimal for fileio is as high as possible ;-) This is a
       * compromise between "as high as possible" and not too restricitve
       * for people low on memory */
      if (info->id_BytesPerBlock)
        bytesperblock = info->id_BytesPerBlock;
      stb->st_blksize = bytesperblock * ix.ix_fs_buf_factor;
      stb->st_blocks = (stb->st_blocks * bytesperblock) / 512;
    }

  if (! stb->st_blksize) stb->st_blksize = 512;

  __unlock (lock);
  if (ix.ix_unix_names)
    {
      ix_lock_base();
      if (find_unix_name(fname))
        stb->st_mode = (stb->st_mode & ~S_IFMT) | S_IFSOCK;
      ix_unlock_base();
    }

rest_sigmask:
  syscall (SYS_sigsetmask, omask);
  return 0;
}

/***************************************************************************/

int
stat (const char *fname, struct stat *stb)
{
  return __stat (fname, stb, __lock);
}

int
lstat (const char *fname, struct stat *stb)
{
  return __stat (fname, stb, __llock);
}

int
filenamecmp(const char *fname)
{
  BPTR lock;
  struct FileInfoBlock *fib;
  int omask, result = 0;

  omask = syscall (SYS_sigsetmask, ~0);

  if (!(lock = __llock((char *)fname, ACCESS_READ)))
    {
      syscall (SYS_sigsetmask, omask);
      return 0;
    }

  fib = alloca (sizeof(*fib) + 2);
  fib = LONG_ALIGN (fib);

  /* Only reject this filename if:
   *
   * 1) fname differs from fib_FileName (case sensitive compare)
   *
   * AND
   *
   * 2) compared case insensitively, fname is equal to fib_FileName.
   *
   * Test 2 is needed for links (both hard and soft), since it is
   * impossible to get the name of the link with Examine. Only
   * ExAll/ExNext can obtain the link name.
   */
   
  if (Examine (lock, fib))
    result = strcmp(basename((char *)fname), fib->fib_FileName) &&
	     !stricmp(basename((char *)fname), fib->fib_FileName);
  __unlock (lock);
  syscall (SYS_sigsetmask, omask);
  return result;
}
