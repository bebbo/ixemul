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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ix.h>
#include "ixemul.h"
#include "ixprefs.h"

void cleanup(void);

static int (*OpenLibraries)(void) = NULL;
static void (*CloseLibraries)(void) = NULL;
static void (*Cleanup)(void) = NULL;
static int (*GUI)(void) = NULL;

int translateslash, membuf, blocks, cases, suppress,
    amigawildcard, noflush, ignoreenv, networking, enforcerhit,
    profilemethod, stackusage, mufs;

int advanced = FALSE;
int watchAvailMem, catchfailedallocs, killappallocerr;

static int in_wb = FALSE;

/// keeping old version for Legacy reasons..we are updating 12 years later(!)
//char ixprefs_version[] = "$VER: ixprefs 2.7 (15.08.97)";
char ixprefs_version[] = "$VER: ixprefs 2.8 (29.05.2009)";

int
main(int argc, char *argv[])
{
  long status = 0;

  switch (ix_os) {
#ifndef NO_AMIGAOS_SUPPORT
    case OS_IS_AMIGAOS:
      OpenLibraries = OpenAmigaOSLibraries;
      CloseLibraries = CloseAmigaOSLibraries;
      Cleanup = AmigaOSCleanup;
      GUI = AmigaOSGUI;
      break;
#endif /* NO_AMIGAOS_SUPPORT */
#ifndef NO_POS_SUPPORT
    case OS_IS_POS:
      OpenLibraries = OpenPOSLibraries;
      CloseLibraries = ClosePOSLibraries;
      Cleanup = POSCleanup;
      GUI = POSGUI;
      break;
#endif /* NO_POS_SUPPORT */
    default:
      break;
  }

  if (argc < 1) {
    in_wb = TRUE;
  }
  if (ix_get_long(IXID_VERSION, 0) < MIN_IXEMUL_VERSION_SUPPORTED) {
    ShowRequester("This program requires ixemul.library version %ld or higher",
		  MIN_IXEMUL_VERSION_SUPPORTED, "EXIT");
    return FAILURE;
  }

  LoadSettings();       /* load ixprefs settings from ixemulbase */
  
  if (argc >= 2) {
    in_wb = FALSE;
    return parse_cli_commands(argc, argv);
  }else{
    in_wb = TRUE;
  }

  if (in_wb && GUI == NULL) {
    ShowRequester("No GUI available. Use the CLI interface", 0, "EXIT");
    return FAILURE;
  }

  atexit(cleanup);

  status = OpenLibraries();
  if (status != 0) {
    return FAILURE;
  }

#ifndef NO_POS_SUPPORT
  if (ix_os == OS_IS_POS) {
    ShowRequester("GUI not implemented under p.OS", 0, "EXIT");
    return FAILURE;
  }
#endif /* NO_POS_SUPPORT */

  return GUI();
}

void
cleanup()
{
  Cleanup();
  CloseLibraries();
}

void
ShowRequester(char *text, int arg, char *buttontext)
{
  if (in_wb){
    ix_req("ixprefs", buttontext, NULL, text, arg);
  }else{
    fprintf(stderr, text, arg);
    fprintf(stderr, "\n");
  }
}
