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
 *  _main.c,v 1.1.1.1 1994/04/04 04:30:43 amiga Exp
 *
 *  _main.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:43  amiga
 * Initial CVS check in.
 *
 *  Revision 1.3  1992/08/09  20:41:54  amiga
 *  change to use 2.x header files by default
 *  add option to ignore global environment (ix.ix_ignore_global_env).
 *
 *  Revision 1.2  1992/07/04  19:10:06  mwild
 *  add call to ix_install_sigwinch().
 *
 * Revision 1.1  1992/05/17  21:01:29  mwild
 * Initial revision
 *
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#include <dos/var.h>
#include <workbench/startup.h>
#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


int __ix_wb_parse();

char **get_global_environment(void)
{
  DIR *dp;
  struct dirent *de;
  int num_env;
  char **cp, **env;

  /* now go for global variables */
  
  dp = (DIR *)syscall (SYS_opendir, "ENV:");

  if (dp == NULL)
    /* `panic!', no ENV: logical ! */
    return NULL;

  /* first count how many entries we have */
  for (num_env = 0; syscall(SYS_readdir, dp); num_env++) ;
      
  /* skip . and .. */
  syscall (SYS_rewinddir, dp);
  if ((de = (struct dirent *) syscall (SYS_readdir, dp)) 
      && de->d_namlen == 1 && de->d_name[0] == '.' && --num_env &&
      (de = (struct dirent *) syscall (SYS_readdir, dp)) 
      && de->d_namlen == 2 && de->d_name[0] == '.' && de->d_name[1] == '.')
    {
      num_env --;
      de = (struct dirent *) syscall (SYS_readdir, dp);
    }

  if ((cp = (char **)kmalloc((num_env + 1) * 4)))
    env = cp;
  else
    /* out of memory !!! */
    return NULL;
	  
  for (; de; de = (struct dirent *) syscall (SYS_readdir, dp))
    {
      struct stat stb;
      int len = 0;
      char envfile[MAXPATHLEN];

      /* Don't include variables with funny names, and don't include
         multiline variables either, they totally confuse ksh.. */
      if (strchr(de->d_name, '.'))
        continue;

      sprintf (envfile, "ENV:%s", de->d_name);

      if (syscall(SYS_stat, envfile, &stb) == 0)
        {
          len = stb.st_size;
          /* skip directories... shudder */
          if (!S_ISREG (stb.st_mode))
            continue;
        }

      /* NAME=CONTENTS\0 */
      *cp = (char *)kmalloc(de->d_namlen + 1 + len + 1);
      if (*cp)
        {
          int written = sprintf (*cp, "%s=", de->d_name);
          int fd;

    	  if (len)
            {
    	      fd = syscall(SYS_open, envfile, 0);
    	      if (fd >= 0)
    	        {
    	          written += syscall(SYS_read, fd, *cp + written, len);
    	          (*cp)[written] = 0;
    	          if ((*cp)[--written] == '\n')
    		    (*cp)[written] = 0;
    	          syscall(SYS_close, fd);
    	        }
    	    
    	      /* now filter out those multiliners (that is, 
    	         variables containing \n, you can have variables
    	         as long as want, but don't use \n... */
    	      if (strchr(*cp, '\n'))
    	        {
    	          kfree(*cp);
    	          continue;
    	        }
    	    }
        }
      else
        break;

      cp++;
    }

  *cp = 0;
  syscall (SYS_closedir, dp);
  return env;
}

char **__ix_get_environ (void)
{
  static char *endmarker = 0;
  char **env = &endmarker;
  int num_env, num_local = 0;
  char **cp, **tmp;

  /* 2.0 local environment overrides 1.3 global environment */
  struct Process *me = (struct Process *)FindTask (0);
  struct LocalVar *lv, *nlv;

  /* count total number of local variables (skip aliases) */
  for (num_local = 0, lv = (void *)me->pr_LocalVars.mlh_Head;
       (nlv = (void *)lv->lv_Node.ln_Succ);
       lv = nlv)
    if (lv->lv_Node.ln_Type == LV_VAR)
      num_local++;

  if ((cp = (char **) syscall (SYS_malloc, (num_local + 1) * 4)))
    {
      env = cp;
      for (lv = (void *)me->pr_LocalVars.mlh_Head;
	   (nlv = (void *)lv->lv_Node.ln_Succ);
	   lv = nlv)
	{
	  /* I'm not interested in aliases, really not ;-)) */
	  if (lv->lv_Node.ln_Type != LV_VAR)
	    continue;
	    
	  /* NAME=CONTENTS\0 */
	  *cp = (char *)syscall(SYS_malloc, strlen(lv->lv_Node.ln_Name) 
						   + 1 + lv->lv_Len + 1);
	  if (*cp)
	    sprintf (*cp, "%s=%*.*s", lv->lv_Node.ln_Name, 
	      (int)lv->lv_Len, (int)lv->lv_Len, lv->lv_Value);
	  else
	    break;
	  cp++;
	}
	*cp = NULL;
    }
  else
    return env;

  if (ix.ix_flags & ix_ignore_global_env)
    return env;

  if (ix.ix_global_environment == NULL)
    ix.ix_global_environment = get_global_environment();
  else if (ix.ix_env_has_changed)
    {
      ix.ix_env_has_changed = 0;
      /* free the old environment */
      for (tmp = ix.ix_global_environment; *tmp; kfree(*tmp++)) ;
      kfree(ix.ix_global_environment);

      ix.ix_global_environment = get_global_environment();
    }

  if (ix.ix_global_environment == NULL)
    return env;

  for (num_env = 0, tmp = ix.ix_global_environment; *tmp; tmp++, num_env++);

  tmp = (char **)syscall (SYS_realloc, env, (num_local + num_env + 1) * 4);
  if (tmp == NULL)
    return env;
  env = tmp;
  cp = &env[num_local];

  for (tmp = ix.ix_global_environment; *tmp; tmp++)
    {
      char *p = strchr(*tmp, '='), *f;
      int offset;
      
      *p = 0;
      f = _findenv(env, *tmp, &offset);
      *p = '=';
      if (f)
        continue;
      *cp++ = (char *)syscall(SYS_strdup, *tmp);
      *cp = NULL;
    }
  return env;
}

/* init the uid/gid handling */

void
__ix_init_ids(void)
{
  char *var;
  struct passwd *pw;
  usetup;

  if (u.u_logname_valid) return; /* we inherited all the stuff */

  if (!(var = (char *)syscall(SYS_getenv, "LOGNAME")) &&
      !(var = (char *)syscall(SYS_getenv, "USER")))
    {
        var = "nobody";
    }

  strncpy(u.u_logname,var,MAXLOGNAME);
  u.u_logname[MAXLOGNAME] = '\0';

  if (muBase) return;

  pw = (struct passwd *)syscall(SYS_getpwnam, u.u_logname);

  if (pw)
    {
      u.u_ruid = u.u_euid = pw->pw_uid;
      u.u_rgid = u.u_egid = pw->pw_gid;
    }
  else
    {
      u.u_ruid = u.u_euid = (uid_t)(-2);
      u.u_rgid = u.u_egid = (uid_t)(-2);
    }

  u.u_logname_valid = 1;

  if (syscall(SYS_initgroups, u.u_logname, (int)u.u_rgid))
    {
      u.u_grouplist[1] = (int)u.u_rgid;
      u.u_ngroups = 2;
    }

  if ((var = (char *)syscall(SYS_getenv, "UID")))
    u.u_ruid = u.u_euid = (uid_t)atoi(var);

  if ((var = (char *)syscall(SYS_getenv, "GID")))
    u.u_rgid = u.u_egid = (gid_t)atoi(var);

  if ((var = (char *)syscall(SYS_getenv, "EUID")))
    u.u_euid = (uid_t)atoi(var);

  if ((var = (char *)syscall(SYS_getenv, "EGID")))
    u.u_egid = (gid_t)atoi(var);

  u.u_grouplist[0] = (int)u.u_egid;
}

/* smells abit kludgy I know.. but can live with quite few variables in
 * the user area 
 */

int
_main (union { char *_aline; struct WBStartup *_wb_msg; } a1,
       union { int   _alen;  char *_def_window;         } a2,
       int (*main)(int, char **, char **))
#define aline           a1._aline
#define alen            a2._alen
#define wb_msg          a1._wb_msg
#define def_window      a2._def_window
{
  struct Process        *me = (struct Process *)FindTask(0);
  struct user           *u_ptr = getuser(me);
  char                  **argv, **env;
  int                   argc;
  int			exitcode;

  KPRINTF (("entered __main()\n"));
  if (! me->pr_CLI)
    {
      /* Workbench programs expect to have their CD where the executed
       * program lives. */
      if (wb_msg->sm_ArgList)
	{
	  CurrentDir (wb_msg->sm_ArgList->wa_Lock);
	  __ix_wb_parse(me, wb_msg, def_window);
	}
      /* argc==0: this means, that argv is really a WBenchMsg, not a vector */
      argc = 0;
      argv = (char **) wb_msg;
    }
  else
    {
      /* if we were started from the CLI, alen & aline are valid and
       * should now be split into arguments. This is done by the
       * function "__ix_cli_parse()", which does wildcard expansion if not
       * disabled (see cli_parse.c). */
      __ix_cli_parse (me, alen, aline, &argc, &argv);
      if (is_ixconfig(argv[0]))
        return 10;
    }

  env = __ix_get_environ ();

  /* this is not really the right thing to do, the user should call
     ix_get_vars2 () to initialize environ to the address of the variable
     in the calling program. However, this setting guarantees that 
     the user area entry is valid for getenv() calls. */
  u.u_environ = &env;

  __ix_install_sigwinch ();
  
  /* init the uid/gid handling NP */ 

  __ix_init_ids();

  /* The finishing touch :-) Not really necessary, though. */
  errno = 0;

  exitcode = main (argc, argv, env);

  syscall(SYS_exit,exitcode);
  return 0;
}
