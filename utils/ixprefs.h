/*
    Ixprefs v.2.8--ixemul.library configuration program
    Copyright © 1995,1996 Kriton Kyrimis
    Copyright © 2009 Diego Casorran

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

#define IXPREFS_VERSION "2.8"
#define CONFIGFILE "ENVARC:" IX_ENV_SETTINGS
#define ENVFILE "ENV:" IX_ENV_SETTINGS
#define MIN_IXEMUL_VERSION_SUPPORTED 47

#define SUCCESS 0
#define FAILURE 20

extern int translateslash, membuf, blocks, cases, suppress,
	   amigawildcard, noflush, ignoreenv, networking, enforcerhit,
	   profilemethod, stackusage, mufs;
extern int advanced;
extern int watchAvailMem, catchfailedallocs, killappallocerr;

extern void ShowRequester(char *, int, char *);
extern void About(void);
extern int LastSaved(void);
extern int Save(void);
extern int Use(void);
extern void Defaults(void);
extern void LoadSettings(void);
extern int parse_cli_commands(int argc, char *argv[]);

#ifndef NO_AMIGAOS_SUPPORT
#include <intuition/intuition.h>

extern void DisplayPrefs(void);
extern int OpenAmigaOSLibraries(void);
extern void CloseAmigaOSLibraries(void);
extern void AmigaOSCleanup(void);
extern int AmigaOSGUI(void);
extern void EraseGadget(struct Window *, struct Gadget *);
extern void ShowChecked(int, int);
#endif /* NO_AMIGAOS_SUPPORT */

#ifndef NO_POS_SUPPORT
extern int OpenPOSLibraries(void);
extern void ClosePOSLibraries(void);
extern void POSCleanup(void);
extern int POSGUI(void);
#endif /* NO_POS_SUPPORT */
