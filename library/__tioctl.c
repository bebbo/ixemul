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
 *  __tioctl.c,v 1.1.1.1 1994/04/04 04:30:13 amiga Exp
 *
 *  __tioctl.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:13  amiga
 * Initial CVS check in.
 *
 *  Revision 1.5  1993/11/05  21:51:08  mw
 *  seems I got oldstyle tty handling oposite way..
 *
 *  Revision 1.4  1992/08/09  20:39:01  amiga
 *  add a cast to get rid of a warning
 *
 *  Revision 1.3  1992/07/04  19:07:25  mwild
 *  send DISK_INFO packet with synchronous port, async delivery seems to be
 *  broken with CNC:
 *
 * Revision 1.2  1992/06/08  02:36:00  mwild
 * fix TIOCGWINSZ, row/column was off by one
 *
 * Revision 1.1  1992/05/14  19:55:40  mwild
 * Initial revision
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"
#include <string.h>
#include <sgtty.h>

#define OEXTB 15
#undef B0	
#undef B50	
#undef B75	
#undef B110	
#undef B134	
#undef B150	
#undef B200	
#undef B300	
#undef B600	
#undef B1200	
#undef B1800	
#undef B2400	
#undef B4800	
#undef B9600	
#undef EXTA	
#undef EXTB	
#include <sys/termios.h>
#include <ctype.h>
#include <devices/conunit.h>
#include <intuition/intuition.h>

/* IOCTLs on "interactive" files */

int
__tioctl(struct file *f, unsigned int cmd, unsigned int inout, 
	 unsigned int arglen, unsigned int arg)
{
  int omask, result, err = 0;
  usetup;

  omask = syscall (SYS_sigsetmask, ~0);
  __get_file (f);
  result = -1;

  if (!IsInteractive(CTOBPTR(f->f_fh)))
    {
      err = ENOTTY;
    }
  else switch (cmd)
    {
    case TIOCGETA:
      {
        struct termios *t = (struct termios *)arg;
        unsigned char *cp;

        t->c_iflag = IGNBRK | IGNPAR | IXON;
	if (f->f_ttyflags & IXTTY_ICRNL)
	  t->c_iflag |= ICRNL;
	if (f->f_ttyflags & IXTTY_INLCR)
	  t->c_iflag |= INLCR;
        t->c_oflag = 0;
	if (f->f_ttyflags & IXTTY_OPOST)
	  {
	    t->c_oflag |= OPOST;
	    if (f->f_ttyflags & IXTTY_ONLCR)
	      t->c_oflag |= ONLCR;
	  }
        t->c_cflag = CS8|CLOCAL;
        t->c_ispeed=
        t->c_ospeed= EXTB;
	/* Conman does ECHOCTL, Commo doesn't.. I use Conman:-)) */
        t->c_lflag = ECHOCTL | ((f->f_ttyflags & IXTTY_RAW) ? 0 : ICANON | ECHO);
	cp = t->c_cc;
	cp[VSTART] = 'q' & 31;
	cp[VSTOP] = 's' & 31;
	cp[VSUSP] = 0; /* sneef.. would that be nice... */
	cp[VDSUSP] = 0;
	cp[VREPRINT] = 0;
	cp[VDISCARD] = 'x' & 31;
	cp[VWERASE] = 0;
	cp[VLNEXT] = 0;
	cp[VSTATUS] = 0;
	cp[VINTR] = 3;
	cp[VQUIT] = 0;
	cp[VERASE] = 8;
	cp[VKILL] = 'x' & 31;
	cp[VEOF] = '\\' & 31;
	cp[VEOL] = 10;
	cp[VEOL2] = 0;
	result = 0;
	break;
      }

    case TIOCSETA:
    case TIOCSETAW:
    case TIOCSETAF:
      {
        struct termios *t = (struct termios *)arg;
        int makeraw;
        
        makeraw = (t->c_lflag & (ICANON | ECHO)) != (ICANON | ECHO);
	/* the only thing that counts so far.. if ICANON is disabled,        
	 * we disable ECHO too, no matter what the user wanted, and 
	 * send a RAW-packet.. */
	if (!makeraw && (f->f_ttyflags & IXTTY_RAW))
          {
	    f->f_ttyflags &= ~IXTTY_RAW;
	    SetMode(CTOBPTR(f->f_fh), 0);
	  }
	else if (makeraw && !(f->f_ttyflags & IXTTY_RAW))
	  {
	    f->f_ttyflags |= IXTTY_RAW;
	    SetMode(CTOBPTR(f->f_fh), 1);
	  }
	if (t->c_iflag & ICRNL)
	  f->f_ttyflags |= IXTTY_ICRNL;
	else
	  f->f_ttyflags &= ~IXTTY_ICRNL;
	if (t->c_iflag & INLCR)
	  f->f_ttyflags |= IXTTY_INLCR;
	else
	  f->f_ttyflags &= ~IXTTY_INLCR;
	if (t->c_oflag & OPOST)
	  {
	    f->f_ttyflags |= IXTTY_OPOST;
	    if (t->c_oflag & ONLCR)
	      f->f_ttyflags |= IXTTY_ONLCR;
	  }
	else
	  {
	    f->f_ttyflags &= ~IXTTY_OPOST;
	  }
	result = 0;
	break;
      }

    case TIOCGETP:
      {
	struct sgttyb *s = (struct sgttyb *)arg;
	s->sg_erase = 8;
	s->sg_kill = 'x' & 31;
	s->sg_flags = ODDP|EVENP|ANYP|
		      ((f->f_ttyflags & IXTTY_RAW) ? CBREAK|RAW : ECHO|CRMOD) |
		      ((f->f_ttyflags & IXTTY_ICRNL) ? CRMOD : 0);
	s->sg_ispeed =
	s->sg_ospeed = OEXTB;
	result = 0;
	break;
      }

    case TIOCSETN:
    case TIOCSETP:
      {
	struct sgttyb *s = (struct sgttyb *)arg;

	if (!(s->sg_flags & (RAW|CBREAK)))
          {
            f->f_ttyflags &= ~IXTTY_RAW;
	    SetMode(CTOBPTR(f->f_fh), 0);
	  }
        else if ((s->sg_flags & (RAW|CBREAK)))
	  {
	    f->f_ttyflags |= IXTTY_RAW;
	    SetMode(CTOBPTR(f->f_fh), 1);
	  }
	f->f_ttyflags &= ~IXTTY_INLCR;
	if (s->sg_flags & CRMOD)
	  f->f_ttyflags |= IXTTY_ICRNL | IXTTY_OPOST | IXTTY_ONLCR;
	else
          f->f_ttyflags &= ~(IXTTY_ICRNL | IXTTY_OPOST | IXTTY_ONLCR);
	result = 0;
	break;
      }

    case TIOCGWINSZ:
      {
	struct winsize *ws = (struct winsize *) arg;
	struct Window *w;
	struct ConUnit *cu;
	struct IOStdReq *ios;
	struct InfoData *info;

	info = alloca (sizeof (struct InfoData) + 2);
	info = LONG_ALIGN (info);
	bzero (info, sizeof (struct InfoData));

	LastResult (f) = 0; LastError (f) = 0;
	SendPacket1 (f, __srwport, ACTION_DISK_INFO, CTOBPTR (info));
	__wait_sync_packet (&f->f_sp);
	if (LastResult (f) != -1)
          break;

	w = (struct Window *) info->id_VolumeNode;
	if (! w) 
	  break;
	/* this information is from DevCon notes, not from the Bantam book */
	ios = (struct IOStdReq *) info->id_InUse;
	if (! ios || ((int)ios & 1)) 
	  break;
	cu = (struct ConUnit *) ios->io_Unit;
	if (!cu) 
	  break;

	/* paranoid check.. */
	if (cu->cu_Window != w)
	  break;

	ws->ws_col = cu->cu_XMax + 1;	/* Thanks Rob! those values are off */
	ws->ws_row = cu->cu_YMax + 1;	/* by one! */

    	ws->ws_xpixel = w->Width - w->BorderLeft - w->BorderRight;
    	ws->ws_ypixel = w->Height - w->BorderTop - w->BorderBottom;

	result = 0;
	break;
      }

    case TIOCSPGRP:
      {
	int *pgrp = (int *)arg;

        if (u.u_session)
          u.u_session->pgrp = *pgrp;
	result = 0;
        break;
      }

    case TIOCGPGRP:
      {
	int *pgrp = (int *)arg;

        *pgrp = (u.u_session ? u.u_session->pgrp : syscall(SYS_getpgrp));
	result = 0;
        break;
      }

    case TIOCOUTQ:
      {
	int *count = (int *)arg;

	*count = 0;
	result = 0;
	break;
      }

    case TIOCPKT:
      {
        int *on = (int *)arg;

	if (*on)
            f->f_ttyflags |= IXTTY_PKT;
	else
            f->f_ttyflags &= ~IXTTY_PKT;
	result = 0;
        break;
      }

    case TIOCSWINSZ:
      /* should I really try to resize the window ?? */

    default:
      /* this is no error, but nevertheless we don't take any actions.. */      
      result = 0;
      break;
    }

    __release_file (f);
    syscall (SYS_sigsetmask, omask);
    errno = err;
    KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
    return result;
}
