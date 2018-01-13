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
 *  statfs.c,v 1.1.1.1 1994/04/04 04:31:00 amiga Exp
 *
 *  statfs.c,v
 * Revision 1.1.1.1  1994/04/04  04:31:00  amiga
 * Initial CVS check in.
 *
 *  Revision 1.2  1992/08/09  21:00:32  amiga
 *  add alternate fs id's...
 *
 *  Revision 1.1  1992/05/22  01:50:27  mwild
 *  Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"
#include <sys/mount.h>
#include <string.h>

#ifndef ID_FFS_DISK
#define ID_FFS_DISK		(0x444F5301L)	/* 'DOS\1' */
#define ID_INTER_DOS_DISK	(0x444F5302L)	/* 'DOS\2' */
#define ID_INTER_FFS_DISK	(0x444F5303L)	/* 'DOS\3' */
#define ID_MSDOS_DISK		(0x4d534400L)	/* 'MSD\0' */
#endif

/* no comments here ;-) */
#define ID_ALT_OFS_DISK		(0x444F5304L)	/* 'DOS\4' */
#define ID_ALT_FFS_DISK		(0x444F5305L)	/* 'DOS\5' */
#define ID_ALT_INTER_DOS_DISK	(0x444F5306L)	/* 'DOS\6' */
#define ID_ALT_INTER_FFS_DISK	(0x444F5307L)	/* 'DOS\7' */

/* dunno, would be logical ;-)) */
#define ID_NFS_DISK		(0x4E465300)	/* 'NFS\0' */	
/* did somebody already do this ??? */
#define ID_UFS_DISK		(0x55465300)	/* 'UFS\0' */	


/* this function fills in InfoData information from a statfs structure
   that has at least its f_fsid field set to the handler */
static void
internal_statfs (struct statfs *buf, struct InfoData *info, struct StandardPacket *sp)
{
  usetup;

  sp->sp_Pkt.dp_Port = u.u_sync_mp;
  sp->sp_Pkt.dp_Type = ACTION_DISK_INFO;
  sp->sp_Pkt.dp_Arg1 = CTOBPTR (info);
  PutPacket ((struct MsgPort *) buf->f_fsid.val[0], sp);
  __wait_sync_packet (sp);
      
  /* if packet is implemented on handler side */
  if (sp->sp_Pkt.dp_Res1 == -1)
    {
      struct DeviceList *dl = BTOCPTR (info->id_VolumeNode);

      if (dl)
	{
          char *name = BTOCPTR (dl->dl_Name);
	  int len = (MNAMELEN - 2 < name[0]) ? MNAMELEN - 2 : name[0];

	  /* use this field to represent the volume name */
          bcopy (name + 1, buf->f_mntfromname + 1, len);
          buf->f_mntfromname[0] = '/';
          buf->f_mntfromname[len + 1] = 0;
        }
      else
	/* for stuff like FIFO that doesn't have an associated volume */
	strcpy (buf->f_mntfromname, buf->f_mntonname);

      switch (info->id_DiskType)
        {
        case ID_DOS_DISK:
        case ID_ALT_OFS_DISK:
          buf->f_type = MOUNT_ADOS_OFS;
	  break;
	      
	case ID_FFS_DISK:
	case ID_ALT_FFS_DISK:
	  buf->f_type = MOUNT_ADOS_FFS;
	  break;

	case ID_INTER_DOS_DISK:
	case ID_ALT_INTER_DOS_DISK:
	  buf->f_type = MOUNT_ADOS_IOFS;
	  break;

	case ID_INTER_FFS_DISK:
	case ID_ALT_INTER_FFS_DISK:
	  buf->f_type = MOUNT_ADOS_IFFS;
	  break;

	case ID_MSDOS_DISK:
	  buf->f_type = MOUNT_PC;
	  break;
	  
	case ID_NFS_DISK:
	  buf->f_type = MOUNT_NFS;
	  break;
	  
	case ID_UFS_DISK:
	  buf->f_type = MOUNT_UFS;
	  break;

	default:	      
	  buf->f_type = MOUNT_NONE;
	  break;
	}
	    
      buf->f_flags  = MNT_NOSUID | MNT_NODEV;
      if (info->id_DiskState != ID_VALIDATED)
        /* can be ID_WRITE_PROTECTED or ID_VALIDATING */
	buf->f_flags |= MNT_RDONLY;
	    
      buf->f_fsize  = info->id_BytesPerBlock;
      buf->f_bsize  = info->id_BytesPerBlock * ix.ix_fs_buf_factor;
      buf->f_blocks = info->id_NumBlocks;
      buf->f_bfree =
        buf->f_bavail = info->id_NumBlocks - info->id_NumBlocksUsed;
      /* no inode information available, thus set to -1 */
      buf->f_files =
      buf->f_ffree = -1;
   }
}


int
getfsstat (struct statfs *buf, long bufsize, int flags)
{
  int num_devs = 0;
  struct statfs *orig_buf = buf;
  long orig_bufsize = bufsize;
  
  struct DosLibrary *dl;
  struct RootNode *rn;
  struct DosInfo *di;
  struct DevInfo *dv;
  
  struct InfoData *info;
  struct StandardPacket *sp;

  /* could probably use less drastic measures under 2.0... */
  Forbid ();
  dl = DOSBase;
  rn = (struct RootNode *) dl->dl_Root;
  di = BTOCPTR (rn->rn_Info);
  for (dv = BTOCPTR (di->di_DevInfo); dv; dv = BTOCPTR (dv->dvi_Next))
    {
      if (dv->dvi_Type == DLT_DEVICE && dv->dvi_Task && dv->dvi_Name)
	{
	  num_devs ++;
	  if (buf && bufsize >= sizeof (*buf))
	    {
	      char *name = BTOCPTR (dv->dvi_Name);
	      int len = (MNAMELEN - 2 < name[0]) ? MNAMELEN - 2 : name[0];

	      /* only remember the name and the address of the
	         handler so that we can later send INFO packets there */
	      bzero (buf, sizeof (*buf));
	      bcopy (name + 1, buf->f_mntonname + 1, len);
	      buf->f_mntonname[0] = '/';
	      buf->f_mntonname[len + 1] = 0;
	      buf->f_fsid.val[1] = 0;
	      buf->f_fsid.val[0] = (long) dv->dvi_Task;
	      
	      buf++;
	      bufsize -= sizeof (struct statfs);
	    }
	}
    }
  Permit ();

  if (! orig_buf || orig_bufsize < sizeof (struct statfs))
    return num_devs;

  info = alloca (sizeof (*info) + 2);
  info = LONG_ALIGN (info);

  sp = alloca (sizeof (*sp) + 2);
  sp = LONG_ALIGN (sp);
  __init_std_packet (sp);

  /* have a second run, and inquire more data from the associated volumes */
  while (orig_buf < buf)
    {
      internal_statfs (orig_buf, info, sp);
      orig_buf++;
    }

  return num_devs;
}

int
fstatfs (int fd, struct statfs *buf)
{
  usetup;
  struct file *f = u.u_ofile[fd];
  struct DosLibrary *dl;
  struct RootNode *rn;
  struct DosInfo *di;
  struct DevInfo *dv;
  
  struct InfoData *info;
  struct StandardPacket *sp;

  if ((unsigned)fd < NOFILE && f)
    {
      if (! buf)
        {
          errno = EFAULT;
	  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
          return -1;
        }

      if (HANDLER_NIL (f) || f->f_type != DTYPE_FILE)
        {
          errno = EOPNOTSUPP;
	  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
	  return -1;
	}

      bzero (buf, sizeof (struct statfs));
      buf->f_fsid.val[1] = 0;
      buf->f_fsid.val[0] = (long) f->f_fh->fh_Type;
      /* could probably use less drastic measures under 2.0... */
      Forbid ();
      dl = DOSBase;
      rn = (struct RootNode *) dl->dl_Root;
      di = BTOCPTR (rn->rn_Info);
      for (dv = BTOCPTR (di->di_DevInfo); dv; dv = BTOCPTR (dv->dvi_Next))
        {
          if (dv->dvi_Type == DLT_DEVICE && 
	      (struct MsgPort *) dv->dvi_Task == f->f_fh->fh_Type)
	    {
	      char *name = BTOCPTR (dv->dvi_Name);
	      int len = (MNAMELEN - 2 < name[0]) ? MNAMELEN - 2 : name[0];

	      bcopy (name + 1, buf->f_mntonname + 1, len);
	      buf->f_mntonname[0] = '/';
	      buf->f_mntonname[len + 1] = 0;
	      break;
	    }
	}
      Permit ();

      if (! dv)
	{
	  errno = EOPNOTSUPP;
	  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
          return -1;
        }

      info = alloca (sizeof (*info) + 2);
      info = LONG_ALIGN (info);

      sp = alloca (sizeof (*sp) + 2);
      sp = LONG_ALIGN (sp);
      __init_std_packet (sp);

      internal_statfs (buf, info, sp);

      return 0;
    }
    
  errno = EBADF;  
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
  return -1;
}


int
statfs (const char *path, struct statfs *buf)
{
  usetup;
  struct MsgPort *handler;
  BPTR lock;
  int omask, err;
  struct DosLibrary *dl;
  struct RootNode *rn;
  struct DosInfo *di;
  struct DevInfo *dv;
  
  struct InfoData *info;
  struct StandardPacket *sp;

  omask = syscall (SYS_sigsetmask, ~0);
  lock = __lock ((char *)path, ACCESS_READ);
  handler = lock ? ((struct FileLock *) BTOCPTR (lock))->fl_Task : 0;
  err = __ioerr_to_errno (IoErr ());
  if (lock)
    __unlock (lock);
  syscall (SYS_sigsetmask, omask);
  
  if (! handler)
    {
      errno = err;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return -1;
    }
  else
    {
      if (! buf)
        {
          errno = EFAULT;
	  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
          return -1;
        }

      bzero (buf, sizeof (struct statfs));
      buf->f_fsid.val[1] = 0;
      buf->f_fsid.val[0] = (long) handler;
      /* could probably use less drastic measures under 2.0... */
      Forbid ();
      dl = DOSBase;
      rn = (struct RootNode *) dl->dl_Root;
      di = BTOCPTR (rn->rn_Info);
      for (dv = BTOCPTR (di->di_DevInfo); dv; dv = BTOCPTR (dv->dvi_Next))
        {
          if (dv->dvi_Type == DLT_DEVICE && 
	      (struct MsgPort *) dv->dvi_Task == handler)
	    {
	      char *name = BTOCPTR (dv->dvi_Name);
	      int len = (MNAMELEN - 2 < name[0]) ? MNAMELEN - 2 : name[0];

	      bcopy (name + 1, buf->f_mntonname + 1, len);
	      buf->f_mntonname[0] = '/';
	      buf->f_mntonname[len + 1] = 0;
	      break;
	    }
	}
      Permit ();

      if (! dv)
	{
	  errno = EOPNOTSUPP;
	  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
          return -1;
        }

      info = alloca (sizeof (*info) + 2);
      info = LONG_ALIGN (info);

      sp = alloca (sizeof (*sp) + 2);
      sp = LONG_ALIGN (sp);
      __init_std_packet (sp);

      internal_statfs (buf, info, sp);

      return 0;
    }
}
