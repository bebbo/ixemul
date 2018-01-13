/*	$NetBSD: sysv_msg.c,v 1.19 1996/02/09 19:00:18 christos Exp $	*/

/*
 * Implementation of SVID messages
 *
 * Author:  Daniel Boulet
 *
 * Copyright 1993 Daniel Boulet and RTMX Inc.
 *
 * This system call was implemented by Daniel Boulet under contract from RTMX.
 *
 * Redistribution and use in source forms, with and without modification,
 * are permitted provided that this entire comment appears intact.
 *
 * Redistribution in binary form may occur without any restrictions.
 * Obviously, it would be nice if you gave credit where credit is due
 * but requiring it would be too onerous.
 *
 * This software is provided ``AS IS'' without any warranties of any kind.
 */

#define _KERNEL
#include <ixemul.h>
#include <sys/msg.h>
#include <time.h>

#define MSG_DEBUG
#undef MSG_DEBUG_OK

static int nfree_msgmaps;		/* # of free map entries */
static short free_msgmaps;		/* head of linked list of free map entries */
static struct msg *free_msghdrs;	/* list of free msg headers */

static char msgpool[MSGMAX];		/* MSGMAX byte long msg buffer pool */
static struct msgmap msgmaps[MSGSEG];	/* MSGSEG msgmap structures */
static struct msg msghdrs[MSGTQL];	/* MSGTQL msg headers */
static struct msqid_ds msqids[MSGMNI];	/* MSGMNI msqid_ds struct's */

static void msg_freehdr __P((struct msg *));

static struct msginfo msginfo = {
	MSGMAX,		/* max chars in a message */
	MSGMNI,		/* # of message queue identifiers */
	MSGMNB,		/* max chars in a queue */
	MSGTQL,		/* max messages in system */
	MSGSSZ,		/* size of a message segment */
			/* (must be small power of 2 greater than 4) */
	MSGSEG		/* number of message segments */
};


void
msginit()
{
	register int i;

	/*
	 * msginfo.msgssz should be a power of two for efficiency reasons.
	 * It is also pretty silly if msginfo.msgssz is less than 8
	 * or greater than about 256 so ...
	 */

	i = 8;
	while (i < 1024 && i != msginfo.msgssz)
		i <<= 1;
    	if (i != msginfo.msgssz) {
		panic("msginfo.msgssz=%d (0x%x) (not a small power of 2)", msginfo.msgssz,
		    msginfo.msgssz);
	}

	if (msginfo.msgseg > 32767) {
		panic("msginfo.msgseg=%d (> 32767)", msginfo.msgseg);
	}

	for (i = 0; i < msginfo.msgseg; i++) {
		if (i > 0)
			msgmaps[i-1].next = i;
		msgmaps[i].next = -1;	/* implies entry is available */
	}
	free_msgmaps = 0;
	nfree_msgmaps = msginfo.msgseg;

	for (i = 0; i < msginfo.msgtql; i++) {
		msghdrs[i].msg_type = 0;
		if (i > 0)
			msghdrs[i-1].msg_next = &msghdrs[i];
		msghdrs[i].msg_next = NULL;
    	}
	free_msghdrs = &msghdrs[0];

	for (i = 0; i < msginfo.msgmni; i++) {
		msqids[i].msg_qbytes = 0;	/* implies entry is available */
		msqids[i].msg_perm.seq = 0;	/* reset to a known value */
	}
}

static void
msg_freehdr(struct msg *msghdr)
{
	while (msghdr->msg_ts > 0) {
		short next;
		if (msghdr->msg_spot < 0 || msghdr->msg_spot >= msginfo.msgseg)
			panic("msghdr->msg_spot out of range");
		next = msgmaps[msghdr->msg_spot].next;
		msgmaps[msghdr->msg_spot].next = free_msgmaps;
		free_msgmaps = msghdr->msg_spot;
		nfree_msgmaps++;
		msghdr->msg_spot = next;
		if (msghdr->msg_ts >= msginfo.msgssz)
			msghdr->msg_ts -= msginfo.msgssz;
		else
			msghdr->msg_ts = 0;
	}
	if (msghdr->msg_spot != -1)
		panic("msghdr->msg_spot != -1");
	msghdr->msg_next = free_msghdrs;
	free_msghdrs = msghdr;
}

static int
ix_msgctl(int msqid, int cmd, struct msqid_ds *user_msqptr)
{
	usetup;
	struct ucred cred;
	int eval;
	register struct msqid_ds *msqptr;

	cred.cr_uid = geteuid();
	cred.cr_gid = getegid();

	msqid = IPCID_TO_IX(msqid);

	if (msqid < 0 || msqid >= msginfo.msgmni) {
		errno_return(EINVAL, -1);
	}

	msqptr = &msqids[msqid];

	if (msqptr->msg_qbytes == 0) {
		errno_return(EINVAL, -1);
	}
	if (msqptr->msg_perm.seq != IPCID_TO_SEQ(msqid)) {
		errno_return(EINVAL, -1);
	}

	eval = 0;

	switch (cmd) {

	case IPC_RMID:
	{
		struct msg *msghdr;
		if ((eval = ipcperm(&cred, &msqptr->msg_perm, IPC_M)) != 0)
			errno_return(eval, -1);
		/* Free the message headers */
		msghdr = msqptr->msg_first;
		while (msghdr != NULL) {
			struct msg *msghdr_tmp;

			/* Free the segments of each message */
			msqptr->msg_cbytes -= msghdr->msg_ts;
			msqptr->msg_qnum--;
			msghdr_tmp = msghdr;
			msghdr = msghdr->msg_next;
			msg_freehdr(msghdr_tmp);
		}

		if (msqptr->msg_cbytes != 0)
			panic("msg_cbytes is screwed up");
		if (msqptr->msg_qnum != 0)
			panic("msg_qnum is screwed up");

		msqptr->msg_qbytes = 0;	/* Mark it as free */

		ix_wakeup((u_int)msqptr);
	}

		break;

	case IPC_SET:
		if ((eval = ipcperm(&cred, &msqptr->msg_perm, IPC_M)))
			errno_return(eval, -1);
		if (user_msqptr->msg_qbytes > msqptr->msg_qbytes && cred.cr_uid != 0)
			errno_return(EPERM, -1);
		if (user_msqptr->msg_qbytes > msginfo.msgmnb) {
			user_msqptr->msg_qbytes = msginfo.msgmnb;	/* silently restrict qbytes to system limit */
		}
		if (user_msqptr->msg_qbytes == 0) {
			errno_return(EINVAL, -1);		/* non-standard errno! */
		}
		msqptr->msg_perm.uid = user_msqptr->msg_perm.uid;	/* change the owner */
		msqptr->msg_perm.gid = user_msqptr->msg_perm.gid;	/* change the owner */
		msqptr->msg_perm.mode = (msqptr->msg_perm.mode & ~0777) |
		    (user_msqptr->msg_perm.mode & 0777);
		msqptr->msg_qbytes = user_msqptr->msg_qbytes;
		msqptr->msg_ctime = time(NULL);
		break;

	case IPC_STAT:
		if ((eval = ipcperm(&cred, &msqptr->msg_perm, IPC_R))) {
			errno_return(eval, -1);
		}
		memcpy(user_msqptr, (caddr_t)msqptr, sizeof(struct msqid_ds));
		break;

	default:
		errno_return(EINVAL, -1);
	}
	return 0;
}

static int
ix_msgget(key_t key, int msgflg)
{
	usetup;
	struct ucred cred;
	int msqid, eval;
	register struct msqid_ds *msqptr = NULL;

	cred.cr_uid = geteuid();
	cred.cr_gid = getegid();

	if (key != IPC_PRIVATE) {
		for (msqid = 0; msqid < msginfo.msgmni; msqid++) {
			msqptr = &msqids[msqid];
			if (msqptr->msg_qbytes != 0 &&
			    msqptr->msg_perm.key == key)
				break;
		}
		if (msqid < msginfo.msgmni) {
			if ((msgflg & IPC_CREAT) && (msgflg & IPC_EXCL)) {
				errno_return(EEXIST, -1);
			}
			if ((eval = ipcperm(&cred, &msqptr->msg_perm, msgflg & 0700 ))) {
				errno_return(eval, -1);
			}
			goto found;
		}
	}

	if (key == IPC_PRIVATE || (msgflg & IPC_CREAT)) {
		for (msqid = 0; msqid < msginfo.msgmni; msqid++) {
			/*
			 * Look for an unallocated and unlocked msqid_ds.
			 * msqid_ds's can be locked by msgsnd or msgrcv while
			 * they are copying the message in/out.  We can't
			 * re-use the entry until they release it.
			 */
			msqptr = &msqids[msqid];
			if (msqptr->msg_qbytes == 0 &&
			    (msqptr->msg_perm.mode & MSG_LOCKED) == 0)
				break;
		}
		if (msqid == msginfo.msgmni) {
			errno_return(ENOSPC, -1);	
		}
		msqptr->msg_perm.key = key;
		msqptr->msg_perm.cuid = cred.cr_uid;
		msqptr->msg_perm.uid = cred.cr_uid;
		msqptr->msg_perm.cgid = cred.cr_gid;
		msqptr->msg_perm.gid = cred.cr_gid;
		msqptr->msg_perm.mode = (msgflg & 0777);
		/* Make sure that the returned msqid is unique */
		msqptr->msg_perm.seq++;
		msqptr->msg_first = NULL;
		msqptr->msg_last = NULL;
		msqptr->msg_cbytes = 0;
		msqptr->msg_qnum = 0;
		msqptr->msg_qbytes = msginfo.msgmnb;
		msqptr->msg_lspid = 0;
		msqptr->msg_lrpid = 0;
		msqptr->msg_stime = 0;
		msqptr->msg_rtime = 0;
		msqptr->msg_ctime = time(NULL);
	} else {
		errno_return(ENOENT, -1);
	}

found:
	/* Construct the unique msqid */
	return IXSEQ_TO_IPCID(msqid, msqptr->msg_perm);
}

static int
ix_msgsnd(int msqid, void *user_msgp, size_t msgsz, int msgflg)
{
	usetup;
	struct ucred cred;
	int segs_needed, eval;
	register struct msqid_ds *msqptr;
	register struct msg *msghdr;
	short next;

	cred.cr_uid = geteuid();
	cred.cr_gid = getegid();

	msqid = IPCID_TO_IX(msqid);

	if (msqid < 0 || msqid >= msginfo.msgmni) {
		errno_return(EINVAL, -1);
	}

	msqptr = &msqids[msqid];
	if (msqptr->msg_qbytes == 0) {
		errno_return(EINVAL, -1);
	}
	if (msqptr->msg_perm.seq != IPCID_TO_SEQ(msqid)) {
		errno_return(EINVAL, -1);
	}

	if ((eval = ipcperm(&cred, &msqptr->msg_perm, IPC_W))) {
		errno_return(eval, -1);
	}

	segs_needed = (msgsz + msginfo.msgssz - 1) / msginfo.msgssz;

	for (;;) {
		int need_more_resources = 0;

		/*
		 * check msgsz [cannot be negative since it is unsigned]
		 * (inside this loop in case msg_qbytes changes while we sleep)
		 */

		if (msgsz > msqptr->msg_qbytes) {
			errno_return(EINVAL, -1);
		}

		if (msqptr->msg_perm.mode & MSG_LOCKED) {
			need_more_resources = 1;
		}
		if (msgsz + msqptr->msg_cbytes > msqptr->msg_qbytes) {
			need_more_resources = 1;
		}
		if (segs_needed > nfree_msgmaps) {
			need_more_resources = 1;
		}
		if (free_msghdrs == NULL) {
			need_more_resources = 1;
		}

		if (need_more_resources) {
			int we_own_it;

			if ((msgflg & IPC_NOWAIT) != 0) {
				errno_return(EAGAIN, -1);
			}

			if ((msqptr->msg_perm.mode & MSG_LOCKED) != 0) {
				we_own_it = 0;
			} else {
				/* Force later arrivals to wait for our
				   request */
				msqptr->msg_perm.mode |= MSG_LOCKED;
				we_own_it = 1;
			}
			eval = ix_sleep((caddr_t)msqptr, "msgwait");
			if (we_own_it)
				msqptr->msg_perm.mode &= ~MSG_LOCKED;
			if (eval != 0) {
				errno_return(EINTR, -1);
			}

			/*
			 * Make sure that the msq queue still exists
			 */

			if (msqptr->msg_qbytes == 0) {
				/* The SVID says to return EIDRM. */
#ifdef EIDRM
				errno_return(EIDRM, -1);
#else
				/* Unfortunately, BSD doesn't define that code
				   yet! */
				errno_return(EINVAL, -1);
#endif
			}

		} else {
			break;
		}
	}

	/*
	 * We have the resources that we need.
	 * Make sure!
	 */

	if (msqptr->msg_perm.mode & MSG_LOCKED)
		panic("msg_perm.mode & MSG_LOCKED");
	if (segs_needed > nfree_msgmaps)
		panic("segs_needed > nfree_msgmaps");
	if (msgsz + msqptr->msg_cbytes > msqptr->msg_qbytes)
		panic("msgsz + msg_cbytes > msg_qbytes");
	if (free_msghdrs == NULL)
		panic("no more msghdrs");

	/*
	 * Re-lock the msqid_ds in case we page-fault when copying in the
	 * message
	 */

	if ((msqptr->msg_perm.mode & MSG_LOCKED) != 0)
		panic("msqid_ds is already locked");
	msqptr->msg_perm.mode |= MSG_LOCKED;

	/*
	 * Allocate a message header
	 */

	msghdr = free_msghdrs;
	free_msghdrs = msghdr->msg_next;
	msghdr->msg_spot = -1;
	msghdr->msg_ts = msgsz;

	/*
	 * Allocate space for the message
	 */

	while (segs_needed > 0) {
		if (nfree_msgmaps <= 0)
			panic("not enough msgmaps");
		if (free_msgmaps == -1)
			panic("nil free_msgmaps");
		next = free_msgmaps;
		if (next <= -1)
			panic("next too low #1");
		if (next >= msginfo.msgseg)
			panic("next out of range #1");
		free_msgmaps = msgmaps[next].next;
		nfree_msgmaps--;
		msgmaps[next].next = msghdr->msg_spot;
		msghdr->msg_spot = next;
		segs_needed--;
	}

	/*
	 * Copy in the message type
	 */

        memcpy(&msghdr->msg_type, user_msgp, sizeof(msghdr->msg_type));

	user_msgp += sizeof(msghdr->msg_type);

	/*
	 * Validate the message type
	 */

	if (msghdr->msg_type < 1) {
		msg_freehdr(msghdr);
		msqptr->msg_perm.mode &= ~MSG_LOCKED;
		ix_wakeup((u_int)msqptr);
		errno_return(EINVAL, -1);
	}

	/*
	 * Copy in the message body
	 */

	next = msghdr->msg_spot;
	while (msgsz > 0) {
		size_t tlen;
		if (msgsz > msginfo.msgssz)
			tlen = msginfo.msgssz;
		else
			tlen = msgsz;
		if (next <= -1)
			panic("next too low #2");
		if (next >= msginfo.msgseg)
			panic("next out of range #2");
		memcpy(&msgpool[next * msginfo.msgssz], user_msgp, tlen);
		msgsz -= tlen;
		user_msgp += tlen;
		next = msgmaps[next].next;
	}
	if (next != -1)
		panic("didn't use all the msg segments");

	/*
	 * We've got the message.  Unlock the msqid_ds.
	 */

	msqptr->msg_perm.mode &= ~MSG_LOCKED;

	/*
	 * Make sure that the msqid_ds is still allocated.
	 */

	if (msqptr->msg_qbytes == 0) {
		msg_freehdr(msghdr);
		ix_wakeup((u_int)msqptr);
		/* The SVID says to return EIDRM. */
#ifdef EIDRM
		errno_return(EIDRM, -1);
#else
		/* Unfortunately, BSD doesn't define that code yet! */
		errno_return(EINVAL, -1);
#endif
	}

	/*
	 * Put the message into the queue
	 */

	if (msqptr->msg_first == NULL) {
		msqptr->msg_first = msghdr;
		msqptr->msg_last = msghdr;
	} else {
		msqptr->msg_last->msg_next = msghdr;
		msqptr->msg_last = msghdr;
	}
	msqptr->msg_last->msg_next = NULL;

	msqptr->msg_cbytes += msghdr->msg_ts;
	msqptr->msg_qnum++;
	msqptr->msg_lspid = getpid();
	msqptr->msg_stime = time(NULL);

	ix_wakeup((u_int)msqptr);
	return 0;
}

static int
ix_msgrcv(int msqid, void *user_msgp, size_t msgsz, long msgtyp, int msgflg)
{
	usetup;
	struct ucred cred;
	size_t len;
	register struct msqid_ds *msqptr;
	register struct msg *msghdr;
	int eval;
	short next;

	cred.cr_uid = geteuid();
	cred.cr_gid = getegid();

	msqid = IPCID_TO_IX(msqid);

	if (msqid < 0 || msqid >= msginfo.msgmni) {
		errno_return(EINVAL, -1);
	}

	msqptr = &msqids[msqid];
	if (msqptr->msg_qbytes == 0) {
		errno_return(EINVAL, -1);
	}
	if (msqptr->msg_perm.seq != IPCID_TO_SEQ(msqid)) {
		errno_return(EINVAL, -1);
	}

	if ((eval = ipcperm(&cred, &msqptr->msg_perm, IPC_R))) {
		errno_return(eval, -1);
	}

	msghdr = NULL;
	while (msghdr == NULL) {
		if (msgtyp == 0) {
			msghdr = msqptr->msg_first;
			if (msghdr != NULL) {
				if (msgsz < msghdr->msg_ts &&
				    (msgflg & MSG_NOERROR) == 0) {
					errno_return(E2BIG, -1);
				}
				if (msqptr->msg_first == msqptr->msg_last) {
					msqptr->msg_first = NULL;
					msqptr->msg_last = NULL;
				} else {
					msqptr->msg_first = msghdr->msg_next;
					if (msqptr->msg_first == NULL)
						panic("msg_first/last screwed up #1");
				}
			}
		} else {
			struct msg *previous;
			struct msg **prev;

			for (previous = NULL, prev = &msqptr->msg_first;
			     (msghdr = *prev) != NULL;
			     previous = msghdr, prev = &msghdr->msg_next) {
				/*
				 * Is this message's type an exact match or is
				 * this message's type less than or equal to
				 * the absolute value of a negative msgtyp?
				 * Note that the second half of this test can
				 * NEVER be true if msgtyp is positive since
				 * msg_type is always positive!
				 */

				if (msgtyp == msghdr->msg_type ||
				    msghdr->msg_type <= -msgtyp) {
					if (msgsz < msghdr->msg_ts &&
					    (msgflg & MSG_NOERROR) == 0) {
						errno_return(E2BIG, -1);
					}
					*prev = msghdr->msg_next;
					if (msghdr == msqptr->msg_last) {
						if (previous == NULL) {
							if (prev !=
							    &msqptr->msg_first)
								panic("msg_first/last screwed up #2");
							msqptr->msg_first =
							    NULL;
							msqptr->msg_last =
							    NULL;
						} else {
							if (prev ==
							    &msqptr->msg_first)
								panic("msg_first/last screwed up #3");
							msqptr->msg_last =
							    previous;
						}
					}
					break;
				}
			}
		}

		/*
		 * We've either extracted the msghdr for the appropriate
		 * message or there isn't one.
		 * If there is one then bail out of this loop.
		 */

		if (msghdr != NULL)
			break;

		/*
		 * Hmph!  No message found.  Does the user want to wait?
		 */

		if ((msgflg & IPC_NOWAIT) != 0) {
			/* The SVID says to return ENOMSG. */
#ifdef ENOMSG
			errno_return(ENOMSG, -1);
#else
			/* Unfortunately, BSD doesn't define that code yet! */
			errno_return(EAGAIN, -1);
#endif
		}

		/*
		 * Wait for something to happen
		 */

		eval = ix_sleep((caddr_t)msqptr, "msgwait");

		if (eval != 0) {
			errno_return(EINTR, -1);
		}

		/*
		 * Make sure that the msq queue still exists
		 */

		if (msqptr->msg_qbytes == 0 ||
		    msqptr->msg_perm.seq != IPCID_TO_SEQ(msqid)) {
			/* The SVID says to return EIDRM. */
#ifdef EIDRM
			errno_return(EIDRM, -1);
#else
			/* Unfortunately, BSD doesn't define that code yet! */
			errno_return(EINVAL, -1);
#endif
		}
	}

	/*
	 * Return the message to the user.
	 *
	 * First, do the bookkeeping (before we risk being interrupted).
	 */

	msqptr->msg_cbytes -= msghdr->msg_ts;
	msqptr->msg_qnum--;
	msqptr->msg_lrpid = getpid();
	msqptr->msg_rtime = time(NULL);

	/*
	 * Make msgsz the actual amount that we'll be returning.
	 * Note that this effectively truncates the message if it is too long
	 * (since msgsz is never increased).
	 */

	if (msgsz > msghdr->msg_ts)
		msgsz = msghdr->msg_ts;

	/*
	 * Return the type to the user.
	 */

   	memcpy(user_msgp, (caddr_t)&msghdr->msg_type, sizeof(msghdr->msg_type));
	user_msgp += sizeof(msghdr->msg_type);

	/*
	 * Return the segments to the user
	 */

	next = msghdr->msg_spot;
	for (len = 0; len < msgsz; len += msginfo.msgssz) {
		size_t tlen;

		if (msgsz > msginfo.msgssz)
			tlen = msginfo.msgssz;
		else
			tlen = msgsz;
		if (next <= -1)
			panic("next too low #3");
		if (next >= msginfo.msgseg)
			panic("next out of range #3");
		memcpy(user_msgp, (caddr_t)&msgpool[next * msginfo.msgssz], tlen);
		user_msgp += tlen;
		next = msgmaps[next].next;
	}

	/*
	 * Done, return the actual number of bytes copied out.
	 */

	msg_freehdr(msghdr);
	ix_wakeup((u_int)msqptr);
	return msgsz;
}

int
msgget(key_t key, int msgflg)
{
  int result;

  Forbid();
  result = ix_msgget(key, msgflg);
  Permit();
  return result;
}

int
msgctl(int msqid, int cmd, struct msqid_ds *buf)
{
  int result;

  if (cmd == GETMSGINFO)
    return (int)&msginfo;
  if (cmd == GETMSQIDSA)
    return (int)msqids;
  Forbid();
  result = ix_msgctl(msqid, cmd, buf);
  Permit();
  return result;
}

int
msgsnd(int msqid, void *msgp, size_t msgsz, int msgflg)
{
  int result;

  Forbid();
  result = ix_msgsnd(msqid, msgp, msgsz, msgflg);
  Permit();
  return result;
}

int
msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp, int msgflg)
{
  int result;

  Forbid();
  result = ix_msgrcv(msqid, msgp, msgsz, msgtyp, msgflg);
  Permit();
  return result;
}
