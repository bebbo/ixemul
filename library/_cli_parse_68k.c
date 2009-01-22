/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1991, 1992  Markus M. Wild
 *  Portions (C) 1995 Jeff Shepherd
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
 *  _cli_parse.c,v 1.1.1.1 1994/04/04 04:29:41 amiga Exp
 *
 *  _cli_parse.c,v
 * Revision 1.1.1.1  1994/04/04  04:29:41  amiga
 * Initial CVS check in.
 *
 *  Revision 1.3  1992/08/09  20:41:17  amiga
 *  change to use 2.x header files by default
 *
 *  Revision 1.2  1992/07/04  19:09:27  mwild
 *  make stderr (desc 2) *really* read/write, don't just say so...
 *
 * Revision 1.1  1992/05/14  19:55:40  mwild
 * Initial revision
 *
 */

/*
 *	This routine is called from the _main() routine and is used to
 *	parse the arguments passed from the CLI to the program. It sets
 *	up an array of pointers to arguments in the global variables and
 *	and sets up _argc and _argv which will be passed by _main() to
 *	the main() procedure. If no arguments are ever going to be
 *	parsed, this routine may be replaced by a stub routine to reduce
 *	program size.
 *
 */

#define _KERNEL
#include "ixemul.h"
#include "kprintf.h"
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <glob.h>

extern int __read(), __write(), __ioctl(), __fselect(), __close();

// Initialize file structure
static void init_file(struct file *f, BPTR fh, char *defname)
{
  f->f_fh = (struct FileHandle *)BTOCPTR(fh);

  __init_std_packet(&f->f_sp);
  __init_std_packet((void *)&f->f_select_sp);
  __fstat(f);

  f->f_flags = FEXTOPEN;
  f->f_type  = DTYPE_FILE;
  f->f_read  = __read;
  f->f_write = __write;
  f->f_ioctl = __ioctl;
  f->f_close = __close;
  f->f_select= __fselect;

  if (!IsInteractive(fh))
    {
      char buf[256];

      if (NameFromFH(fh, buf, sizeof(buf)))
        {
          if ((f->f_name = (void *)kmalloc(strlen(buf) + 1)))
            strcpy(f->f_name, buf);
        }
    }
  if (f->f_name == NULL)
    {
      f->f_name = defname;
      f->f_flags |= FEXTNAME;   // don't free f_name
    }
}

/* We will store all arguments in this double linked list, the list
 * is always sorted according to elements, ie. order of given arguments
 * is preserved, but all elements, that get expanded will be sorted
 * alphabetically in this list. */

struct ArgList {
  struct ixlist  al_list;  /* the list - head */
  long		 al_num;   /* number of arguments in the whole list */
};

struct Argument {
  struct ixnode a_node;   /* the link in the arg-list */
  char		*a_arg;	   /* a malloc'd string, the argument */
};

/* insert a new argument into the argument vector, we have to keep the
 * vector sorted, but only element-wise, otherwise we would break the
 * order of arguments, and "copy b a" is surely not the same as "copy a b"..
 * so we don't scan the whole list, but start with element "start",
 * if set, else we start at the list head */

static void 
AddArgument (struct ArgList *ArgList,
	     struct Argument *start, struct Argument *arg, long size)
{
  register struct Argument *el;

  /* depending on "start", start scan for right position in list at
   * successor of start or at head of list  */
  for (el = (struct Argument *)
	    (start ? start->a_node.next : ArgList->al_list.head);
       el;
       el = (struct Argument *)el->a_node.next)
    if (strcmp (el->a_arg, arg->a_arg) > 0) break;
  if (el == NULL)
    el = (struct Argument *)ArgList->al_list.tail;

  ixinsert ((struct ixlist *)ArgList, (struct ixnode *)arg, (struct ixnode *)el);

  /* and bump up the argument counter once */
  ++ArgList->al_num;
}

/* if an argument contains one or more of these characters, we have to
 * call the glob() stuff, else don't bother expanding and
 * quickly append to list. Here's the meaning of all these characters, some
 * seem to be not widely used:
 * *	match any number (incl. zero) of characters
 * #?	match any number (incl. zero) of characters (for Amiga compatibility)
 * []   match any character that's contained in the set inside the brackets
 * ?	match any character (exactly one)
 * !	negate the following expression
 */

#define iswild(ch) (index ("*[!?#", ch) ? 1 : 0)

void
__ix_cli_parse(struct Process *this_proc, long alen, char *_aptr,
	   int *argc, char ***argv)
{
  usetup;
  char *arg0;
  struct CommandLineInterface *cli;
  char *next, *lmax;
  struct Argument *arg, *narg;
  char *line, **cpp;
  int do_expand;
  int arglen;
  char *aptr;
  struct ArgList ArgList;
  int expand_cmd_line = u.u_expand_cmd_line;
  struct file *fin, *fout;
  int fd;  
  BPTR fh;
  int omask;

  KPRINTF (("entered __ix_cli_parse()\n"));
  KPRINTF (("command line length = %ld\n", alen));
  KPRINTF (("command line = '%s'\n", _aptr));

  /* this stuff has been in ix_open before, but it really belongs here, since
   * I don't want it to happen by default on OpenLibrary, since it would
   * disturb any vfork() that wants to inherit files from its parent 
   */

  omask = syscall (SYS_sigsetmask, ~0);

  if (! falloc (&fin, &fd))
    {
      /*
       * NOTE: if there's an error creating one of the standard
       *       descriptors, we just go on, the descriptor in
       *       question will then not be set up, no problem ;-)
       */
      if (fd != 0)
	ix_warning("allocated stdin is not fd #0!");
		       
      if (! falloc (&fout, &fd))
	{
	  if (fd != 1)
	    ix_warning("allocated stdout is not fd #1!");

	  if ((fh = Input ()))
	    {
	      init_file(fin, fh, "<Standard Input>");
	      fin->f_flags |= FREAD;
	      fin->f_ttyflags = IXTTY_ICRNL;
	      fin->f_write = 0;
	    }
	  else
	    {
	      u.u_ofile[0] = 0;
	      fin->f_count--;
	    }
			        
	  if ((fh = Output ()))
	    {
	      init_file(fout, fh, "<Standard Output>");
	      fout->f_flags |= FWRITE;
	      fout->f_ttyflags = IXTTY_OPOST | IXTTY_ONLCR;
	      fout->f_read  = 0;
	    }
	  else
	    {
	      u.u_ofile[1] = 0;
	      fout->f_count--;
	    }

	  /* deal with stderr. Seems this was a last minute addition to 
	     dos 2, it's hardly documented, there are no access functions,
	     nobody seems to know what to do with pr_CES...
	     If pr_CES is valid, then we use it, otherwise we open the
	     console. */

	  fd = -1;
	  if ((fh = this_proc->pr_CES))
	    {
	      struct file *fp;

	      if (!falloc (&fp, &fd))
	        {
	          init_file(fp, fh, "<Standard Error>");
		  fp->f_flags |= FREAD|FWRITE;
	          fp->f_ttyflags = IXTTY_OPOST | IXTTY_ONLCR;
	        }
	    }
	    /* Apparently use of CONSOLE: gave problems with
               Emacs, so we continue to use "*" instead. */

            /* Here is some more information on this from Joerg Hoehle:
             *
	     * While writing fifolib38_1 I found that console handlers are sent
	     * ACTION_FIND* packets with names of either "*" or "Console:", depending
	     * on what the user typed. Old handlers that do not recognize "CONSOLE:"
	     * will produce strange results which could explain the above problems.
             *
	     * That's the reason why
	     * 	echo foo >*	(beware of * expansion in a non-AmigaOS shell)
	     * works in an Emacs shell buffer, whereas
	     * 	echo foo >console:
	     * won't with fifolib prior to version 38.1.
             *
	     * I believe that programs opening stderr should continue to open "*" for
	     * compatibility reasons. Opening "CONSOLE:" first and "*" if it fails is
	     * _not_ a solution: for example, FIFO: (prior to 38.1) accepts the
	     * Open("CONSOLE:") call, giving a FIFO that can be neither read nor
	     * written to :-(
	     */
	  if (fd == -1)
	    fd = syscall(SYS_open, "*", 2);
	  if (fd > -1 && fd != 2)
	    {
	      syscall(SYS_dup2, fd, 2);
	      syscall(SYS_close, fd);
	    }

	} /* falloc (&fout, &fd) */
    } /* falloc (&fin, &fd) */

  aptr = alloca (alen + 1);
  memcpy(aptr, _aptr, alen + 1);

  cli = (struct CommandLineInterface *) BTOCPTR (this_proc->pr_CLI);
  arg0 = (char *) BTOCPTR (cli->cli_CommandName);

  /* init our argument list */
  ixnewlist ((struct ixlist *)&ArgList);

  /* lets start humble.. no arguments at all:-)) */
  ArgList.al_num = 0;

  /* find end of command-line, stupid BCPL-stuff.. line can end
   * either with \n or with \0 .. */
  for (lmax = aptr; *lmax && *lmax != '\n' && *lmax != '\r'; ++lmax) ;
  *lmax = 0;

  /* loop over all arguments, expand all */
  for (line = aptr, narg = arg = 0; line < lmax; )
    {
      do_expand = 0;

      KPRINTF (("remaining cmd line = '%s'\n", aptr));
      /* skip over leading whitespace */
      while (isspace (*line)) line++;
      if (line >= lmax)
	break;

      /* if argument starts with ", don't expand it and remove the " */
      if (*line == '\"')
	{
	  KPRINTF (("begin quoted argument at '%s'\n", line));
	  /* scan for end of quoted argument, this can be either at
	   * end of argumentline or at a second " */
	  line++;
	  next = line;
	  while (next < lmax && *next != '\"')
	    {
	      /* Prevent that the loop terminates due to an escaped quote.
	       * However, if the character after the quote is a space, then
	       * it is ambiguous whether or not the quote is escaped or is
	       * the end of the argument.  Consider what happens when you give
	       * /bin/sh a 'FS=\' argument.  This gets passed to ixemul.library
	       * as "FS=\" <other args> */
	      if ((*next == '\'' || *next == '\\') && next[1] == '\"')
		{
		  /* in this case we have to shift the whole remaining
		   * line one position to the left to skip the 
		   * escape-character */
		  bcopy (next + 1, next, (lmax - next) + 1);
		  --lmax;
		}

	      ++next;
	    }
	  *next = 0;
	  KPRINTF (("got arg '%s'\n", line));
	}
      else
	{
          /* strange kind of BCPL-quoting, if you want to get a " thru,
           * you have to quote it with a ', eq. HELLO'"WORLD'" will preserve
           * the " inside the argument. Since hardly anyone knows this
           * "feature", I allow for the more common Unix-like escaping, ie
           * \" will give you the same effect as '". */
          if ((*line == '\'' || *line == '\\') && line[1] == '\"')
            {
  	      KPRINTF (("found escaped quote at '%s'\n", line));
  	      line++;
  	    }
	  /* plain, vanilla argument.. */
	  next = line + 1;
	  /* check, whether we have to run thru the expander, or
	   * if we rather can just copy over the whole argument */
	  do_expand = iswild (*line);
	  /* skip over element and make it 0-terminated .. */
	  while (next < lmax && !isspace (*next))
	    {
	      do_expand |= iswild (*next);
	      if ((*next == '\'' || *next == '\\') && next[1] == '\"')
		{
		  bcopy (next + 1, next, (lmax - next) + 1);
		  --lmax;
		}

	      ++next;
	    }
	  *next = 0;
	}

      if (expand_cmd_line && do_expand)
	{
          glob_t g;
          char **p;
          
          syscall (SYS_sigsetmask, omask);
          syscall (SYS_glob, line,
                   ((ix.ix_flags & ix_unix_pattern_matching_case_sensitive) ? 0 : GLOB_NOCASE) |
                   ((ix.ix_flags & ix_allow_amiga_wildcard) ? GLOB_AMIGA : 0) |
	           GLOB_NOCHECK, NULL, &g);
          omask = syscall (SYS_sigsetmask, ~0);
          for (p = g.gl_pathv; *p; p++)
            {
              arg = (struct Argument *)syscall(SYS_malloc, sizeof(*arg));
              arg->a_arg = *p;
              AddArgument(&ArgList, narg, arg, strlen(*p));
              narg = (struct Argument *) ArgList.al_list.tail;
            }
          syscall(SYS_free, g.gl_pathv);
	}
      else  /* ! do_expand */
	{
	  /* just add the argument "as is" */
	  arg = (struct Argument *) syscall (SYS_malloc, sizeof (*arg));
	  arglen = strlen (line);
	  arg->a_arg = (char *) syscall (SYS_malloc, arglen + 1);
	  strcpy (arg->a_arg, line);
	  AddArgument (&ArgList, narg, arg, arglen);
	}

      narg = (struct Argument *) ArgList.al_list.tail;
      line = next + 1;
    } /* for */

  /* prepend the program name */
  arg = (struct Argument *) syscall (SYS_malloc, sizeof (*arg));

  /* some stupid shells (like Wsh...) pass the WHOLE path of the
   * started program. We simply cut off what we don't want ;-)) */
  for (arglen = 1; arglen <= arg0[0]; arglen++)
    if (arg0[arglen] == ' ' || arg0[arglen] == '\t')
      break;

  line = arg0 + 1;

  arg->a_arg = (char *) syscall (SYS_malloc, arglen);

  strncpy (arg->a_arg, line, arglen - 1);
  arg->a_arg[arglen - 1] = 0;
  ixaddhead ((struct ixlist *)&ArgList, (struct ixnode *)arg);
  ++ ArgList.al_num;

  /* build _argv array */
  *argv = (char **) syscall (SYS_malloc, (ArgList.al_num+1) * sizeof(char *));
  for (cpp = *argv, arg = (struct Argument *) ArgList.al_list.head;
       arg;
       arg = (struct Argument *)arg->a_node.next)
    *cpp++ = arg->a_arg;

  /* guarantee last element == 0 */
  *cpp = 0;
  KPRINTF_ARGV ("argv", *argv);
  *argc = ArgList.al_num;
  
  if (u.u_ixnetbase)
    {
      int daemon = netcall(NET_init_inet_daemon, argc, argv);

      if (daemon >= 0)
        set_socket_stdio(daemon);
    }

  KPRINTF (("argc = %ld\n", *argc));
  KPRINTF (("leaving __ix_cli_parse()\n"));

  syscall (SYS_setsid); /* setup new session */
  syscall (SYS_sigsetmask, omask);
}
