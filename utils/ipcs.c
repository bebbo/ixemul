/*	$NetBSD: ipcs.c,v 1.10.6.1 1996/06/07 01:53:47 thorpej Exp $	*/

/*
 * Copyright (c) 1994 SigmaSoft, Th. Lockert <tholo@sigmasoft.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by SigmaSoft, Th.  Lockert.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL
 * THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#define _KERNEL
#include <sys/ucred.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <sys/msg.h>
#undef _KERNEL

#include <err.h>
#include <fcntl.h>
#include <kvm.h>
#include <limits.h>
#include <nlist.h>
#include <paths.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int	semconfig __P((int, ...));
void	usage __P((void));

extern	char *__progname;		/* from crt0.o */

char   *
fmt_perm(mode)
	u_short mode;
{
	static char buffer[100];

	buffer[0] = '-';
	buffer[1] = '-';
	buffer[2] = ((mode & 0400) ? 'r' : '-');
	buffer[3] = ((mode & 0200) ? 'w' : '-');
	buffer[4] = ((mode & 0100) ? 'a' : '-');
	buffer[5] = ((mode & 0040) ? 'r' : '-');
	buffer[6] = ((mode & 0020) ? 'w' : '-');
	buffer[7] = ((mode & 0010) ? 'a' : '-');
	buffer[8] = ((mode & 0004) ? 'r' : '-');
	buffer[9] = ((mode & 0002) ? 'w' : '-');
	buffer[10] = ((mode & 0001) ? 'a' : '-');
	buffer[11] = '\0';
	return (&buffer[0]);
}

void
cvt_time(t, buf)
	time_t  t;
	char   *buf;
{
	struct tm *tm;

	if (t == 0) {
		strcpy(buf, "no-entry");
	} else {
		tm = localtime(&t);
		sprintf(buf, "%2d:%02d:%02d",
			tm->tm_hour, tm->tm_min, tm->tm_sec);
	}
}
#define	SHMINFO		1
#define	SHMTOTAL	2
#define	MSGINFO		4
#define	MSGTOTAL	8
#define	SEMINFO		16
#define	SEMTOTAL	32

#define BIGGEST		1
#define CREATOR		2
#define OUTSTANDING	4
#define PID		8
#define TIME		16

struct seminfo *seminfo;
struct msginfo *msginfo;

int
main(argc, argv)
	int     argc;
	char   *argv[];
{
	int     display = SHMINFO | MSGINFO | SEMINFO;
	int     option = 0;
	char   *core = NULL, *namelist = NULL;
	char	errbuf[_POSIX2_LINE_MAX];
	int     i;

	while ((i = getopt(argc, argv, "MmQqSsabC:cN:optT")) != EOF)
		switch (i) {
		case 'M':
			display = SHMTOTAL;
			break;
		case 'm':
			display = SHMINFO;
			break;
		case 'Q':
			display = MSGTOTAL;
			break;
		case 'q':
			display = MSGINFO;
			break;
		case 'S':
			display = SEMTOTAL;
			break;
		case 's':
			display = SEMINFO;
			break;
		case 'T':
			display = SHMTOTAL | MSGTOTAL | SEMTOTAL;
			break;
		case 'a':
			option |= BIGGEST | CREATOR | OUTSTANDING | PID | TIME;
			break;
		case 'b':
			option |= BIGGEST;
			break;
		case 'C':
			core = optarg;
			break;
		case 'c':
			option |= CREATOR;
			break;
		case 'N':
			namelist = optarg;
			break;
		case 'o':
			option |= OUTSTANDING;
			break;
		case 'p':
			option |= PID;
			break;
		case 't':
			option |= TIME;
			break;
		default:
			usage();
		}

	/*
	 * Discard setgid privelidges if not the running kernel so that
	 * bad guys can't print interesting stuff from kernel memory.
	 */
	if (namelist != NULL || core != NULL)
		setgid(getgid());

	if ((display & (MSGINFO | MSGTOTAL)) &&
	    (msginfo = (void *)msgctl(0, GETMSGINFO, 0))) {
		if (display & MSGTOTAL) {
			printf("msginfo:\n");
			printf("\tmsgmax: %6d\t(max characters in a message)\n",
			    msginfo->msgmax);
			printf("\tmsgmni: %6d\t(# of message queues)\n",
			    msginfo->msgmni);
			printf("\tmsgmnb: %6d\t(max characters in a message queue)\n",
			    msginfo->msgmnb);
			printf("\tmsgtql: %6d\t(max # of messages in system)\n",
			    msginfo->msgtql);
			printf("\tmsgssz: %6d\t(size of a message segment)\n",
			    msginfo->msgssz);
			printf("\tmsgseg: %6d\t(# of message segments in system)\n\n",
			    msginfo->msgseg);
		}
		if (display & MSGINFO) {
			struct msqid_ds *xmsqids = malloc(sizeof(struct msqid_ds) *
			    msginfo->msgmni);

			memcpy(xmsqids, (void *)msgctl(0, GETMSQIDSA, 0),
			       sizeof(struct msqid_ds) * msginfo->msgmni);

			printf("Message Queues:\n");
			printf("T     ID     KEY        MODE       OWNER    GROUP");
			if (option & CREATOR)
				printf("  CREATOR   CGROUP");
			if (option & OUTSTANDING)
				printf(" CBYTES  QNUM");
			if (option & BIGGEST)
				printf(" QBYTES");
			if (option & PID)
				printf(" LSPID LRPID");
			if (option & TIME)
				printf("   STIME    RTIME    CTIME");
			printf("\n");
			for (i = 0; i < msginfo->msgmni; i += 1) {
				if (xmsqids[i].msg_qbytes != 0) {
					char    stime_buf[100], rtime_buf[100],
					        ctime_buf[100];
					struct msqid_ds *msqptr = &xmsqids[i];

					cvt_time(msqptr->msg_stime, stime_buf);
					cvt_time(msqptr->msg_rtime, rtime_buf);
					cvt_time(msqptr->msg_ctime, ctime_buf);

					printf("q %6d %10d %s %8s %8s",
					    IXSEQ_TO_IPCID(i, msqptr->msg_perm),
					    msqptr->msg_perm.key,
					    fmt_perm(msqptr->msg_perm.mode),
					    user_from_uid(msqptr->msg_perm.uid, 0),
					    group_from_gid(msqptr->msg_perm.gid, 0));

					if (option & CREATOR)
						printf(" %8s %8s",
						    user_from_uid(msqptr->msg_perm.cuid, 0),
						    group_from_gid(msqptr->msg_perm.cgid, 0));

					if (option & OUTSTANDING)
						printf(" %6d %6d",
						    msqptr->msg_cbytes,
						    msqptr->msg_qnum);

					if (option & BIGGEST)
						printf(" %6d",
						    msqptr->msg_qbytes);

					if (option & PID)
						printf(" %6d %6d",
						    msqptr->msg_lspid,
						    msqptr->msg_lrpid);

					if (option & TIME)
						printf("%s %s %s",
						    stime_buf,
						    rtime_buf,
						    ctime_buf);

					printf("\n");
				}
			}
			printf("\n");
		}
	} else
		if (display & (MSGINFO | MSGTOTAL)) {
			fprintf(stderr,
			    "SVID messages facility not configured in the system\n");
		}
	if (display & (SHMINFO | SHMTOTAL)) {
		if (display & SHMINFO) {
			struct shm_list *shmlist = (struct shm_list *)shmctl(0, IPC_GETLIST, 0);
			struct shmid_ds *xshmids;

			printf("Shared Memory:\n");
			printf("T         ID        KEY        MODE    OWNER    GROUP");
			if (option & CREATOR)
				printf("  CREATOR   CGROUP");
			if (option & OUTSTANDING)
				printf(" NATTCH");
			if (option & BIGGEST)
				printf("  SEGSZ");
			if (option & PID)
				printf("       CPID       LPID");
			if (option & TIME)
				printf("    ATIME    DTIME    CTIME");
			printf("\n");
			while (shmlist) {
				xshmids = &shmlist->ds;
				shmlist = shmlist->next;
				if (xshmids->shm_perm.mode & 0x0800) {
					char    atime_buf[100], dtime_buf[100],
					        ctime_buf[100];
					struct shmid_ds *shmptr = xshmids;

					cvt_time(shmptr->shm_atime, atime_buf);
					cvt_time(shmptr->shm_dtime, dtime_buf);
					cvt_time(shmptr->shm_ctime, ctime_buf);

					printf("m %10d %10d %s %8s %8s",
					    IPC_SHMID(shmptr->shm_perm),
					    shmptr->shm_perm.key,
					    fmt_perm(shmptr->shm_perm.mode),
					    user_from_uid(shmptr->shm_perm.uid, 0),
					    group_from_gid(shmptr->shm_perm.gid, 0));

					if (option & CREATOR)
						printf(" %8s %8s",
						    user_from_uid(shmptr->shm_perm.cuid, 0),
						    group_from_gid(shmptr->shm_perm.cgid, 0));

					if (option & OUTSTANDING)
						printf(" %6d",
						    shmptr->shm_nattch);

					if (option & BIGGEST)
						printf(" %6d",
						    shmptr->shm_segsz);

					if (option & PID)
						printf(" %10d %10d",
						    shmptr->shm_cpid,
						    shmptr->shm_lpid);

					if (option & TIME)
						printf(" %s %s %s",
						    atime_buf,
						    dtime_buf,
						    ctime_buf);

					printf("\n");
				}
			}
			printf("\n");
		}
	} else
		if (display & (SHMINFO | SHMTOTAL)) {
			fprintf(stderr,
			    "SVID shared memory facility not configured in the system\n");
		}

	if ((display & (SEMINFO | SEMTOTAL)) &&
	    (seminfo = (void *)semctl(0, 0, GETSEMINFO, 0))) {
		struct semid_ds *xsema;

		if (display & SEMTOTAL) {
			printf("seminfo:\n");
			printf("\tsemmap: %6d\t(# of entries in semaphore map)\n",
			    seminfo->semmap);
			printf("\tsemmni: %6d\t(# of semaphore identifiers)\n",
			    seminfo->semmni);
			printf("\tsemmns: %6d\t(# of semaphores in system)\n",
			    seminfo->semmns);
			printf("\tsemmnu: %6d\t(# of undo structures in system)\n",
			    seminfo->semmnu);
			printf("\tsemmsl: %6d\t(max # of semaphores per id)\n",
			    seminfo->semmsl);
			printf("\tsemopm: %6d\t(max # of operations per semop call)\n",
			    seminfo->semopm);
			printf("\tsemume: %6d\t(max # of undo entries per process)\n",
			    seminfo->semume);
			printf("\tsemusz: %6d\t(size in bytes of undo structure)\n",
			    seminfo->semusz);
			printf("\tsemvmx: %6d\t(semaphore maximum value)\n",
			    seminfo->semvmx);
			printf("\tsemaem: %6d\t(adjust on exit max value)\n\n",
			    seminfo->semaem);
		}
		if (display & SEMINFO) {
			if (semctl(0, 0, SEMLCK, 0) != 0) {
				perror("semctl");
				fprintf(stderr,
				    "Can't lock semaphore facility - winging it...\n");
			}
			xsema = (void *)semctl(0, 0, GETSEMA, 0);

			printf("Semaphores:\n");
			printf("T     ID     KEY        MODE       OWNER    GROUP");
			if (option & CREATOR)
				printf("  CREATOR   CGROUP");
			if (option & BIGGEST)
				printf(" NSEMS");
			if (option & TIME)
				printf("   OTIME    CTIME");
			printf("\n");
			for (i = 0; i < seminfo->semmni; i += 1) {
				if ((xsema[i].sem_perm.mode & SEM_ALLOC) != 0) {
					char    ctime_buf[100], otime_buf[100];
					struct semid_ds *semaptr = &xsema[i];
					int     j, value;
					union semun junk;

					cvt_time(semaptr->sem_otime, otime_buf);
					cvt_time(semaptr->sem_ctime, ctime_buf);

					printf("s %6d %10d %s %8s %8s",
					    IXSEQ_TO_IPCID(i, semaptr->sem_perm),
					    semaptr->sem_perm.key,
					    fmt_perm(semaptr->sem_perm.mode),
					    user_from_uid(semaptr->sem_perm.uid, 0),
					    group_from_gid(semaptr->sem_perm.gid, 0));

					if (option & CREATOR)
						printf(" %8s %8s",
						    user_from_uid(semaptr->sem_perm.cuid, 0),
						    group_from_gid(semaptr->sem_perm.cgid, 0));

					if (option & BIGGEST)
						printf(" %6d",
						    semaptr->sem_nsems);

					if (option & TIME)
						printf("%s %s",
						    otime_buf,
						    ctime_buf);

					printf("\n");
				}
			}

			(void) semctl(0, 0, SEMUNLK, 0);

			printf("\n");
		}
	} else
		if (display & (SEMINFO | SEMTOTAL)) {
			fprintf(stderr, "SVID semaphores facility not configured in the system\n");
		}

	exit(0);
}

void
usage()
{

	fprintf(stderr,
	    "usage: %s [-abcmopqst] [-C corefile] [-N namelist]\n",
	    __progname);
	exit(1);
}
