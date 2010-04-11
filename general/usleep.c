/*
 * Copyright (c) 1989 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that: (1) source distributions retain this entire copyright
 * notice and comment, and (2) distributions including binaries display
 * the following acknowledgement:  ``This product includes software
 * developed by the University of California, Berkeley and its contributors''
 * in the documentation or other materials provided with the distribution
 * and in all advertising materials mentioning features or use of this
 * software. Neither the name of the University nor the names of its
 * contributors may be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)usleep.c    5.5 (Berkeley) 6/1/90";
#endif /* LIBC_SCCS and not lint */

#define _KERNEL
#include "ixemul.h"
#define NEWUSLEEP

#include <sys/time.h>

#define TICK    10000           /* system clock resolution in microseconds */
#define USPS    1000000         /* number of microseconds in a second */


#ifdef NEWUSLEEP
void delete_timer(struct timerequest *tr )
{
struct MsgPort *tp;

if (tr != 0 )
    {
    tp = tr->tr_node.io_Message.mn_ReplyPort;

    if (tp != 0)
        DeleteMsgPort(tp);

    CloseDevice( (struct IORequest *) tr );
    DeleteIORequest( (struct IORequest *) tr );
    }
}

struct timerequest *create_timer( ULONG unit )
{
/* return a pointer to a timer request.  If any problem, return NULL */
LONG error;
struct MsgPort *timerport;
struct timerequest *TimerIO;

timerport = CreateMsgPort();
if (timerport == NULL )
    return( NULL );

TimerIO = (struct timerequest *)
    CreateIORequest( timerport, sizeof( struct timerequest ) );
if (TimerIO == NULL )
    {
    DeleteMsgPort(timerport);   /* Delete message port */
    return( NULL );
    }
 error = 0;
error = OpenDevice( TIMERNAME, unit,(struct IORequest *) TimerIO, 0L );
 //see powersdl source

        //TimerIO->tr_node.io_Device                  = TimerReq[0]->tr_node.io_Device;
		//TimerIO->tr_node.io_Unit                    = TimerReq[0]->tr_node.io_Unit;

if (error != 0 )
    {
	kprintf("cant open timer device \n");
    delete_timer( TimerIO );
    return( NULL );
    }

return( TimerIO );
}


/* more precise timer than AmigaDOS Delay() */



void wait_for_timer(struct timerequest *tr, struct timeval *tv )
{

tr->tr_node.io_Command = TR_ADDREQUEST; /* add a new timer request */

/* structure assignment */
tr->tr_time = *tv;

/* post request to the timer -- will go to sleep till done */
DoIO((struct IORequest *) tr );
}

LONG time_delay( struct timeval *tv, LONG unit )
{
struct timerequest *tr;
/* get a pointer to an initialized timer request block */
tr = create_timer( unit );

/* any nonzero return says timedelay routine didn't work. */
if (tr == NULL )
    return( -1L );

wait_for_timer( tr, tv );

/* deallocate temporary structures */
delete_timer( tr );
return( 0L );
}

void usleep (u_int useconds)
{

	struct timeval tv;
	if (useconds == 0)
	{
	   return;
	}
		tv.tv_secs	= useconds / 1000000;
		tv.tv_micro	= (useconds % 1000000);

	time_delay(&tv, UNIT_MICROHZ );
}


#else
static void usleephandler(void)
{
	usetup;
	u.u_ringring = 1;
}

void usleep(u_int useconds)
{
	usetup;
	return ;
	register struct itimerval *itp;
	struct itimerval itv, oitv;
	struct sigvec vec, ovec;
	long omask;
	static void usleephandler(void);
    //useconds+=3000;
	itp = &itv;
	if (!useconds)
		return;
	timerclear(&itp->it_interval);
	timerclear(&itp->it_value);
	if (syscall (SYS_setitimer, ITIMER_REAL, itp, &oitv) < 0)
		return;
	itp->it_value.tv_sec = useconds / USPS;
	itp->it_value.tv_usec = useconds % USPS;
	if (timerisset(&oitv.it_value)) {
		if (timercmp(&oitv.it_value, &itp->it_value, >)) {
			oitv.it_value.tv_sec -= itp->it_value.tv_sec;
			oitv.it_value.tv_usec -= itp->it_value.tv_usec;
			if (oitv.it_value.tv_usec < 0) {
				oitv.it_value.tv_usec += USPS;
				oitv.it_value.tv_sec--;
			}
		} else {
			itp->it_value = oitv.it_value;
			oitv.it_value.tv_sec = 0;
			oitv.it_value.tv_usec = 2 * TICK;
		}
	}
	vec.sv_handler = usleephandler;
	vec.sv_mask = vec.sv_onstack = 0;
	syscall (SYS_sigvec, SIGALRM, &vec, &ovec);
	omask = syscall (SYS_sigblock, sigmask(SIGALRM));
	u.u_ringring = 0;
	syscall (SYS_setitimer, ITIMER_REAL, itp, (struct itimerval *)0);
	while (!u.u_ringring)
	{
		syscall (SYS_sigpause, omask & ~sigmask(SIGALRM));
	}
	syscall (SYS_sigvec, SIGALRM, &ovec, (struct sigvec *)0);
	syscall (SYS_sigsetmask, omask);
	syscall (SYS_setitimer, ITIMER_REAL, &oitv, (struct itimerval *)0);

}
#endif
