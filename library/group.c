/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1991, 1992  Markus M. Wild
 *  Portions Copyright (C) 1994 Rafael W. Luebbert
 *  Portions Copyright (C) 1996 Jeff Shepherd
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
 */


/* stubs for group-file access functions */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"
#include <stdlib.h>
#include <grp.h>
#include <stdio.h>
#include "multiuser.h"
#include <amitcp/usergroup.h>
#include "ixprotos.h"

/* builtin group info */

static const char *dummy_members[] = { 0 };

static const struct group builtin_wheel =

{ "wheel", "*", 0, (char **)dummy_members };

static const struct group builtin_nogroup =

{ "nogroup", "*", -2, (char **)dummy_members };

/* multiuser specific stuff */

static struct group *
GroupInfo2grp (struct muGroupInfo *GI, struct muUserInfo *UI)
{
  static char *members[] = { NULL };
  usetup;
  struct group *grp = &u.u_group;

  if (GI)
  {
    grp->gr_name = GI->GroupID;
    grp->gr_passwd = "*";
    grp->gr_gid = __amiga2unixid(GI->gid);
    grp->gr_mem = members;        /* much work has to be done on this */

    return grp;
  }
  return NULL;
}

/* builtin group file parsing */

#define MAXGRP		200
#define MAXLINELENGTH	1024
#define UNIX_STRSEP     ":\n"
#define AMIGAOS_STRSEP  "|\n"

static int
start_gr(void)
{
  usetup;

  if (u.u_grp_fp) {
    syscall(SYS_rewind, u.u_grp_fp);
    return 1;
  }
  return ((u.u_grp_fp = (FILE *)syscall(SYS_fopen, _PATH_GROUP, "r")) ? 1 : 0);
}

static int
grscan(int search, gid_t gid, char *name)
{
  register char *cp, **m;
  char *bp;
  char *fgets(), *strsep(), *index();
  char *sep;
  usetup;

  if (u.u_grp_line == NULL)
    u.u_grp_line = malloc(MAXLINELENGTH);
  if (u.u_members == NULL)
    u.u_members = malloc(MAXGRP * sizeof(char *));
  if (u.u_grp_line == NULL || u.u_members == NULL)
    {
      errno = ENOMEM;
      return 0;
    }
  for (;;) {
    if (!syscall(SYS_fgets, u.u_grp_line, MAXLINELENGTH, u.u_grp_fp))
      return 0;

    bp = u.u_grp_line;
    /* skip lines that are too big */
    if (!index(u.u_grp_line, '\n')) {
      int ch;

      while ((ch = getc(u.u_grp_fp)) != '\n' && ch != EOF) ;
      continue;
    }

    sep = index(u.u_grp_line,'|') ? AMIGAOS_STRSEP : UNIX_STRSEP;

    u.u_group.gr_name = strsep(&bp, sep);
    if (search && name && strcmp(u.u_group.gr_name, name))
      continue;

    u.u_group.gr_passwd = strsep(&bp, sep);
    if (!(cp = strsep(&bp, sep)))
      continue;

    u.u_group.gr_gid = (gid_t)atoi(cp);
    if (search && name == NULL && u.u_group.gr_gid != gid)
      continue;

    for (m = u.u_group.gr_mem = u.u_members;; ++m) {
      if (m == &u.u_members[MAXGRP - 1]) {
        *m = NULL;
	break;
      }
      if ((*m = strsep(&bp, ", \n")) == NULL)
        break;
    }
    return 1;
  }
  /* NOTREACHED */
}

/* the real stuff */

gid_t
getgid (void)
{
  usetup;

  if (muBase)
    return (__amiga2unixid(muGetTaskOwner(NULL) & muMASK_GID));

  return u.u_rgid;
}

gid_t
getegid (void)
{
  usetup;

  if (muBase)
    return (__amiga2unixid(muGetTaskOwner(NULL) & muMASK_GID));

  return u.u_egid;
}

int
setregid (int rgid, int egid)
{
  usetup;

  if (muBase)
    {
      errno = ENOSYS;
      return -1;
    }

  if (rgid != -1)
    u.u_rgid = (gid_t)rgid;

  if (egid != -1)
    {
      u.u_egid = (gid_t)egid;
      u.u_grouplist[0] = egid;
    }

  /* just always succeed... */
  return 0;
}

int
setgid (gid_t gid)
{
  return syscall(SYS_setregid, (int)gid, (int)gid);
}

int
setegid (gid_t gid)
{
  return syscall(SYS_setregid, -1, (int)gid);
}

struct group *getgrgid (gid_t gid)
{
  struct group *gr;
  usetup;

  if (gid == (gid_t)(-2))
    return (struct group *)&builtin_nogroup; /* always handle nogroup */

  if (muBase)
    {
      /* active multiuser */
      struct muGroupInfo *GI = u.u_GroupInfo;
      struct muUserInfo *UI = u.u_UserInfo;

      GI->gid = __unix2amigaid(gid);
      GI = muGetGroupInfo (GI, muKeyType_gid);

      return GroupInfo2grp (GI, UI);       /* handles errors */
    }

  if (u.u_ixnetbase && (gr = (struct group *)netcall(NET_getgrgid, (int)gid)))
    return gr;

  if (start_gr())
    {
      int rval;

      rval = grscan(1, gid, NULL);
      if (!u.u_grp_stayopen)
        syscall(SYS_endgrent);

      return (rval ? &u.u_group : NULL);
    }

  if (gid == 0)
    return (struct group *)&builtin_wheel;

  if (gid == syscall(SYS_getgid) &&
      ((u.u_group.gr_name = (char *)syscall(SYS_getenv, "GROUP"))))
    {
      u.u_group.gr_gid = gid;
      u.u_group.gr_passwd = "*";
      u.u_group.gr_mem  = (char **)dummy_members;

      return(&u.u_group);
  
    }

  return 0;
}

struct group *getgrnam (const char *name)
{
  struct group *gr;
  usetup;

  if (!strcmp(name,"nogroup"))
    return (struct group *)&builtin_nogroup; /* always handle nogroup */

  if (muBase)
    {
      /* active multiuser */
      struct muGroupInfo *GI = u.u_GroupInfo;
      struct muUserInfo *UI = u.u_UserInfo;

      /*
       * some validation checks
       */

      if (name == NULL)
        return NULL;

      if ((muGROUPIDSIZE - 1) < strlen (name))
        return NULL;

      strcpy (GI->GroupID, name);
      GI = muGetGroupInfo (GI, muKeyType_GroupID);

      return GroupInfo2grp (GI, UI);       /* handles errors */
    }

  if (u.u_ixnetbase && (gr = (struct group *)netcall(NET_getgrnam, name)))
    return gr;

  if (start_gr())
    {
      int rval;

      rval = grscan(1, 0, (char *)name);
      if (!u.u_grp_stayopen)
        syscall(SYS_endgrent);

      return (rval ? &u.u_group : NULL);
    }

  if (!strcmp(name,"wheel"))
    return (struct group *)&builtin_wheel;

  if (((u.u_group.gr_name = (char *)syscall(SYS_getenv, "GROUP"))) &&
      !strcmp(name,u.u_group.gr_name))
    {
      u.u_group.gr_gid = syscall(SYS_getgid);
      u.u_group.gr_passwd = "*";
      u.u_group.gr_mem  = (char **)dummy_members;

      return(&u.u_group);
  
    }

  return 0;
}

struct group *
getgrent(void)
{
  char *name;
  usetup;

  if (muBase)
    {
      /* active multiuser */
      struct muGroupInfo *GI = u.u_fileGroupInfo;
      struct muUserInfo *UI  = u.u_UserInfo;

      GI = muGetGroupInfo (GI, u.u_groupfileopen ? muKeyType_Next : muKeyType_First);
      u.u_groupfileopen = TRUE;

      return GroupInfo2grp (GI, UI);       /* handles errors */
    }

  if (u.u_ixnetbase)
    return (struct group *)netcall(NET_getgrent);

  if ((u.u_grp_fp || start_gr()) && grscan(0, 0, NULL))
    return &u.u_group;

  switch (u.u_nextgid) {
    case 0:
      u.u_nextgid = 1;
      return (struct group *)&builtin_wheel;
    default:
      u.u_nextgid = -2;
     
      if ((name = (char *)syscall(SYS_getenv, "GROUP")))
        return (struct group *)syscall(SYS_getgrnam, name);
    case (gid_t)(-2):
      u.u_nextgid = -1;
      return (struct group *)&builtin_nogroup;
    case (gid_t)(-1):
      return 0;        
  }
}

int
setgroupent(int stayopen)
{
  usetup;

  if (muBase)
    {
      u.u_groupfileopen = FALSE;
      return 1;
    }

  if (u.u_ixnetbase)
    return netcall(NET_setgroupent, stayopen);

  if (start_gr())
    u.u_grp_stayopen = stayopen;

  u.u_nextgid = 0;
  return 1;
}

int
setgrent(void)
{
  usetup;

  if (muBase)
    {
      u.u_groupfileopen = FALSE;
      return 1;
    }

  if (u.u_ixnetbase)
    return netcall(NET_setgrent);

  if (start_gr())
    u.u_grp_stayopen = 0;

  u.u_nextgid = 0;
  return 1;
}

void
endgrent(void)
{
  usetup;

  if (muBase)
    {
      u.u_groupfileopen = FALSE;
      return;
    }

  if (u.u_ixnetbase)
    netcall(NET_endgrent);

  if (u.u_grp_fp) {
    syscall(SYS_fclose, u.u_grp_fp);
    u.u_grp_fp = NULL;
  }

  u.u_nextgid = 0;
}

int
setgroups (int ngroups, const int *gidset)
{
  usetup;

  if (muBase)
    return 0;

  /* parameter check */
  if (gidset == NULL || ngroups < 0)
    {
      errno = EFAULT;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));

      return -1;
    }

  if (ngroups == 0 || ngroups > NGROUPS_MAX)
    {
      errno = EINVAL;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));

      return -1;
    }

  u.u_ngroups = ngroups;

  for (ngroups = 0; ngroups < u.u_ngroups; ngroups++)
    {
      u.u_grouplist[ngroups] = gidset[ngroups];
    }

  u.u_egid = (gid_t)(u.u_grouplist[0]);

  return 0;
}

int
initgroups (const char *name, int basegid)
{
  int ngroups = 2;
  int gidset[NGROUPS_MAX];
  struct group *gr;
  char **grm;
  usetup;

  /* parameter check */
  if (name == NULL || basegid == -1)
    {
      errno = EFAULT;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));

      return -1;
    }

  gidset[0] = gidset[1] = (gid_t)basegid;

  syscall(SYS_setgrent);

  while ((gr = (struct group *)syscall(SYS_getgrent)))
    {
      if (gr->gr_gid != (gid_t)basegid)
        for (grm = gr->gr_mem; grm && *grm; grm++)
          {
            if (!strcmp(name,*grm))
              {
                gidset[ngroups++] = (int)(gr->gr_gid);
                break;
              }
          }
      if (ngroups == NGROUPS_MAX) break;
    }

  syscall(SYS_endgrent);

  return syscall(SYS_setgroups, ngroups, gidset);
}

int getgroups(int gidsetlen, int *gidset)
{
  usetup;

  /* parameter check */
  if (gidset == NULL || gidsetlen < 0)
    {
      errno = EFAULT;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));

      return -1;
    }

  if (gidsetlen < 1)
    {
      errno = EINVAL;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));

      return -1;
    }

  if (muBase)
    {
      /* muFS detected */
      UWORD *Grps;
      int i;

      /* get the information */
      struct muExtOwner *me = muGetTaskExtOwner (NULL);
      if (me == 0)
      {
        *gidset = -2;
        return 1;             /* nobody */
      }

      /* store primary group */
      gidset[0] = (int)__amiga2unixid(me->gid);
      gidset++;
      gidsetlen--;

      /* ensure enough place */
      if (gidsetlen < me->NumSecGroups)
      {
        errno = EINVAL;
        KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));

        return -1;
      }

      /* slow, but have to copy from UWORD[] --> int[] */
      Grps = muSecGroups (me);
      for (i = me->NumSecGroups; i >= 0; i--)
      gidset[i] = (int)__amiga2unixid(Grps[i]);

      /* clean up */
      i = me->NumSecGroups + 1;
      muFreeExtOwner (me);

      return i;
    }

  if (gidsetlen < u.u_ngroups)
    {
      errno = EINVAL;
      KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));

      return -1;
    }

  for (gidsetlen = 0; gidsetlen < u.u_ngroups; gidsetlen++)
     {
       gidset[gidsetlen] = u.u_grouplist[gidsetlen];
     }

  return u.u_ngroups;
}
