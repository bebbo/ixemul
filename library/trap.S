/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1991, 1992  Markus M. Wild
 *  Portions Copyright (C) 1994 Rafael W. Luebbert
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
 *  $Id: trap.s,v 1.1 1994/06/19 15:17:35 rluebbert Exp $
 *
 *  $Log: trap.s,v $
 *  Revision 1.1  1994/06/19  15:17:35  rluebbert
 *  Initial revision
 *
 */

	.globl	_trap_00
	.globl	_restore_00
	.globl	_sup00_do_sigresume
	.globl	_sup00_do_sigreturn
	.globl	_sup00_do_sigreturn_ssp
	.globl	_trap_20
	.globl	_restore_20
	.globl	_sup20_do_sigresume
	.globl	_sup20_do_sigreturn
	.globl	_sup20_do_sigreturn_ssp
	.globl	_supervisor
	.globl	_do_sigreturn
	.globl	_launch_glue
	.globl	_addupc
	.globl	_resetfpu
	.globl	_install_vector
	.globl	_restore_vector
	.globl  _vector_install_count
	.globl	_vector_old_pc
	.globl	_vector_nop

	| struct offsets are determined by create_defines
#include "ix_internals.h"

	| This is the trap processing function for the mc68000.
	| Things are quite easy here, just save the general purpose registers
	| and the pc/sr combo, call trap(), then restore the previous
	| context and return
_trap_00:
	movel	a5,sp@-		| need a scratch register
	movel	usp,a5		| get usp
	movel	a5,a5@(-10)	| store usp
	lea	a5@(-10),a5	| make room for sr, pc and usp.
	moveml	d0-d7/a0-a6,a5@-| store registers on usp
	movel	sp@+,a5@(0x34)	| insert the saved a5 into the saveset
	movel	sp@+,d1		| remember trapnumber
	movew	sp@+,a5@(0x44)	| copy SR
	movel	sp@+,d2		| remember and
	movel	d2,a5@(0x40)	| copy (offending?) PC
	movel	a5,a3		| save pointer to registers

	movel	#0,a5@-		| pass NULL pointer to stored fpu registers
	movel	a3,a5@-		| pass pointer to stored registers
	movel	d2,a5@-		| pass offending PC
	addl	d1,d1		| convert the passed trap number into a fake
	addl	d1,d1		| 68020 frame format word
	movel	d1,a5@-		| pass frame format word
	|
	| pass return address and ssp-value on userstack
	| This happens for the same reason as we have a glue_launch entry.
	| trap cleans up these 8 bytes on the user stack itself
	movel	sp,a5@-
	movel	#_restore_00,a5@-
	movel	a5,usp		| and remember current value of usp (a5)

	| fine, now process this trap. This might (doesn't have to) push
	| additional frames. If not, we just return where the exception 
	| took place (and probably will again...)

	jsr	_trap		| to the higher level C trap processor 

_restore_00:
	movel	usp,a5		| get usp
	lea	a5@(16),a5	| skip trap argument
	movel	a5@(0x3c),a1	| get stored usp
	movel	a1,usp
	
	| set up the original supervisor stack frame
	movel	a5@(0x40),sp@-	| copy PC
	movew	a5@(0x44),sp@-	| and SR
	moveml	a5@,d0-d7/a0-a6	| and the other registers
	rte



	| This is the trap processor for the mc68020 and above, paired with
	| an fpu (don't *need* an fpu though).
	| What is done: start is same as with 68000, but then the complete
	| additional exception frame is saved on the usp, together with the
	| fpu state. Then trap() is called, and then the previous context
	| is restored (involves copying back the frame from the usp over to the ssp)
_trap_20:
	movel	a5,sp@		| nuke the trap number, we use the frame format word
	movel	usp,a5		| get usp
	movel	a5,a5@(-10)	| store usp
	lea	a5@(-10),a5	| make room for sr, pc and usp.
	moveml	d0-d7/a0-a6,a5@-| store registers on usp
	movel	sp@+,a5@(0x34)	| insert the saved a5 into the saveset
	movew	sp@+,a5@(0x44)	| copy SR
	movel	sp@+,d2		| remember and
	movel	d2,a5@(0x40)	| copy (offending?) PC
	movel	a5,a3		| save pointer to registers

	| find out more about the frame (according to the MC68030 user manual)
	clrl	d1
	movew	sp@+,d1		| remember frame format word
	movew	d1,d0
	andw	#0xf000,d0
	beq	Lfmt_S0		| S0
	cmpw	#0x1000,d0
	beq	Lfmt_S1		| S1 this (interrupt) frame shouldn't be here...
	cmpw	#0x2000,d0
	beq	Lfmt_S2		| CHK{2},cpTRAPcc,TRAPV,Trace,Div0,MMUcfg,cp post instr
	cmpw	#0x9000,d0
	beq	Lfmt_S9		| cp mid instr,main det prot viol,int during cp instr
	cmpw	#0xa000,d0
	beq	Lfmt_SA_SB	| address or bus error, short and long frame
	cmpw	#0xb000,d0
	bne	Lfmt_S0		| ??? frame, this will probably not fully cleanup sp..

Lfmt_SA_SB:
	| this part (upto Lbe10) inspired by locore.s in sys/hp300/ of BSD4.3-reno
	movew	sp@(2),d0	| grab SSW for fault processing
	btst	#12,d0		| RB set?
	beq	LbeX0		| no, test RC
	bset	#14,d0		| yes, must set FB
	movew	d0,sp@(2)	| for hardware too
LbeX0:
	btst	#13,d0		| RC set?
	beq	LbeX1		| no, skip
	bset	#15,d0		| yes, must set FC
	movew	d0,sp@(2)	| for hardware too
LbeX1:
	btst	#8,d0		| data fault?
	beq	Lbe0		| no, check for hard cases
	movel	sp@(8),d2	| fault address is as given in frame
	bra	Lbe10		| thats it
Lbe0:
	btst	#12,d1		| long (type B) stack frame?
	bne	Lbe4		| yes, go handle
	btst	#14,d0		| no, can use saved PC. FB set?
	beq	Lbe3		| no, try FC
	addql	#4,d2		| yes, adjust address
	bra	Lbe10		| done
Lbe3:
	btst	#15,d0		| FC set?
	beq	Lbe10		| no, done
	addql	#2,d2		| yes, adjust address
	bra	Lbe10		| done
Lbe4:
	movel	sp@(28),d2	| long format, use stage B address
	btst	#15,d0		| FC set?
	beq	Lbe10		| no, all done
	subql	#2,d2		| yes, adjust address
Lbe10:

	| now move the frame over to the usp (6/21 longwords remain)
	
	moveml	sp@+,d3-d7/a0	| may trash as many registers as I like, I saved
	moveml	d3-d7/a0,a5@-	| them already ;-) First copy 6 longs

	btst	#12,d1		| long (type B) stack frame?
	beq	Lfmt_S0		| nope, done

	moveml	sp@+,d3-d7/a0-a2 | first copy 8 longs
	moveml	d3-d7/a0-a2,a5@-
	moveml	sp@+,d3-d7/a0-a1 | plus 7 gives 15, plus already stored 6 is 21
	moveml	d3-d7/a0-a1,a5@-
	bra	Lfmt_S0		| finito

Lfmt_S9:
	movel	sp@+,a5@-	| S9 is an S2 plus 4 internal (word length) registers
	movel	sp@+,a5@-	| so store those registers, and fall into S2

Lfmt_S2:
	movel	sp@+,d2		| S2 contains the offending instruction address
				| and the frame format word
	movel	d2,a5@-		| we have the offending instruction address here

	| fall into

Lfmt_S0:
Lfmt_S1:
	movew	d1,a5@-		| and as the last thing store the frame format word

	|
	| now lets look at the fpu, if there is an fpu in the system
	|

	movel	#0,a4		| clear pointer to fpu registers
	tstl    _has_fpu        | do we have a fpu?
	beq	Lno_fpu
	fsave	a5@-		| dump the fpu state onto the usp
	moveb	a5@,d0		| and get the fpu state identifier
	beq	Lno_fpu		| null frame?

	fmoveml	fpcr/fpsr/fpi,a5@-	| push the fpu control registers and
	fmovemx	fp0-fp7,a5@-		| the fpu data registers
	movel	a5,a4		| store pointer to fpu registers

	movew	#-1,a5@-	| mark that there is fpu stuff on the stack
Lno_fpu:

	|
	| pass return address and ssp-value on userstack
	| This happens for the same reason as we have a glue_launch entry.
	| trap cleans up these 8 bytes on the user stack itself
	movel	a4,a5@-		| pass pointer to stored fpu registers
	movel	a3,a5@-		| pass pointer to stored registers
	movel	d2,a5@-		| pass offending PC
	movel	d1,a5@-		| pass frame format word

	movel	sp,a5@-
	movel	#_restore_20,a5@-

	movel	a5,usp		| set the new value of the usp

	| that's it, phew.. now process this frame, and perhaps throw some
	| frames on it as well to deal with the signal
	
	jsr	_trap		| do distribution in C ;-)

_restore_20:
	
	|
	| restore the saved stack frame from the usp, and copy the necessary
	| parts over to the ssp
	|

	movel	usp,a5
	lea	a5@(16),a5	| skip trap() arguments
	
	| first deal with fpu stuff, if there's an fpu

        tstl    _has_fpu        | do we have a fpu?
	beq	Lno_fpu2
	tstb	a5@
	beq	Lrst_fpu_frame	| there's only the null frame, go and restore it

	lea	a5@(2),a5	| skip fpu indicator
	fmovemx	a5@+,fp0-fp7		| restore fpu data registers and
	fmoveml	a5@+,fpcr/fpsr/fpi	| fpu control registers
	
Lrst_fpu_frame:
	frestore a5@+		| and restore the internal fpu state
Lno_fpu2:
	movew	a5@+,d1		| get frame format word
	movew	d1,d0
	andw	#0xf000,d0
	beq	Lrfmt_S0	| S0
	cmpw	#0x1000,d0
	beq	Lrfmt_S1	| S1
	cmpw	#0x2000,d0
	beq	Lrfmt_S2	| S2
	cmpw	#0x9000,d0
	beq	Lrfmt_S9	| S9
	cmpw	#0xa000,d0
	beq	Lrfmt_SA	| SA
	cmpw	#0xb000,d0
	bne	Lrfmt_S0	| ??? frame

Lrfmt_SB:
	moveml	a5@+,d3-d7/a0-a2
	moveml	d3-d7/a0-a2,sp@-
	moveml	a5@+,d3-d7/a0-a1
	moveml	d3-d7/a0-a1,sp@- | copy 15 longs

Lrfmt_SA:
	movel	a5@+,sp@-	| copy  3 longs
	movel	a5@+,sp@-
	movel	a5@+,sp@-
	
Lrfmt_S9:
	movel	a5@+,sp@-	| copy  2 longs
	movel	a5@+,sp@-

Lrfmt_S2:
	movel	a5@+,sp@-	| copy  1 long

Lrfmt_S1:
Lrfmt_S0:
	movew	d1,sp@-		| insert frame format word
	movel	a5@(0x40),sp@-	| copy PC
	movew	a5@(0x44),sp@-	| and SR
	
	movel	a5@(0x3c),a1	| get the stored usp
	movel	a1,usp
	
	moveml	a5@,d0-d7/a0-a6	| restore the cpu registers
	rte			| that's it (finally) .. 

	|
	| jump to the given argument in supervisor mode
	| does NOT return
	|

_supervisor:
	movel	sp@(4),a5	| where to go in supervisor
	movel	sp@(8),a0	| with this signal context
	movel	a6,a1		| store a6
	movel	4:w,a6
	jmp	a6@(-0x1e)	| do it (Supervisor() system call)

	|
	| restore signal context
	| argument comes in usp
	|
	| mc68020 entry
_sup20_do_sigresume:
	jsr	_resume_signal_check
	bra	_sup20_do_sigreturn

_sup20_do_sigreturn_ssp:	| entry via jsr from supervisor mode
	movel	sp@(4),sp	| set ssp
_sup20_do_sigreturn:
	lea	sp@(-8),sp	| make room for an exception frame
	movew	#0x20,sp@(6)	| fake format word
	bra	fromsup_sigreturn

	| mc68000 entry
_sup00_do_sigresume:
	jsr	_resume_signal_check
	bra	_sup00_do_sigreturn

_sup00_do_sigreturn_ssp:
	movel	sp@(4),sp	| set ssp
_sup00_do_sigreturn:
	lea	sp@(-6),sp	| make room for an exception frame

fromsup_sigreturn:
	moveml	d0/d1/a0/a1,sp@-

	movel	usp,a0		| get signal context
	bra	resume_sigreturn

	| Supervisor() entry (already comes on exception frame)
_do_sigreturn:
	| make the sigreturn() function preserve all registers
	moveml	d0/d1/a0/a1,sp@-

	movel	a1,a6		| restore a6, a0 already contains the signal context

resume_sigreturn:
        movel   a0,sp@-
        jsr     _get_current_task       | get current task
        movel   sp@+,a0
        movel   d0,a1
	movel	a1@(USERPTR_OFFSET),a1	| (struct user *)
	
	| ok, now a0:sc, a1:u

	movel	a0@+,a1@(U_ONSTACK_OFFSET)	| u.u_onstack = sc->sc_onstack
	movel	a0@+,a1@(P_SIGMASK_OFFSET)	| u.p_sigmask = sc->sc_mask
	movel	a0@+,a1		| usp = sc->sc_sp
	movel	a1,usp
	movel	a0@+,a5		| fp  = sc->sc_fp

	movel	d0,a1		| get back current task
	lea	a0@(2),a0	| skip unused part of sc_ap
	movew	a0@,a1@(IDNESTPTR_OFFSET)	| store IDNestCnt,TDNestCnt
	movel	4:w,a1
	movew	a0@+,d1		| get IDNestCnt and TDNestCnt
	movew	d1,a1@(0x126)	| store them in SysBase
	tstb	a1@(0x126)
	bmi	Lenable
Ldisabled:
	movew	#0x4000,0xdff09a	| disable interrupts
	bra	Lint_twiddle
Lenable:
	movew	#0xc000,0xdff09a	| enable interrupts
Lint_twiddle:
	| set pc and sr in current exception frame
	movel	a0@+,sp@(2+4*4)	| set PC
	movew	a0@(2),sp@(4*4)	| and SR
	moveml	sp@+,d0/d1/a0/a1
	rte



	|
	| launch_glue is used to invoke the sig_launch handler. We have to care to
	| clean the supervisor stack, if we should call a signal handler from
	| the launch handler.
	| The bad thing is, that doing this right needs information on how
	| the tc_Launch entry is called from the OS. Thus I pass two parameters
	| on the user stack, the value of the `virgin' ssp and the value of
	| a4, which happens to contain the address of the context restore
	| function.
	| It is assumed, that tc_Launch is called via 
	| ... jsr sub
	| ...
	| sub:jsr tc_Launch
	| Thus we have to backup the sp by two jsr's, which is 8.

_launch_glue:
        jsr     _get_current_task
        movel   d0,a0
	movel	4:w,a4
        movel   a4@(560),a4	| ex_LaunchPoint
	movel	a0@(SPREGPTR_OFFSET),a0	| usp isn't setup correctly, do it now from tc_SPReg
	movel	sp,a0@-
	addl	#8,a0@
	movel	a4,a0@-
	movel	a0,usp
	
	jsr	_sig_launch	| sig_launch `(void *pc_ret, *ssp_ret)' (`' on usp)

	| sig_launch() already corrects the usp to pop those two arguments
	rts

/*
 * update profiling information for the user
 * addupc(pc, &u.u_prof, ticks)
 */
_addupc:
	movl	a2,sp@-			| scratch register
	movl	sp@(12),a2		| get &u.u_prof
	movl	sp@(8),d0		| get user pc
	subl	a2@(8),d0		| pc -= pr->pr_off
	jlt	Lauexit			| less than 0, skip it
	movl	a2@(12),d1		| get pr->pr_scale
	lsrl	#1,d0			| pc /= 2
	lsrl	#1,d1			| scale /= 2

	movel	d1,sp@-
	movel	d0,sp@-
	jsr	___mulsi3
	addqw	#8,sp

	moveq	#14,d1
	lsrl	d1,d0			| pc >>= 14
	bclr	#0,d0			| pc &= ~1
	cmpl	a2@(4),d0		| too big for buffer?
	jge	Lauexit			| yes, screw it
	addl	a2@,d0			| no, add base

	movel	d0,a0
	movew	a0@,d0

	addw	sp@(18),d0		| add tick to current value

	movew	d0,a0@

Lauexit:
	movl	sp@+,a2			| restore scratch reg
	rts

Lreset_fpu:
	clrl	sp@-			| prepare 060 fpu null state frame
	clrl	sp@-			| needs 2 additional longs on stack
	clrl	sp@-
	frestore sp@+

	| Note that after using frestore with a null state frame we have
	| to set up the control register to restore rounding to truncation
	| rather than round-to-nearest, as required by the ANSI C standard.

	moveq	#72,d0
	addl	d0,d0
	fmovel	d0,fpcr
	rte

_resetfpu:
	movel	a6,sp@-
	movel	a5,sp@-
	movel	4:w,a6

	tstl    _has_68060_or_up       	| do we have a 68060?
	lea	Lreset_fpu,a5
	jne	L_skip
	addqw	#4,a5
L_skip:
	jsr	a6@(-30)		| Supervisor()

	movel	sp@+,a5
	movel	sp@+,a6
	rts

| See the Enforcer manual for the original assembler code of the
| following code.

| This is the new bus error handler for use with Enforcer & GDB.
| There are two entry points, one for the 68020 and 68030, and one
| for the 68040.

_vector_nop:
	nop

| make sure next data is on a 4 byte boundary!

	.align	2

OldVector:
	.long	0			| One long word
_vector_install_count:
	.long	0			| use count
_vector_old_pc:
	.long	0			| store old pc

_new_vector_060:
	cmpl	#4,sp@(8)		| 68060
	jeq	TraceSkip		| If AbsExecBase, OK
	jra	TraceProcess

_new_vector_040:
	cmpl	#4,sp@(20)		| 68040
	jeq	TraceSkip		| If AbsExecBase, OK
	jra	TraceProcess

_new_vector:
	cmpl	#4,sp@(16)
	jeq	TraceSkip		| If AbsExecBase, OK

| Check if the current Task is running under ixemul control and
| is being traced by ixemul.

TraceProcess:
	moveml	a0/a1/d0/d1,sp@-	| Save this...

        jsr     _get_current_task       | get current task
        movel   d0,a0
	movel	a0@(USERPTR_OFFSET:W),d0| UserData -> (struct user *)
	movel	d0,a0
	jeq	NoTrace			| Not ixemul
	movel	a0@,d0			| get u_ixbase
	cmpl	_ixemulbase,d0		| == ixemulbase?
	jne	NoTrace			| Nope, not an ixemul program
	btst	#4,a0@(P_FLAG_OFFSET+3)	| Test STRC flag
	beq	NoTrace
	btst	#7,sp@(16:W)		| Are we already single stepping?
	jne	NoTrace
	bset	#7,sp@(16:W)		| Set trace bit...
	lea	_vector_old_pc,a0
	tstl	a0@			| is this variable already in use?
	jne	NoTrace
	movel	sp@(18:W),a0@		| store old pc
	lea	_vector_nop,a0
	movel	a0,sp@(18:W)		| store new pc

NoTrace:
	moveml	sp@+,a0/a1/d0/d1	| Restore...

TraceSkip:
	movel	OldVector,sp@-		| Ready to return
	rts

| Call this function in Supervisor mode

_restore_vector:
	tstl    _has_68040_or_up       	| do we have a 68040 or up?
	lea	_new_vector,a0
	jeq	got_vector1
	tstl    _has_68060_or_up       	| do we have a 68060 or up?
	lea	_new_vector_040,a0
	jeq	got_vector1
	lea	_new_vector_060,a0

got_vector1:
	lea	_vector_install_count,a1
	tstl	a1@
	beq	restore_exit
	subql	#1,a1@			| decrease use count
	bne	restore_exit
	movec	vbr,a1			| get Vector Base Register
	cmpl	a1@(8:W),a0		| sanity check
	jne	restore_exit
	movel	OldVector,a1@(8:W)

restore_exit:
	rte

| Call this function in Supervisor mode

_install_vector:
	lea	_new_vector,a0
	tstl    _has_68040_or_up       	| do we have a 68040 or up?
	jeq	got_vector2
	lea	_new_vector_040,a0
	tstl    _has_68060_or_up       	| do we have a 68060 or up?
	lea	_new_vector_060,a0
	jeq	got_vector2

got_vector2:
	addql	#1,_vector_install_count | increase use count
	movec	vbr,a1			| get Vector Base Register
	cmpl	a1@(8:W),a0
	jeq	install_done		| sanity check
	movel	a1@(8:W),OldVector
	movel	a0,a1@(8:W)

install_done:
	rte
