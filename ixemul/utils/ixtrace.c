/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1991, 1992  Markus M. Wild
 *  Changed to avoid buffer overflows by J. Hoehle and
 *  restricted output length for: str(n)cat, strlen
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

#include <stdio.h>
#include <stdlib.h>
#include <clib/alib_protos.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <sys/tracecntl.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/ioctl_compat.h>
#include <sys/termios.h>
#include <exec/types.h>
#include <exec/execbase.h>
#include <proto/exec.h>
#include <libraries/dos.h>
#include "ixtrace.h"

#include <ix.h>
#include <proto/exec.h>

#ifdef __MORPHOS__
#define CreatePort(x,y) CreateMsgPort()
#define DeletePort	DeleteMsgPort
#endif

#define OUT_WIDTH  80   /* big enough (>30) to hold first information */

static void print_call (FILE *output, struct trace_packet *tp);
static void show(struct trace_packet *tp, int in);
static void pshow(struct trace_packet *tp, int in);

int print_all = 0;
int skip_sigsetmask = 0;
int skip_calls = 0;
FILE *output;
char VERSION[] = "$VER: ixtrace 1.470 (14-Jun-97)";
int ctrlc = 0;


void
ctrlc_handler ()
{
  ctrlc = 1;
}

int
main (int argc, char *argv[])
{
  char *logfile = "-";
  int c, in = 0;
  struct trace_packet tp;

  memset ((char *) &tp, '\000', sizeof (tp));
  signal (SIGINT, ctrlc_handler);

  while ((c = getopt (argc, argv, "ailwvmzo:c:p:s:n:")) != EOF)
    switch (c)
      {
      case 'a':
	print_all = 1;
	break;

      case 'i':
	in = 1;
	break;

      case 'l':                                 /* list system calls */
	{
		int i;

		for(i=1;i<=MAXCALLS;i=i+2)
		{
		fprintf(stdout,"(%3d) %-25s\t(%3d) %-25s\n",i,call_table[i].name,
			i+1,call_table[i+1].name);
		}
	return 0;
	}
	break;

      case 'c':                                 /* system call by name */
	{
		int i;
		int notfound=1; /* Just to make sure that we know if call is found */
		char *callname;

		callname=optarg;

		for(i=1;i<=MAXCALLS;i++)
		{
			if (!strcmp(callname, call_table[i].name))
				{       tp.tp_syscall = i;
					notfound=0;
					break;
				}
		}
		if (notfound)
		{
			 fprintf(stderr,"system call [%s] is unknown to ixtrace\n",callname);
			 return 1;
		}
	}
	break;

      case 'm':
	skip_sigsetmask = 1;
	break;

      case 'o':
	logfile = optarg;
	break;

      case 'p':
	tp.tp_pid = atoi (optarg);
	break;

      case 'n':
	skip_calls = atoi (optarg);
	if (skip_calls < 0)
	{
	  fprintf(stderr, "invalid argument for option -n\n");
	  exit(1);
	}
	break;

      case 's':
	if (!isdigit(optarg[0]))
	{
	  fprintf(stderr, "The -s option requires a number\n");
	  exit(1);
	}
	tp.tp_syscall = atoi (optarg);
	if (tp.tp_syscall > MAXCALLS)
	{
	  fprintf(stderr, "System call number is out of range 1-%d\n",MAXCALLS);
	  exit(1);
	}
	break;

      case 'w':                                 /* Wipe out the calls you don't want */
	{
		int i;
		char calls[80]="";
		int notfound=1;

		fprintf(stdout,"When done enter \"x\" by itself, followed by a [RETURN]\n");
	do {
		fprintf(stdout,"trace > ");
		gets(calls);
		if (!strcmp("x",calls)) break;
			for(i=1;i<=MAXCALLS;i++)
			{
				if (!strcmp(calls, call_table[i].name))
					{       call_table[i].interesting=0;
						notfound=0;
						break;
					}
			}
			if (notfound) fprintf(stderr,"[%s] is unknown to ixtrace, try again\n"
										, calls);
		notfound=1;
		} while(strcmp("x",calls)); /* A lil' over-kill */

	}
	break;
      case 'z':                                 /* you name the calls --in testing-- */
	{
		int i;
		char calls[80]="";
		int notfound=1;

		/* Right now this is only the beginning, clear all systems calls.
		   In other words, make them all non-interesting.                                 */
		for(i=1;i<=MAXCALLS;i++)
		{
		  call_table[i].interesting=0;
		}
		fprintf(stdout,"When done enter \"x\" by itself, followed by a [RETURN]\n");
	do {
		fprintf(stdout,"trace > ");
		gets(calls);
		if (!strcmp("x",calls)) break;
			for(i=1;i<=MAXCALLS;i++)
			{
				if (!strcmp(calls, call_table[i].name))
					{       call_table[i].interesting=1;
						notfound=0;
						break;
					}
			}
			if (notfound) fprintf(stderr,"[%s] is unknown to ixtrace, try again\n"
										, calls);
		notfound=1;
		} while(strcmp("x",calls)); /* A lil' over-kill */

	}
	break;
      case 'v':
	{
	  fprintf(stdout, "%s\n",VERSION+6); /* get rid of the first 7 chars */
	  return 0;
	}
	break;

      default:
	fprintf (stderr, "%s [-a] [-m] [-l] [-v] [-z] [-c syscall-name] [-n N] [-o logfile] [-p pid] [-s syscall-number]\n", argv[0]);
	fprintf (stderr, "  -a  trace all calls (else __-calls are skipped)\n");
	fprintf (stderr, "  -m  skip sigsetmask() calls (they're heavily used inside the library)\n");
	fprintf (stderr, "  -i  trace entry to functions. Default is exit.\n");
	fprintf (stderr, "  -l  list system calls (syscall #) [syscall name]\n");
	fprintf (stderr, "  -v  version\n");
	fprintf (stderr, "  -w  wipe out syscalls by name\n");
	fprintf (stderr, "  -z  only trace the syscalls you want to trace\n");
	fprintf (stderr, "  -c  only trace this syscall (by name)\n");
	fprintf (stderr, "  -n  skip the first N traces\n");
	fprintf (stderr, "  -o  log output to logfile (default is stdout)\n");
	fprintf (stderr, "  -p  only trace process pid (default is to trace all processes)\n");
	fprintf (stderr, "  -s  only trace this syscall (default is to trace all calls)\n");

	return 1;
      }

  if (logfile[0] == '-' && !logfile[1])
    output = stdout;
  else
    output = fopen (logfile, "w");

  if (!output)
    {
      perror ("fopen");
      return 1;
    }
  show(&tp, in);
  return 0;
}

static void show(struct trace_packet *tp, int in)
{
  struct MsgPort *mp;

  if ((mp = CreatePort (0, 0)))
    {
      tp->tp_tracer_port = mp;
      if (ix_tracecntl (TRACE_INSTALL_HANDLER, tp) == 0)
	{
	  while (!ctrlc)
	    {
	      struct Message *msg;
	      long sigs;

	      sigs = (1 << mp->mp_SigBit) | SIGBREAKF_CTRL_C;
	      ix_wait(&sigs);
	      while ((msg = GetMsg (mp)))
		{
		  if (msg != (struct Message *)tp)
		    {
		      fprintf (stderr, "Got alien message! Don't do that ever again ;-)\n");
		    }
		  else
		    {
		      if (in)
			tp->tp_action = TRACE_ACTION_JMP;
		      if (! tp->tp_is_entry || tp->tp_action == TRACE_ACTION_JMP)
			print_call (output, tp);
		    }
		  Signal ((struct Task *) msg->mn_ReplyPort, /*SIGBREAKF_CTRL_E fixme, see tracecntl.c */);
		}
	      if (sigs & SIGBREAKF_CTRL_C)
		break;
	    }
	  ix_tracecntl (TRACE_REMOVE_HANDLER, tp);
	}
      else
	perror ("ix_tracecntl");

      DeletePort (mp);
    }
  else
    perror ("CreatePort");
}

/* should help make things less mystic... */
#define TP_SCALL(tp) (tp->tp_argv[0])

/* this is only valid if !tp->tp_is_entry */
#define TP_RESULT(tp) (tp->tp_argv[1])

#define TP_ERROR(tp) (* tp->tp_errno)

/* tp_argv[2] is the return address */
/* tp_argv[3-6] are d0/d1/a0/a1 */
#define TP_FIRSTARG(tp) (tp->tp_argv[7])

void
print_call (FILE *output, struct trace_packet *tp)
{
  char line[OUT_WIDTH+2];       /* for \n\0 */
  char *argfield;
  int space, len;
  struct call *c;

  space = sizeof (line) - 1;
  len = sprintf (line, "$%lx: %c", (unsigned long) tp->tp_message.mn_ReplyPort,
		 tp->tp_is_entry ? '>' : '<');
  argfield = line + len;
  space -= len;

  if (TP_SCALL (tp) > sizeof (call_table) / sizeof (struct call))
    {
      if (tp->tp_is_entry)
	sprintf (argfield, "SYS_%d()\n", TP_SCALL (tp));
      else
	sprintf (argfield, "SYS_%d() = $%lx (%d)\n",
		  TP_SCALL (tp), (unsigned long) TP_RESULT (tp), TP_ERROR (tp));
    }
  else
    {
      c = call_table + TP_SCALL (tp);

      if ((!print_all && !c->interesting) ||
	  (skip_sigsetmask && TP_SCALL (tp) == SYS_sigsetmask))
	return;

      /* we can write space-1 real characters in the buffer, \n is not counted */
      c->func (argfield, space-1, c, tp);
    }

  if (skip_calls == 0)
    fputs (line, output);
  else
    skip_calls--;
}

/* the program contained a bug due to the fact that
 * when snprintf or vsnprintf are called with a zero or negative size,
 * they return -1 as an error marker but not any length.
 */

static void
vp (char *buf, int len, struct call *c, struct trace_packet *tp)
{
  int nl = snprintf (buf, len+1, "%s", c->name);

  len -= nl; if (len <= 0) goto finish;
  buf += nl;
  if (tp->tp_is_entry || !c->rfmt[0])
    vsnprintf (buf, len+1, c->fmt, (_BSD_VA_LIST_) & TP_FIRSTARG (tp));
  else
    {
      nl = vsnprintf (buf, len+1, c->fmt, (_BSD_VA_LIST_) & TP_FIRSTARG (tp));
      len -= nl; if (len <= 0) goto finish;
      buf += nl;
      nl = snprintf (buf, len+1, "=");
      len -= nl; if (len <= 0) goto finish;
      buf += nl;
      nl = vsnprintf (buf, len+1, c->rfmt, (_BSD_VA_LIST_) & TP_RESULT (tp));
      len -= nl; if (len <= 0) goto finish;
      buf += nl;
      nl = snprintf (buf, len+1, " (%d)", TP_ERROR (tp));
    }

  finish:
  strcat (buf, "\n");
}

const char *
get_fcntl_cmd (int cmd)
{
  switch (cmd)
    {
    case F_DUPFD:
	return "F_DUPFD";

    case F_GETFD:
	return "F_GETFD";

    case F_SETFD:
	return "F_SETFD";

    case F_GETFL:
	return "F_GETFL";

    case F_SETFL:
	return "F_SETFL";

    case F_GETOWN:
	return "F_GETOWN";

    case F_SETOWN:
	return "F_SETOWN";

#ifdef F_GETLK
    case F_GETLK:
	return "F_GETLK";
#endif

#ifdef F_SETLK
    case F_SETLK:
	return "F_SETLK";
#endif

#ifdef F_SETLKW
    case F_SETLKW:
	return "F_SETLKW";
#endif

    default:
	return "F_unknown";
    }
}

char *
get_open_mode (int mode)
{
  static char buf[120];

  switch (mode & O_ACCMODE)
    {
    case O_RDONLY:
      strcpy (buf, "O_RDONLY");
      break;

    case O_WRONLY:
      strcpy (buf, "O_WRONLY");
      break;

    case O_RDWR:
      strcpy (buf, "O_RDWR");
      break;

    default:
      strcpy (buf, "O_illegal");
      break;
    }

#define ADD(flag) \
  if (mode & flag) strcat (buf, "|" #flag);

  ADD (O_NONBLOCK);
  ADD (O_APPEND);
  ADD (O_SHLOCK);
  ADD (O_EXLOCK);
  ADD (O_ASYNC);
  ADD (O_FSYNC);
  ADD (O_CREAT);
  ADD (O_TRUNC);
  ADD (O_EXCL);
#undef ADD

  return buf;
}

char *
get_ioctl_cmd (int cmd)
{
  static char buf[12];

  /* only deal with those commands that are really implemented somehow in
     ixemul.library. The others are dummies anyway, so they don't matter */

  switch (cmd)
    {
    case FIONREAD:
      return "FIONREAD";

    case FIONBIO:
      return "FIONBIO";

    case FIOASYNC:
      /* not yet implemented, but important to know if some program tries
	 to use it ! */
      return "FIOASYNC";

    case TIOCGETA:
      return "TIOCGETA";

    case TIOCSETA:
      return "TIOCSETA";

    case TIOCSETAW:
      return "TIOCSETAW";

    case TIOCSETAF:
      return "TIOCSETAF";

    case TIOCGETP:
      return "TIOCGETP";

    case TIOCSETN:
      return "TIOCSETN";

    case TIOCSETP:
      return "TIOCSETP";

    case TIOCGWINSZ:
      return "TIOCGWINSZ";

    case TIOCOUTQ:
      return "TIOCOUTQ";

    case TIOCSWINSZ:
      return "TIOCSWINSZ";

    default:
      sprintf (buf, "$%lx", (unsigned long) cmd);
      return buf;
    }
}


static void
vp_fcntl (char *buf, int len, struct call *c, struct trace_packet *tp)
{
  int *argv =  & TP_FIRSTARG (tp);

  if (tp->tp_is_entry)
    if (argv[1] == F_GETFL || argv[1] == F_SETFL)
      snprintf (buf, len+1, "fcntl(%d, %s, %s)",
		argv[0], get_fcntl_cmd (argv[1]), get_open_mode (argv[2]));
    else
      snprintf (buf, len+1, "fcntl(%d, %s, %d)",
		argv[0], get_fcntl_cmd (argv[1]), argv[2]);
  else
    if (argv[1] == F_GETFL || argv[1] == F_SETFL)
      snprintf (buf, len+1, "fcntl(%d, %s, %s) = %d (%d)",
		argv[0], get_fcntl_cmd (argv[1]),
		get_open_mode (argv[2]), TP_RESULT (tp), TP_ERROR (tp));
    else
      snprintf (buf, len+1, "fcntl(%d, %s, %d) = %d (%d)",
		argv[0], get_fcntl_cmd (argv[1]), argv[2],
		TP_RESULT (tp), TP_ERROR (tp));

  strcat (buf, "\n");
}


static void
vp_ioctl (char *buf, int len, struct call *c, struct trace_packet *tp)
{
  int *argv = & TP_FIRSTARG (tp);

  if (tp->tp_is_entry)
    snprintf (buf, len+1, "ioctl(%d, %s, $%lx)",
	      argv[0], get_ioctl_cmd (argv[1]), argv[2]);
  else
    snprintf (buf, len+1, "ioctl(%d, %s, $%lx) = %d (%d)",
	      argv[0], get_ioctl_cmd (argv[1]), argv[2],
	      TP_RESULT (tp), TP_ERROR (tp));

  strcat (buf, "\n");
}


static void
vp_open (char *buf, int len, struct call *c, struct trace_packet *tp)
{
  int *argv = & TP_FIRSTARG (tp);

  if (tp->tp_is_entry)
    snprintf (buf, len+1, "open(\"%s\", %s)", argv[0], get_open_mode (argv[1]));
  else
    snprintf (buf, len+1, "open(\"%s\", %s) = %d (%d)", argv[0],
	      get_open_mode (argv[1]), TP_RESULT (tp), TP_ERROR (tp));

  strcat (buf, "\n");
}

static void
vp_pipe (char *buf, int len, struct call *c, struct trace_packet *tp)
{
  int *argv = & TP_FIRSTARG (tp);

  if (tp->tp_is_entry)
    snprintf (buf, len+1, "pipe($%lx)", argv[0]);
  else
    {
      int *pv = (int *) argv[0];

      snprintf (buf, len+1, "pipe([%d, %d]) = %d (%d)",
		pv[0], pv[1], TP_RESULT (tp), TP_ERROR (tp));
    }

  strcat (buf, "\n");
}
