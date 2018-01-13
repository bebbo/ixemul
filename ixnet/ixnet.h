/*
 *  This file is part of ixnet.library for the Amiga.
 *  Copyright (C) 1996 Jeff Shepherd
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
 */

#ifndef _IXNET_H_
#define _IXNET_H_

#ifdef START

/* definitions for the assembler startup file */

/* when I've REALLY lots of free time, I'll rewrite header files, but now... */

/* amazingly works, contains only defines ;-)) */
#include <exec/alerts.h>

#define _LVOOpenLibrary 	-0x228
#define _LVOCloseLibrary	-0x19e
#define _LVOAlert		-0x6c
#define _LVOFreeMem		-0xd2
#define _LVORemove		-0xfc

#define RTC_MATCHWORD	0x4afc
#define RTF_AUTOINIT	(1<<7)

#define LIBF_CHANGED	(1<<1)
#define LIBF_SUMUSED	(1<<2)
/* seems there is an assembler bug in expression evaluation here.. */
#define LIBF_CHANGED_SUMUSED 0x6
#define LIBF_DELEXP	(1<<3)
#define LIBB_DELEXP	3

#define LN_TYPE 	8
#define LN_NAME 	10
#define NT_LIBRARY	9
#define MP_FLAGS	14
#define PA_IGNORE	2

#define INITBYTE(field,val)     .word 0xe000; .word (field); .byte (val); .byte 0
#define INITWORD(field,val)     .word 0xd000; .word (field); .word (val)
#define INITLONG(field,val)     .word 0xc000; .word (field); .long (val)

/*
 * our library base..
 */

/* struct library */
#define IXNETBASE_NODE	   0
#define IXNETBASE_FLAGS    14
#define IXNETBASE_NEGSIZE  16
#define IXNETBASE_POSSIZE  18
#define IXNETBASE_VERSION  20
#define IXNETBASE_REVISION 22
#define IXNETBASE_IDSTRING 24
#define IXNETBASE_SUM	   28
#define IXNETBASE_OPENCNT  32
#define IXNETBASE_LIBRARY  34  /* size of library */

/* custom part */
#define IXNETBASE_MYFLAGS	   (IXNETBASE_LIBRARY + 0)
#define IXNETBASE_SEGLIST	   (IXNETBASE_MYFLAGS + 2)
#define IXNETBASE_C_PRIVATE	   (IXNETBASE_SEGLIST + 4)


#else  /* C-part */

#include "ixemul.h"

#include <errno.h>

struct ixnet_base {
    struct Library     ixnet_lib;
#ifdef _KERNEL
    unsigned char      ix_myflags;     /* used by start.s */
    unsigned char      ix_pad;
    BPTR	       ix_seg_list;    /* used by start.s */

    LONG	       dummy;
#endif
};

/*
 * ixnet.library stuff -  don't peek into this structure yourself
 */
struct ixnet {
	void			*u_InetBase;
	void			*u_SockBase;
	void			*u_TCPBase;
	void			*u_UserGroupBase;
	int			u_sigurg;
	int			u_sigio;
	int			u_networkprotocol;
	int			sock_id;     /* used for AS225 to reply to inetd message */
	short			u_daemon;    /* 1 if started from inetd */
	char			*u_progname; /* useful pointer */
};

extern struct ixnet_base *ixnetbase;

#include <stdio.h>

#define h_errno (* u.u_h_errno)

/* need to do this so I don't get unresolved symbols */
/* #ifdef put in by dz for cross-compiling */
#ifdef __amigaos__
#undef stdin
#undef stdout
#undef stderr
#define stdin	(u.u_sF[0])
#define stdout	(u.u_sF[1])
#define stderr	(u.u_sF[2])
#endif

#include "usergroup.h"
#include "amitcp.h"
#include "as225.h"

/* I need these prototypes but <sys/file.h> includes <fcntl.h>
 * with _KERNEL defined hence not including these
 */
int	open __P((const char *, int, ...));
int	fcntl __P((int, int, ...));

#endif /* START */
#endif /* IXNET_H */
