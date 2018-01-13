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
 *  select.c,v 1.1.1.1 1994/04/04 04:30:33 amiga Exp
 *
 *  select.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:33  amiga
 * Initial CVS check in.
 *
 *  Revision 1.2  1993/11/05  22:00:59  mwild
 *  extensively rewritten to work better along with inet.library
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#include <sys/time.h>
#include <unistd.h>
#include <string.h>

#include "select.h"

#define __time_req (u.u_time_req)
#define __tport    (u.u_sync_mp)

static void handle_select_port(void)
{
  usetup;

  struct StandardPacket *prw;

  while ((prw = GetPacket(u.u_select_mp)))
      prw->sp_Pkt.dp_Port = 0;
}

static void inline setcopy(int nfd, u_int *ifd, u_int *ofd)
{
  /* this procedure is here, because it's "normal" that if you only
   * want to select on fd 0,1,2 eg. you only pass a long to select,
   * not a whole fd_set, so we can't simply copy over results in the
   * full size of an fd_set.. */
  
  /* we have to copy that many longs... */
  nfd = (nfd+31) >> 5;
  while (nfd--) *ofd++ = *ifd++;
}

int
ix_select(int nfd, fd_set *ifd, fd_set *ofd, fd_set *efd, struct timeval *timeout, long *mask)
{
  usetup;
  struct file *f;
  int i, waitin, waitout, waitexc, dotout;
  int result, ostat, sigio;
  u_int wait_sigs;
  u_int origmask = mask ? *mask : 0;
  int skipped_wait;
  struct user *p = &u;
  u_long recv_wait_sigs = 0;
  u_long net_nfds;

  if (CURSIG (p))
    {
      *(p->u_errno) = EINTR;
      return -1;
    }

  /* as long as I don't support anything similar to a network, I surely
   * won't get any exceptional conditions, so *efd is mapped into 
   * *ifd, if it's set. */
   
  /* first check, that all included descriptors are valid and support
   * the requested operation. If the user included a request to wait
   * for a descriptor to be ready to read, while the descriptor was only
   * opened for writing, the requested bit is immediately cleared
   */
  waitin = waitout = waitexc = 0;
  if (nfd > NOFILE) nfd = NOFILE;

  for (i = 0; i < nfd; i++)
    {
      if (ifd && FD_ISSET(i, ifd) && (f = p->u_ofile[i]))
	{
	  if (!f->f_read || !f->f_select)
	    FD_CLR(i, ifd);
	  else
	    ++waitin;
	}
      if (ofd && FD_ISSET(i, ofd) && (f = p->u_ofile[i]))
	{
	  if (!f->f_write || !f->f_select)
	    FD_CLR(i, ofd);
	  else
	    ++waitout;
	}
      if (efd && FD_ISSET(i, efd) && (f = p->u_ofile[i]))
	{
	  /* question: can an exceptional condition also occur on a 
	   * write-only fd?? */
	  if (!f->f_read || !f->f_select)
	    FD_CLR(i, efd);
	  else
	    ++waitexc;
	}
    }

  dotout = (timeout && timerisset(timeout));

  /* have to make sure we can clean up the timer-request ! */
  ostat = p->p_stat;
  p->p_stat = SSLEEP;
  p->p_wchan = (caddr_t) select; /* will once be an own variable */
  p->p_wmesg = "select";

  for (skipped_wait = 0; ; skipped_wait=1)
    {
      fd_set readyin, readyout, readyexc;
      fd_set netin, netout, netexc;		/* used by ixnet.library */
      int tout, readydesc, cmd;

      FD_ZERO(&readyin);
      FD_ZERO(&readyout);
      FD_ZERO(&readyexc);
      
      if (u.u_ixnetbase)
        {
          FD_ZERO(&netin);
          FD_ZERO(&netout);
          FD_ZERO(&netexc);
          net_nfds = 0;
        }

      tout = readydesc = 0;

      /* have to always wait for the `traditional' ^C and the library internal
       * sleep_sig as well */
      wait_sigs = SIGBREAKF_CTRL_C | (1 << p->u_sleep_sig) | origmask;
      
      handle_select_port();

      if (skipped_wait)
	{
	  cmd = SELCMD_CHECK;

	  if (dotout)
	    {
	      __time_req->tr_time.tv_sec = timeout->tv_sec;
	      __time_req->tr_time.tv_usec = timeout->tv_usec;
              __time_req->tr_node.io_Command = TR_ADDREQUEST;
              SendIO((struct IORequest *)__time_req);
              /* clear the bit, it's used for sync packets too, and might be set */
              SetSignal (0, 1 << __tport->mp_SigBit);
	      wait_sigs |= 1 << __tport->mp_SigBit;
	    }

	  /* have all watched files get prepared for selecting */
          for (i = 0; i < nfd; i++)
	    {
	      if (ifd && FD_ISSET (i, ifd) && (f = p->u_ofile[i]))
	        wait_sigs |= f->f_select (f, SELCMD_PREPARE, SELMODE_IN, &netin, &net_nfds);
	      if (ofd && FD_ISSET (i, ofd) && (f = p->u_ofile[i]))
	        wait_sigs |= f->f_select (f, SELCMD_PREPARE, SELMODE_OUT, &netout, &net_nfds);
	      if (efd && FD_ISSET (i, efd) && (f = p->u_ofile[i]))
	        wait_sigs |= f->f_select (f, SELCMD_PREPARE, SELMODE_EXC, &netexc, &net_nfds);
	    }
	  if (!(u.p_sigignore & sigmask (SIGIO)))
	    {
	      struct file **f = u.u_ofile;

	      for (i = 0; i < u.u_lastfile; i++)
	        if (f[i] && (f[i]->f_flags & FASYNC) && !(ifd && FD_ISSET(i, ifd)))
		  wait_sigs |= f[i]->f_select (f[i], SELCMD_PREPARE, SELMODE_IN, &netin, &net_nfds);
	    }

          /* now wait for all legally possible signals, this includes BSD
           * signals (but want at least one signal set!) */
	  if (u.u_ixnetbase)
            recv_wait_sigs = netcall(NET_waitselect, wait_sigs,
			 &netin, &netout, &netexc, net_nfds);
	  else
            while (!(recv_wait_sigs = Wait (wait_sigs))) ;

          if (mask)
            *mask = recv_wait_sigs & origmask;

	  if (dotout)
	    {
	      /* IMPORTANT: unqueue the timer request BEFORE polling the fd's,
	       *            or __wait_packet() will treat the timer request
	       *            as a packet... */

	      if (!CheckIO ((struct IORequest *)__time_req))
                AbortIO ((struct IORequest *)__time_req);
              else
                recv_wait_sigs |= 1 << __tport->mp_SigBit;
              WaitIO ((struct IORequest *)__time_req);
            }

          handle_select_port();
        }
      else
	cmd = SELCMD_POLL;

      /* no matter what caused Wait() to return, wait for all requests to
       * complete (we CAN'T abort a DOS packet, sigh..) */

      /* collect information from the file descriptors */
      for (i = 0; i < nfd; i++)
	{
	  if (ifd && FD_ISSET (i, ifd) && (f = p->u_ofile[i])
	      && f->f_select (f, cmd, SELMODE_IN, &netin, NULL))
	    {
	      FD_SET (i, &readyin);
	      ++ readydesc;
	    }
	  if (ofd && FD_ISSET (i, ofd) && (f = p->u_ofile[i])
	      && f->f_select (f, cmd, SELMODE_OUT, &netout, NULL))
	    {
	      FD_SET (i, &readyout);
	      ++ readydesc;
	    }
	  if (efd && FD_ISSET (i, efd) && (f = p->u_ofile[i])
              && f->f_select (f, cmd, SELMODE_EXC, &netexc, NULL))
	    {
	      FD_SET (i, &readyexc);
	      ++ readydesc;
	    }
	}

      /* we have a timeout condition, if readydesc == 0, dotout == 1 and 
       * end_time < current time */
      if (!readydesc && dotout)
        tout = recv_wait_sigs & (1 << __tport->mp_SigBit);

      sigio = 0;
      if (!(u.p_sigignore & sigmask (SIGIO)))
        {
          struct file **f = u.u_ofile;

          for (i = 0; i < u.u_lastfile; i++)
            if (f[i] && (f[i]->f_flags & FASYNC) && !(ifd && FD_ISSET(i, ifd)))
    	      if (f[i]->f_select (f[i], cmd, SELMODE_IN, &netin, NULL))
    	        sigio = 1;
        }

      if (readydesc || tout || (timeout && !timerisset(timeout)))
	{
	  if (ifd) setcopy(nfd, (u_int *)&readyin,  (u_int *)ifd);
	  if (ofd) setcopy(nfd, (u_int *)&readyout, (u_int *)ofd);
	  if (efd) setcopy(nfd, (u_int *)&readyexc, (u_int *)efd);
	  result = readydesc; /* ok for tout, since then readydesc is already 0 */
	  break;
	}

      if (sigio || (recv_wait_sigs & (SIGBREAKF_CTRL_C | (1 << p->u_sleep_sig) | origmask)))
        {
          result = -1;
          break;
        }
    }

  p->p_wchan = 0;
  p->p_wmesg = 0;
  p->p_stat = ostat;
  if (recv_wait_sigs == (u_long)-1)
    return -1;
  /* need special processing for ^C here, as that is completely disabled
     when we're SSLEEPing */
  if (recv_wait_sigs & SIGBREAKF_CTRL_C)
    _psignal(FindTask(0), SIGINT);
  if (sigio)
    _psignal(FindTask(0), SIGIO);
  setrun(FindTask(0));

  if (result == -1)
    /* have to set this here, since errno can be changed in signal handlers */
    *(p->u_errno) = EINTR;

  return result;
}

int
select(int nfd, fd_set *ifd, fd_set *ofd, fd_set *efd, struct timeval *timeout)
{
  return ix_select(nfd, ifd, ofd, efd, timeout, NULL);
}
