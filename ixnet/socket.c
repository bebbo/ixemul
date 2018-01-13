/*
 *  This file is part of ixnet.library for the Amiga.
 *  Copyright (C) 1996 by Jeff Shepherd
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
 *  $Id:$
 *
 *  $Log:$
 *
 */

#define _KERNEL
#include "ixnet.h"

#include <sys/socket.h>
#include <sys/socketvar.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/route.h>
#include <netinet/in.h>
#include <machine/param.h>
#include <string.h>
#include <inetd.h>
#include <stdlib.h>
#include "select.h"
#include "ixprotos.h"

int _tcp_read	(struct file *fp, char *buf, int len);
int _tcp_write	(struct file *fp, char *buf, int len);
int _tcp_ioctl	(struct file *fp, int cmd, int inout, int arglen, caddr_t data);
int _tcp_select (struct file *fp, int select_cmd, int io_mode, fd_set *, u_long *);
int _tcp_close	(struct file *fp);

int
_socket (int domain, int type, int protocol)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int err = -1;

    switch (network_protocol) {
	case IX_NETWORK_AS225:
	    err = SOCK_socket(domain, type, protocol);
	break;

	case IX_NETWORK_AMITCP:
	    err = TCP_Socket(domain, type, protocol);
	break;
    }
    return err;
}


int
_bind (struct file *fp, const struct sockaddr *name, int namelen)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int error = -1, oldlen;

    switch (network_protocol) {
	case IX_NETWORK_AS225:
	    oldlen = name->sa_len;
	    ((struct sockaddr *)name)->sa_len = 0;
	    error = SOCK_bind(fp->f_so, name, namelen);
	    ((struct sockaddr *)name)->sa_len = oldlen;
	break;

	case IX_NETWORK_AMITCP:
	    error = TCP_Bind(fp->f_so, name, namelen);
	break;
    }
    return error;
}

int
_listen (struct file *fp, int backlog)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int error = -1;

    switch (network_protocol) {
	case IX_NETWORK_AS225:
	    error = SOCK_listen(fp->f_so, backlog);
	break;

	case IX_NETWORK_AMITCP:
	    error = TCP_Listen(fp->f_so, backlog);
	break;
    }
    return error;
}

int
_accept (struct file *fp, struct sockaddr *name, int *namelen)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int err = -1;
    switch (network_protocol) {
	case IX_NETWORK_AS225:
	    err = SOCK_accept(fp->f_so, name, namelen);
	break;

	case IX_NETWORK_AMITCP:
	    err = TCP_Accept(fp->f_so, name, namelen);
	break;
    }
    return err;
}


int
_dup(struct file *fp)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int error = -1;

    switch (network_protocol) {
        case IX_NETWORK_AS225:
            /* only INET-225 has dup */
            if (((struct Library *)p->u_SockBase)->lib_Version >= 8)
        	error = SOCK_dup(fp->f_so);
            else
        	error = fp->f_so;
            break;

        case IX_NETWORK_AMITCP:
            error = TCP_Dup2Socket(fp->f_so, -1);
            break;
    }
    return error;
}

int release_socket(struct file *fp)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int error = -1;
    /* dup the socket first, since for AmiTCP, we can only release once */
    int s2 = _dup(fp);

    if (s2 != -1) {
        switch (network_protocol) {
            case IX_NETWORK_AS225:
        	error = (int)SOCK_release(s2);
        	SOCK_close(s2);
                break;

            case IX_NETWORK_AMITCP:
        	error = TCP_ReleaseSocket(s2, -1);
        	TCP_CloseSocket(s2);
                break;
        }
    }
    return error;
}

int obtain_socket(long id, int inet, int stream, int protocol)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int error = -1;

    switch (network_protocol) {
        case IX_NETWORK_AS225:
            error = SOCK_inherit((void *)id);
            break;

        case IX_NETWORK_AMITCP:
            error = TCP_ObtainSocket(id, inet, stream, protocol);
            break;
    }
    return error;
}

int
_connect (struct file *fp, const struct sockaddr *name, int namelen)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int error = -1, oldlen;

    switch (network_protocol) {
	case IX_NETWORK_AS225:
	    oldlen = name->sa_len;
	    ((struct sockaddr *)name)->sa_len = 0;
	    error = SOCK_connect(fp->f_so, name,namelen);
	    ((struct sockaddr *)name)->sa_len = oldlen;
	break;

	case IX_NETWORK_AMITCP:
	    error = TCP_Connect(fp->f_so, name,namelen);
	break;
    }
    return error;
}

int
_sendto (struct file *fp, const void *buf, int len, int flags, const struct sockaddr *to, int tolen)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int rc = -1, oldlen;

    switch (network_protocol) {

	case IX_NETWORK_AS225:
	    oldlen = to->sa_len;
	    ((struct sockaddr *)to)->sa_len = 0;
	    rc = SOCK_sendto(fp->f_so,buf,len,flags,to,tolen);
	    ((struct sockaddr *)to)->sa_len = oldlen;
	break;

	case IX_NETWORK_AMITCP:
	    rc = TCP_SendTo(fp->f_so,buf,len,flags,to,tolen);
	break;
    }

    return rc;
}


int
_send (struct file *fp, const void *buf, int len, int flags)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int rc = -1 ;

    switch (network_protocol) {

	case IX_NETWORK_AS225:
	    rc = SOCK_send(fp->f_so,buf,len,flags);
	break;

	case IX_NETWORK_AMITCP:
	    rc = TCP_Send(fp->f_so,buf,len,flags);
	break;
    }

    return rc;
}


int
_sendmsg (struct file *fp, const struct msghdr *msg, int flags)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int rc = -1;

    switch (network_protocol) {

	case IX_NETWORK_AS225:
	    rc = SOCK_sendmsg(fp->f_so,msg,flags);
	break;

	case IX_NETWORK_AMITCP:
	    rc = TCP_SendMsg(fp->f_so,msg,flags);
	break;
    }

    return rc;
}


int
_recvfrom (struct file *fp, void *buf, int len, int flags, struct sockaddr *from, int *fromlen)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int rc = -1;

    switch (network_protocol) {

	case IX_NETWORK_AS225:
	    rc = SOCK_recvfrom(fp->f_so,buf,len,flags, from, fromlen);
	break;

	case IX_NETWORK_AMITCP:
	    rc = TCP_RecvFrom(fp->f_so,buf,len,flags, from, fromlen);
	break;
    }

    return rc;
}


int
_recv (struct file *fp, void *buf, int len, int flags)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int rc = -1;

    switch (network_protocol) {

	case IX_NETWORK_AS225:
	    rc = SOCK_recv(fp->f_so,buf,len,flags);
	break;

	case IX_NETWORK_AMITCP:
	    rc = TCP_Recv(fp->f_so,buf,len,flags);
	break;
    }

    return rc;
}


int
_recvmsg (struct file *fp, struct msghdr *msg, int flags)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int rc = -1;

    switch (network_protocol) {

	case IX_NETWORK_AS225:
	    rc = SOCK_recvmsg(fp->f_so,msg,flags);
	break;

	case IX_NETWORK_AMITCP:
	    rc = TCP_RecvMsg(fp->f_so,msg,flags);
	break;
    }

    return rc;
}

int _socketpair(int d, int type, int protocol, int sv[2])
{
    usetup;
    errno = ENOSYS;
    return -1;
}

int
_shutdown (struct file *fp, int how)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int err = 0;

    switch (network_protocol) {

	case IX_NETWORK_AS225:
	    err = SOCK_shutdown(fp->f_so,how);
	break;

	case IX_NETWORK_AMITCP:
	    err = TCP_ShutDown(fp->f_so,how);
	break;
    }
    return err;
}


int
_setsockopt (struct file *fp, int level, int name, const void *val, int valsize)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int err = 0;

    switch (network_protocol) {

	case IX_NETWORK_AS225:
	    err = SOCK_setsockopt(fp->f_so,level,name,val, valsize);
	break;

	case IX_NETWORK_AMITCP:
	    err = TCP_SetSockOpt(fp->f_so,level,name,val, valsize);
	break;
    }

    return err;
}

int
_getsockopt (struct file *fp, int level, int name, void *val, int *valsize)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int err = 0;

    switch (network_protocol) {

	case IX_NETWORK_AS225:
	    err = SOCK_getsockopt(fp->f_so,level,name,val, valsize);
	break;

	case IX_NETWORK_AMITCP:
	    err = TCP_GetSockOpt(fp->f_so,level,name,val, valsize);
	break;
    }

    return err;
}


/*
 * Get socket name.
 */
int
_getsockname (struct file *fp, struct sockaddr *asa, int *alen)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int err = -1;

    switch (network_protocol) {

	case IX_NETWORK_AS225:
	    err = SOCK_getsockname(fp->f_so,asa,alen);
	break;

	case IX_NETWORK_AMITCP:
	    err = TCP_GetSockName(fp->f_so,asa,alen);
	break;
    }

    return err;
}

/*
 * Get name of peer for connected socket.
 */
int
_getpeername (struct file *fp, struct sockaddr *asa, int *alen)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int err = -1;

    switch (network_protocol) {

	case IX_NETWORK_AS225:
	    err = SOCK_getpeername(fp->f_so,asa,alen);
	break;

	case IX_NETWORK_AMITCP:
	    err = TCP_GetPeerName(fp->f_so,asa,alen);
	break;
    }

    return err;
}

int
_tcp_read (struct file *fp, char *buf, int len)
{
    usetup;
    int ostat, rc;
    struct user *p = &u;

    ostat = p->p_stat;
    p->p_stat = SWAIT;

    rc = _recv(fp,buf,len, 0);

    if (CURSIG (p))
	SetSignal (0, SIGBREAKF_CTRL_C);

    p->p_stat = ostat;

    if (errno == EINTR)
	setrun (FindTask (0));

    return rc;
}


int
_tcp_write (struct file *fp, char *buf, int len)
{
    usetup;
    struct user *p = &u;
    int ostat, rc;

    ostat = p->p_stat;
    p->p_stat = SWAIT;

    rc = _send(fp,buf,len,0);

    if (CURSIG (p))
	SetSignal (0, SIGBREAKF_CTRL_C);

    p->p_stat = ostat;

    if (errno == EINTR)
	setrun (FindTask (0));

    return rc;
}

int
_tcp_ioctl (struct file *fp, int cmd, int inout, int arglen, caddr_t data)
{
    usetup;
    register struct user *usr = &u;
    register struct ixnet *p = (struct ixnet *)usr->u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int ostat, err = 0;

    ostat = usr->p_stat;
    usr->p_stat = SWAIT;

    switch (network_protocol) {

	case IX_NETWORK_AS225:

	    /* _SIGH_... they left almost everything neatly as it was in the BSD kernel
	     *	code they used, but for whatever reason they decided they needed their
	     *	own kind of ioctl encoding :-((
	     *
	     *	Well then, here we go, and map `normal' cmds into CBM cmds:
	     */

	    switch (cmd) {
		case SIOCADDRT	     : cmd = ('r'<<8)|1; break;
		case SIOCDELRT	     : cmd = ('r'<<8)|2; break;
		case SIOCSIFADDR     : cmd = ('i'<<8)|3; break;
		case SIOCGIFADDR     : cmd = ('i'<<8)|4; break;
		case SIOCSIFDSTADDR  : cmd = ('i'<<8)|5; break;
		case SIOCGIFDSTADDR  : cmd = ('i'<<8)|6; break;
		case SIOCSIFFLAGS    : cmd = ('i'<<8)|7; break;
		case SIOCGIFFLAGS    : cmd = ('i'<<8)|8; break;
		case SIOCGIFCONF     : cmd = ('i'<<8)|9; break;
		case SIOCSIFMTU      : cmd = ('i'<<8)|10; break;
		case SIOCGIFMTU      : cmd = ('i'<<8)|11; break;
		case SIOCGIFBRDADDR  : cmd = ('i'<<8)|12; break;
		case SIOCSIFBRDADDR  : cmd = ('i'<<8)|13; break;
		case SIOCGIFNETMASK  : cmd = ('i'<<8)|14; break;
		case SIOCSIFNETMASK  : cmd = ('i'<<8)|15; break;
		case SIOCGIFMETRIC   : cmd = ('i'<<8)|16; break;
		case SIOCSIFMETRIC   : cmd = ('i'<<8)|17; break;
		case SIOCSARP	     : cmd = ('i'<<8)|18; break;
		case SIOCGARP	     : cmd = ('i'<<8)|19; break;
		case SIOCDARP	     : cmd = ('i'<<8)|20; break;
		case SIOCATMARK      : cmd = ('i'<<8)|21; break;
		case FIONBIO	     : cmd = ('m'<<8)|22; break;
		case FIONREAD	     : cmd = ('m'<<8)|23; break;
		case FIOASYNC	     : cmd = ('m'<<8)|24; break;
		case SIOCSPGRP	     : cmd = ('m'<<8)|25; break;
		case SIOCGPGRP	     : cmd = ('m'<<8)|26; break;

		default:
		/* we really don't have to bother the library with cmds we can't even
		 * map over...
		 */
	    }
	    err = SOCK_ioctl(fp->f_so,cmd,data);
	break;

	case IX_NETWORK_AMITCP:
	    err = TCP_IoctlSocket(fp->f_so,cmd,data);
	break;
    }
    if (CURSIG (usr))
	SetSignal (0, SIGBREAKF_CTRL_C);

    usr->p_stat = ostat;

    if (errno == EINTR)
	setrun (FindTask (0));

    return err;
}

/* looks like ixemul.library can't grog ixnet.library calling ix_lock_base()
 * moved most of this code back into ixemul.library
 */
int
_tcp_close (struct file *fp)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;
    int err = 0;

#if 0
    ix_lock_base ();
    fp->f_count--;

    if (fp->f_count == 0) {
	/* don't have the base locked for IN_close, this MAY block!! */
	ix_unlock_base ();
#endif
	switch (network_protocol) {

	    case IX_NETWORK_AS225:
		err = SOCK_close (fp->f_so);
	    break;

	    case IX_NETWORK_AMITCP:
		err = TCP_CloseSocket(fp->f_so);
	    break;
	}
#if 0
    }
    else
	ix_unlock_base ();
#endif
    return err;
}

static int
_tcp_poll(struct file *fp, int io_mode)
{
    usetup;
    int rc = -1;
    fd_set in, out, exc;
    struct timeval tv = {0, 0};
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    register int network_protocol = p->u_networkprotocol;

    FD_ZERO(&in);
    FD_ZERO(&out);
    FD_ZERO(&exc);

    switch (io_mode) {
	case SELMODE_IN:
	    FD_SET(fp->f_so,&in);
	break;

	case SELMODE_OUT:
	    FD_SET(fp->f_so,&out);
	break;

	case SELMODE_EXC:
	    FD_SET(fp->f_so,&exc);
	break;
    }

    switch (network_protocol) {
	case IX_NETWORK_AS225:
	    rc = SOCK_selectwait(fp->f_so+1,&in,&out,&exc,&tv,NULL);
	break;

	case IX_NETWORK_AMITCP:
	    rc = TCP_WaitSelect(fp->f_so+1,&in,&out,&exc,&tv,NULL);
	break;
    }

    return ((rc == 1) ? 1 : 0);
}

int
_tcp_select (struct file *fp, int select_cmd, int io_mode, fd_set *set, u_long *nfds)
{
  usetup;

  if (select_cmd == SELCMD_PREPARE)
    {
      register struct ixnet *p = (struct ixnet *)u.u_ixnet;

      FD_SET(fp->f_so, set);
      if (fp->f_so > *nfds)
        *nfds = fp->f_so;
      return (1L << p->u_sigurg | 1L << p->u_sigio);
    }
  if (select_cmd == SELCMD_CHECK)
    return FD_ISSET(fp->f_so, set);
  if (select_cmd == SELCMD_POLL)
    return _tcp_poll(fp, io_mode);
  return 0;
}

u_long
waitselect(long wait_sigs, fd_set *in, fd_set *out, fd_set *exc, u_long nfds)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    int rc = -1;

    switch (p->u_networkprotocol) {
	case IX_NETWORK_AS225:
	    rc = SOCK_selectwait(nfds + 1, in, out, exc, NULL, &wait_sigs);
	break;

	case IX_NETWORK_AMITCP:
	    rc = TCP_WaitSelect(nfds + 1, in, out, exc, NULL, &wait_sigs);
	break;
    }
    return (rc == -1 ? -1 : wait_sigs);
}

/*
 *	init_inet_daemon.c - obtain socket accepted by the inetd
 *
 *	Copyright © 1994 AmiTCP/IP Group,
 *			 Network Solutions Development Inc.
 *			 All rights reserved.
 *	Portions Copyright © 1995 by Jeff Shepherd
 */

/* AS225 inet daemon stuff */
struct inetmsg {
    struct Message  msg;
    ULONG   id;
};

int
init_inet_daemon(int *argc, char ***argv)
{
    usetup;
    register struct user *usr = &u;
    register struct ixnet *p = (struct ixnet *)usr->u_ixnet;
    struct file *fp;
    register int network_protocol = p->u_networkprotocol;
    int sock;

    if (network_protocol == IX_NETWORK_AS225) {
	static int init_d(int *, char ***);
	return init_d(argc,argv);
    }
    else if (network_protocol == IX_NETWORK_AMITCP) {
	struct Process *me = (struct Process *)FindTask(0);
	struct DaemonMessage *dm = (struct DaemonMessage *)me->pr_ExitData;
	int fd,ostat;
	int err;

	if (dm == NULL) {
	    /*
	    * No DaemonMessage, return error code - probably not an inet daemon
	    */
	    return -1;
	}

	/*
	 * Obtain the server socket
	 */
	sock = TCP_ObtainSocket(dm->dm_Id, dm->dm_Family, dm->dm_Type, 0);
	if (sock < 0) {
	    /*
	    * If ObtainSocket fails we need to exit with this specific exit code
	    * so that the inetd knows to clean things up
	    */
	    exit(DERR_OBTAIN);
	}

	ostat = usr->p_stat;
	usr->p_stat = SWAIT;

	do {

	    if ((err = falloc(&fp, &fd)))
		break;

	    fp->f_so = sock;
	    _set_socket_params(fp, dm->dm_Family, dm->dm_Type, 0);
	} while (0);

	if (CURSIG (usr))
	    SetSignal (0, SIGBREAKF_CTRL_C);

	usr->p_stat = ostat;

	if (err == EINTR)
	    setrun (FindTask (0));

	errno = err;
	return err ? -1 : fd;
    }
    else
	return -1;
}

/* code loosely derived from timed.c from AS225r2 */
/* this program was called from inetd if :
 * 1> the first arg is a valid protocol(call getprotobyname)
 * 2> inetd is started - FindPort("inetd") returns non-NULL
 * NOT 3> argv[0] is the program found in inetd.conf for the program (scan inetd.conf)
 */
#include <netdb.h>
#include <stdio.h>

static int init_d(int *argc, char ***argv)
{
    usetup;
    struct user *usr = &u;
    register struct ixnet *p = (struct ixnet *)usr->u_ixnet;
    int ostat;
    int err = 1;
    int fd = -1;

    ostat = usr->p_stat;
    usr->p_stat = SWAIT;

    /* save a little time with this comparison */
    if (*argc >= 4) {
	struct servent *serv, *serv2;
	serv = SOCK_getservbyname((*argv)[1],"tcp");
	serv2 = SOCK_getservbyname((*argv)[1],"udp");
	if (serv || serv2) {
	    if (FindPort("inetd")) {
#if 0 /* I think this isn't needed, SOCK_inherit should be enough */
		char daemon[MAXPATHLEN];
		char line[1024];
		char protocol[MAXPATHLEN];
		FILE *inetdconf;
		int founddaemon = 0;
		if (inetdconf = fopen("inet:db/inetd.conf","r")) {
		    while (!feof(inetdconf)) {
			fgets(line,sizeof(line),inetdconf);
			sscanf(line,"%s %*s %*s %*s %s",protocol,daemon);
			if (!strcmp(protocol,(*argv)[1])) {
			    founddaemon = 1;
			    break;
			}
		    }
		    fclose(inetdconf);

		    if (founddaemon)  {
			char *filename = FilePart(daemon);
#endif
			if (/*!stricmp((*argv)[0],filename)*/1) {
			    struct file *fp;
			long sock_arg;
			int sock;
			    int i;

			    sock_arg = atol((*argv)[2]);
			    p->sock_id = atoi((*argv)[3]);
			    sock = SOCK_inherit((void *)sock_arg);
			    if (sock != -1) {
				p->u_daemon = 1; /* I was started from inetd */
				do {
				    int type;
				    int optlen = sizeof(type);

				    fd = 0;
				    if ((err = falloc(&fp, &fd)))
					break;

				    fp->f_so = sock;
				    _set_socket_params(fp, AF_INET, type, 0);

				    /* get rid of the args that AS225 put in */
				    for (i=1; i < (*argc)-3; i++) {
					(*argv)[i] = (*argv)[i+3];
				    }
				    (*argc) -= 3;
				} while (0);

				if (CURSIG (usr))
				    SetSignal (0, SIGBREAKF_CTRL_C);
			    }
			}
#if 0
		    }
		}
#endif
	    }
	}
    }
    usr->p_stat = ostat;
    errno = err;
    return err ? -1 : fd;
}

/* This is only needed for AS225 */
void shutdown_inet_daemon(void)
{
    usetup;
    register struct ixnet *p = (struct ixnet *)u.u_ixnet;
    struct inetmsg inet_message;
    struct MsgPort *msgport, *replyport;

    if (p->u_networkprotocol != IX_NETWORK_AS225 || !p->u_daemon)
	return;

    if ((inet_message.id = p->sock_id)) {
	replyport = CreateMsgPort();
	if (replyport) {
	    inet_message.msg.mn_Node.ln_Type = NT_MESSAGE;
	    inet_message.msg.mn_Length = sizeof(struct inetmsg);
	    inet_message.msg.mn_ReplyPort = replyport;

	    msgport = FindPort("inetd");
	    if (msgport) {
		PutMsg(msgport,(struct Message *)&inet_message);
		/* we can't exit until we received a reply */
		WaitPort(replyport);
	    }
	    DeleteMsgPort(replyport);
	}
    }
}

