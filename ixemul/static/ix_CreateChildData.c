/*
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
 *  $Id: ix_CreateNewProc.c,v 1.0 2009/03/24 20:47:00 bernd_afa Exp $
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#include <sys/syscall.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <stddef.h>
#include <setjmp.h>
#include <string.h>
#include <stdlib.h>

#include <utility/tagitem.h>
#include <dos/dostags.h>


void ixnewlist(struct ixlist *list)
{
  list->head = list->tail = NULL;
} 

char **
dupvec (char **vec)
{
  int n;
  char **vp;
  char **res;
  static char *empty[] = { NULL };
  
  if (! vec)
    return empty;

  for (n = 0, vp = vec; *vp; n++, vp++) ;

  /* contrary to `real' vfork(), malloc() works in the child on its own
     data, that is it won't clobber anything in the parent  */
  
  res = (char **) malloc((n + 1) * 4);
  if (res)
    {
      for (vp = res; n-- > 0; vp++, vec++)
        *vp = (char *) strdup( *vec);
      *vp = 0;
    }

  return res;
} 

struct Process * ix_CreateChildData(struct Process *parent,struct TagList *tags)
{

volatile struct user *pu,*mu;
APTR ixb;
  struct Task *new_ = FindTask(0);
  mu = getuser(new_);
  pu = getuser(parent);
  if (mu && pu)
  {
	  int a4_size = pu->u_a4_pointers_size * 4;
	  int fd;
	  // child tasks use on Unix file handles and mem lists of parent process. 
	  if (!pu->u_parent_userdata)mu->u_parent_userdata = pu;
	  else mu->u_parent_userdata = pu->u_parent_userdata;
		  //copied code from vfork
	  
	  mu->u_environ = (char ***) malloc (4);
	  *mu->u_environ = dupvec (* pu->u_environ);
	  mu->u_errno = (int *) malloc (4);
	  *mu->u_errno = 0;
	  mu->u_h_errno = (int *) malloc (4);
	  *mu->u_h_errno = 0;
	  /* and inherit several other things as well, upto not including u_md */
      bcopy ((void *)&pu->u_a4_pointers_size, (void *)&mu->u_a4_pointers_size,
	     offsetof (struct user, u_md) - offsetof (struct user, u_a4_pointers_size));
      bcopy ((char *)pu - a4_size, (char *)mu - a4_size, a4_size);

      /* some things have been copied that should be reset */
      mu->p_flag &= ~(SFREEA4 | STRC);
      mu->p_xstat = 0;
      bzero ((void *)&mu->u_ru, sizeof (struct rusage));
      bzero ((void *)&mu->u_prof, sizeof (struct uprof));
      mu->u_prof_last_pc = 0;
      bzero ((void *)&mu->u_cru, sizeof (struct rusage));
      bzero ((void *)&mu->u_timer[0], sizeof (struct itimerval)); /* just the REAL timer! */
     
	  //gettimeofday( & mu->u_start, 0);
	  /* and adjust the open count of each of the copied filedescriptors */
	 
 //     for (fd = 0; fd < NOFILE; fd++)
	//if (mu->u_ofile[fd])
	//{
	//  /* obtain (and create a new fd) for INET sockets */
	//  if (mu->u_ofile[fd]->f_type == DTYPE_SOCKET)
	//  {
	//    /* Was this socket released? */
	//    if (mu->u_ofile[fd]->f_socket_id)
	//    {
	//      /* Yes, it was. So we now obtain a new socket from the underlying TCP/IP stack */
	//      int newfd = syscall(SYS_ix_obtain_socket, mu->u_ofile[fd]->f_socket_id,
	//	  mu->u_ofile[fd]->f_socket_domain,
	//	  mu->u_ofile[fd]->f_socket_type,
	//	  mu->u_ofile[fd]->f_socket_protocol);

	//      mu->u_ofile[fd]->f_socket_id = 0;
	//      /* move the newly created fd to the old one */
	//      if (newfd != -1)
	//	{
	//	  /* Tricky bit: we have to remember that for dupped sockets all file descriptors
	//	     point to the same file structure: obtaining one released file descriptor will
	//	     obtain them all. So we have to scan for file descriptors that point to the
	//	     same file structure for which we just obtained the socket and copy the new
	//	     file structure into the dupped file descriptors.

	//	     Note that the newly created file structure has the field f_socket_id set to 0,
	//	     so we won't come here again because of the if-statement above that tests whether
	//	     the socket was released. */
	//	  int fd2;

	//	  /* scan the file descriptors */
	//	  for (fd2 = fd + 1; fd2 < NOFILE; fd2++)
	//	    {
	//	      /* do they point to the same file structure? */
	//	      if (mu->u_ofile[fd] == mu->u_ofile[fd2])
	//	      {
	//		/* in that case copy the newly obtained socket into this file descriptor
	//		   and increase the open count */
	//		mu->u_ofile[fd2] = mu->u_ofile[newfd];
	//		mu->u_ofile[newfd]->f_count++;
	//	      }
	//	    }
	//	  /* and finally do the same for fd */
	//	  mu->u_ofile[fd] = mu->u_ofile[newfd];
	//	  mu->u_ofile[newfd] = 0;
	//	}
	//    }
	//  }
	//  else
	//    mu->u_ofile[fd]->f_count++;
	//}
     
  }

}