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
 */

/*
 * Glue for backward compatibility with code compiled by GCC with
 * PCC_STATIC_STRUCT_RETURN defined (i.e., upto and including GCC 2.7.2 Geek Gadgets
 * snapshot 960902).
 */
asm(" \n\
.lcomm LF0,8 \n\
.text \n\
	.even \n\
.globl	___obsolete_div \n\
___obsolete_div: \n\
	movel	#LF0,a1 \n\
	jmp	_div \n\
\n\
.lcomm LF1,8 \n\
.globl	___obsolete_ldiv \n\
___obsolete_ldiv: \n\
	movel	#LF1,a1 \n\
	jmp	_ldiv \n\
\n\
.lcomm LF2,4 \n\
.globl	___obsolete_inet_makeaddr \n\
___obsolete_inet_makeaddr: \n\
	movel	#LF2,a1 \n\
	jmp	_inet_makeaddr \n\
");
