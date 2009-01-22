/*
    Ixprefs v.2.7--ixemul.library configuration program
    Copyright © 1995,1996 Kriton Kyrimis

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <dos/dos.h>
#include <stdio.h>
#include <stdlib.h>
#include "getopt.h"
#include "ixprefs.h"

#define FAILURE 20
#define SUCCESS 0

void
usage(char *prog)
{
  printf("Usage: %s [OPTION]...\n\nOptions:\n", prog);
  printf("-a 1, --allow-amiga-wildcard\n");
  printf("  accept AmigaOS wildcard notation\n");
  printf("-a 0, --no-allow-amiga-wildcard\n");
  printf("  do not accept AmigaOS wildcard notation\n");
  printf("-s 1, --unix-pattern-matching-case-sensitive\n");
  printf("  use case sensitive UNIX pattern matching\n");
  printf("-s 0, --no-unix-pattern-matching-case-sensitive\n");
  printf("  use case insensitive UNIX patter matching\n");
  printf("-/ 1, --translate-slash\n");
  printf("  translate /foo -> foo: and a//b -> a/b\n");
  printf("-/ 0, --no-translate-slash\n");
  printf("  do not translate /foo -> foo: and a//b -> a/b\n");
  printf("-m N, --membuf-limit N\n");
  printf("  files up to N bytes are cached in memory\n");
  printf("-b N, --fs-buf-factor N\n");
  printf("  N physical blocks map into 1 logical (stdio) block\n");
  printf("-i 1, --ignore-global-env\n");
  printf("  ignore global environment (ENV:)\n");
  printf("-i 0, --no-ignore-global-env\n");
  printf("  do not ignore global environment (ENV:)\n");
  printf("-e 1, --enforcer-hit\n");
  printf("  generate Enforcer hit when a trap occurs\n");
  printf("-e 0, --no-enforcer-hit\n");
  printf("  do not generate Enforcer hit when a trap occurs\n");
  printf("-v 0, --insert-disk-requester\n");
  printf("  suppress the \"Insert volume in drive\" requester\n");
  printf("-v 1, --no-insert-disk-requester\n");
  printf("  do not suppress the \"Insert volume in drive\" requester\n");
  printf("-u 0, --no-stack-usage\n");
  printf("  do not show the stack usage\n");
  printf("-u 1, --stack-usage\n");
  printf("  show the stack usage\n");
  printf("-f 0, --flush-library\n");
  printf("  prevent ixemul.library from being flushed from memory\n");
  printf("-f 1, --no-flush-library\n");
  printf("  do not prevent ixemul.library from being flushed from memory\n");
  printf("-n 0, --auto-detect\n");
  printf("  set networking support to auto detect\n");
  printf("-n 1, --no-networking\n");
  printf("  turn off networking support\n");
  printf("-n 2, --as225\n");
  printf("  use AS225 networking support\n");
  printf("-n 3, --amitcp\n");
  printf("  use AmiTCP networking support\n");
  printf("-p 0, --profile-program\n");
  printf("  profile the program only\n");
  printf("-p 1, --profile-task\n");
  printf("  profile while the task is running\n");
  printf("-p 2, --profile-always\n");
  printf("  always profile your program\n");
  printf("-d, --default\n");
  printf("  reset settings to defaults (other options are ignored)\n");
  printf("-L, --last-saved\n");
  printf("  reset settings from configuration file (other options are ignored)\n");
  printf("-M 0, --no-support-mufs\n");
  printf("  do not enable MuFS support\n");
  printf("-M 1, --support-mufs\n");
  printf("  enable MuFS support\n");
  printf("-S, --save\n");
  printf("  save new configuration\n");
  printf("-R, --report\n");
  printf("  display new configuration\n");
  printf("-V, --version\n");
  printf("  display program version information (other options are ignored)\n");
  printf("-h, --help\n");
  printf("  display this text\n");
  printf("\nUse no arguments to get the GUI\n");
}

void
display_config(void)
{
  printf((amigawildcard ? "A" : "Do not a"));
  printf("ccept AmigaOS wildcard notation,\n");
  printf("Case ");
  printf((cases ? "" : "in"));
  printf("sensitive UNIX pattern matching,\n");
  printf((translateslash ? "" : "do not "));
  printf("translate /, ");
  printf("membuf size = %d,\n", membuf);
  printf("%d physical block", blocks);
  printf(((blocks == 1) ? "" : "s"));
  printf(" build");
  printf(((blocks == 1) ? "s" : ""));
  printf(" one logical block (for stdio),\n");
  printf((ignoreenv ? "" : "do not "));
  printf("ignore global environment (ENV:),\n");
  printf((enforcerhit ? "" : "do not "));
  printf("generate Enforcer hit when a trap occurs,\n");
  printf((stackusage ? "" : "do not "));
  printf("show the stack usage,\n");
  printf((mufs ? "" : "do not "));
  printf("enable MuFS support,\n");
  printf((suppress ? "" : "do not "));
  printf("suppress the \"Insert volume in drive\" requester,\n");
  printf("use ");
  switch (networking)
  {
    case 0: printf("auto-detect"); break;
    case 1: printf("no"); break;
    case 2: printf("AS225"); break;
    case 3: printf("AmiTCP"); break;
    default: printf("unknown"); break;
  }
  printf(" networking support,\n");
  printf("profile ");
  switch (profilemethod)
  {
    case 0: printf("your program,\n"); break;
    case 1: printf("while your task is running,\n"); break;
    case 2: printf("always,\n"); break;
  }
  printf((noflush ? "" : "do not "));
  printf("prevent ixemul.library from being flushed from memory.\n");
}

int
parse_cli_commands(int argc, char *argv[])
{
  int c, error = 0, status;
  static int reset_defaults = 0, save_config = 0, help = 0, report = 0,
	     last = 0, version = 0;

  while (1) {
    int option_index = 0;
    static struct option long_options[] = {
      {"insert-disk-requester", 0, &suppress, 0},               /* -v */
      {"no-insert-disk-requester", 0, &suppress, 1},
      {"unix-pattern-matching-case-sensitive", 0, &cases, 1},   /* -s */
      {"no-unix-pattern-matching-case-sensitive", 0, &cases, 0},
      {"translate-slash", 0, &translateslash, 1},               /* -/ */
      {"no-translate-slash", 0, &translateslash, 0},
      {"allow-amiga-wildcard", 0, &amigawildcard, 1},           /* -a */
      {"no-allow-amiga-wildcard", 0, &amigawildcard, 0},
      {"flush-library", 0, &noflush, 0},                        /* -f */
      {"no-flush-library", 0, &noflush, 1},
      {"ignore-global-env", 0, &ignoreenv, 1},                  /* -i */
      {"no-ignore-global-env", 0, &ignoreenv, 0},
      {"stack-usage", 0, &stackusage, 1},                       /* -u */
      {"no-stack-usage", 0, &stackusage, 0},
      {"support-mufs", 0, &mufs, 1},                            /* -M */
      {"no-support-mufs", 0, &mufs, 0},
      {"enforcer-hit", 0, &enforcerhit, 1},                     /* -e */
      {"no-enforcer-hit", 0, &enforcerhit, 0},
      {"auto-detect", 0, &networking, 0},                       /* -n */
      {"no-networking", 0, &networking, 1},
      {"as225", 0, &networking, 2},
      {"amitcp", 0, &networking, 3},
      {"auto-detect", 0, &networking, 0},
      {"no-networking", 0, &networking, 1},
      {"as225", 0, &networking, 2},
      {"profile-program", 0, &profilemethod, 0},                /* -p */
      {"profile-task", 0, &profilemethod, 1},
      {"profile-always", 0, &profilemethod, 2},
      {"membuf-limit", 1, 0, 'm'},                              /* -m */
      {"fs-buf-factor", 1, 0, 'b'},                             /* -b */
      {"default", 0, &reset_defaults, 1},                       /* -d */
      {"save", 0, &save_config, 1},                             /* -S */
      {"report", 0, &report, 1},                                /* -R */
      {"last-saved", 0, &last, 1},                              /* -L */
      {"version", 0, &version, 1},                              /* -V */
      {"help", 0, &help, 1},                                    /* -h */
      {0, 0, 0, 0}
    };
    c = getopt_long (argc, argv, "v:s:x:a:/:f:i:e:u:m:n:b:p:dShRLM:V",
				 long_options, &option_index);
    if (c == EOF) {
      break;
    }
    switch (c) {
      case 0:
	break;
      case 'v':
	suppress = (atoi(optarg) ? 0 : 1);
	break;
      case 's':
	cases = (atoi(optarg) ? 1 : 0);
	break;
      case 'a':
	amigawildcard = (atoi(optarg) ? 1 : 0);
	break;
      case '/':
	translateslash = (atoi(optarg) ? 1 : 0);
	break;
      case 'f':
	noflush = (atoi(optarg) ? 0 : 1);
	break;
      case 'i':
	ignoreenv = atoi(optarg);
	break;
      case 'u':
	stackusage = atoi(optarg);
	break;
      case 'e':
	enforcerhit = atoi(optarg);
	break;
      case 'm':
	membuf = atoi(optarg);
	break;
      case 'n':
	networking = atoi(optarg);
	break;
      case 'p':
	profilemethod = atoi(optarg);
	break;
      case 'b':
	blocks = atoi(optarg);
	break;
      case 'd':
	reset_defaults = 1;
	break;
      case 'S':
	save_config = 1;
	break;
      case 'h':
	help = 1;
	break;
      case 'R':
	report = 1;
	break;
      case 'L':
	last = 1;
	break;
      case 'M':
	mufs = atoi(optarg);
	break;
      case 'V':
	version = 1;
	break;
      default:
	error = 1;
	break;
    }
  }
  if (optind < argc) {
    error = 1;
  }
  if (error) {
    fprintf(stderr, "\n");
    usage(argv[0]);
    status = FAILURE;
  }else{
    status = SUCCESS;
    if (version) {
      About();
    }else{
      if (help) {
	usage(argv[0]);
      }else{
	if (last) {
	  if (reset_defaults) {
	    fprintf(stderr,
  "--last-saved and --default specified together.  --default ignored.\n");
	  }
	  (void)LastSaved();
	}
	if (reset_defaults) {
	  Defaults();
	}
	if (save_config) {
	  Save();
	}else{
	  Use();
	}
	if (report) {
	  display_config();
	}
      }
    }
  }
  return status;
}
