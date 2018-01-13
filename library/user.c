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

#define _KERNEL
#include "ixemul.h"
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include "multiuser.h"
#include <amitcp/usergroup.h>

/* builtin passwd info */

static const struct passwd builtin_root =
{ "root", "", 0, 0, 0, "", "The Master of All Things", "SYS:", "sh", 0 };

static const struct passwd builtin_nobody =
{ "nobody", "", -2, -2, 0, "", "The Anonymous One", "T:", "sh", 0 };

/* multiuser specific stuff */

static char *
getUserPasswd(char *name)
{
  char *retval = NULL;
  char passdir[MAXPATHLEN];
  BPTR passwddir;

  if (!muBase || !strcmp(name,"nobody")) return("");

  if ((passwddir = muGetPasswdDirLock())) {
    if (NameFromLock(passwddir, passdir, MAXPATHLEN)) {
      if (AddPart(passdir, muPasswd_FileName, MAXPATHLEN)) {
        FILE *fi = (FILE *)syscall(SYS_fopen,passdir,"r");
        char passwdline[1024];
        char username[muUSERNAMESIZE];
        char passwd[muPASSWORDSIZE];
        int found_it = 0;

        username[0] = passwd[0] = '\0';

        if (fi) {
          while (fgets(passwdline,1024,fi)) {
            char *sep = strchr(passwdline,'|');
            char *old;
            strncpy(username,passwdline,sep-passwdline);
            username[sep-passwdline] = '\0';

            old = ++sep;
            sep = strchr(old,'|');
            strncpy(passwd,old,sep - old);
            passwd[sep-old] = '\0';

            if (!strcmp(username,name)) {
              found_it = 1;
              break;
            }
          }
          if (found_it) {
            retval = strdup(passwd);
          }
          fclose(fi);
        }
      }
    }
    UnLock(passwddir);
  }
  if (!retval)
    retval = strdup("*");

  return retval;
}

static struct passwd *
UserInfo2pw (struct muUserInfo *UI)
{
  usetup;
  struct passwd *pw = &u.u_passwd;

  if (UI == NULL)
    return NULL;

  pw->pw_name = UI->UserID;
  pw->pw_dir = UI->HomeDir;
  if (pw->pw_passwd)
    free(pw->pw_passwd);
  pw->pw_passwd = getUserPasswd(pw->pw_name);
  pw->pw_uid = __amiga2unixid(UI->uid);
  pw->pw_gid = __amiga2unixid(UI->gid);
  pw->pw_change = 0;
  pw->pw_class = "";
  pw->pw_gecos = UI->UserName;
  pw->pw_shell = UI->Shell;
  pw->pw_expire = 0;

  return pw;
}

/* builtin passwd file parsing */

#define MAXLINELENGTH	1024
#define UNIX_STRSEP     ":\n"
#define AMIGAOS_STRSEP  "|\n"

static int
start_pw(void)
{
  usetup;

  if (u.u_pwd_fp) {
    syscall(SYS_rewind, u.u_pwd_fp);
    return 1;
  }
  return ((u.u_pwd_fp = (FILE *)syscall(SYS_fopen, _PATH_PASSWD, "r")) ? 1 : 0);
}

static int
pwscan(int search, uid_t uid, char *name)
{
  register char *cp;
  char *bp;
  char *fgets(), *strsep(), *index();
  char *sep;
  usetup;

  if (u.u_pwd_line == NULL)
    u.u_pwd_line = malloc(MAXLINELENGTH);
  if (u.u_pwd_line == NULL)
    {
      errno = ENOMEM;
      return 0;
    }
  for (;;) {
    if (!syscall(SYS_fgets, u.u_pwd_line, MAXLINELENGTH, u.u_pwd_fp))
      return 0;

    bp = u.u_pwd_line;
    /* skip lines that are too big */
    if (!index(u.u_pwd_line, '\n')) {
      int ch;

      while ((ch = getc(u.u_pwd_fp)) != '\n' && ch != EOF) ;
      continue;
    }

    sep = index(u.u_pwd_line,'|') ? AMIGAOS_STRSEP : UNIX_STRSEP;

    u.u_passwd.pw_name = strsep(&bp, sep);
    if (search && name && strcmp(u.u_passwd.pw_name, name))
      continue;

    u.u_passwd.pw_passwd = strsep(&bp, sep);
    if (!(cp = strsep(&bp, sep)))
      continue;

    u.u_passwd.pw_uid = (uid_t)atoi(cp);
    if (search && name == NULL && u.u_passwd.pw_uid != uid)
      continue;

    if (!(cp = strsep(&bp, sep))) continue;

    u.u_passwd.pw_gid = (gid_t)atoi(cp);

    u.u_passwd.pw_gecos = cp = strsep(&bp, sep);
    if (!cp) continue;
    
    u.u_passwd.pw_dir = cp = strsep(&bp, sep);
    if (!cp) continue;
    
    u.u_passwd.pw_shell = cp = strsep(&bp, "\n");
    if (!cp) continue;

    return 1;
  }
  /* NOTREACHED */
}

/* the real stuff */

uid_t
getuid(void)
{
  usetup;

  if (muBase)
    return (__amiga2unixid(muGetTaskOwner(NULL) >> 16));

  return u.u_ruid;
}

uid_t
geteuid(void)
{
  usetup;

  if (muBase)
    return (__amiga2unixid(muGetTaskOwner(NULL) >> 16));

  return u.u_euid;
}

int
setreuid (int ruid, int euid)
{
  usetup;

  if (muBase)
  {
    int retval = 0;

    if (syscall(SYS_geteuid) == 0)
    {
      if (euid != -1)
        {
          struct muUserInfo *UI = u.u_UserInfo;

          UI->uid = __unix2amigaid(euid);
          if ((UI = muGetUserInfo (UI, muKeyType_uid)))
            {

              if (muLogin(muT_Task, (ULONG)FindTask(NULL),
                          muT_UserID, (ULONG)UI->UserID,
                          muT_NoLog, TRUE,
                          TAG_DONE)) {
                u.u_setuid++;
	    }
          }
       }
    }
    else
      {
        errno = EPERM;
        retval = -1;
      }

    return retval;
  }
  else
    {
      if (ruid != -1)
        u.u_ruid = (uid_t)ruid;

      if (euid != -1)
        u.u_euid = (uid_t)euid;

      /* just always succeed... */
      return 0;
    }
}

int
setuid (uid_t uid)
{
  return syscall(SYS_setreuid, (int)uid, (int)uid);
}

int
seteuid (uid_t uid)
{
  return syscall(SYS_setreuid, -1, (int)uid);
}

struct passwd *getpwuid (uid_t uid)
{
  struct passwd *pw;
  usetup;

  if (uid == (uid_t)(-2))
    return (struct passwd *)&builtin_nobody; /* always handle nobody */

  if (muBase)
    {
      /* active multiuser */
      struct muUserInfo *UI = u.u_UserInfo;

      UI->uid = __unix2amigaid(uid);

      UI = muGetUserInfo (UI, muKeyType_uid);

      return UserInfo2pw (UI);      /* handles errors */
    }

  if (u.u_ixnetbase && (pw = (struct passwd *)netcall(NET_getpwuid, (int)uid)))
    return pw;

  if (start_pw())
    {
      int rval;

      rval = pwscan(1, uid, NULL);
      if (!u.u_pwd_stayopen)
        syscall(SYS_endpwent);

      return (rval ? &u.u_passwd : NULL);
    }

  if (uid == 0)
    return (struct passwd *)&builtin_root;

  if (uid == syscall(SYS_getuid) &&
      ((u.u_passwd.pw_name = (char *)syscall(SYS_getlogin))))
    {
      if (!(u.u_passwd.pw_dir = (char *)syscall(SYS_getenv, "HOME"))) {
        u.u_passwd.pw_dir = "SYS:";
      }

      if (!(u.u_passwd.pw_gecos = (char *)syscall(SYS_getenv, "REALNAME"))) {
        u.u_passwd.pw_gecos = "Amiga User";
      }

      u.u_passwd.pw_uid = uid;
      u.u_passwd.pw_gid = syscall(SYS_getgid);

      u.u_passwd.pw_change = u.u_passwd.pw_expire = 0;
      u.u_passwd.pw_class  = "";
      u.u_passwd.pw_passwd = "*";
      u.u_passwd.pw_shell  = "sh";

      return(&u.u_passwd);
  
    }

  return 0;
}

struct passwd *getpwnam (const char *name)
{
  struct passwd *pw;
  usetup;

  if (!strcmp(name,"nobody"))
    return (struct passwd *)&builtin_nobody;

  if (muBase)
    {
      struct muUserInfo *UI = u.u_UserInfo;

      /*
       * some validation checks
       */
    
      if (name == NULL)
        return NULL;
    
      if ((muUSERIDSIZE - 1) < strlen (name))
        return NULL;

      strcpy (UI->UserID, name);
      UI = muGetUserInfo (UI, muKeyType_UserID);

      return UserInfo2pw (UI);      /* handles errors */
    }

  if (u.u_ixnetbase && (pw = (struct passwd *)netcall(NET_getpwnam, name)))
    return pw;

  if (start_pw())
    {
      int rval;

      rval = pwscan(1, 0, (char *)name);
      if (!u.u_pwd_stayopen)
        syscall(SYS_endpwent);

      return (rval ? &u.u_passwd : NULL);
    }

  if (!strcmp(name,"root"))
    return (struct passwd *)&builtin_root;

  if (((u.u_passwd.pw_name = (char *)syscall(SYS_getlogin))) &&
      !strcmp(name,u.u_passwd.pw_name))
    {
      if (!(u.u_passwd.pw_dir = (char *)syscall(SYS_getenv, "HOME"))) {
        u.u_passwd.pw_dir = "SYS:";
      }

      if (!(u.u_passwd.pw_gecos = (char *)syscall(SYS_getenv, "REALNAME"))) {
        u.u_passwd.pw_gecos = "Amiga User";
      }

      u.u_passwd.pw_uid = syscall(SYS_getuid);
      u.u_passwd.pw_gid = syscall(SYS_getgid);

      u.u_passwd.pw_change = u.u_passwd.pw_expire = 0;
      u.u_passwd.pw_class  = "";
      u.u_passwd.pw_passwd = "*";
      u.u_passwd.pw_shell  = "sh";

      return(&u.u_passwd);
  
    }

  return 0;
}

struct passwd *
getpwent(void)
{
  char *name;
  usetup;

  if (muBase)
    {
      struct muUserInfo *UI  = u.u_fileUserInfo;

      UI = muGetUserInfo (UI, u.u_passwdfileopen ? muKeyType_Next : muKeyType_First);
      u.u_passwdfileopen = TRUE;

      return UserInfo2pw (UI);       /* handles errors */
    }

  if (u.u_ixnetbase)
    return (struct passwd *)netcall(NET_getpwent);

  if ((u.u_pwd_fp || start_pw()) && pwscan(0, 0, NULL))
    return &u.u_passwd;

  switch (u.u_nextuid) {
    case 0:
      u.u_nextuid = 1;
      return (struct passwd *)&builtin_root;
    default:
      u.u_nextuid = -2;
     
      if ((name = (char *)syscall(SYS_getlogin)))
        return (struct passwd *)syscall(SYS_getpwnam, name);
    case (uid_t)(-2):
      u.u_nextuid = -1;
      return (struct passwd *)&builtin_nobody;
    case (uid_t)(-1):
      return 0;        
  }
}

int
setpassent(int stayopen)
{
  usetup;

  if (muBase)
    {
      u.u_passwdfileopen = FALSE;
      return 1;
    }

  if (u.u_ixnetbase)
    return netcall(NET_setpassent, stayopen);

  if (start_pw())
    u.u_pwd_stayopen = stayopen;

  u.u_nextuid = 0;
  return 1;
}

int
setpwent(void)
{
  usetup;

  if (muBase)
    {
      u.u_passwdfileopen = FALSE;
      return 1;
    }

  if (u.u_ixnetbase)
    return netcall(NET_setpwent);

  if (start_pw())
    u.u_pwd_stayopen = 0;

  u.u_nextuid = 0;
  return 1 ;
}

void
endpwent(void)
{
  usetup;

  if (muBase)
    {
      u.u_passwdfileopen = FALSE;
      return;
    }

  if (u.u_ixnetbase)
    netcall(NET_endpwent);

  if (u.u_pwd_fp) {
    syscall(SYS_fclose, u.u_pwd_fp);
    u.u_pwd_fp = NULL;
  }

  u.u_nextuid = 0;
}

int
_getlogin (char *s, int size)
{
  char *cp = (char *)syscall(SYS_getlogin);

  if (cp)
    strncpy (s, cp, size);
  else
    *s = '\0';

  return 0;
}

char *
getlogin(void) 
{
  usetup;

  if (u.u_logname_valid)
    {
      strcpy(u.u_logname_buf,u.u_logname);
      return (*u.u_logname_buf ? u.u_logname_buf : 0);
    }
  else
    return 0;
}

int
setlogin (const char *s)
{
  usetup;

  strncpy (u.u_logname, s, MAXLOGNAME);
  u.u_logname[MAXLOGNAME] = '\0';
  u.u_logname_valid = 1;

  return 0;
}
