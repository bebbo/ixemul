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
 *  _wb_parse.c,v 1.1.1.1 1994/04/04 04:30:42 amiga Exp
 *
 *  _wb_parse.c,v
 * Revision 1.1.1.1  1994/04/04  04:30:42  amiga
 * Initial CVS check in.
 *
 *  Revision 1.2  1992/08/09  20:42:46  amiga
 *  get rid of some warnings
 *
 *  Revision 1.1  1992/05/14  19:55:40  mwild
 *  Initial revision
 *
 */

/*
 * Originally written by Olaf "Olsen" Barthel. Thanks Olsen!
 * I left his indentation style...
 */

#define _KERNEL
#include "ixemul.h"

#include <workbench/workbench.h>
#include <workbench/startup.h>

#include <proto/icon.h>

#include <string.h>

/* wb_parse():
 *
 *	A more or less decent rewrite of Manx' original
 *	wb_parse() routine which will do the following:
 *
 *	- if open_wb_window is zero no console window
 *	  will be opened.
 *
 *	- if default_wb_window points to a valid string
 *	  it will be used to open the window.
 *
 *	- if everything fails, read the icon file,
 *	  scan it for a "WINDOW" tool type and take
 *	  the corresponding tool type entry string
 *	  (somewhat primitive if compared to the Macintosh
 *	  way of doing it...).
 *
 *	This routine will return TRUE if it was able to
 *	open an output file, FALSE otherwise.
 */

int
__ix_wb_parse(struct Process *ThisProcess, struct WBStartup *WBenchMsg,
	  char *default_wb_window)
{
  struct Library *IconBase;
  char	*WindowName = NULL;
  int	fd = -1;
  usetup;

  /* Are we to open a console window? */

  if (default_wb_window != (char *)-1)
    {
      /* Any special name preference? */

      if (default_wb_window)
	WindowName = default_wb_window;
      else
	{
	  /* Do we have a valid tool window? */

	  if (WBenchMsg->sm_ToolWindow)
	    WindowName = WBenchMsg->sm_ToolWindow;
	  else
	    {
	      /* Open icon.library. */

	      if ((IconBase = OpenLibrary("icon.library", 0)))
		{
		  struct DiskObject *Icon;

		  /* Try to load the icon file. */

		  if ((Icon = GetDiskObject(WBenchMsg->sm_ArgList[0].wa_Name)))
		    {
		      /* Look for a "WINDOW" tool type. */

		      WindowName = (u_char *) FindToolType((u_char **)Icon->do_ToolTypes, "WINDOW");
		      if (WindowName)
			WindowName = strdup (WindowName);
		      FreeDiskObject(Icon);
		    }
		  CloseLibrary(IconBase);
		}
	    }
	}

	/* Are we to open a file? */

	if (WindowName)
	  fd = syscall (SYS_open, WindowName, 2);
	if (fd != -1)
	  {
	    /* this fd "should" be 0, since we didn't open any files
	     * till now when running under workbench */
	    if (fd != 0)
	      ix_warning("__ix_wb_parse: first opened fd != 0!");

	    ThisProcess->pr_ConsoleTask = u.u_ofile[fd]->f_fh->fh_Type;
	    syscall (SYS_dup2, fd, 2); 
	    /* now dup() the descriptor to cover an additional descriptor, 
	     * so that by closing 0-2 pr_ConsoleTask doesn't vanish yet */
	    syscall (SYS_dup2, fd, NOFILE - 1);
	    syscall (SYS_close, fd);

            /* Apparently use of CONSOLE: gave problems with
               Emacs, so we continue to use "*" instead. */
	    fd = syscall (SYS_open, "*", 0);
	    if (fd != -1)
	      ThisProcess->pr_CIS = CTOBPTR (u.u_ofile[fd]->f_fh);

	    fd = syscall (SYS_open, "*", 1);
	    ThisProcess->pr_COS = CTOBPTR (u.u_ofile[fd]->f_fh);
	    return 1;
	  }
    }
  return FALSE;
}

