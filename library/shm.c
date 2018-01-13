#define _KERNEL
#include "ixemul.h"
#include <sys/ipc.h>
#include <sys/ucred.h>
#include <sys/shm.h>
#include <time.h>

struct shm_list *shmlist = NULL;

static struct shmid_ds *shm_find_ds_by_key(key_t key)
{
  struct shm_list *l = shmlist;
  
  while (l && l->ds.shm_perm.key != key)
    l = l->next;
  return (struct shmid_ds *)l;
}

static struct shmid_ds *shm_find_ds_by_shmid(int shmid)
{
  struct shm_list *l = shmlist;
  
  while (l && IPC_SHMID(l->ds.shm_perm) != shmid)
    l = l->next;
  return (struct shmid_ds *)l;
}

static struct shmid_ds *shm_find_ds_by_addr(void *addr)
{
  struct shm_list *l = shmlist;
  
  while (l && l->ds.shm_internal != addr)
    l = l->next;
  return (struct shmid_ds *)l;
}

static int
shmget_allocate_segment(struct ucred *cred, key_t key, int size, int shmflg)
{
  struct shmid_ds *shmseg;
  struct shm_list *l;
  void *data;
  static ushort seq = 0;
  usetup;
	
  if (size < 1)
    errno_return(EINVAL, -1);

  l = kmalloc(sizeof(struct shm_list));
  if (l == NULL)
    errno_return(ENOSPC, -1);
  data = kmalloc(size);
  if (data == NULL)
  {
    kfree(l);
    errno_return(ENOSPC, -1);
  }

  shmseg = &l->ds;
  shmseg->shm_perm.key = key;
  shmseg->shm_perm.seq = ++seq;
  shmseg->shm_internal = data;
  shmseg->shm_perm.cuid = shmseg->shm_perm.uid = cred->cr_uid;
  shmseg->shm_perm.cgid = shmseg->shm_perm.gid = cred->cr_gid;
  shmseg->shm_perm.mode = (shmflg & ACCESSPERMS) | SHMSEG_ALLOCATED;
  shmseg->shm_segsz = size;
  shmseg->shm_cpid = getpid();
  shmseg->shm_lpid = shmseg->shm_nattch = 0;
  shmseg->shm_atime = shmseg->shm_dtime = 0;
  shmseg->shm_ctime = time(NULL);
  l->next = shmlist;
  shmlist = l;
  return IPC_SHMID(shmseg->shm_perm);
}

static int _shmget(key_t key, int size, int shmflg)
{
  struct shmid_ds *ds = NULL;
  struct ucred cred;
  usetup;

  cred.cr_uid = geteuid();
  cred.cr_gid = getegid();

  if (key != IPC_PRIVATE)
  {
    ds = shm_find_ds_by_key(key);
    
    if (!ds && !(shmflg & IPC_CREAT))
      errno_return(ENOENT, -1);
    if (ds)
    {
      int error;

      if ((error = ipcperm(&cred, &ds->shm_perm, shmflg)) != 0)
        errno_return(error, -1);
      if (size && size > ds->shm_segsz)
        errno_return(EINVAL, -1);
      if ((shmflg & (IPC_CREAT | IPC_EXCL)) == (IPC_CREAT | IPC_EXCL))
        errno_return(EEXIST, -1);
      return IPC_SHMID(ds->shm_perm);
    }
  }
  return shmget_allocate_segment(&cred, key, size, shmflg);
}

int shmget(key_t key, int size, int shmflg)
{
  int result;
  
  ix_lock_base();
  result = _shmget(key, size, shmflg);
  ix_unlock_base();
  return result;
}

static void *_shmat(int shmid, char *shmaddr, int shmflg)
{
  struct shmid_ds *ds;
  struct ucred cred;
  int error;
  usetup;

  cred.cr_uid = geteuid();
  cred.cr_gid = getegid();
  
  ds = shm_find_ds_by_shmid(shmid);
  if (ds == NULL)
    errno_return(EINVAL, (void *)-1);
  if (shmaddr && shmaddr != ds->shm_internal)
    errno_return(EINVAL, (void *)-1);
  if ((error = ipcperm(&cred, &ds->shm_perm,
                       (shmflg & SHM_RDONLY) ? IPC_R : IPC_R|IPC_W)))
    errno_return(error, (void *)-1);
  ds->shm_lpid = getpid();
  ds->shm_atime = time(NULL);
  ds->shm_nattch++;
  if (u.u_shmused == u.u_shmsize)
  {
    u.u_shmsize += 256;
    u.u_shmarray = krealloc(u.u_shmarray, 4 * u.u_shmsize);
  }
  u.u_shmarray[u.u_shmused++] = ds;
  return ds->shm_internal;  
}

void *shmat(int shmid, void *shmaddr, int shmflg)
{
  void *result;
  
  ix_lock_base();
  result = _shmat(shmid, shmaddr, shmflg);
  ix_unlock_base();
  return result;
}

static void shm_deallocate_segment(struct shmid_ds *ds)
{
  struct shm_list *l = (struct shm_list *)ds, *p;

  kfree(ds->shm_internal);
  if (shmlist == l)
  {
    shmlist = l->next;
  }
  else
  {
    for (p = shmlist; p && p->next != l; p = p->next);
    if (p)
      p->next = l->next;
  }
  kfree(l);
}

static void shm_delete_mapping(struct shmid_ds *ds)
{
  ds->shm_dtime = time(NULL);
  if ((--ds->shm_nattch <= 0) && (ds->shm_perm.mode & SHMSEG_REMOVED))
    shm_deallocate_segment(ds);
}

static int _shmdt(void *addr)
{
  struct shmid_ds *ds;
  int i;
  usetup;
  
  ds = shm_find_ds_by_addr(addr);
  if (ds == NULL)
    errno_return(EINVAL, -1);
  for (i = 0; i < u.u_shmused && u.u_shmarray[i] != ds; i++) ;
  if (i < u.u_shmused)
    memcpy(&u.u_shmarray[i], &u.u_shmarray[i + 1], 4 * (--u.u_shmused - i));
  shm_delete_mapping(ds);
  return 0;
}

int shmdt(void *addr)
{
  int result;
  
  ix_lock_base();
  result = _shmdt(addr);
  ix_unlock_base();
  return result;
}

static int _shmctl(int shmid, int cmd, struct shmid_ds *buf)
{
  struct shmid_ds *ds;
  struct ucred cred;
  int error;
  usetup;

  if (cmd == IPC_GETLIST)
    return (int)shmlist;

  cred.cr_uid = geteuid();
  cred.cr_gid = getegid();
  
  ds = shm_find_ds_by_shmid(shmid);
  if (ds == NULL)
    errno_return(EINVAL, -1);
  switch (cmd)
  {
    case IPC_STAT:
      if ((error = ipcperm(&cred, &ds->shm_perm, IPC_R)) != 0)
        errno_return(error, -1);
      *buf = *ds;
      return 0;

    case IPC_SET:
      if ((error = ipcperm(&cred, &ds->shm_perm, IPC_M)) != 0)
        errno_return(error, -1);
      ds->shm_perm.uid = buf->shm_perm.uid;
      ds->shm_perm.gid = buf->shm_perm.gid;
      ds->shm_perm.mode = (ds->shm_perm.mode & ~ACCESSPERMS) |
                 (buf->shm_perm.mode & ACCESSPERMS);
      ds->shm_ctime = time(NULL);
      return 0;

    case IPC_RMID:
      if ((error = ipcperm(&cred, &ds->shm_perm, IPC_M)) != 0)
        errno_return(error, -1);
      ds->shm_perm.key = IPC_PRIVATE;
      ds->shm_perm.mode |= SHMSEG_REMOVED;
      if (ds->shm_nattch <= 0)
	shm_deallocate_segment(ds);
      return 0;
  }
  errno_return(EINVAL, -1);
}

int shmctl(int shmid, int cmd, struct shmid_ds *buf)
{
  int result;
  
  ix_lock_base();
  result = _shmctl(shmid, cmd, buf);
  ix_unlock_base();
  return result;
}

int shmfork(struct user *parent, struct user *child)
{
  if (parent->u_shmused)
  {
    int i;

    child->u_shmarray = kmalloc(parent->u_shmused * 4);
    if (child->u_shmarray == NULL)
      return -1;
    child->u_shmused = child->u_shmsize = parent->u_shmused;
    memcpy(child->u_shmarray, parent->u_shmarray, 4 * child->u_shmused);
    for (i = 0; i < child->u_shmsize; i++)
      ((struct shmid_ds *)(child->u_shmarray[i]))->shm_nattch++;
  }
  return 0;
}

void shmexit(struct user *p)
{
  int i;

  for (i = 0; i < p->u_shmused; i++)
    shm_delete_mapping(p->u_shmarray[i]);
  if (p->u_shmsize)
    kfree(p->u_shmarray);
}
