/*
 *  Source machine generated by GadToolsBox V2.0b
 *  which is (c) Copyright 1991-1993 Jaba Development
 *
 *  GUI Designed by : Kriton Kyrimis
 */

#include <exec/types.h>
#include <intuition/intuition.h>
#include <intuition/classes.h>
#include <intuition/classusr.h>
#include <intuition/imageclass.h>
#include <intuition/gadgetclass.h>
#include <libraries/gadtools.h>
#include <graphics/displayinfo.h>
#include <graphics/gfxbase.h>
#include <clib/exec_protos.h>
#include <clib/intuition_protos.h>
#include <clib/gadtools_protos.h>
#include <clib/graphics_protos.h>
#include <clib/utility_protos.h>
#include <string.h>
#include <clib/diskfont_protos.h>

#include <pragmas/exec_pragmas.h>
#include <pragmas/intuition_pragmas.h>
#include <pragmas/gadtools_pragmas.h>
#include <pragmas/graphics_pragmas.h>
#include <pragmas/utility_pragmas.h>

#include "amiga_gui.h"

struct Screen         *Scr = NULL;
UBYTE                 *PubScreenName = NULL;
APTR                   VisualInfo = NULL;
struct Window         *ixprefsWnd = NULL;
struct Gadget         *ixprefsGList = NULL;
struct Menu           *ixprefsMenus = NULL;
struct IntuiMessage    ixprefsMsg;
struct Gadget         *ixprefsGadgets[17];
UWORD                  ixprefsLeft = 0;
UWORD                  ixprefsTop = 11;
UWORD                  ixprefsWidth = 515;
UWORD                  ixprefsHeight = 146;
UBYTE                 *ixprefsWdt = (UBYTE *)"ixprefs";
struct TextAttr       *Font, Attr;
UWORD                  FontX, FontY;
UWORD                  OffX, OffY;
struct TextFont       *ixprefsFont = NULL;

UBYTE *networking0Labels[] = {
	(UBYTE *)"Auto detect",
	(UBYTE *)"No networking",
	(UBYTE *)"AS225",
	(UBYTE *)"AmiTCP",
	NULL };

UBYTE *profilemethod0Labels[] = {
	(UBYTE *)"Only in program",
	(UBYTE *)"Only in task",
	(UBYTE *)"Profile always",
	NULL };

struct NewMenu ixprefsNewMenu[] = {
	NM_TITLE, (STRPTR)"Project", NULL, 0, NULL, NULL,
	NM_ITEM, (STRPTR)"Save", (STRPTR)"S", 0, 0L, (APTR)ixprefssave,
	NM_ITEM, (STRPTR)"Use", (STRPTR)"U", 0, 0L, (APTR)ixprefsuse,
	NM_ITEM, (STRPTR)NM_BARLABEL, NULL, 0, 0L, NULL,
	NM_ITEM, (STRPTR)"About", (STRPTR)"?", 0, 0L, (APTR)ixprefsabout,
	NM_ITEM, (STRPTR)NM_BARLABEL, NULL, 0, 0L, NULL,
	NM_ITEM, (STRPTR)"Quit", (STRPTR)"Q", 0, 0L, (APTR)ixprefsquit,
	NM_TITLE, (STRPTR)"Edit", NULL, 0, NULL, NULL,
	NM_ITEM, (STRPTR)"Reset to defaults", (STRPTR)"D", 0, 0L, (APTR)ixprefsreset,
	NM_ITEM, (STRPTR)"Last Saved", (STRPTR)"L", 0, 0L, (APTR)ixprefslast,
	NM_ITEM, (STRPTR)"Restore", (STRPTR)"R", 0, 0L, (APTR)ixprefsrestore,
	NM_END, NULL, NULL, 0, 0L, NULL };

UWORD ixprefsGTypes[] = {
	BUTTON_KIND,
	BUTTON_KIND,
	BUTTON_KIND,
	CHECKBOX_KIND,
	INTEGER_KIND,
	INTEGER_KIND,
	CHECKBOX_KIND,
	CHECKBOX_KIND,
	CHECKBOX_KIND,
	CHECKBOX_KIND,
	CHECKBOX_KIND,
	CYCLE_KIND,
	CHECKBOX_KIND,
	CYCLE_KIND,
	CHECKBOX_KIND,
	CHECKBOX_KIND,
	CHECKBOX_KIND
};

struct NewGadget ixprefsNGad[] = {
	4, 132, 63, 11, (UBYTE *)"Save", NULL, GD_savegad, PLACETEXT_IN, NULL, (APTR)savegadClicked,
	226, 132, 63, 11, (UBYTE *)"Use", NULL, GD_usegad, PLACETEXT_IN, NULL, (APTR)usegadClicked,
	448, 132, 63, 11, (UBYTE *)"Cancel", NULL, GD_cancelgad, PLACETEXT_IN, NULL, (APTR)cancelgadClicked,
	4, 15, 26, 11, (UBYTE *)"Translate /", NULL, GD_translateslash, PLACETEXT_RIGHT, NULL, (APTR)translateslashClicked,
	4, 70, 60, 12, (UBYTE *)"Membuf size", NULL, GD_membuf, PLACETEXT_RIGHT, NULL, (APTR)membufClicked,
	4, 57, 60, 12, (UBYTE *)"Physical blocks build one logical block (for stdio)", NULL, GD_blocks, PLACETEXT_RIGHT, NULL, (APTR)blocksClicked,
	215, 2, 26, 11, (UBYTE *)"Case sensitive pattern matching", NULL, GD_case, PLACETEXT_RIGHT, NULL, (APTR)caseClicked,
	4, 41, 26, 11, (UBYTE *)"Suppress the \"Insert volume in drive\" requester", NULL, GD_suppress, PLACETEXT_RIGHT, NULL, (APTR)suppressClicked,
	4, 2, 26, 11, (UBYTE *)"Allow Amiga wildcards", NULL, GD_amigawildcard, PLACETEXT_RIGHT, NULL, (APTR)amigawildcardClicked,
	215, 15, 26, 11, (UBYTE *)"Do not flush library", NULL, GD_noflush, PLACETEXT_RIGHT, NULL, (APTR)noflushClicked,
	215, 28, 26, 11, (UBYTE *)"Ignore global environment (ENV:)", NULL, GD_ignoreenv, PLACETEXT_RIGHT, NULL, (APTR)ignoreenvClicked,
	4, 84, 162, 13, (UBYTE *)"Network support", NULL, GD_networking, PLACETEXT_RIGHT, NULL, (APTR)networkingClicked,
	214, 70, 26, 11, (UBYTE *)"Create Enforcer hit on trap", NULL, GD_enforcerhit, PLACETEXT_RIGHT, NULL, (APTR)enforcerhitClicked,
	4, 99, 162, 13, (UBYTE *)"Profiling method", NULL, GD_profilemethod, PLACETEXT_RIGHT, NULL, (APTR)profilemethodClicked,
	4, 28, 26, 11, (UBYTE *)"Check stack usage", NULL, GD_stackusage, PLACETEXT_RIGHT, NULL, (APTR)stackusageClicked,
	310, 85, 26, 11, (UBYTE *)"Enable MuFS support", NULL, GD_mufs, PLACETEXT_RIGHT, NULL, (APTR)mufsClicked,
	4, 117, 26, 11, (UBYTE *)"Enable advanced ixprefs options", NULL, GD_advanced, PLACETEXT_RIGHT, NULL, (APTR)advancedClicked
};

ULONG ixprefsGTags[] = {
	(TAG_DONE),
	(TAG_DONE),
	(TAG_DONE),
	(TAG_DONE),
	(GTIN_Number), 0, (GTIN_MaxChars), 10, (TAG_DONE),
	(GTIN_Number), 0, (GTIN_MaxChars), 10, (TAG_DONE),
	(TAG_DONE),
	(TAG_DONE),
	(TAG_DONE),
	(TAG_DONE),
	(TAG_DONE),
	(GTCY_Labels), (ULONG)&networking0Labels[ 0 ], (TAG_DONE),
	(TAG_DONE),
	(GTCY_Labels), (ULONG)&profilemethod0Labels[ 0 ], (TAG_DONE),
	(TAG_DONE),
	(TAG_DONE),
	(TAG_DONE)
};

static UWORD ComputeX( UWORD value )
{
	return(( UWORD )((( FontX * value ) + 4 ) / 8 ));
}

static UWORD ComputeY( UWORD value )
{
	return(( UWORD )((( FontY * value ) + 4 ) / 8 ));
}

static void ComputeFont( UWORD width, UWORD height )
{
	Forbid();
	Font = &Attr;
	Font->ta_Name = (STRPTR)GfxBase->DefaultFont->tf_Message.mn_Node.ln_Name;
	Font->ta_YSize = FontY = GfxBase->DefaultFont->tf_YSize;
	FontX = GfxBase->DefaultFont->tf_XSize;
	Permit();

	OffX = Scr->WBorLeft;
	OffY = Scr->RastPort.TxHeight + Scr->WBorTop + 1;

	if ( width && height ) {
		if (( ComputeX( width ) + OffX + Scr->WBorRight ) > Scr->Width )
			goto UseTopaz;
		if (( ComputeY( height ) + OffY + Scr->WBorBottom ) > Scr->Height )
			goto UseTopaz;
	}
	return;

UseTopaz:
	Font->ta_Name = (STRPTR)"topaz.font";
	FontX = FontY = Font->ta_YSize = 8;
}

int SetupScreen( void )
{
	if ( ! ( Scr = LockPubScreen( PubScreenName )))
		return( 1L );

	ComputeFont( 0, 0 );

	if ( ! ( VisualInfo = GetVisualInfo( Scr, TAG_DONE )))
		return( 2L );

	return( 0L );
}

void CloseDownScreen( void )
{
	if ( VisualInfo ) {
		FreeVisualInfo( VisualInfo );
		VisualInfo = NULL;
	}

	if ( Scr        ) {
		UnlockPubScreen( NULL, Scr );
		Scr = NULL;
	}
}

int HandleixprefsIDCMP( void )
{
	struct IntuiMessage     *m;
	struct MenuItem         *n;
	int                     (*func)();
	BOOL                    running = TRUE;

	while( m = GT_GetIMsg( ixprefsWnd->UserPort )) {

		CopyMem(( char * )m, ( char * )&ixprefsMsg, (long)sizeof( struct IntuiMessage ));

		GT_ReplyIMsg( m );

		switch ( ixprefsMsg.Class ) {

			case    IDCMP_REFRESHWINDOW:
				GT_BeginRefresh( ixprefsWnd );
				GT_EndRefresh( ixprefsWnd, TRUE );
				break;

			case    IDCMP_CLOSEWINDOW:
				running = ixprefsCloseWindow();
				break;

			case    IDCMP_GADGETUP:
				func = ( void * )(( struct Gadget * )ixprefsMsg.IAddress )->UserData;
				running = func();
				break;

			case    IDCMP_MENUPICK:
				while( ixprefsMsg.Code != MENUNULL ) {
					n = ItemAddress( ixprefsMenus, ixprefsMsg.Code );
					func = (void *)(GTMENUITEM_USERDATA( n ));
					running = func();
					ixprefsMsg.Code = n->NextSelect;
				}
				break;
		}
	}
	return( running );
}

int OpenixprefsWindow( void )
{
	struct NewGadget        ng;
	struct Gadget   *g;
	UWORD           lc, tc;
	UWORD           wleft = ixprefsLeft, wtop = ixprefsTop, ww, wh;

	ComputeFont( ixprefsWidth, ixprefsHeight );

	ww = ComputeX( ixprefsWidth );
	wh = ComputeY( ixprefsHeight );

	if (( wleft + ww + OffX + Scr->WBorRight ) > Scr->Width ) wleft = Scr->Width - ww;
	if (( wtop + wh + OffY + Scr->WBorBottom ) > Scr->Height ) wtop = Scr->Height - wh;

	if ( ! ( ixprefsFont = OpenDiskFont( Font )))
		return( 5L );

	if ( ! ( g = CreateContext( &ixprefsGList )))
		return( 1L );

	for( lc = 0, tc = 0; lc < ixprefs_CNT; lc++ ) {

		CopyMem((char * )&ixprefsNGad[ lc ], (char * )&ng, (long)sizeof( struct NewGadget ));

		ng.ng_VisualInfo = VisualInfo;
		ng.ng_TextAttr   = Font;
		ng.ng_LeftEdge   = OffX + ComputeX( ng.ng_LeftEdge );
		ng.ng_TopEdge    = OffY + ComputeY( ng.ng_TopEdge );
		ng.ng_Width      = ComputeX( ng.ng_Width );
		ng.ng_Height     = ComputeY( ng.ng_Height);

		ixprefsGadgets[ lc ] = g = CreateGadgetA((ULONG)ixprefsGTypes[ lc ], g, &ng, ( struct TagItem * )&ixprefsGTags[ tc ] );

		while( ixprefsGTags[ tc ] ) tc += 2;
		tc++;

		if ( NOT g )
			return( 2L );
	}

	if ( ! ( ixprefsMenus = CreateMenus( ixprefsNewMenu, GTMN_FrontPen, 0L, TAG_DONE )))
		return( 3L );

	LayoutMenus( ixprefsMenus, VisualInfo, TAG_DONE );

	if ( ! ( ixprefsWnd = OpenWindowTags( NULL,
				WA_Left,        wleft,
				WA_Top,         wtop,
				WA_Width,       ww + OffX + Scr->WBorRight,
				WA_Height,      wh + OffY + Scr->WBorBottom,
				WA_IDCMP,       BUTTONIDCMP|CHECKBOXIDCMP|INTEGERIDCMP|CYCLEIDCMP|IDCMP_MENUPICK|IDCMP_CLOSEWINDOW|IDCMP_REFRESHWINDOW,
				WA_Flags,       WFLG_DRAGBAR|WFLG_DEPTHGADGET|WFLG_CLOSEGADGET|WFLG_SMART_REFRESH|WFLG_ACTIVATE,
				WA_Gadgets,     ixprefsGList,
				WA_Title,       ixprefsWdt,
				WA_ScreenTitle, "ixprefs",
				WA_PubScreen,   Scr,
				WA_AutoAdjust,  TRUE,
				WA_PubScreenFallBack,   TRUE,
				TAG_DONE )))
	return( 4L );

	SetMenuStrip( ixprefsWnd, ixprefsMenus );
	GT_RefreshWindow( ixprefsWnd, NULL );

	return( 0L );
}

void CloseixprefsWindow( void )
{
	if ( ixprefsMenus      ) {
		ClearMenuStrip( ixprefsWnd );
		FreeMenus( ixprefsMenus );
		ixprefsMenus = NULL;    }

	if ( ixprefsWnd        ) {
		CloseWindow( ixprefsWnd );
		ixprefsWnd = NULL;
	}

	if ( ixprefsGList      ) {
		FreeGadgets( ixprefsGList );
		ixprefsGList = NULL;
	}

	if ( ixprefsFont ) {
		CloseFont( ixprefsFont );
		ixprefsFont = NULL;
	}
}
