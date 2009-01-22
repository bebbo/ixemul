
#define _KERNEL
#include "ixemul.h"

ino_t get_unique_id(BPTR lock, void *fh)
{
  ino_t result;

  if (fh)
    lock = DupLockFromFH(CTOBPTR(fh));
  if (lock)
    /* The fl_Key field of the FileLock structure is always unique. This is
       also true for filesystems like AFS and AFSFloppy. Thanks to Michiel
       Pelt (the AFS author) for helping me with this. */
    result = (ino_t)((struct FileLock *)(BTOCPTR(lock)))->fl_Key;
  else
    result = (ino_t)fh;
  if (fh)
    UnLock(lock);
  return result;
}

