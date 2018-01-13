/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1991, 1992  Markus M. Wild
 *  Portions Copyright (C) 1996 by Jeff Shepherd
 *  Portions Copyright (C) 1996 by Hans Verkuil
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
#include "unp.h"
#include "kprintf.h"

#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/unix_socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <machine/param.h>
#include <string.h>

static struct file *getsock (int fdes);
static int soo_read   (struct file *fp, char *buf, int len);
static int soo_write  (struct file *fp, char *buf, int len);
static int soo_ioctl  (struct file *fp, int cmd, int inout, int arglen, caddr_t data);
static int soo_select (struct file *fp, int select_cmd, int io_mode, fd_set *ignored, u_long *also_ignored);
static int soo_close  (struct file *fp);

static void socket_cleanup(int ostat)
{
  usetup;

  if (CURSIG (&u))
    SetSignal (0, SIGBREAKF_CTRL_C);

  u.p_stat = ostat;

  if (errno == EINTR)
    setrun (FindTask (0));

  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
}

static void socket_cleanup_epipe(int ostat)
{
  usetup;

  if (CURSIG (&u))
    SetSignal (0, SIGBREAKF_CTRL_C);

  u.p_stat = ostat;

  /* the library doesn't send this to us of course ;-) */
  if (errno == EPIPE)
    _psignal (FindTask (0), SIGPIPE);

  if (errno == EINTR)
    setrun (FindTask (0));

  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
}

int
socket (int domain, int type, int protocol)
{
  struct file *fp;
  int fd, err, ostat, omask;
  usetup;

  if (domain == PF_UNIX)
    return unp_socket(domain, type, protocol, NULL);

  if (!u.u_ixnetbase)
    {
      errno = ENOSYS;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return -1;
    }

  ostat = u.p_stat;
  u.p_stat = SWAIT;
  omask = syscall (SYS_sigsetmask, ~0);

  do
    {
      if ((err = falloc (&fp, &fd)))
        break;

      fp->f_so = netcall(NET__socket, domain, type, protocol);
      err = (fp->f_so == -1) ? errno : 0;

      if (err)
        {
	  /* free the allocated fd */
          u.u_ofile[fd] = 0;
          fp->f_count = 0;
          break;
        }

      _set_socket_params(fp, domain, type, protocol);
    }
  while (0);

  syscall (SYS_sigsetmask, omask);
  errno = err;
  socket_cleanup(ostat);
  return err ? -1 : fd;
}


int
bind (int s, const struct sockaddr *name, int namelen)
{
  struct file *fp = getsock (s);
  int ostat, error;
  usetup;

  if (!fp)
    return -1;

  if (fp->f_type == DTYPE_USOCKET)
    return unp_bind(s, name, namelen);

  ostat = u.p_stat;
  u.p_stat = SWAIT;
  error = netcall(NET__bind, fp, name, namelen);
  socket_cleanup(ostat);
  return error;
}

int
listen (int s, int backlog)
{
  struct file *fp = getsock (s);
  int ostat, error;
  usetup;

  if (!fp)
    return -1;

  if (fp->f_type == DTYPE_USOCKET)
    return unp_listen(s, backlog);

  ostat = u.p_stat;
  u.p_stat = SWAIT;

  error = netcall(NET__listen, fp, backlog);
  socket_cleanup(ostat);
  return error;
}

int
accept (int s, struct sockaddr *name, int *namelen) 
{
  struct file *fp = getsock (s), *fp2;
  int err, fd2, ostat;
  int domain;
  usetup;

  if (!fp)
    return -1;

  if (fp->f_type == DTYPE_USOCKET)
    return unp_accept(s, name, namelen);

  ostat = u.p_stat;
  u.p_stat = SWAIT;

  do
    {
      /* first try to get a new descriptor. If that fails, don't even
         bother to call the library */
      if ((err = falloc (&fp2, &fd2)))
        break;

      fp2->f_so = netcall(NET__accept, fp, name, namelen);
      err = (fp2->f_so == -1) ? errno : 0;
      if (err)
        {
          /* the second file */
          u.u_ofile[fd2] = 0;
          fp2->f_count = 0;
          break;
        }
      domain = (fp->f_type == DTYPE_SOCKET) ? AF_INET : AF_UNIX;
      _set_socket_params(fp2, domain, fp->f_socket_type, fp->f_socket_protocol);
    }
  while (0);

  errno = err;
  socket_cleanup(ostat);
  return err ? -1 : fd2;
}


int
connect (int s, const struct sockaddr *name, int namelen)
{
  struct file *fp = getsock (s);
  int ostat, error;
  usetup;

  if (!fp)
    return -1;

  if (fp->f_type == DTYPE_USOCKET)
    return unp_connect(s, name, namelen);

  ostat = u.p_stat;
  u.p_stat = SWAIT;

  error = netcall(NET__connect, fp, name, namelen);
  socket_cleanup(ostat);
  return error;
}

int
socketpair (int domain, int type, int protocol, int sv[2])
{
  usetup;
  errno = EPFNOSUPPORT;
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
  return -1;
}

int
sendto (int s, const void *buf, int len, int flags, const struct sockaddr *to, int tolen)
{
  struct file *fp = getsock (s);
  int ostat;
  int rc;
  usetup;

  if (!fp || fp->f_type == DTYPE_USOCKET)
    return -1;

  ostat = u.p_stat;
  u.p_stat = SWAIT;

  rc = netcall(NET__sendto, fp, buf, len, flags, to, tolen);
  socket_cleanup_epipe(ostat);
  return rc;
}


int
send (int s, const void *buf, int len, int flags)
{
  struct file *fp = getsock (s);
  int ostat;
  int rc;
  usetup;

  if (!fp)
    return -1;

  if (fp->f_type == DTYPE_USOCKET)
    return unp_send(s, buf, len, flags);

  ostat = u.p_stat;
  u.p_stat = SWAIT;

  rc = netcall(NET__send, fp, buf, len, flags);
  socket_cleanup_epipe(ostat);
  return rc;
}


int
sendmsg (int s, const struct msghdr *msg, int flags)
{
  struct file *fp = getsock (s);
  int ostat, rc;
  usetup;

  if (!fp || fp->f_type == DTYPE_USOCKET)
    return -1;

  ostat = u.p_stat;
  u.p_stat = SWAIT;

  rc = netcall(NET__sendmsg, fp, msg, flags);
  socket_cleanup_epipe(ostat);
  return rc;
}


int
recvfrom (int s, void *buf, int len, int flags, struct sockaddr *from, int *fromlen)
{
  struct file *fp = getsock (s);
  int ostat, rc;
  usetup;

  if (!fp || fp->f_type == DTYPE_USOCKET)
    return -1;

  ostat = u.p_stat;
  u.p_stat = SWAIT;

  rc = netcall(NET__recvfrom, fp, buf, len, flags, from, fromlen);
  socket_cleanup(ostat);
  return rc;
}


int
recv (int s, void *buf, int len, int flags)
{
  struct file *fp = getsock (s);
  int ostat, rc;
  usetup;

  if (!fp)
    return -1;

  if (fp->f_type == DTYPE_USOCKET)
    return unp_recv(s, buf, len, flags);

  ostat = u.p_stat;
  u.p_stat = SWAIT;

  rc = netcall(NET__recv, fp, buf, len, flags);
  socket_cleanup(ostat);
  return rc;
}


int
recvmsg (int s, struct msghdr *msg, int flags)
{
  struct file *fp = getsock (s);
  int ostat, rc;
  usetup;

  if (!fp || fp->f_type == DTYPE_USOCKET)
    return -1;

  ostat = u.p_stat;
  u.p_stat = SWAIT;

  rc = netcall(NET__recvmsg, fp, msg, flags);
  socket_cleanup(ostat);
  return rc;
}


int
shutdown (int s, int how) 
{
  struct file *fp = getsock (s);
  int ostat, err;
  usetup;

  if (!fp)
    return -1;

  if (fp->f_type == DTYPE_USOCKET)
    return unp_shutdown(s, how);

  ostat = u.p_stat;
  u.p_stat = SWAIT;

  err = netcall(NET__shutdown, fp, how);
  socket_cleanup(ostat);
  return err;
}


int
setsockopt (int s, int level, int name, const void *val, int valsize)
{
  struct file *fp = getsock (s);
  int ostat, err;
  usetup;

  if (!fp)
    return -1;

  if (fp->f_type == DTYPE_USOCKET)
    return unp_setsockopt(s, level, name, val, valsize);

  ostat = u.p_stat;
  u.p_stat = SWAIT;

  err = netcall(NET__setsockopt, fp, level, name, val, valsize);
  socket_cleanup(ostat);
  return err;
}


int
getsockopt (int s, int level, int name, void *val, int *valsize)
{
  struct file *fp = getsock (s);
  int ostat, err;
  usetup;

  if (!fp)
    return -1;

  if (fp->f_type == DTYPE_USOCKET)
    return unp_getsockopt(s, level, name, val, valsize);

  ostat = u.p_stat;
  u.p_stat = SWAIT;

  err = netcall(NET__getsockopt, fp, level, name, val, valsize);
  socket_cleanup(ostat);
  return err;
}


/*
 * Get socket name.
 */
int
getsockname (int fdes, struct sockaddr *asa, int *alen)
{
  struct file *fp = getsock (fdes);
  int ostat, err;
  usetup;

  if (!fp)
    return -1;

  if (fp->f_type == DTYPE_USOCKET)
    return unp_getsockname(fdes, asa, alen);

  ostat = u.p_stat;
  u.p_stat = SWAIT;

  err = netcall(NET__getsockname, fp, asa, alen);
  socket_cleanup(ostat);
  return err;
}

/*
 * Get name of peer for connected socket.
 */
int
getpeername (int fdes, struct sockaddr *asa, int *alen)
{
  struct file *fp = getsock (fdes);
  int ostat, err;
  usetup;

  if (!fp)
    return -1;

  if (fp->f_type == DTYPE_USOCKET)
    return unp_getpeername(fdes, asa, alen);

  ostat = u.p_stat;
  u.p_stat = SWAIT;

  err = netcall(NET__getpeername, fp, asa, alen);
  socket_cleanup(ostat);
  return err;
}


int
ix_release_socket(int fdes)
{
  struct file *fp = getsock (fdes);
  int ostat, err = -1;
  usetup;

  if (!fp)
    return -1;

  if (fp->f_type == DTYPE_SOCKET)
    {
      /* If this socket has already been released before, then we just return the
         previous result. This can happen if a socket was dupped. In that case both
         file descriptors point to the same file structure and releasing one of the
         two file descriptors will in fact release them both. */
      if (fp->f_socket_id)
        {
          int retval = fp->f_socket_id;
          
          fp->f_socket_id = 0;
          return retval;
        }
      ostat = u.p_stat;
      u.p_stat = SWAIT;
      err = netcall(NET_release_socket, fp);
      socket_cleanup(ostat);
    }
  return err;
}

int
ix_obtain_socket(long id, int inet, int stream, int protocol)
{
  usetup;
  int ostat, err;
  struct file *fp2;
  int fd2;

  ostat = u.p_stat;
  u.p_stat = SWAIT;

  do
  {
    if ((err = falloc (&fp2, &fd2)))
      break;

    fp2->f_so = netcall(NET_obtain_socket,id,inet,stream,protocol);
    err = (fp2->f_so == -1) ? errno : 0;

    if (err)
    {
      /* free the allocated fd */
      u.u_ofile[fd2] = 0;
      fp2->f_count = 0;
      break;
    }

    _set_socket_params(fp2, inet, stream, protocol);
  } while (0);

  errno = err;

  socket_cleanup(ostat);
  return err ? -1 : fd2;
}


static struct file *
getsock (int fdes)
{
  struct file *fp;
  usetup;

  if ((unsigned) fdes >= NOFILE)
    {
      errno = EBADF;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return 0;
    }

  fp = u.u_ofile[fdes];

  if (fp == NULL)
    {
      errno = EBADF;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return 0;
    }

  if (fp->f_type != DTYPE_SOCKET && fp->f_type != DTYPE_USOCKET)
    {
      errno = ENOTSOCK;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return 0;
    }

  if (fp->f_type == DTYPE_SOCKET && !u.u_ixnetbase)
    {
      errno = EPIPE; /* ????? */
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
      return 0;
    }

  return (fp);
}

static int
soo_read (struct file *fp, char *buf, int len)
{
  usetup;

  return netcall(NET__tcp_read, fp, buf, len);
}

static int
soo_write (struct file *fp, char *buf, int len)
{
  usetup;

  return netcall(NET__tcp_write, fp, buf, len);
}

static int
soo_ioctl (struct file *fp, int cmd, int inout, int arglen, caddr_t data)
{
  usetup;

  return netcall(NET__tcp_ioctl, fp, cmd, inout, arglen, data);
}

/* ix_lock_base() is very fussy - so put most of the close() code here */
static int
soo_close (struct file *fp)
{
  int err;
  usetup;

  ix_lock_base ();
  err = --fp->f_count;
  ix_unlock_base ();

  if (err)
    return 0;

  err = netcall(NET__tcp_close, fp);

  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
  return err;
}

static int
soo_select(struct file *fp, int select_cmd, int io_mode,
	   fd_set *ignored, u_long *also_ignored)
{
  usetup;

  return netcall(NET__tcp_select, fp, select_cmd, io_mode, ignored, also_ignored);
}

/* needed to set of the function pointers */
void _set_socket_params(struct file *fp, int domain, int type, int protocol)
{
  fp->f_stb.st_mode = 0666 | S_IFSOCK; /* not always, but.. */
  fp->f_stb.st_size = 128;	/* sizeof mbuf. */
  fp->f_stb.st_blksize = 128;
  fp->f_flags = FREAD | FWRITE;
  fp->f_type = ((domain == AF_INET) ? DTYPE_SOCKET : DTYPE_USOCKET);

  if (fp->f_type == DTYPE_SOCKET)
    {
      fp->f_socket_domain = domain;
      fp->f_socket_type = type;
      fp->f_socket_protocol = protocol;
      fp->f_read   = soo_read;
      fp->f_write  = soo_write;
      fp->f_ioctl  = soo_ioctl;
      fp->f_close  = soo_close;
      fp->f_select = soo_select;
    }
  else
    {
      fp->f_read   = unp_read;
      fp->f_write  = unp_write;
      fp->f_ioctl  = unp_ioctl;
      fp->f_close  = unp_close;
      fp->f_select = unp_select;
    }
}
