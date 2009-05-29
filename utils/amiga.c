/*
    Ixprefs v.2.6--ixemul.library configuration program
    Copyright © 1995,1996 Kriton Kyrimis

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURAmigaOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef NO_AmigaOS_SUPPORT

#include <stdio.h>
#include <graphics/gfxbase.h>
#include <intuition/intuitionbase.h>
#include <exec/libraries.h>
#include <dos/dos.h>
#include <proto/exec.h>
#include <proto/intuition.h>
#include <proto/gadtools.h>
#include <proto/graphics.h>
#include "ixprefs.h"
#include "amiga_gui.h"

struct GfxBase *GfxBase = NULL;
struct IntuitionBase *IntuitionBase = NULL;
struct Library *GadToolsBase = NULL;

int
OpenAmigaOSLibraries(void)
{
  GfxBase = (struct GfxBase *)OpenLibrary("graphics.library", 0L);
  if (!GfxBase) {
    ShowRequester("Cannot open graphics.library", 0, "EXIT");
    return 1;
  }
  IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 0L);
  if (!IntuitionBase) {
    ShowRequester("Cannot open intuition.library", 0, "EXIT");
    return 1;
  }
  GadToolsBase = OpenLibrary("gadtools.library", 0L);
  if (!GadToolsBase) {
    ShowRequester("Cannot open gadtools.library", 0, "EXIT");
    return 1;
  }
  return 0;
}

void
CloseAmigaOSLibraries(void)
{
  if (GadToolsBase) {
    CloseLibrary(GadToolsBase);
    GadToolsBase = NULL;
  }
  if (IntuitionBase) {
    CloseLibrary((struct Library *)IntuitionBase);
    IntuitionBase = NULL;
  }
  if (GfxBase) {
    CloseLibrary((struct Library *)GfxBase);
    GfxBase = NULL;
  }
}

void
AmigaOSCleanup(void)
{
  CloseixprefsWindow();
  CloseDownScreen();
}

void
Check(int which)
{
  ixprefsGadgets[which]->Flags |= GFLG_SELECTED;
}

void
UnCheck(int which)
{
  ixprefsGadgets[which]->Flags &= ~GFLG_SELECTED;
}

void
ShowChecked(int which, int checkit)
{
  if (checkit) {
    Check(which);
  }else{
    UnCheck(which);
  }
  RefreshGList(ixprefsGadgets[which], ixprefsWnd, NULL, 1);
}

void
ShowNum(int which, int num)
{
  sprintf(GetString(ixprefsGadgets[which]), "%d", num);
  GetNumber(ixprefsGadgets[which]) = num;
  RefreshGList(ixprefsGadgets[which], ixprefsWnd, NULL, 1);
}

void
ShowCycle(int which, int num)
{
  GT_SetGadgetAttrs(ixprefsGadgets[which], ixprefsWnd, NULL,
		    GTCY_Active, num, TAG_DONE);
  RefreshGList(ixprefsGadgets[which], ixprefsWnd, NULL, 1);
}

void
DisplayPrefs(void)
{
  if (Scr == NULL)
    return;
  ShowChecked(GDX_amigawildcard, amigawildcard);
  ShowChecked(GDX_case, cases);
  if (advanced) {
    ShowChecked(GDX_translateslash, translateslash);
  }
  ShowNum(GDX_membuf, membuf);
  ShowNum(GDX_blocks, blocks);
  ShowChecked(GDX_suppress, suppress);
  ShowChecked(GDX_noflush, noflush);
  ShowChecked(GDX_ignoreenv, ignoreenv);
  ShowChecked(GDX_stackusage, stackusage);
  if (advanced) {
    ShowChecked(GDX_enforcerhit, enforcerhit);
  }
  ShowChecked(GDX_mufs, mufs);
  ShowCycle(GDX_networking, networking);
  ShowCycle(GDX_profilemethod, profilemethod);
  ShowChecked(GDX_watchAvailMem, watchAvailMem);
  ShowChecked(GDX_catchfailedallocs, catchfailedallocs);
  ShowChecked(GDX_killappallocerr, killappallocerr);
}

int
AmigaOSGUI(void)
{
  long status;

  status = SetupScreen();
  if (status != 0) {
    ShowRequester("SetupScreen failed, status = %ld", status, "EXIT");
    return FAILURE;
  }
  ixprefsTop=Scr->BarHeight+1;
  status = OpenixprefsWindow();
  if (status != 0) {
    ShowRequester("OpenixprefsWindow failed, status = %ld", status, "EXIT");
    return FAILURE;
  }

  DisplayPrefs();
  if (!advanced) {
    EraseGadget(ixprefsWnd, ixprefsGadgets[GDX_translateslash]);
    EraseGadget(ixprefsWnd, ixprefsGadgets[GDX_enforcerhit]);
  }

  while (1) {
    WaitPort(ixprefsWnd->UserPort);
    status = HandleixprefsIDCMP();
    if (status == 0) {
      return RETURN_OK;
    }
  }
  return SUCCESS;
}

void
EraseGadget(struct Window *win, struct Gadget *gad)
{
  int ht;

  RemoveGadget(win, gad);
  EraseRect(win->RPort,
	    gad->LeftEdge,
	    gad->TopEdge,
	    gad->LeftEdge + gad->Width - 1,
	    gad->TopEdge + gad->Height - 1);
  if (gad->GadgetText->ITextFont) {
    ht = gad->GadgetText->ITextFont->ta_YSize;
  }else{
    ht = win->IFont->tf_YSize;
  }
#if 0
  SetAPen(win->RPort, 255);
  RectFill(win->RPort,
	    gad->LeftEdge + gad->GadgetText->LeftEdge,
	    gad->TopEdge + gad->GadgetText->TopEdge,
	    gad->LeftEdge + gad->GadgetText->LeftEdge +
	      IntuiTextLength(gad->GadgetText) - 1,
	    gad->TopEdge + gad->GadgetText->TopEdge + ht - 1);
  SetAPen(win->RPort, 1);
#else
  EraseRect(win->RPort,
	    gad->LeftEdge + gad->GadgetText->LeftEdge,
	    gad->TopEdge + gad->GadgetText->TopEdge,
	    gad->LeftEdge + gad->GadgetText->LeftEdge +
	      IntuiTextLength(gad->GadgetText) - 1,
	    gad->TopEdge + gad->GadgetText->TopEdge + ht - 1);
#endif
}

#endif /* NO_AmigaOS_SUPPORT */
