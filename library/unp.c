/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1996 by Hans Verkuil
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

/*
   Missing features:

   datagram support
   setsockopt/getsockopt are dummy functions
   no timeout while connecting

*/

#define _KERNEL
#include "ixemul.h"
#include "unp.h"
#include "kprintf.h"

#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/unix_socket.h>
#include <sys/un.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <machine/param.h>
#include <string.h>
#include "select.h"

#define can_read(ss)  (ss->reader != ss->writer)
#define can_write(ss) (!(ss->reader == ss->writer + 1 \
	                 || (ss->reader == ss->buffer \
	                     && ss->writer == ss->buffer + UNIX_SOCKET_SIZE - 1)))

struct ix_unix_name *
find_unix_name(const char *path)
{
  struct ix_unix_name *un;

  for (un = ix.ix_unix_names; un; un = un->next)
    if (!strcmp(un->path, path))
      break;
  return un;
}

struct sock_stream *
init_stream(void)
{
  struct sock_stream *s = kmalloc(sizeof(struct sock_stream));

  if (s)
    {
      s->reader = s->writer = s->buffer;
      s->flags = 0;
      s->task = 0;
    }
  return s;
}

static struct ix_unix_name *
add_unix_name(const char *path, int queue_size)
{
  struct ix_unix_name *un = kmalloc(sizeof(struct ix_unix_name));
  usetup;

  if (un == NULL)
    errno_return(ENOMEM, NULL);

  if (queue_size == 0)
    queue_size = 1;
  un->queue = kmalloc(queue_size * 4);
  if (un->queue == NULL)
    {
      kfree(un);
      errno_return(ENOMEM, NULL);
    }
  strcpy(un->path, path);
  un->next = ix.ix_unix_names;
  ix.ix_unix_names = un;
  un->queue_size = queue_size;
  un->queue_index = 0;
  un->task = 0;
  return un;
}

struct sock_stream *
find_stream(struct file *f, int read_stream)
{
  struct unix_socket *us;

  if (f->f_type == DTYPE_PIPE)
    return f->f_ss;

  us = f->f_sock;
  if (us->server != f)
    read_stream = !read_stream;
  return (read_stream ? us->to_server : us->from_server);
}

struct sock_stream *
get_stream(struct file *f, int read_stream)
{
  struct sock_stream *ss = find_stream(f, read_stream);

  if (ss == NULL)
    return ss;

  Forbid();
  for (;;)
    {
      if (!(ss->flags & UNF_LOCKED))
        {
          ss->flags &= ~UNF_WANT_LOCK;
          ss->flags |= UNF_LOCKED;
          /* got it ! */
          break;
	}
      ss->flags |= UNF_WANT_LOCK;
      if (ix_sleep((caddr_t)&ss->flags, "get_sock") < 0)
        {
	  Permit();
	  setrun(FindTask(0));
	  Forbid();
        }
      /* have to always recheck whether we really got the lock */
    }
  Permit();
  return ss;
}

void
release_stream(struct sock_stream *ss)
{
  if (ss)
    {
      Forbid ();
      if (ss->flags & UNF_WANT_LOCK)
        ix_wakeup ((u_int)&ss->flags);
        
      ss->flags &= ~(UNF_WANT_LOCK | UNF_LOCKED);
      Permit ();
    }
}

static int stream_is_closed(struct sock_stream *ss)
{
  return (ss == NULL || (ss->flags & (UNF_NO_READER | UNF_NO_WRITER)) ==
                                     (UNF_NO_READER | UNF_NO_WRITER));
}

static void close_stream(struct file *f, int read_stream, int from_close)
{
  struct sock_stream **ss;
  struct unix_socket *us = f->f_sock;
  int to_server;
  usetup;

  if (us == NULL)
    return;
  to_server = (us->server != f) ? !read_stream : read_stream;
  ss = (to_server ? &us->to_server : &us->from_server);

  if (*ss)
    {
      (*ss)->flags |= (read_stream ? UNF_NO_READER : UNF_NO_WRITER);
      if (stream_is_closed(*ss))
        {
          kfree(*ss);
          *ss = NULL;
        }
      else
        {
          if ((*ss)->task)
            Signal((*ss)->task, 1UL << u.u_pipe_sig);
          ix_wakeup ((u_int)*ss);
        }
    }
  if (read_stream && us->unix_name && f->f_count == 0)
    {
      struct ix_unix_name *un;
    
      if (us->unix_name == ix.ix_unix_names)
        {
          ix.ix_unix_names = ix.ix_unix_names->next;
        }
      else
        {
          for (un = ix.ix_unix_names; un; un = un->next)
            if (un->next == us->unix_name)
              {
                un->next = us->unix_name->next;
                break;
              }
        }
      kfree(us->unix_name->queue);
      kfree(us->unix_name);
      us->unix_name = NULL;
    }
  if (stream_is_closed(us->to_server) && stream_is_closed(us->from_server) &&
      us->unix_name == NULL && f->f_count == 0 && from_close)
    {
      f->f_sock = 0;
      kfree(us);
    }
}

int unp_socket(int domain, int type, int protocol, struct unix_socket *sock)
{
  int omask, err, fd;
  struct file *fp;
  struct unix_socket *us;
  usetup;

  if (type != SOCK_STREAM || protocol != 0)
    errno_return(EPROTONOSUPPORT, -1);

  if (sock)
    us = sock;
  else if ((us = kmalloc(sizeof(struct unix_socket))) == NULL)
    errno_return(ENOMEM, -1);
  else
    {
      us->path[0] = 0;
      us->from_server = us->to_server = NULL;
      us->unix_name = NULL;
      us->server = NULL;
      us->state = UNS_WAITING;
    }

  omask = syscall (SYS_sigsetmask, ~0);

  if ((err = falloc (&fp, &fd)))
    {
      errno = err;
      syscall (SYS_sigsetmask, omask);
      if (sock == NULL)
        kfree(us);
      return -1;
    }

  fp->f_sock = us;
  _set_socket_params(fp, domain, 0, 0);

  syscall (SYS_sigsetmask, omask);

  return fd;
}

int unp_bind(int s, const struct sockaddr *name, int namelen)
{
  usetup;
  struct file *fp = u.u_ofile[s];
  struct ix_unix_name *un;
  int tmp;
  char *path = ((struct sockaddr_un *)name)->sun_path;

  if (fp->f_sock->path[0])
    errno_return(EINVAL, -1);

  ix_lock_base();
  un = find_unix_name(path);
  ix_unlock_base();
  if (un)  
    errno_return(EADDRINUSE, -1);

  tmp = syscall(SYS_creat, path, 0777);
  if (tmp < 0)
    return -1;
  syscall(SYS_close, tmp);
  strcpy(fp->f_sock->path, path);
  return 0;
}

int unp_listen(int s, int backlog)
{
  usetup;
  struct file *fp = u.u_ofile[s];
  struct unix_socket *us = fp->f_sock;

  if (!us->path[0] || us->unix_name || us->to_server || us->from_server)
    errno_return(EOPNOTSUPP, -1);
  
  ix_lock_base();
  us->unix_name = add_unix_name(us->path, backlog);
  ix_unlock_base();
  return (us->unix_name ? 0 : -1);
}

int unp_accept(int s, struct sockaddr *name, int *namelen)
{
  usetup;
  struct file *f = u.u_ofile[s];
  struct unix_socket *client = NULL;
  struct ix_unix_name *un = f->f_sock->unix_name;
  int omask, err = 0, sleep_rc, fd;
  struct sockaddr_un *sa = (struct sockaddr_un *)name;

  if (un == NULL)
    errno_return(EOPNOTSUPP, -1);
  omask = syscall (SYS_sigsetmask, ~0);
  __get_file (f);

  do {
    while (un->queue_index == 0)
      {
        if (f->f_flags & FNDELAY)
          {
            err = EWOULDBLOCK;
            goto error;
          }
        Forbid ();
        __release_file (f);
        syscall (SYS_sigsetmask, omask);
        sleep_rc = ix_sleep((caddr_t)un, "accept");
        Permit ();
        if (sleep_rc < 0)
          setrun (FindTask (0));
        omask = syscall (SYS_sigsetmask, ~0);
        __get_file (f);
      }
    ix_lock_base();
    client = NULL;
    if (un->queue_index)
      {
        client = (struct unix_socket *)un->queue[0];
        if (--un->queue_index)
          memcpy(un->queue, un->queue + 1, un->queue_index * 4);
        if (client)
          client->state = UNS_PROCESSING;
      }
    ix_unlock_base();
  } while (client == NULL);
    
error:
  __release_file (f);
  if (err)
    {
      syscall (SYS_sigsetmask, omask);
      errno_return(err, -1);
    }
  fd = unp_socket(PF_UNIX, SOCK_STREAM, 0, client);
  if (fd == -1)
    client->state = UNS_ERROR;
  else
    {
      f = u.u_ofile[fd];
      client->server = f;
      client->state = UNS_ACCEPTED;
      sa->sun_family = AF_UNIX;
      strcpy(sa->sun_path, un->path);
      sa->sun_len = *namelen = 3 + strlen(sa->sun_path);
    }
  ix_wakeup((u_int)client);
  syscall (SYS_sigsetmask, omask);
  return fd;
}

int unp_connect(int s, const struct sockaddr *name, int namelen)
{
  usetup;
  struct file *f = u.u_ofile[s];
  struct unix_socket *us = f->f_sock;
  struct ix_unix_name *un;
  int sleep_rc, state;
  char *path = ((struct sockaddr_un *)name)->sun_path;

  if (us->unix_name)
    errno_return(EOPNOTSUPP, -1);
  if (us->to_server || us->from_server)
    errno_return(EISCONN, -1);
  strcpy(us->path, path);
  ix_lock_base();
  un = find_unix_name(path);
  if (un == NULL)
    {
      ix_unlock_base();
      errno_return(EADDRNOTAVAIL, -1);
    }
  if (un->queue_size == un->queue_index)
    {
      ix_unlock_base();
      errno_return(ECONNREFUSED, -1);
    }
  us->to_server = init_stream();
  if (us->to_server)
    us->from_server = init_stream();
  if (!us->from_server)
    {
      if (us->to_server)
        kfree(us->to_server);
      us->to_server = NULL;
      ix_unlock_base();
      errno_return(ENOMEM, -1);
    }
  un->queue[un->queue_index++] = (int)us;
  ix_unlock_base();
  Forbid();
  if (un->task)
    Signal(un->task, 1 << getuser(un->task)->u_pipe_sig);
  ix_wakeup((u_int)un);

  state = us->state;
  if (state != UNS_ACCEPTED && state != UNS_ERROR)
    do {
      sleep_rc = ix_sleep((caddr_t)us, "connect");
      state = us->state;
      Permit ();
      if (sleep_rc < 0)
        setrun (FindTask (0));
      Forbid ();
    } while (sleep_rc < 0 || (state != UNS_ERROR && state != UNS_ACCEPTED));
  Permit();

  if (state == UNS_ERROR)
    {
      kfree(us->to_server);
      kfree(us->from_server);
      us->to_server = us->from_server = NULL;
      errno_return(ECONNREFUSED, -1);
    }
  return 0;
}

int unp_send(int s, const void *buf, int len, int flags)
{
  usetup;
  if (flags)
    errno_return(EOPNOTSUPP, -1);
  return unp_write(u.u_ofile[s], buf, len);
}

int unp_recv(int s, void *buf, int len, int flags)
{
  usetup;
  if (flags)
    errno_return(EOPNOTSUPP, -1);
  return unp_read(u.u_ofile[s], buf, len);
}

int unp_shutdown(int s, int how)
{
  usetup;
  struct file *f = u.u_ofile[s];

  ix_lock_base();
  switch (how)
  {
    case 0:
      close_stream(f, TRUE, FALSE);
      break;
    case 1:
      close_stream(f, FALSE, FALSE);
      break;
    case 2:
      close_stream(f, TRUE, FALSE);
      close_stream(f, FALSE, FALSE);
      break;
  }
  ix_unlock_base();
  return 0;
}

int unp_setsockopt(int s, int level, int name, const void *val, int valsize)
{
  return 0;
}

int unp_getsockopt(int s, int level, int name, void *val, int *valsize)
{
  *valsize = 0;
  return 0;
}

int unp_getsockname(int s, struct sockaddr *asa, int *alen)
{
  usetup;
  struct file *f = u.u_ofile[s];
  struct sockaddr_un *un = (struct sockaddr_un *)asa;

  strcpy(un->sun_path, f->f_sock->path);
  un->sun_family = AF_UNIX;
  un->sun_len = *alen = 5 + strlen(un->sun_path);
  return 0;
}

int unp_getpeername(int s, struct sockaddr *asa, int *alen)
{
  return unp_getsockname(s, asa, alen);
}

int
stream_read (struct file *f, char *buf, int len)
{
  usetup;
  int omask = syscall (SYS_sigsetmask, ~0);
  int err = errno;
  int really_read = 0;
  int sleep_rc;
  struct sock_stream *ss = get_stream(f, TRUE);

  while (len)
    {
      if (ss == NULL || !can_read(ss))
	{
	  if (really_read || ss == NULL || (ss->flags & UNF_NO_WRITER))
	    {
	      err = 0;
	      break;
	    }
	
	  if (f->f_flags & FNDELAY)
	    {
	      if (!really_read)
		{
		  really_read = -1;
		  err = EAGAIN;
		}
	      break;
	    }

	  /* wait for something to be read or all readers to close */
	  Forbid ();
	  /* sigh.. Forbid() is necessary, or the other end may change
	     the pipe, and in the worst case also settle for sleep(), and
	     there it is.. deadlock.. */
	  release_stream(ss);

	  /* make read interruptible */
	  syscall (SYS_sigsetmask, omask);
	  sleep_rc = ix_sleep ((caddr_t)ss, "sockread");
	  Permit ();
	  if (sleep_rc < 0)
	    setrun (FindTask (0));
	  omask = syscall (SYS_sigsetmask, ~0);

	  ss = get_stream(f, TRUE);
	  continue;		/* retry */
	}

      /* okay, there's something to read from the pipe */
      if (ss->reader > ss->writer)
        {
	  /* read till end of buffer and wrap around */
	  int avail = UNIX_SOCKET_SIZE - (ss->reader - ss->buffer);
	  int do_read = len < avail ? len : avail;

	  really_read += do_read;
	  bcopy (ss->reader, buf, do_read);
	  ss->reader += do_read;
	  len -= do_read;
	  buf += do_read;
	  if (ss->reader - ss->buffer == UNIX_SOCKET_SIZE)
	    /* wrap around */
	    ss->reader = ss->buffer;
	}
      if (len && ss->reader < ss->writer)
        {
	  int avail = ss->writer - ss->reader;
	  int do_read = len < avail ? len : avail;

	  really_read += do_read;
	  bcopy (ss->reader, buf, do_read);
	  ss->reader += do_read;
	  len -= do_read;
	  buf += do_read;
	}
      Forbid();
      if (ss->task)
        Signal(ss->task, 1 << getuser(ss->task)->u_pipe_sig);
      Permit();

      ix_wakeup((u_int)ss);
    }

  release_stream(ss);
 
  syscall (SYS_sigsetmask, omask);
  errno = err;
  return really_read;
}

int unp_read(struct file *f, char *buf, int len)
{
  usetup;

  if (f->f_sock->state != UNS_ACCEPTED)
    errno_return(ENOTCONN, -1);
  return stream_read(f, buf, len);
}

int
stream_write (struct file *f, const char *buf, int len)
{
  usetup;
  int omask = syscall (SYS_sigsetmask, ~0);
  int err = errno;
  int sleep_rc;
  int really_written = 0;
  struct sock_stream *ss = get_stream(f, FALSE);

  while (len)
    {
      if (ss == NULL || (ss->flags & UNF_NO_READER))
	{
	  really_written = -1;
	  err = EPIPE;
	  /* this is something no `real' Amiga pipe handler will do ;-)) */
	  _psignal (FindTask (0), SIGPIPE);
	  break;
        }
	
      /* buffer full ?? */
      if (!can_write(ss))
	{
	  if (f->f_flags & FNDELAY)
	    {
	      if (! really_written)
	        {
	          really_written = -1;
	          err = EAGAIN;
	        }
	      break;
	    }

	  /* wait for something to be read or all readers to close */
	  Forbid ();
	  /* sigh.. Forbid() is necessary, or the other end may change
	     the pipe, and in the worst case also settle for sleep(), and
	     there it is.. deadlock.. */
	  release_stream(ss);

	  /* make write interruptible */
	  syscall (SYS_sigsetmask, omask);
	  sleep_rc = ix_sleep ((caddr_t)ss, "sockwrite");
	  Permit ();
	  if (sleep_rc < 0)
	    setrun (FindTask (0));
	  omask = syscall (SYS_sigsetmask, ~0);

	  ss = get_stream(f, FALSE);
	  continue;		/* retry */
	}

      /* okay, there's some space left to write to the pipe */

      if (ss->writer >= ss->reader)
        {
          /* write till end of buffer */
	  int avail = UNIX_SOCKET_SIZE - 1 - (ss->writer - ss->buffer);
	  int do_write;

	  if (ss->reader > ss->buffer)
	    avail++;
	  do_write = len < avail ? len : avail;

	  really_written += do_write;
	  bcopy (buf, ss->writer, do_write);
	  len -= do_write;
	  buf += do_write;
	  ss->writer += do_write;
	  if (ss->writer - ss->buffer == UNIX_SOCKET_SIZE)
	    ss->writer = ss->buffer;
	}

      if (ss->writer < ss->reader - 1)
        {
	  int avail = ss->reader - ss->writer - 1;
	  int do_write = len < avail ? len : avail;

	  really_written += do_write;
	  bcopy (buf, ss->writer, do_write);
	  ss->writer += do_write;
	  len -= do_write;
	  buf += do_write;
	}
      Forbid();
      if (ss->task)
        Signal(ss->task, 1 << getuser(ss->task)->u_pipe_sig);
      Permit();
	
      ix_wakeup((u_int)ss);
    }

  release_stream(ss);

  syscall (SYS_sigsetmask, omask);
  errno = err;
  return really_written;
}

int unp_write(struct file *f, const char *buf, int len)
{
  usetup;

  if (f->f_sock->state != UNS_ACCEPTED)
    errno_return(ENOTCONN, -1);
  return stream_write(f, buf, len);
}

int unp_ioctl(struct file *f, int cmd, int inout, int arglen, caddr_t arg)
{
  int omask;
  int result = 0;
  struct sock_stream *ss;
  
  omask = syscall (SYS_sigsetmask, ~0);
  ss = get_stream(f, TRUE);

  switch (cmd)
    {
    case FIONREAD:
      {
	unsigned int *pt = (unsigned int *)arg;

        if (ss == NULL)
	  *pt = 0;
	else if (ss->reader < ss->writer)
	  *pt = ss->writer - ss->reader;
	else if (ss->reader > ss->writer)
	  *pt = UNIX_SOCKET_SIZE - (ss->reader - ss->writer);
	else
	  *pt = 0;
	result = 0;
        break;
      }

    case FIONBIO:
      {
	result = f->f_flags & FNDELAY ? 1 : 0;
	if (*(unsigned int *)arg)
	  f->f_flags |= FNDELAY;
	else
	  f->f_flags &= ~FNDELAY;
	/* I didn't find it documented in a manpage, but I assume, we
	 * should return the former state, not just zero.. */
	break;
      }

    case FIOASYNC:
      {
	/* DOESN'T WORK YET */

	int flags = *(unsigned long*)arg;
	result = f->f_flags & FASYNC ? 1 : 0;
	if (flags)
	  f->f_flags |= FASYNC;
	else
	  f->f_flags &= ~FASYNC;

	/* ATTENTION: have to call some function here in the future !!! */

	/* I didn't find it documented in a manpage, but I assume, we
	 * should return the former state, not just zero.. */
	break;
      }

    case FIOCLEX:
    case FIONCLEX:
    case FIOSETOWN:
    case FIOGETOWN:
      /* this is no error, but nevertheless we don't take any actions.. */      
      result = 0;
      break;
    }

  release_stream(ss);
  syscall (SYS_sigsetmask, omask);
  return result;
}

int unp_select(struct file *f, int select_cmd, int io_mode, fd_set *ignored, u_long *also_ignored)
{
  struct sock_stream *ss = find_stream(f, io_mode == SELMODE_IN);
  usetup;

  if (f->f_type != DTYPE_PIPE && ss == NULL)
    {
      struct ix_unix_name *un = f->f_sock->unix_name;
      int result = 1UL << u.u_pipe_sig;

      if (un == NULL)
        return 0;
      ix_lock_base();
      un->task = NULL;
      if (select_cmd == SELCMD_CHECK || select_cmd == SELCMD_POLL)
        {
          if (io_mode == SELMODE_IN)
    	    result = un->queue_index != 0;
          else
            result = 0;
        }
      else
	un->task = FindTask(0);
      ix_unlock_base();
      return result;
    }
  if (select_cmd == SELCMD_CHECK || select_cmd == SELCMD_POLL)
    {
      if (select_cmd == SELCMD_CHECK && ss->task == FindTask(0))
        ss->task = NULL;
      /* we support both, read and write checks (hey, something new ;-)) */
      if (io_mode == SELMODE_IN)
	return can_read(ss) || (ss->flags & UNF_NO_WRITER);
      if (io_mode == SELMODE_OUT)
	return can_write(ss) || (ss->flags & UNF_NO_READER);
      return 0;
    }
  ss->task = FindTask(0);
  if (io_mode == SELMODE_IN)
    if (can_read(ss) || (ss->flags & UNF_NO_WRITER))
      Signal(ss->task, 1 << u.u_pipe_sig);
  if (io_mode == SELMODE_OUT)
    if (can_write(ss) || (ss->flags & UNF_NO_READER))
      Signal(ss->task, 1 << u.u_pipe_sig);
  return 1 << u.u_pipe_sig;
}

int unp_close(struct file *f)
{
  ix_lock_base();

  f->f_count--;
  if (f->f_count == 0 && f->f_sock)
    {
      close_stream(f, TRUE, FALSE);
      close_stream(f, FALSE, FALSE);
    }

  ix_unlock_base();
  return 0;
}
