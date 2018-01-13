/*	$NetBSD: sysv_ipc.c,v 1.10 1995/06/03 05:53:28 mycroft Exp $	*/

/*
 * Copyright (c) 1995 Charles M. Hannum.  All rights reserved.
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
 *      This product includes software developed by Charles M. Hannum.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#define _KERNEL
#include "ixemul.h"

#include <sys/vnode.h>
#include <sys/ipc.h>
#include <sys/ucred.h>

#define IPC_PERM_ALWAYS_SUCCEED

#ifndef IPC_PERM_ALWAYS_SUCCEED
/*
 * Do the usual access checking.
 * file_mode, uid and gid are from the vnode in question,
 * while acc_mode and cred are from the VOP_ACCESS parameter list
 */
int
vaccess(file_mode, uid, gid, acc_mode, cred)
	mode_t file_mode;
	uid_t uid;
	gid_t gid;
	mode_t acc_mode;
	struct ucred *cred;
{
	mode_t mask;
	
	/* User id 0 always gets access. */
	if (cred->cr_uid == 0)
		return 0;
	
	mask = 0;
	
	/* Otherwise, check the owner. */
	if (cred->cr_uid == uid) {
		if (acc_mode & VEXEC)
			mask |= S_IXUSR;
		if (acc_mode & VREAD)
			mask |= S_IRUSR;
		if (acc_mode & VWRITE)
			mask |= S_IWUSR;
		return (file_mode & mask) == mask ? 0 : EACCES;
	}
	
	/* Otherwise, check the groups. */
	if (cred->cr_gid == gid/* || groupmember(gid, cred)*/) {
		if (acc_mode & VEXEC)
			mask |= S_IXGRP;
		if (acc_mode & VREAD)
			mask |= S_IRGRP;
		if (acc_mode & VWRITE)
			mask |= S_IWGRP;
		return (file_mode & mask) == mask ? 0 : EACCES;
	}
	
	/* Otherwise, check everyone else. */
	if (acc_mode & VEXEC)
		mask |= S_IXOTH;
	if (acc_mode & VREAD)
		mask |= S_IROTH;
	if (acc_mode & VWRITE)
		mask |= S_IWOTH;
	return (file_mode & mask) == mask ? 0 : EACCES;
}
#endif

/*
 * Check for ipc permission
 */

int
ipcperm(cred, perm, mode)
	struct ucred *cred;
	struct ipc_perm *perm;
	int mode;
{
#ifdef IPC_PERM_ALWAYS_SUCCEED
	return 0;
// Always succeed for now :-(
#else
	if (mode == IPC_M) {
		if (cred->cr_uid == 0 ||
		    cred->cr_uid == perm->uid ||
		    cred->cr_uid == perm->cuid)
			return (0);
		return (EPERM);
	}

	if (vaccess(perm->mode, perm->uid, perm->gid, mode, cred) == 0 ||
	    vaccess(perm->mode, perm->cuid, perm->cgid, mode, cred) == 0)
		return (0);
	return (EACCES);
#endif
}
