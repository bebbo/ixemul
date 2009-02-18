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
 *  $Id: ixnet_open.c,v 1.2 2006/11/01 11:24:49 piru Exp $
 *
 *  $Log: ixnet_open.c,v $
 *  Revision 1.2  2006/11/01 11:24:49  piru
 *  2006-11-01  Harry Sintonen <sintonen@iki.fi>
 *  	* Fixed ixemul and ixnet library open to check for AllocSignal()
 *  	  failures.
 *  	* Disabled SetExcept() optimization until GDB issue is sorted out.
 *
 *  Revision 1.1.1.1  2005/03/15 15:57:09  laire
 *  a new beginning
 *
 *  Revision 1.3  2004/08/14 04:09:50  henes
 *  fixed some amitcp assign references
 *
 *  I note ixnet is lacking error checking... I see some AllocSignal are not checked.
 *  Also some other stuff...
 *
 *  Revision 1.2  2000/06/20 22:17:10  emm
 *  First attempt at a native MorphOS ixemul
 *
 *  Revision 1.1.1.1  2000/05/07 19:37:46  emm
 *  Imported sources
 *
 *  Revision 1.1.1.1  2000/04/29 00:44:34  nobody
 *  Initial import
 *
 */

#define _KERNEL
#include "ixnet.h"
#include "kprintf.h"

#include <amitcp/socketbasetags.h>
#include <amitcp/usergroup.h>
#include <exec/memory.h>
#include <string.h>
#include "ixprotos.h"

extern struct ixemul_base *ixemulbase;
#ifdef NATIVE_MORPHOS
int (**_ixbasearray)();
#endif

struct ixnet_base *
ixnet_open (struct ixnet_base *ixbase)
{
    struct ixnet *p;
    struct Task *me;
    struct user *ix_u;
    struct ix_settings *settings; /* don't call this *ix there is a macro called ix!!! */
    int network_type;

    me = SysBase->ThisTask;
    ix_u = getuser(me); /* already initialized by ixemul.library */

    /* need this here instead of ixnet_init.c */
	
    ixemulbase = ix_u->u_ixbase;
#ifdef NATIVE_MORPHOS
    _ixbasearray = ixemulbase->basearray;
#endif

    if (ixnetbase->ixnet_lib.lib_Version != ixemulbase->ix_lib.lib_Version ||
	ixnetbase->ixnet_lib.lib_Revision != ixemulbase->ix_lib.lib_Revision)
      {
	ix_panic(
"ixnet.library has version %ld.%ld while ixemul.library has version %ld.%ld. Both libraries should have the same version, therefore ixnet.library won't be used.", ixnetbase->ixnet_lib.lib_Version, ixnetbase->ixnet_lib.lib_Revision,
		 ixemulbase->ix_lib.lib_Version, ixemulbase->ix_lib.lib_Revision);
	settings = ix_get_settings();
	settings->network_type = IX_NETWORK_NONE;
	ix_set_settings(settings);
	return 0;
      }

    p = ix_u->u_ixnet = (struct ixnet *)AllocMem(sizeof(struct ixnet),MEMF_PUBLIC|MEMF_CLEAR);

    settings = ix_get_settings();
    network_type = settings->network_type;

    if (p) {
      p->u_networkprotocol = IX_NETWORK_NONE;
      switch (network_type) {
	case IX_NETWORK_AUTO:
	case IX_NETWORK_AMITCP:
	    /* We could check for the existance of the AMITCP port here
	       to make sure we're using the AmiTCP bsdsocket.library,
	       and not the bsdsocket.library emulation for AS225.
	       But in that case, the Miami package isn't recognized,
	       because they don't open an AMITCP port. Oh well... */
	    if ((p->u_TCPBase = OpenLibrary ("bsdsocket.library",3))) {
		p->u_sigurg      = AllocSignal (-1);
		p->u_sigio       = AllocSignal (-1);

		if ((BYTE) p->u_sigurg != -1 && (BYTE) p->u_sigio != -1) {
		    struct Process *thisproc = (struct Process *)me;
		    struct CommandLineInterface *cli = BTOCPTR(thisproc->pr_CLI);
		    char *progname = ((!thisproc->pr_CLI) ? me->tc_Node.ln_Name : BTOCPTR(cli->cli_CommandName));
		    struct TagItem list[] = {
			/* { SBTM_SETVAL(SBTC_ERRNOPTR(sizeof(int))), (ULONG)&u.u_errno }, */
			/* { SBTM_SETVAL(SBTC_HERRNOLONGPTR), (ULONG)&h_errno }, */
			{ SBTM_SETVAL(SBTC_LOGTAGPTR), (ULONG)progname },
			{ SBTM_SETVAL(SBTC_SIGIOMASK), NULL },
			{ SBTM_SETVAL(SBTC_SIGURGMASK), NULL },
			{ SBTM_SETVAL(SBTC_BREAKMASK), SIGBREAKF_CTRL_C },
			{ TAG_END }
		    };

		    list[1].ti_Data = (1L << p->u_sigio);
		    list[2].ti_Data = (1L << p->u_sigurg);

		    /* I will assume this always is successful */
		    TCP_SocketBaseTagList(list);

		    /* only use usergroup stuff when AmiTCP is started only */
		    /* I call OpenLibrary() with the full path since
		     * usergroup.library might open yet - some people bypass the
		     * "login" command which loads usergroup.library
		     */
#ifdef NATIVE_MORPHOS
		    p->u_UserGroupBase = OpenLibrary("usergroup.library",1);
#else
		    p->u_UserGroupBase = OpenLibrary("AmiTCP:libs/usergroup.library",1);
#endif

		    if (p->u_UserGroupBase) {
			struct TagItem ug_list[] = {
			    { UGT_INTRMASK, SIGBREAKB_CTRL_C } ,
			    { UGT_ERRNOPTR(sizeof(int)), (ULONG)ix_u->u_errno },
			    { TAG_END }
			};
			ug_SetupContextTagList(progname, ug_list);
			p->u_networkprotocol = IX_NETWORK_AMITCP;
			break;
		    }
		}
		FreeSignal(p->u_sigurg);
		FreeSignal(p->u_sigio);
		CloseLibrary(p->u_TCPBase);
	    }
	    /* don't fall through if not auto-detect */
	    if (network_type != IX_NETWORK_AUTO)
		break;

	/* falls through if something failed above */
	case IX_NETWORK_AS225:
	    p->u_SockBase = OpenLibrary("socket.library",3);

	    if (p->u_SockBase) {
		p->u_sigurg      = AllocSignal (-1);
		p->u_sigio       = AllocSignal (-1);
		if ((BYTE) p->u_sigurg != -1 && (BYTE) p->u_sigio != -1) {
		    p->u_networkprotocol = IX_NETWORK_AS225;
		    break;
		}
		FreeSignal(p->u_sigurg);
		FreeSignal(p->u_sigio);
		CloseLibrary(p->u_SockBase);
	    }
	    break;
	}

	if (p->u_networkprotocol == IX_NETWORK_NONE) {
	    extern struct ixnet_base *ixnetbase;
	    FreeMem(ix_u->u_ixnet,sizeof(struct ixnet));
	    return 0;
	}
	return ixbase;
    }
    return 0;
}


