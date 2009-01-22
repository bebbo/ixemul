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
 *  $Id: __load_seg.c,v 1.2 2007/03/19 17:40:09 piru Exp $
 */

/* This module implements:
 * - basename(fullpath) -> filename
 * - loadseg() functions
 *   __load_seg(name, args):
 *     magic load_seg, with script-sniffing, stashes clean-up information
 *     in the `my_seg structure (see ixemul.h)
 *   __free_seg(seg):
 *     `unload' a segment obtained through __load_seg
 */
#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"

#include <ctype.h>
#include <string.h>

/* 2.0 support */
#include <utility/tagitem.h>
#include <dos/dostags.h>

extern void *kmalloc (size_t size);
extern char *ix_to_ados(char *buf, const char *name);
extern char *check_root(char *);

static struct my_seg *load_seg_or_script (BPTR lock, const char *name, const char *adosname, char **args, const char *progname);

char *basename(char *tmp)
{
  char *cp = rindex (tmp, '/');

  if (cp)
    return cp + 1;
  if ((cp = index (tmp, ':')))
    return cp + 1;
  return tmp;
}

/* dirlock(name) -> lock on that filename's directory */
static BPTR
dirlock (const char *tmp)
{
  BPTR dir;
  char *cp;
  char o;

    /* explicit lock for a complex pathname */
  cp = rindex (tmp, '/');
  if (cp)
    {
      *cp = 0;
      dir = Lock(tmp, ACCESS_READ);
      *cp = '/';
    }
  else {
    cp = rindex (tmp, ':');
    if (cp)
      {
	o = cp[1];
	cp[1] = 0;
	dir = Lock(tmp, ACCESS_READ);
	cp[1] = o;
      }
    else {  /* duplicate the current directory lock for a simple pathname */
      BPTR orig_dir = CurrentDir(NULL);

      dir = DupLock(orig_dir);
      CurrentDir(orig_dir);
    }
  }
  return dir;
}

/* priv_seg = check_and_load_resident (char *filename):
 *   `loadseg' a program on the resident list, setting up
 *   the private segment information for easy unloading
 */
static struct my_seg *
check_and_load_resident (const char *filename)
{
  struct my_seg *res = 0;
  struct Segment *seg;
  const char *base;

  /* big problem: Commo only stores the bare names in the resident
     list. So we have to truncate to the filename part, and so lose
     the ability to explicitly load the disk version even if a
     resident version is installed */

  base = basename((char *) filename);
  Forbid();
  seg = FindSegment(base, 0, 0);
  if (seg)
    {
      /* strange they didn't provide a function for this... */
      if (seg->seg_UC >= 0)
	seg->seg_UC++;
    }
  Permit();

  if (seg) /* ok, we have the seg */
    {
      KPRINTF(("__loadseg: \"%s\" is resident.\n", filename));
      if ((res = (struct my_seg *)kmalloc(sizeof(*res) + strlen(filename) + 1)))
	{
	  res->segment = seg->seg_Seg;
	  res->type    = RESSEG;
	  res->priv    = (u_int)seg;
	  res->programdir = NULL;         /* no possible programdir as Commo didn't provide for it */
	  res->name    = (char *)&res[1];
	  strcpy(res->name, filename);
	}
      else
	{     /* oops, we can't deal with it */
	  Forbid();
	  if (seg->seg_UC > 0)
	    seg->seg_UC--;
	  Permit();
	}
    }
  return res;
}


/* really loadseg from disk */
static struct my_seg *
check_and_load_seg (const char *filename)
{
  struct my_seg *res = 0;
  BPTR seg;
  int err;
  usetup;

  /* Note that LoadSeg returns NULL and sets IoErr() to 0 if the file it
     is trying to load is less than 4 bytes long. */
  seg = LoadSeg (filename);
  if (seg)
    {
      if ((res = kmalloc (sizeof (*res) + strlen(filename) + 1)))
	{
	  res->segment = seg;
	  res->type    = LOADSEG;
	  res->priv    = seg;
	  res->programdir = dirlock(filename);
	  res->name    = (char *)&res[1];
	  strcpy(res->name, filename);
	}
      else
	{
	  UnLoadSeg (seg);
	  errno = ENOMEM;
	  return NULL;
	}
    }
  err = IoErr();
  errno = (err ? __ioerr_to_errno(err) : ENOENT);
  if (errno == ENOTDIR || errno == EFTYPE)
    errno = ENOENT;

  return res;
}


void
__free_seg (BPTR *seg)
{
  struct my_seg *ms;

  ms = (struct my_seg *) seg;
  switch (ms->type)
    {
      case RESSEG:
	{
	  struct Segment *s = (struct Segment *) ms->priv;

	  Forbid ();
	  if (s->seg_UC > 0)
	    s->seg_UC--;
	  Permit ();
	}
	break;

      case LOADSEG:
	UnLoadSeg (ms->priv);
	break;

      default:   /* unknown segment type -> this is not a struct my_seg ! */
	ix_panic("Corrupted segment");
	break;
    }

  /* we could unlock the programdir before, but this way, we gain the `dynamic
   * type check' that this is truely a struct my_seg.
   * Also note that __unlock() can deal with NULL locks */
  UnLock (ms->programdir);

  kfree (ms);
}


/*
 * This function does what LoadSeg() does, and a little bit more ;-)
 * Besides walking the PATH of the user, we try to do interpreter expansion as
 * well. But, well, we do it a little bit different then a usual Amiga-shell.
 * We check the magic cookies `#!' and `;!', and if found, run the interpreter
 * specified on this first line of text. This does *not* depend on any script
 * bit set!
 */

/*
 * IMPORTANT: only call this function with all signals masked!!!
 */

/*
 * name:        the name of the command to load. Can be relative to installed PATH
 * args:        if set, a string to the first part of an expanded command is stored
 */

BPTR *
__load_seg (char *name, char **args)
{
  BPTR lock;
  struct my_seg *seg;
  usetup;
  char *buf = alloca(strlen(name) + 4);
  char *adosname;

  adosname = ix_to_ados(buf, name);

  /* perhaps the name is vanilla enough, so that even LoadSeg() groks it? */
  if (args) *args = 0;

  seg = check_and_load_resident (adosname);

  if (!seg)
    {
      /*
       * If the currentdir is / and the commandname is relative,
       * try path right away without bothering with currentdir. - Piru
       */
      if (u.u_is_root && !strpbrk (name, ":/"))
        goto trypath;

      seg = check_and_load_seg (adosname);
    }

  if (!seg)
    {
      LONG err = IoErr();
      if (err == ERROR_OBJECT_NOT_FOUND)
        {
	  char *adosname2 = check_root(adosname);
	  if (adosname2 && *adosname2)
	    {
	      adosname = adosname2;
	      seg = check_and_load_seg(adosname);
	      if (seg)
		err = 0;
	    }
	  SetIoErr(err);
	}
    }

  if (seg)
    return (BPTR *)&seg->segment;

  if (errno != ENOENT)
    return 0;

  /* try to lock the file */
  lock = Lock(adosname, ACCESS_READ);
  if (lock)
    {
      int err;
      BPTR parent = ParentDir(lock);
      struct FileInfoBlock *fib;

      fib = alloca (sizeof(*fib) + 2);
      fib = LONG_ALIGN (fib);
      if (Examine (lock, fib))
	seg = load_seg_or_script (parent, fib->fib_FileName, fib->fib_FileName, args, name);
      else
	{
	  char *base = basename(name);
	  seg = load_seg_or_script (parent, base, base, args, name);
	}
      err = errno;

      UnLock (parent);
      UnLock (lock);

      errno = err;
      if (!seg && errno != ENOENT)
	return 0;
    }

  /* now we may have a valid segment */
  if (seg)
    return (BPTR *)&seg->segment;

  /* if the command was specified with some kind of path, for example with a
   * device or a directory in it, we don't run it thru the PATH expander
   */
  if (strpbrk (name, ":/"))
    {
      errno = ENOENT;
      return 0;
    }

trypath:
  /* so the command is not directly addressable, but perhaps it's in our PATH? */
  {
    struct Process *me = (struct Process *)SysBase->ThisTask;
    struct CommandLineInterface *cli;

    /* but we need a valid CLI then */
    if ((cli = BTOCPTR (me->pr_CLI)))
      {
	struct path_element {
	  BPTR  next;
	  BPTR  lock;
	} *lock_list;
	char progname[MAXPATHLEN];

	for (lock_list = BTOCPTR (cli->cli_CommandDir);
	     lock_list;
	     lock_list = BTOCPTR (lock_list->next))
	  {
	    if (NameFromLock(lock_list->lock, progname, MAXPATHLEN) &&
		AddPart(progname, name, MAXPATHLEN))
	      {
		seg = load_seg_or_script (lock_list->lock, name, name, args, progname);
	      }
	    else
	      errno = ENOENT;
	    if (seg)
	      break;
	    if (errno != ENOENT)
	      return 0;
	  }
      }
  }

  if (seg)
    return (BPTR *)&seg->segment;

  errno = ENOENT;
  KPRINTF (("&errno = %lx, errno = %ld\n", &errno, errno));
  return 0;
}

static struct my_seg *
load_seg_or_script (BPTR parent, const char *name, const char *adosname, char **args, const char *progname)
{
  BPTR ocd, tmpcd;
  short oroot;
  struct my_seg *seg;
  usetup;

  if (args) *args = 0;
  ocd = CurrentDir (parent);
  oroot = u.u_is_root;
  u.u_is_root = 0;

  seg = check_and_load_seg (adosname);

  if (!seg && errno != ENOENT)
    {
      CurrentDir(ocd);
      u.u_is_root = oroot;
      return 0;
    }

  /* try to do interpreter - expansion, but only if args is non-zero */
  if (!seg && args)
    {
      int fd, n;
      char magic[5];
      struct stat stb;

      if (syscall (SYS_stat, name, &stb) == 0 && S_ISREG (stb.st_mode))
	{
	  if ((fd = syscall (SYS_open, name, 0)) >= 0)
	    {
	      /*
	       *  If the .key line of an AmigaOS script isn't the first
	       *  line of the script, the AmigaOS shell gets very confused.
	       *  Therefore, we skip the first line if it begins with .key,
	       *  and we test the second line for #! or ;!.
	       */
	      if ((n = syscall (SYS_read, fd, magic, 4)) == 4)
		{
		  magic[4] = 0;
		  if (!strcasecmp(magic, ".key"))
		    {
		      n = 0;
		      /* skip this line */
		      while (syscall (SYS_read, fd, magic, 1) == 1)
			if (magic[0] == '\n')
			  {
			    n = syscall (SYS_read, fd, magic, 4);
			    break;
			  }
		    }
		}
	      if (n >= 2 && (magic[0] == '#' || magic[0] == ';') && magic[1] == '!')
		{
		  char interp[MAXINTERP + MAXPATHLEN + 2], *interp_start;

		  interp[0] = magic[2];
		  interp[1] = magic[3];
		  n -= 2;
		  n = n + syscall (SYS_read, fd, interp + n, MAXINTERP - n);
		  if (n > 0)
		    {
		      char *cp, *cp2, ch;

		      /* okay.. got one.. terminate with 0 and try to find end of it */
		      interp[n] = 0;
		      for (interp_start = interp; isspace(*interp_start) && interp_start < interp + n; interp_start++);
		      for (cp = interp_start; cp < interp + n; cp++)
			if (*cp == 0 || isspace (*cp))
			  break;
		      ch = *cp;
		      *cp = 0;

		      /* okay, lets try to load this instead. Call us recursively,
		       * but leave out the argument-argument, so we can't get
		       * into infinite recursion. Interpreter-Expansion is only
		       * done the first time __load_seg() is called from
		       * execve()
		       */
		      tmpcd = CurrentDir(ocd);
		      u.u_is_root = oroot;
		      seg = (struct my_seg *) __load_seg (interp_start, 0);
		      CurrentDir(tmpcd);
		      u.u_is_root = 0;

		      if (!seg)
			goto fd_close;

		      /* using \001 as seperator for progam|argument|script */
		      *cp++ = '\001';

		      if (ch && ch != '\n' && cp < interp + n)
		      {
			cp2 = cp;

			/* first skip intergap whitespace */
			while (isspace(*cp2) && *cp2 != '\n') cp2++;

			/* in this case, set the argument as well. */
			if (*cp2 && *cp2 != '\n')
			{
			  while (*cp2 && !isspace(*cp2) && *cp2 != '\n' && cp < interp + n)
			    *cp++ = *cp2++;

			  *cp++ = '\001';
			}
		      }

		      if (index (progname, ':'))
			{
			  *cp++ = '/';
			  strcpy (cp, progname);
			  *index (cp, ':') = '/';
			}
		      else
			strcpy (cp, progname);

		      *args = (char *) syscall (SYS_strdup, interp_start);
		    }
		}
fd_close:
	      syscall (SYS_close, fd);
	    }
	}
    }
  CurrentDir (ocd);
  u.u_is_root = oroot;

  if (!seg)
    errno = ENOENT;
  return seg;
}
