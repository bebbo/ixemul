/*
 *  This file is part of the ixemul package for the Amiga.
 *  Copyright (C) 1994 Rafael W. Luebbert
 *  Copyright (C) 1997 Hans Verkuil
 *
 *  This source is placed in the public domain.
 */

asm("
	.globl	_KPrintF

KPutChar:
	movel	a6,sp@-
	movel	4:W,a6
	jsr	a6@(-516:W)
	movel	sp@+,a6
	rts

KDoFmt:
	movel	a6,sp@-
	movel	4:W,a6
	jsr	a6@(-522:W)
	movel	sp@+,a6
	rts

_KPrintF:
	lea	sp@(4),a1
	movel	a1@+,a0
	movel	a2,sp@-
	lea	KPutChar,a2
	jbsr	KDoFmt
	movel	sp@+,a2
	rts
");
