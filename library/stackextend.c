#define _KERNEL
#include "ixemul.h"

#include <exec/memory.h>
#include <signal.h>
#include <unistd.h>



#include "kprintf.h"

#ifdef TRACK_ALLOCS

#undef AllocMem
#undef FreeMem

#define AllocMem(x,y)    debug_AllocMem("stack",x,y)
#define FreeMem(x,y)     debug_FreeMem(x,y)

#endif

#define GUARD		0xabababab
#define HIT		(*(long*)0 = 0)

/*
 * Signal routines may want to benefit from stackextension too -
 * so make all the stack handling functions atomic.
 * FIXME: This seems to have the potential to break a Forbid() ?!?
 */
void atomic_on(sigset_t *old)
{
  usetup;

  *old = u.p_sigmask;
  u.p_sigmask = ~0;
/*  sigset_t fill;
  sigfillset(&fill);
  sigprocmask(SIG_SETMASK, &fill, &old);*/
}

void atomic_off(sigset_t old)
{
  usetup;

  u.p_sigmask = old;
//  sigprocmask(SIG_SETMASK, &old, NULL);
}


#ifdef NATIVE_MORPHOS

#define STK_UPPER_PPC   \
(u.u_ppc_stk_used != NULL ? u.u_ppc_stk_used->upper : u.u_ppc_org_upper)

#define STK_LOWER_PPC   \
(u.u_ppc_stk_used != NULL ? (void *)(u.u_ppc_stk_used + 1) : u.u_ppc_org_lower)

#define STK_UPPER_68K   \
(u.u_68k_stk_used != NULL ? u.u_68k_stk_used->upper : u.u_68k_org_upper)

#define STK_LOWER_68K   \
(u.u_68k_stk_used != NULL ? (void *)(u.u_68k_stk_used + 1) : u.u_68k_org_lower)

#define stk_safezone_68k    6144    /* into ixprefs? */
#define stk_minframe_68k    32768
#define stk_safezone_ppc    16000    /* into ixprefs? */
#define stk_minframe_ppc    200000

static void popframes_ppc(struct stackframe *sf, struct StackSwapStruct *sss);
static void popframes_68k(struct stackframe *sf, struct StackSwapStruct *sss);
static void pushframe_ppc(ULONG requiredstack, struct StackSwapStruct *sss, sigset_t *old, int startup);
static void pushframe_68k(ULONG requiredstack, struct StackSwapStruct *sss, sigset_t *old, int startup);
void __stkrst_f(void);
void __stkovf(void);
void __stkovf68k(void);


#define SAVE_68K_REGS \
	LONG old_d0 = REG_D0; \
	LONG old_d1 = REG_D1; \
	LONG old_d2 = REG_D2; \
	LONG old_d3 = REG_D3; \
	LONG old_d4 = REG_D4; \
	LONG old_d5 = REG_D5; \
	LONG old_d6 = REG_D6; \
	LONG old_d7 = REG_D7; \
	LONG old_a0 = REG_A0; \
	LONG old_a1 = REG_A1; \
	LONG old_a2 = REG_A2; \
	LONG old_a3 = REG_A3; \
	LONG old_a4 = REG_A4; \
	LONG old_a5 = REG_A5; \
	LONG old_a6 = REG_A6;

#define RESTORE_68K_REGS \
	REG_D0 = old_d0; \
	REG_D1 = old_d1; \
	REG_D2 = old_d2; \
	REG_D3 = old_d3; \
	REG_D4 = old_d4; \
	REG_D5 = old_d5; \
	REG_D6 = old_d6; \
	REG_D7 = old_d7; \
	REG_A0 = old_a0; \
	REG_A1 = old_a1; \
	REG_A2 = old_a2; \
	REG_A3 = old_a3; \
	REG_A4 = old_a4; \
	REG_A5 = old_a5; \
	REG_A6 = old_a6;

static int get_68k_stack_size(struct Process *proc, int stack)//seem not use on 68k 
{
  struct CommandLineInterface *CLI;

  if (stack < STACKSIZE)
    stack = STACKSIZE;

  /* NOTE: This is the most appropriate place for future support of
     user-specified, process-specific stacksizes. */

  CLI = BTOCPTR (proc->pr_CLI);

  if (stack <= (CLI ? CLI->cli_DefaultStack * 4 : proc->pr_StackSize))
    return 0;

  KPRINTF(("get_stack_size() = %ld\n", stack));

  return stack;
}


static int get_ppc_stack_size(struct Task *task, int stack)
{
  /*char *tmp;*/
  char buf[80];

  if (stack < STACKSIZE * 3)
    stack = STACKSIZE * 3;

  /* Don't use our getenv here, because u_environ is not
   * initialized yet. */
  /*if (tmp = getenv ("IXPPCSTACK"))
    {
      int stack_size = atoi (tmp);
      if (stack < stack_size)
	stack = stack_size;
    }*/
  if (GetVar("IXPPCSTACK", buf, sizeof(buf), 0) >= 0)
    {
      int stack_size = atoi (buf);
      if (stack < stack_size)
	stack = stack_size;
    }

  if (stack <= (int)task->tc_ETask->PPCSPUpper - (int)task->tc_ETask->PPCSPLower)
    return 0;

  KPRINTF(("get_stack_size() = %ld\n", stack));

  return stack;
}


void *stkrst(void *sp, void *callsp, void *d0)
{
  int cpsize = (char *)callsp - (char *)sp;
  struct stackframe *sf1, *sf2;
  struct StackSwapStruct sss;
  struct Task *me = SysBase->ThisTask;
  usetup;

  if (d0 >= STK_LOWER_PPC && d0 < STK_UPPER_PPC)
    return sp;

  KPRINTF(("stkrst(%lx, %lx, %lx, %lx)\n", sp, callsp, u.u_ppc_stk_used, d0));

  sf1 = u.u_ppc_stk_used;
  if (sf1 == NULL)
    return sp;

  if (*(unsigned *)me->tc_ETask->PPCSPLower != GUARD)
  {
    dprintf("**************** Stack overflow detected ********************\n");
    HIT;
  }

  for (;;)
  {
    sf2 = sf1->next;
    if (sf2 == NULL)
    {
      if (d0 < u.u_ppc_org_lower || d0 >= u.u_ppc_org_upper)
	return sp;
      break;
    }
    if (d0 >= (void *)(sf2 + 1) && d0 < sf2->upper) /* This stackframe fits */
      break;
    sf1 = sf2;
  }
  popframes_ppc(sf1, &sss);
  sss.stk_Pointer = (char *)d0 - cpsize;
  CopyMem(sp, sss.stk_Pointer, cpsize);
  *(int *)(sss.stk_Pointer + cpsize + 4) = *(int *)(callsp + 4);

  me->tc_ETask->PPCSPUpper = (void*)sss.stk_Upper;
  me->tc_ETask->PPCSPLower = sss.stk_Lower;

  KPRINTF(("new r1 = %lx, upper = %lx, lower = %lx, used = %lx\n", sss.stk_Pointer, sss.stk_Upper, sss.stk_Lower, u.u_ppc_stk_used));

  return sss.stk_Pointer;
}

/*
 * return to last stackframe
 */
void *stkrst_f(char *callsp, char *endsp)
{
  int cpsize = endsp - callsp;
  struct StackSwapStruct sss;
  struct Task *me = SysBase->ThisTask;
  unsigned long *p = (void *)callsp;
  usetup;

  KPRINTF(("stkrst_f(%lx, %lx, %lx)\n", callsp, endsp, u.u_ppc_stk_used));

  KPRINTF(("stack %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]));
  KPRINTF(("stack %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]));

  if (*(unsigned *)me->tc_ETask->PPCSPLower != GUARD)
  {
    dprintf("**************** Stack overflow detected ********************\n");
    HIT;
  }
  sss.stk_Pointer = (char *)u.u_ppc_stk_used->savesp - cpsize;
  CopyMem(callsp, sss.stk_Pointer, cpsize);
  popframes_ppc(u.u_ppc_stk_used, &sss);

  me->tc_ETask->PPCSPUpper = (void*)sss.stk_Upper;
  me->tc_ETask->PPCSPLower = sss.stk_Lower;

  KPRINTF(("new r1 = %lx, upper = %lx, lower = %lx, used = %lx\n", sss.stk_Pointer, sss.stk_Upper, sss.stk_Lower, u.u_ppc_stk_used));

  p = (void *)sss.stk_Pointer;
  KPRINTF(("stack %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]));
  KPRINTF(("stack %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]));

  return sss.stk_Pointer;
}

void *stkext_startup_ppc(int stack0, char *sp, sigset_t *old, int ret_offset)
{
  char *argtop;
  int cpsize;
  int stack;
  struct StackSwapStruct sss;
  struct Task *me = SysBase->ThisTask;
  usetup;

  KPRINTF(("stkext_startup_ppc(%ld, %lx)\n", stack0, sp));

  KPRINTF(("sp = %lx, upper = %lx, lower = %lx, limit = %lx\n",
	   sp, u.u_ppc_tc_spupper, u.u_ppc_tc_splower, u.u_ppc_stk_limit ? *u.u_ppc_stk_limit : NULL));

  if (!(stack = get_ppc_stack_size(me, stack0)))
    return sp;

  argtop = sp + u.u_ppc_stk_argbt;      /* Top of area with arguments */
  if ((void *)argtop > STK_UPPER_PPC)
    argtop = STK_UPPER_PPC;
  cpsize = argtop - sp;
  cpsize = (cpsize + 15) & ~15;

  pushframe_ppc(stack + 15, &sss, old, 1);
  sss.stk_Pointer = (void *)((int)sss.stk_Pointer & ~15);
  *(char **)&sss.stk_Pointer -= cpsize;
  CopyMem(sp, sss.stk_Pointer, cpsize);
  u.u_ppc_stk_used->savesp = sp + ret_offset - 4; /* store sp */

  *(void **)((char *)sss.stk_Pointer + ret_offset) = &__stkrst_f; /* set returnaddress */

  *(unsigned*)sss.stk_Lower = GUARD;

  me->tc_ETask->PPCSPUpper = (void *)sss.stk_Upper;
  me->tc_ETask->PPCSPLower = sss.stk_Lower;

  KPRINTF(("new r1 = %lx, upper = %lx, lower = %lx, used = %lx\n", sss.stk_Pointer, sss.stk_Upper, sss.stk_Lower, u.u_ppc_stk_used));

  return sss.stk_Pointer;
}

/*
 * Allocate a new stackframe with d0 bytes minimum, copy the callers arguments
 * and set his returnaddress (offset d1 from the sp when called) to stk_rst_f
 */
void *stkext_f(char *callsp, size_t d0, sigset_t *old, int frame_size, int prev_frame_size)
{
  char *argtop;
  char *new_r1;
  struct StackSwapStruct sss;
  struct Task *me = SysBase->ThisTask;
  unsigned long *p = (void *)callsp;
  int cpsize;
  usetup;

  KPRINTF(("stkext_f(%lx, %lx, %lx, %lx, %lx) limit = %lx frame_size 0x%x prev_frame_size 0x%x\n",
			  callsp, STK_UPPER_PPC, STK_LOWER_PPC, u.u_ppc_stk_used, d0,
			  u.u_ppc_stk_limit ? *u.u_ppc_stk_limit : NULL, frame_size, prev_frame_size));

  KPRINTF(("stack %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]));
  KPRINTF(("stack %08lx %08lx %08lx %08lx %08lx %08lx %08lx %08lx\n",
	p[8], p[9], p[10], p[11], p[12], p[13], p[14], p[15]));

  if ((void *)callsp >= STK_UPPER_PPC || (void *)callsp < STK_LOWER_PPC)
    return callsp; /* User intentionally left area of stackextension */

  argtop = callsp + frame_size + prev_frame_size + u.u_ppc_stk_argbt;      /* Top of area with arguments */
  if ((void *)argtop > STK_UPPER_PPC)
    argtop = STK_UPPER_PPC;
  cpsize = argtop - callsp - frame_size;

  /* FIXME: is "+ u.u_stk_argbt" really necessary? It's added in pushframe(), too. */
  pushframe_ppc(d0 + cpsize + frame_size + 15, &sss, old, 0);
  sss.stk_Pointer = (void *)(((int)sss.stk_Pointer & ~15) - ((cpsize + frame_size + 15) & ~15));
  new_r1 = (char *)sss.stk_Pointer;
  CopyMem(callsp, new_r1, frame_size & ~15);
  CopyMem(callsp + frame_size, new_r1 + (frame_size & ~15), cpsize);
  u.u_ppc_stk_used->savesp = callsp + frame_size + prev_frame_size; /* store sp */

  *(void **)new_r1 = new_r1 + (frame_size & ~15);

  if (prev_frame_size)
  {
    *(void **)(new_r1 + (frame_size & ~15)) = new_r1 + (frame_size & ~15) + prev_frame_size;
    *(void **)(new_r1 + (frame_size & ~15) + prev_frame_size + 4) = &__stkrst_f;
  }
  /*else
    *(void **)(new_r1 + (frame_size & ~15) + 4) = &__stkrst_f;*/

  *(unsigned*)sss.stk_Lower = GUARD;

  me->tc_ETask->PPCSPUpper = (void *)sss.stk_Upper;
  me->tc_ETask->PPCSPLower = sss.stk_Lower;

  KPRINTF(("new r1 = %lx, upper = %lx, lower = %lx, used = %lx\n", sss.stk_Pointer, sss.stk_Upper, sss.stk_Lower, u.u_ppc_stk_used));

  return sss.stk_Pointer;
}

asm("
	.globl  __stkext_f
	.type	__stkext_f,@function
__stkext_f:
/* called with:
    4(r1)   = caller's return address
    ctr     = caller
    r3..r10 = arguments for function
    r1 might not be aligned on 16 bytes boundary, so we align it. We reserve 64 bytes for our stack
    frame plus some padding.
*/
	andi.	11,1,15
	addi	11,11,64
	neg	12,11
	stwux	1,1,12
	stw     3,8(1)
	stw     4,12(1)
	stw     5,16(1)
	mfctr   4
	stw     6,20(1)
	mfcr    5
	stw     7,24(1)
	stw     8,28(1)
	stw     9,32(1)
	stw     10,36(1)
	stw	11,40(1)
	stw     4,48(1)
	stw     5,52(1)

	addi    3,1,60
	bl      atomic_on

	mr      3,1
	li      4,0
	addi    5,1,60
	lwz	6,40(1)
	li	7,0
	bl      stkext_f	/* stkextf returns an aligned new stack. It removes the padding
				   from our frame, so we just have 64 bytes now. */
	mr      1,3

	lwz     3,60(1)
	bl      atomic_off

	lis	3,__stkrst_f@ha
	lwz     4,48(1)
	addi	3,3,__stkrst_f@l
	lwz     5,52(1)
	lwz     6,20(1)
	mtlr    3
	lwz     7,24(1)
	lwz     8,28(1)
	mtctr   4
	lwz	3,4(11)
	lwz     9,32(1)
	lwz     10,36(1)
	mtcr    5
	lwz     3,8(1)
	lwz     4,12(1)
	lwz     5,16(1)
	addi	1,1,64
	bctr
__end__stkext_f:
	.size	__stkext_f,__end__stkext_f-__stkext_f


/* Attn: for ppc, this is actually alloca()
    r3 = size
    The same alignment comments as above apply, except we have a 16 bytes frame.
*/

	.globl  __stkext
	.type   __stkext,@function
__stkext:
	andi.	11,1,15

/*	beq+	l1
	li	12,0
	stw	0,0(12)
l1:*/

	mflr    0
	addi	11,11,16
	neg	12,11
	stw     0,4(1)
	stwux	1,1,12
	stw     3,8(1)

	addi    3,1,12
	bl      atomic_on

	lwz	7,0(1)		/* r6 = end of next frame */
	mr      3,1		/* r3 = sp */
	lwz     4,8(1)		/* r4 = size */
	addi	4,4,32
	sub     6,7,1           /* r6 = offset to the next frame */
	lwz	7,0(7)
	addi    5,1,12		/* r5 = sigset */
	sub	7,7,1
	sub	7,7,6		/* r7 = size to copy after our frame */
	bl      stkext_f	/* Aligns the stack and removes padding */
	mr      1,3

	lwz     3,12(1)
	bl      atomic_off

	lwz     0,20(1)
	lwz	4,8(1)
	lwz     5,16(1)
	mtlr    0
	subfic	4,4,16
	stwux	5,1,4
	blr
__end__stkext:
	.size	__stkext,__end__stkext-__stkext


	.globl  __stkext_startup
	.type	__stkext_startup,@function
__stkext_startup:
/* called with:
    r10 = function to call
    r11 = stack size
    r3... = arguments for function
*/
	stwu    1,-128(1)
	mflr    0
	stw     0,132(1)
	stmw    3,8(1)

	addi    3,1,124         /* 124(r1) = old signals */
	bl      atomic_on

	lwz     3,40(1)         /* r3 = stack size */
	mr      4,1             /* r4 = sp */
	addi    5,1,124         /* r5 = &old_sigs */
	li      6,132           /* r6 = &save_lr */
	bl      stkext_startup_ppc
	mr      1,3             /* set new sp */

	lwz     3,124(1)        /* r3 = old signals */
	bl      atomic_off

	lmw     3,8(1)          /* restore regs */
	lwz     0,132(1)        /* r0 = return address */
	mtctr   10              /* ctr = function to call */
	addi    1,1,128         /* restore frame */
	mtlr    0
	bctr
__end__stkext_startup:
	.size	__stkext_startup,__end__stkext_startup-__stkext_startup


	.globl  __stkrst
	.type   __stkrst,@function
__stkrst:
/* called with r5 = new sp */
	stwu    1,-128(1)
	mflr    0
	stmw    3,8(1)
	stw     0,132(1)

	addi    3,1,124
	bl      atomic_on

	mr      3,1		/* r3 = old sp */
	addi    4,1,128		/* r4 = call sp*/
	lwz     5,16(1)		/* r5 = new sp */
	bl      stkrst
	mr      1,3

	lwz     3,124(1)
	lwz     14,132(1)
	bl      atomic_off

	mtlr    14
	lmw     3,8(1)
	addi    1,1,128
	blr
__end__stkrst:
	.size	__stkrst,__end__stkrst-__stkrst


	.globl  __stkrst_f
	.type   __stkrst_f,@function
__stkrst_f:
	stwu    1,-32(1)
	stw     3,8(1)
	stw     4,12(1)

	addi    3,1,28
	bl      atomic_on

	mr      3,1
	addi    4,1,32
	bl      stkrst_f
	mr      1,3

	addi    3,1,28
	bl      atomic_off

	lwz     0,36(1)
	lwz     4,12(1)
	mtlr    0
	lwz     3,8(1)
	addi    1,1,32
	stw	3,0(2)
	blr
__end__stkrst_f:
	.size	__stkrst_f,__end__stkrst_f-__stkrst_f
");


void* stkrst68k(void *new_sp, void *old_sp)
{
  struct Task *me = SysBase->ThisTask;
  usetup;
  struct StackSwapStruct sss;
  struct stackframe *sf1, *sf2;
  sigset_t old;
  int ret;

  atomic_on(&old);

  ret = *(int *)old_sp;

  if (new_sp >= STK_LOWER_68K && new_sp < STK_UPPER_68K)
    goto end;

  KPRINTF(("stkrst(%lx) [68k]\n", new_sp));

  sf1 = u.u_68k_stk_used;
  if (sf1 == NULL)
    goto end;

  for (;;)
  {
    sf2 = sf1->next;
    if (sf2 == NULL)
    {
      if (new_sp < u.u_68k_org_lower || new_sp >= u.u_68k_org_upper)
	goto end;
      break;
    }
    if (new_sp >= (void *)(sf2 + 1) && new_sp < sf2->upper) /* This stackframe fits */
      break;
    sf1 = sf2;
  }
  popframes_68k(sf1, &sss);
  me->tc_SPUpper = (void *)sss.stk_Upper;
  me->tc_SPLower = sss.stk_Lower;
end:
  new_sp = (void *)((int)new_sp - 4); /* add the return address */
  *(int *)new_sp = ret;
  atomic_off(old);

  return new_sp;
}

static void _trampoline___stkrst(void)
{
  void *new_sp;
  void *old_sp;
  SAVE_68K_REGS
  new_sp = (void *)REG_D0;
  old_sp = (void *)REG_A7;
  REG_A7 = (u_int)stkrst68k(new_sp, old_sp);
  RESTORE_68K_REGS
}

struct EmulLibEntry _gate___stkrst = {
  TRAP_LIBRESTORE, 0, (void(*)())_trampoline___stkrst
};


char *stkrst68k_f(void *old_sp)
{
  usetup;
  struct StackSwapStruct sss;
  struct Task *me = SysBase->ThisTask;
  char *sp;

  KPRINTF(("stkrst_f(%lx, %lx) [68k]\n", u.u_68k_stk_used->savesp, old_sp));

  sss.stk_Pointer = sp = (char *)u.u_68k_stk_used->savesp;
  popframes_68k(u.u_68k_stk_used, &sss);
  me->tc_SPUpper = (void *)sss.stk_Upper;
  me->tc_SPLower = sss.stk_Lower;

  KPRINTF(("sp = %lx\n", sp));

  return sp;
}

void _trampoline___stkrst_f(void)
{
  sigset_t old;
  void *old_sp;
  SAVE_68K_REGS
  atomic_on(&old);
  old_sp = (void *)REG_A7;
  REG_A7 = (u_int)stkrst68k_f(old_sp);
  atomic_off(old);
  RESTORE_68K_REGS
}

struct EmulLibEntry _gate___stkrst_f = {
  TRAP_LIBRESTORE, 0, (void(*)())_trampoline___stkrst_f
};

char *stkrst68k_end(void *old_sp)
{
  usetup;
  struct StackSwapStruct sss;
  struct Task *me = SysBase->ThisTask;
  char *sp;
  int ret;

  KPRINTF(("stkrst_end(%lx, %lx) [68k]\n", u.u_68k_stk_used->savesp, old_sp));

  ret = *(int *)old_sp;
  sss.stk_Pointer = sp = (char *)u.u_68k_stk_used->savesp;
  popframes_68k(u.u_68k_stk_used, &sss);
  me->tc_SPUpper = (void *)sss.stk_Upper;
  me->tc_SPLower = sss.stk_Lower;

  KPRINTF(("sp = %lx\n", sp));

  sp -= 4; /* add the return address */
  *(int *)sp = ret;

  return sp;
}

void _trampoline___stkrst_end(void)
{
  sigset_t old;
  void *old_sp;
  SAVE_68K_REGS
  atomic_on(&old);
  old_sp = (void *)REG_A7;
  REG_A7 = (u_int)stkrst68k_end(old_sp);
  atomic_off(old);
  RESTORE_68K_REGS
}

struct EmulLibEntry _gate___stkrst_end = {
  TRAP_LIBRESTORE, 0, (void(*)())_trampoline___stkrst_end
};

/*
 * Allocate a new stackframe with d0 bytes minimum.
 */
void __stkext_68k(void)
{
  struct StackSwapStruct sss;
  usetup;
  u_int callsp = REG_A7 + 4;
  u_int d0 = REG_D0;
  sigset_t old;
  struct Task *me = SysBase->ThisTask;
  SAVE_68K_REGS

  atomic_on(&old);

  KPRINTF(("stkext_68k(%lx, %lx, %lx, %lx, %lx) limit = %lx\n", callsp, me->tc_SPUpper, me->tc_SPLower, u.u_68k_stk_used, d0, u.u_68k_stk_limit ? *u.u_68k_stk_limit : NULL));

  if ((void *)callsp >= STK_UPPER_68K || (void *)callsp < STK_LOWER_68K)
    return; /* User intentionally left area of stackextension */

  pushframe_68k(d0, &sss, &old, 0);
  me->tc_SPUpper = (void *)sss.stk_Upper;
  me->tc_SPLower = sss.stk_Lower;

  KPRINTF(("new a7 = %lx, upper = %lx, lower = %lx, used = %lx\n", sss.stk_Pointer, sss.stk_Upper, sss.stk_Lower, u.u_68k_stk_used));

  atomic_off(old);

  RESTORE_68K_REGS
  REG_A7 = (u_int)sss.stk_Pointer;
}

struct EmulLibEntry _gate___stkext = {
  TRAP_LIBRESTORE, 0, (void(*)())__stkext_68k
};


/*
 * Allocate a new stackframe with d0 bytes minimum, copy the callers arguments
 * and set his returnaddress (offset d1 from the sp when called) to stk_rst_f
 */
void __stkext_f_68k(void)
{
  char *argtop;
  int cpsize;
  struct StackSwapStruct sss;
  usetup;
  void *callsp = (void *)(REG_A7 + 4);
  u_int d0 = REG_D0;
  u_int d1 = REG_D1;
  sigset_t old;
  struct Task *me = SysBase->ThisTask;

  SAVE_68K_REGS

  atomic_on(&old);

  KPRINTF(("stkext_f_68k(%lx, %lx, %lx, %lx, %lx, %lx) limit = %lx\n", callsp, me->tc_SPUpper, me->tc_SPLower, u.u_68k_stk_used, d0, d1, u.u_68k_stk_limit ? *u.u_68k_stk_limit : NULL));

  if ((void *)callsp >= STK_UPPER_68K || (void *)callsp < STK_LOWER_68K)
    return; /* User intentionally left area of stackextension */

  argtop = (char *)callsp + u.u_68k_stk_argbt;      /* Top of area with arguments */
  if ((void*)argtop > STK_UPPER_68K)
    argtop = STK_UPPER_68K;
  cpsize = (char *)argtop - (char *)callsp + 4;

  /* FIXME: is "+ u.u_stk_argbt" really necessary? It's added in pushframe(), too. */
  pushframe_68k(d0 + u.u_68k_stk_argbt, &sss, &old, 0);
  *(char **)&sss.stk_Pointer -= cpsize;
  CopyMem((char *)callsp - 4, sss.stk_Pointer, cpsize);
  u.u_68k_stk_used->savesp = (char *)callsp + d1; /* store sp */
  *(void **)((char *)sss.stk_Upper - ((char *)argtop - (char *)callsp) + d1)
	= &_gate___stkrst_f; /* set returnaddress */

  me->tc_SPUpper = (void *)sss.stk_Upper;
  me->tc_SPLower = sss.stk_Lower;

  KPRINTF(("new a7 = %lx, upper = %lx, lower = %lx, used = %lx\n", sss.stk_Pointer, sss.stk_Upper, sss.stk_Lower, u.u_68k_stk_used));
  REG_A7 = (u_int)sss.stk_Pointer;

  atomic_off(old);

  RESTORE_68K_REGS
}

struct EmulLibEntry _gate___stkext_f = {
  TRAP_LIBRESTORE, 0, (void(*)())__stkext_f_68k
};



static void *stkext_startup_68k(int stack0, void* a7)
{
  int stack;
  sigset_t old;
  struct StackSwapStruct sss;
  struct Task *me = SysBase->ThisTask;
  usetup;

  KPRINTF(("stkext_startup_68k(%ld, %lx) sz = %ld\n", stack0, a7, u.u_68k_stk_argbt));
  KPRINTF(("stack= %08lx %08lx %08lx %08lx %08lx\n", a7, *(int*)((int)a7+4), *(int*)((int)a7+8), *(int*)((int)a7+12), *(int*)((int)a7+16)));

  stack = get_68k_stack_size((struct Process*)me, stack0);
  if (stack)
  {
    pushframe_68k(stack, &sss, &old, 1);
    u.u_68k_stk_used->savesp = (char *)a7 + 4;
    sss.stk_Pointer = (APTR)((char*)sss.stk_Pointer - u.u_68k_stk_argbt);
    memcpy((char *)sss.stk_Pointer, (char *)a7, u.u_68k_stk_argbt);
    ((void **)sss.stk_Pointer)[1] = &_gate___stkrst_end;
    a7 = sss.stk_Pointer;
    me->tc_SPUpper = (void *)sss.stk_Upper;
    me->tc_SPLower = sss.stk_Lower;
  }

  KPRINTF(("new a7 = %lx\n", a7));
  KPRINTF(("stack= %08lx %08lx %08lx %08lx %08lx\n", a7, *(int*)((int)a7+4), *(int*)((int)a7+8), *(int*)((int)a7+12), *(int*)((int)a7+16)));

  return a7;
}


asm("
/* stack usage:
    0-7:    frame
    8-71:   caos
    72-75:  trash
    76-79:  sigset
    80-155: save r13-r31
    156-159:pad
*/
	.globl  _trampoline___stkext_startup
_trampoline___stkext_startup:
	stwu    1,-160(1)
	mflr    0
	stmw    13,80(1)        /* save ppc regs */
	addi    3,1,76          /* r3 = &sigset */
	lmw     16,0(2)         /* r16-r31 = d0-a7 */
	stw     0,164(1)        /* save lr */
	lwz     15,0(31)        /* r15 = *sp = return address */
	bl      atomic_on
	stmw    15,8(1)         /* fill caos + trash */

	/* install ppc stack */

	mr      3,16            /* r3 = d0 = __stack */
	mr      4,1             /* r4 = r1 */
	addi    5,1,76          /* r5 = &sigset */
	li      6,164           /* r6 = offset to saved lr */
	bl      stkext_startup_ppc /* r3 = new ppc stack */
	mr      1,3             /* install new ppc stack */

	/* install 68k stack */

        mr      3,16            /* r3 = d0 = __stack */
	mr      4,31            /* r4 = sp */
	bl      stkext_startup_68k /* need r3, r4 */
	addi    3,3,8           /* r3 = a7 + 8 (skip two return addresses) */
	lwz     13,4(31)        /* r13 = pop next return address */
	stw     3,60(2)         /* REG_A7 = stk68k_startup() + 8 */

	/* call the startup function */
        lwz     15,0x5c(2)      /* EmulCall68k */
	lwz     3,76(1)         /* r3 = sigset */
	bl      atomic_off
	mtctr   15              /* ctr = EmulCall68k */
	addi    3,1,8           /* r3 = &caos */
	bctrl                   /* r3 = EmulCall68k(&caos) */
	mr      16,3            /* new d0 = r3 */

	/* restore the 68k stack if necessary */
	lwz	4,60(2)		/* r4 = new a7 */
	addi	3,31,8		/* r3 = original a7 */
	stwu	13,-4(4)
	bl      stkrst68k
	mr	31,3
	/*stw     13,0(31)        /* put the 2nd return address back */

	/* the ppc stack will be restored when this function returns */
	lwz     0,164(1)
	stmw    16,0(2)         /* restore d0-a7 (d0 & a7 are changed) */
	mtlr    0
	mr	3,16
	lmw     13,80(1)        /* restore ppc regs */
	addi    1,1,160
	blr
");

void _trampoline___stkext_startup();

const struct EmulLibEntry _gate___stkext_startup = {
  TRAP_LIBRESTORE, 0, (void(*)())_trampoline___stkext_startup
};


void initstack(void)
{
  struct Task *me = SysBase->ThisTask;
  struct user *u_ptr = getuser(me);

  u.u_68k_tc_splower = me->tc_SPLower;
  u.u_68k_tc_spupper = me->tc_SPUpper;
  u.u_ppc_tc_splower = me->tc_ETask->PPCSPLower;
  u.u_ppc_tc_spupper = me->tc_ETask->PPCSPUpper;

  KPRINTF(("68k: lower=%lx, upper=%lx\n", u.u_68k_tc_splower, u.u_68k_tc_spupper));
  KPRINTF(("ppc: lower=%lx, upper=%lx\n", u.u_ppc_tc_splower, u.u_ppc_tc_spupper));

  u.u_68k_org_lower = u.u_68k_tc_splower; /* Lower stack bound */
  u.u_68k_org_upper = u.u_68k_tc_spupper; /* Upper stack bound +1 */
  u.u_68k_stk_used = NULL;   /* Stackframes in use */
  u.u_68k_stk_spare = NULL;  /* Spare stackframes */
  u.u_68k_stk_current = u.u_68k_stk_max = 0; /* No extended stackframes at this point */

  u.u_68k_stk_limit = (void **)-1; /* Used uninitialized? Raise address error */
  u.u_68k_stk_argbt = 256; /* set some useful default */

  u.u_ppc_org_lower = u.u_ppc_tc_splower; /* Lower stack bound */
  u.u_ppc_org_upper = u.u_ppc_tc_spupper; /* Upper stack bound +1 */
  u.u_ppc_stk_used = NULL;   /* Stackframes in use */
  u.u_ppc_stk_spare = NULL;  /* Spare stackframes */
  u.u_ppc_stk_current = u.u_ppc_stk_max = 0; /* No extended stackframes at this point */

  u.u_ppc_stk_limit = (void **)-1; /* Used uninitialized? Raise address error */
  u.u_ppc_stk_argbt = 256; /* set some useful default */
}


void __init_stk_limit(void **limit, unsigned long argbytes)
{
  usetup;

  u.u_68k_stk_limit = NULL;
  u.u_ppc_stk_limit = limit;
  u.u_ppc_stk_argbt = argbytes;
  *limit = (char *)u.u_ppc_org_lower + stk_safezone_ppc + argbytes;
  KPRINTF(("init_stk_limit_ppc(%lx, %ld)\n", *limit, argbytes));
}

void __init_stk_limit_68k(void)
{
  u_int *sp = (u_int*)REG_A7;
  void **limit = (void**)sp[1];
  unsigned long argbytes = sp[2];
  usetup;

  u.u_ppc_stk_limit = NULL;
  u.u_68k_stk_limit = limit;
  u.u_68k_stk_argbt = argbytes;
  *limit = (char *)u.u_68k_org_lower + stk_safezone_68k + argbytes;
  KPRINTF(("init_stk_limit_68k(%lx, %ld)\n", *limit, argbytes));
}

const struct EmulLibEntry _gate___init_stk_limit = {
  TRAP_LIBNR, 0, (void(*)())__init_stk_limit_68k
};

/*
 * Free all spare stackframes
 */
void freestack(void)
{
  struct stackframe *sf, *s2;
  usetup;
  KPRINTF(("freestack\n"));
  sf = u.u_ppc_stk_spare;
  u.u_ppc_stk_spare = NULL;
  while (sf != NULL)
  {
    s2 = sf->next;
    KPRINTF(("FreeMem(%lx, %ld)\n", sf, (char *)sf->upper - (char *)sf));
    FreeMem(sf, (char *)sf->upper - (char *)sf);
    sf = s2;
  }
  sf = u.u_68k_stk_spare;
  u.u_68k_stk_spare = NULL;
  while (sf != NULL)
  {
    s2 = sf->next;
    KPRINTF(("FreeMem(%lx, %ld)\n", sf, (char *)sf->upper - (char *)sf));
    FreeMem(sf, (char *)sf->upper - (char *)sf);
    sf = s2;
  }
}

void __stkovf(void)
{
  usetup;

  KPRINTF(("stkovf\n"));

  u.u_ppc_stk_limit = NULL; /* disable stackextend from now on */
  for (;;) {
    kill(getpid(), SIGSEGV); /* Ciao */
    Delay(50);
  }
}

void __stkovf_68k(void)
{
  usetup;

  KPRINTF(("stkovf68k\n"));

  u.u_68k_stk_limit = NULL; /* disable stackextend from now on */
  for (;;) {
    kill(getpid(), SIGSEGV); /* Ciao */
    Delay(50);
  }
}

const struct EmulLibEntry _gate___stkovf = {
  TRAP_LIBNR, 0, (void(*)())__stkovf_68k
};

/*
 * Move a stackframe with a minimum of requiredstack bytes to the used list
 * and fill the StackSwapStruct structure.
 */
static void pushframe_68k(ULONG requiredstack, struct StackSwapStruct *sss, sigset_t *old, int startup)
{
  struct stackframe *sf;
  ULONG recommendedstack;
  usetup;

  KPRINTF(("pushframe_68k(%ld, %ld)\n", requiredstack, startup));

  if (!startup)
  {
    requiredstack += stk_safezone_68k + u.u_68k_stk_argbt;
    if (requiredstack < stk_minframe_68k)
      requiredstack = stk_minframe_68k;
  }

  recommendedstack=u.u_68k_stk_max-u.u_68k_stk_current;
  if (recommendedstack<requiredstack)
    recommendedstack=requiredstack;

  for (;;)
  {
    sf = u.u_68k_stk_spare; /* get a stackframe from the spares list */
    if (sf == NULL)
    { /* stack overflown */
      for (; recommendedstack>=requiredstack; recommendedstack/=2)
      {
	sf = AllocMem(recommendedstack + sizeof(struct stackframe), MEMF_PUBLIC);
	KPRINTF(("AllocMem(%ld) = %lx\n", recommendedstack + sizeof(struct stackframe), sf));
	if (sf != NULL)
	  break;
      }
      if (sf == NULL)
      { /* and we have no way to extend it :-| */
	sigprocmask(SIG_SETMASK, old, NULL);
	__stkovf_68k();
      }
      sf->upper = (char *)(sf + 1) + recommendedstack;
      KPRINTF(("new frame: %lx - %lx\n", sf + 1, sf->upper));
      break;
    }
    u.u_68k_stk_spare = sf->next;
    if ((char *)sf->upper - (char *)(sf + 1) >= recommendedstack)
      break;
    KPRINTF(("FreeMem(%lx, %ld)\n", sf, (char *)sf->upper - (char *)sf));
    FreeMem(sf, (char *)sf->upper - (char *)sf);
  }

  /* Add stackframe to the used list */
  sf->next = u.u_68k_stk_used;
  u.u_68k_stk_used = sf;
  if (u.u_68k_stk_limit)
    *u.u_68k_stk_limit = (char *)(sf + 1) + stk_safezone_68k + u.u_68k_stk_argbt;

  /* prepare StackSwapStruct */
  (void *)sss->stk_Pointer = (void *)sf->upper;
  sss->stk_Lower = sf + 1;
  (ULONG)sss->stk_Upper = (ULONG)sf->upper;

  KPRINTF(("used = %lx, ptr = %lx, lower = %lx, upper = %lx, limit = %lx\n",
	   u.u_68k_stk_used, sss->stk_Pointer, sss->stk_Lower, sss->stk_Upper, u.u_68k_stk_limit ? *u.u_68k_stk_limit : NULL));

  /* Update stack statistics. */
  u.u_68k_stk_current += (char *)sf->upper - (char *)(sf + 1);
  if (u.u_68k_stk_current > u.u_68k_stk_max)
    u.u_68k_stk_max = u.u_68k_stk_current;
}

static void pushframe_ppc(ULONG requiredstack, struct StackSwapStruct *sss, sigset_t *old, int startup)
{
  struct stackframe *sf;
  ULONG recommendedstack;
  usetup;

  KPRINTF(("pushframe_ppc(%ld, %ld)\n", requiredstack, startup));

  if (!startup)
  {
    requiredstack += stk_safezone_ppc + u.u_ppc_stk_argbt;
    if (requiredstack < stk_minframe_ppc)
      requiredstack = stk_minframe_ppc;
  }

  recommendedstack=u.u_ppc_stk_max-u.u_ppc_stk_current;
  if (recommendedstack<requiredstack)
    recommendedstack=requiredstack;

  for (;;)
  {
    sf = u.u_ppc_stk_spare; /* get a stackframe from the spares list */
    if (sf == NULL)
    { /* stack overflown */
      for (; recommendedstack>=requiredstack; recommendedstack/=2)
      {
	sf = AllocMem(recommendedstack + sizeof(struct stackframe), MEMF_PUBLIC);
	KPRINTF(("AllocMem(%ld) = %lx\n", recommendedstack + sizeof(struct stackframe), sf));
	if (sf != NULL)
	  break;
      }
      if (sf == NULL)
      { /* and we have no way to extend it :-| */
	sigprocmask(SIG_SETMASK, old, NULL);
	__stkovf();
      }
      sf->upper = (char *)(sf + 1) + recommendedstack;
      KPRINTF(("new frame: %lx - %lx\n", sf + 1, sf->upper));
      break;
    }
    u.u_ppc_stk_spare = sf->next;
    if ((char *)sf->upper - (char *)(sf + 1) >= recommendedstack)
      break;
    KPRINTF(("FreeMem(%lx, %ld)\n", sf, (char *)sf->upper - (char *)sf));
    FreeMem(sf, (char *)sf->upper - (char *)sf);
  }

  /* Add stackframe to the used list */
  sf->next = u.u_ppc_stk_used;
  u.u_ppc_stk_used = sf;
  if (u.u_ppc_stk_limit)
    *u.u_ppc_stk_limit = (char *)(sf + 1) + stk_safezone_ppc + u.u_ppc_stk_argbt;

  /* prepare StackSwapStruct */
  (void *)sss->stk_Pointer = (void *)sf->upper;
  sss->stk_Lower = sf + 1;
  (ULONG)sss->stk_Upper = (ULONG)sf->upper;

  KPRINTF(("used = %lx, ptr = %lx, lower = %lx, upper = %lx, limit = %lx\n",
	   u.u_ppc_stk_used, sss->stk_Pointer, sss->stk_Lower, sss->stk_Upper, u.u_ppc_stk_limit ? *u.u_ppc_stk_limit : NULL));

  /* Update stack statistics. */
  u.u_ppc_stk_current += (char *)sf->upper - (char *)(sf + 1);
  if (u.u_ppc_stk_current > u.u_ppc_stk_max)
    u.u_ppc_stk_max = u.u_ppc_stk_current;
}

/*
 * Move all used stackframes upto (and including) sf to the spares list
 * and fill the StackSwapStruct structure.
 */
static void popframes_68k(struct stackframe *sf, struct StackSwapStruct *sss)
{
  struct stackframe *sf2;
  usetup;

  KPRINTF(("popframes\n"));

  if (sf->next != NULL)
  {
    sss->stk_Lower = sf->next + 1;
    (ULONG)sss->stk_Upper = (ULONG)sf->next->upper;
    if (u.u_68k_stk_limit)
      *u.u_68k_stk_limit = (char *)(sf->next + 1) + stk_safezone_68k + u.u_68k_stk_argbt;
  }
  else
  {
    sss->stk_Lower = u.u_68k_tc_splower;
    (ULONG)sss->stk_Upper = (ULONG)u.u_68k_tc_spupper;
    if (u.u_68k_stk_limit)
      *u.u_68k_stk_limit = (char *)u.u_68k_org_lower + stk_safezone_68k + u.u_68k_stk_argbt;
  }
  KPRINTF(("lower = %lx, upper = %lx, limit = %lx\n", sss->stk_Lower, sss->stk_Upper, u.u_68k_stk_limit ? *u.u_68k_stk_limit : NULL));
  sf2 = u.u_68k_stk_spare;
  u.u_68k_stk_spare = u.u_68k_stk_used;
  u.u_68k_stk_used = sf->next;
  sf->next = sf2;

  /* Update stack statistics. */
  for (sf2 = u.u_68k_stk_spare; sf2 != sf->next; sf2 = sf2->next)
    {
      KPRINTF(("poping frame %lx-%lx \n", sf2 + 1, sf2->upper));
      u.u_68k_stk_current -= (char *)sf2->upper - (char *)(sf2 + 1);
    }
}

static void popframes_ppc(struct stackframe *sf, struct StackSwapStruct *sss)
{
  struct stackframe *sf2;
  usetup;

  KPRINTF(("popframes\n"));

  if (sf->next != NULL)
  {
    sss->stk_Lower = sf->next + 1;
    (ULONG)sss->stk_Upper = (ULONG)sf->next->upper;
    if (u.u_ppc_stk_limit)
      *u.u_ppc_stk_limit = (char *)(sf->next + 1) + stk_safezone_ppc + u.u_ppc_stk_argbt;
  }
  else
  {
    sss->stk_Lower = u.u_ppc_tc_splower;
    (ULONG)sss->stk_Upper = (ULONG)u.u_ppc_tc_spupper;
    if (u.u_ppc_stk_limit)
      *u.u_ppc_stk_limit = (char *)u.u_ppc_org_lower + stk_safezone_ppc + u.u_ppc_stk_argbt;
  }
  KPRINTF(("lower = %lx, upper = %lx, limit = %lx\n", sss->stk_Lower, sss->stk_Upper, u.u_ppc_stk_limit ? *u.u_ppc_stk_limit : NULL));
  sf2 = u.u_ppc_stk_spare;
  u.u_ppc_stk_spare = u.u_ppc_stk_used;
  u.u_ppc_stk_used = sf->next;
  sf->next = sf2;

  /* Update stack statistics. */
  for (sf2 = u.u_ppc_stk_spare; sf2 != sf->next; sf2 = sf2->next)
    {
      KPRINTF(("poping frame %lx-%lx \n", sf2 + 1, sf2->upper));
      u.u_ppc_stk_current -= (char *)sf2->upper - (char *)(sf2 + 1);
    }
}
#else



#define STK_UPPER       \
(u.u_stk_used != NULL ? u.u_stk_used->upper : u.u_org_upper)

#define STK_LOWER       \
(u.u_stk_used != NULL ? (void *)(u.u_stk_used + 1) : u.u_org_lower)


#define stk_safezone    6144    /* into ixprefs? */
#define stk_minframe    32768

void clearstack() //68k use that
{
 struct Task *me = SysBase->ThisTask;
 unsigned long tmp = (char *)(get_sp() - 256);  /* 256 bytes safety margin */
 memset(me->tc_SPLower+4, 0xdb, ((u_long)tmp - (u_long)me->tc_SPLower)-4); 
}

static void popframes(struct stackframe *sf, struct StackSwapStruct *sss);
static void pushframe(ULONG requiredstack, struct StackSwapStruct *sss, sigset_t *old, int startup);
void __init_stk_limit(void **limit, unsigned long argbytes);
void __stkrst_f(void);

asm(" \n\
	.globl  ___stkext \n\
___stkext: \n\
	moveml  d0/d1/a0/a1/a6,sp@- \n\
 	subqw   #4,sp           | sigset_t \n\
	pea	sp@ \n\
	jbsr    _atomic_on \n\
	subqw   #8,sp           | struct StackSwapStruct (8+4 from pea above) \n\
	jbsr    _stkext \n\
	tstl    d0 \n\
	jeq     s_noext \n\
	movel   _SysBase,a6 \n\
	movel   sp,a0 \n\
	jsr     a6@(-0x2dc)     | StackSwap(a0) \n\
s_ret: \n\
	jbsr    _atomic_off \n\
	addqw   #4,sp           | StackSwapStruct is not copied \n\
	moveml  sp@+,d0/d1/a0/a1/a6 \n\
	rts \n\
s_noext: \n\
	addw    #12,sp \n\
	jra     s_ret \n\
\n\
	.globl  ___stkext_f    | see above \n\
___stkext_f: \n\
	moveml  d0/d1/a0/a1/a6,sp@- \n\
	subqw   #4,sp \n\
	pea	sp@ \n\
	jbsr    _atomic_on \n\
	subqw   #8,sp \n\
	jbsr    _stkext_f \n\
	tstl    d0 \n\
	jeq     sf_noext \n\
	movel   _SysBase,a6 \n\
	movel   sp,a0 \n\
	jsr     a6@(-0x2dc) \n\
sf_ret: \n\
	jbsr    _atomic_off \n\
	addqw   #4,sp \n\
	moveml  sp@+,d0/d1/a0/a1/a6 \n\
	rts \n\
sf_noext: \n\
	addw    #12,sp \n\
	jra     sf_ret \n\
\n\
	.globl  ___stkext_startup \n\
___stkext_startup: \n\
	moveml  d0/d1/a0/a1/a6,sp@- \n\
	subqw   #4,sp \n\
	pea	sp@ \n\
	jbsr    _atomic_on      | FIXME: Is this necessary/allowed that early? \n\
	subqw   #8,sp \n\
	jbsr    _stkext_startup \n\
	tstl    d0 \n\
	jeq     ss_noext \n\
	movel   _SysBase,a6 \n\
	movel   sp,a0 \n\
	jsr     a6@(-0x2dc) \n\
	jsr _clearstack \n\
ss_ret: \n\
	jbsr    _atomic_off     | FIXME: Is this necessary/allowed that early? \n\
	addqw   #4,sp \n\
	moveml  sp@+,d0/d1/a0/a1/a6 \n\
	rts \n\
ss_noext: \n\
	addw    #12,sp \n\
	jra     ss_ret \n\
 \n\
	.globl  ___stkrst_f    | see above \n\
___stkrst_f: \n\
	moveml  d0/d1/a0/a1/a6,sp@- \n\
	subqw   #4,sp \n\
	pea	sp@ \n\
	jbsr    _atomic_on \n\
	subqw   #8,sp \n\
	jbsr    _stkrst_f \n\
	movel   _SysBase,a6 \n\
	movel   sp,a0 \n\
	jsr     a6@(-0x2dc) \n\
	jbsr    _atomic_off \n\
	addqw   #4,sp \n\
	moveml  sp@+,d0/d1/a0/a1/a6 \n\
	rts \n\
\n\
	.globl  ___stkrst \n\
___stkrst: \n\
	moveml  d0/d1/a0/a1/a6,sp@-         | preserve registers \n\
	subqw   #4,sp           | make room for the signal mask \n\
	pea	sp@ \n\
	jbsr    _atomic_on      | disable all signals \n\
	subqw   #8,sp           | make room for a StackSwapStruct \n\
	jbsr    _stkrst         | calculate either target sp or StackSwapStruct \n\
	tstl    d0              | set target sp? \n\
	jeq     swpfrm          | jump if not \n\
	movel   d0,a0           | I have a lot of preserved registers and \n\
				| returnadresses on the stack. It's necessary \n\
				| to copy them to the new location \n\
	moveq   #6,d0           | 1 rts, 5 regs and 1 signal mask to copy (1+5+1)-1=6 \n\
	lea     sp@(40:W),a1    | get address of uppermost byte+1 (1+5+1)*4+12=40 \n\
	cmpl    a0,a1           | compare with target location \n\
	jls     lp1             | jump if source<=target \n\
	lea     a0@(-28:W),a0   | else start at lower bound (1+5+1)*4=28 \n\
	lea     a1@(-28:W),a1 \n\
	movel   a0,sp           | set sp to reserve the room \n\
lp0:    movel   a1@+,a0@+       | copy with raising addresses \n\
	dbra    d0,lp0          | as long as d0>=0. \n\
	jra     endlp           | ready \n\
lp1:    movel   a1@-,a0@-       | copy with falling addresses \n\
	dbra    d0,lp1          | as long as d0>=0 \n\
	movel   a0,sp           | finally set sp \n\
	jra     endlp           | ready \n\
swpfrm: movel   _SysBase,a6     | If sp wasn't set call StackSwap() \n\
	movel   sp,a0 \n\
	jsr     a6@(-0x2dc) \n\
endlp:  jbsr    _atomic_off     | reenable signals \n\
	addqw   #4,sp           | adjust sp \n\
	moveml  sp@+,d0/d1/a0/a1/a6         | restore registers \n\
	rts                     | and return \n\
");


void initstack(void)
{
  APTR lower, upper;
  struct Task *me = SysBase->ThisTask;
  struct user *u_ptr = getuser(me);
  struct Process *proc = (struct Process *)me;

  u.u_tc_splower = me->tc_SPLower;
  u.u_tc_spupper = me->tc_SPUpper;

  KPRINTF(("lower=%lx, upper=%lx\n", u.u_tc_splower, u.u_tc_spupper));

  if (proc->pr_CLI)
  {
    /* Process stackframe:
     * proc->pr_ReturnAddr points to size of stack (ULONG)
     *         +4                  returnaddress
     *         +8                  stackframe
     */
    lower = (char *)proc->pr_ReturnAddr + 8 - *(ULONG *)proc->pr_ReturnAddr;
    upper = (char *)proc->pr_ReturnAddr + 8;
  }
  else
  {
    lower = u.u_tc_splower;
    upper = u.u_tc_spupper;
  }

  KPRINTF(("lower=%lx, upper=%lx\n", lower, upper));

  u.u_org_lower = lower; /* Lower stack bound */
  u.u_org_upper = upper; /* Upper stack bound +1 */
  u.u_stk_used = NULL;   /* Stackframes in use */
  u.u_stk_spare = NULL;  /* Spare stackframes */
  u.u_stk_current=u.u_stk_max=0; /* No extended stackframes at this point */

  u.u_stk_limit = (void **)-1; /* Used uninitialized? Raise address error */
  u.u_stk_argbt = 256; /* set some useful default */
}

void __init_stk_limit(void **limit, unsigned long argbytes)
{
  usetup;

  u.u_stk_limit = limit;
  u.u_stk_argbt = argbytes;
  *limit = (char *)u.u_org_lower + stk_safezone + argbytes;
  KPRINTF(("init_stk_limit(%lx, %ld)\n", *limit, argbytes));
}

/*
 * Free all spare stackframes
 */
void freestack(void)
{
  struct stackframe *sf, *s2;
  usetup;
  KPRINTF(("freestack\n"));
  sf = u.u_stk_spare;
  u.u_stk_spare = NULL;
  while (sf != NULL)
  {
    s2 = sf->next;
    KPRINTF(("FreeMem(%lx, %ld)\n", sf, (char *)sf->upper - (char *)sf));
    FreeMem(sf, (char *)sf->upper - (char *)sf);
    sf = s2;
  }
}

void __stkovf(void)
{
  usetup;

  KPRINTF(("stkovf\n"));

  u.u_stk_limit = NULL; /* disable stackextend from now on */
  for (;;) {
	
    kill(getpid(), SIGSEGV); /* Ciao */
    Delay(50);
  }
}

/*
 * Move a stackframe with a minimum of requiredstack bytes to the used list
 * and fill the StackSwapStruct structure.
 */
static void pushframe(ULONG requiredstack, struct StackSwapStruct *sss, sigset_t *old, int startup)
{ // used for 68k 
  struct stackframe *sf;
  ULONG recommendedstack;
  usetup;
  
  KPRINTF(("pushframe(%ld, %ld)\n", requiredstack, startup));

  if (!startup)
  {
    requiredstack += stk_safezone + u.u_stk_argbt;
    if (requiredstack < stk_minframe)
      requiredstack = stk_minframe;
  }

  recommendedstack=u.u_stk_max-u.u_stk_current;
  if (recommendedstack<requiredstack)
    recommendedstack=requiredstack;

  for (;;)
  {
    sf = u.u_stk_spare; /* get a stackframe from the spares list */
    if (sf == NULL)
    { /* stack overflown */
      for (; recommendedstack>=requiredstack; recommendedstack/=2)
      {
	sf = AllocMem(recommendedstack + sizeof(struct stackframe), MEMF_PUBLIC);
	KPRINTF(("AllocMem(%ld) = %lx\n", recommendedstack + sizeof(struct stackframe), sf));
	if (sf != NULL)
	  break;
      }
      if (sf == NULL)
      { /* and we have no way to extend it :-| */
	sigprocmask(SIG_SETMASK, old, NULL);
	__stkovf();
      }
      sf->upper = (char *)(sf + 1) + recommendedstack;
      KPRINTF(("new frame: %lx - %lx\n", sf + 1, sf->upper));
      break;
    }
    u.u_stk_spare = sf->next;
    if ((char *)sf->upper - (char *)(sf + 1) >= recommendedstack)
      break;
    KPRINTF(("FreeMem(%lx, %ld)\n", sf, (char *)sf->upper - (char *)sf));
    FreeMem(sf, (char *)sf->upper - (char *)sf);
  }

  /* Add stackframe to the used list */
  sf->next = u.u_stk_used;
  u.u_stk_used = sf;
  *u.u_stk_limit = (char *)(sf + 1) + stk_safezone + u.u_stk_argbt;

  /* prepare StackSwapStruct */
  (void *)sss->stk_Pointer = (void *)sf->upper;
  sss->stk_Lower = sf + 1;
  (ULONG)sss->stk_Upper = (ULONG)sf->upper;

  KPRINTF(("used = %lx, ptr = %lx, lower = %lx, upper = %lx, limit = %lx\n",
	   u.u_stk_used, sss->stk_Pointer, sss->stk_Lower, sss->stk_Upper, *u.u_stk_limit));

  /* Update stack statistics. */
  u.u_stk_current += (char *)sf->upper - (char *)(sf + 1);
  if (u.u_stk_current > u.u_stk_max)
    u.u_stk_max = u.u_stk_current;
}

/*
 * Allocate a new stackframe with d0 bytes minimum.
 */
int stkext(struct StackSwapStruct sss, sigset_t old,
 long d0, long d1, long a0, long a1, long a6, long ret1)
{
  void *callsp = &ret1 + 1;
  int cpsize = (char *)callsp - (char *)&old;
  usetup;

  if (callsp >= STK_UPPER || callsp < STK_LOWER)
    return 0; /* User intentionally left area of stackextension */

  KPRINTF(("stkext\n"));

  pushframe(d0, &sss, &old, 0);
  *(char **)&sss.stk_Pointer -= cpsize;
  CopyMem(&old, sss.stk_Pointer, cpsize);
  return 1;
}

/*
 * Allocate a new stackframe with d0 bytes minimum, copy the callers arguments
 * and set his returnaddress (offset d1 from the sp when called) to stk_rst_f
 */
int stkext_f(struct StackSwapStruct sss, sigset_t old,
 long d0, long d1, long a0, long a1, long a6, long ret1)
{
  void *argtop, *callsp = &ret1 + 1;
  int cpsize;
  usetup;

  if (callsp >= STK_UPPER || callsp < STK_LOWER)
    return 0; /* User intentionally left area of stackextension */

  KPRINTF(("stkext_f: d0=%lx, argbt=%lx, old=%08lx\n", d0, u.u_stk_argbt, old));

  argtop = (char *)callsp + u.u_stk_argbt;      /* Top of area with arguments */
  if (argtop > STK_UPPER)
    argtop = STK_UPPER;
  cpsize = (char *)argtop - (char *)&old;

  /* FIXME: is "+ u.u_stk_argbt" really necessary? It's added in pushframe(), too. */
  pushframe(d0 + u.u_stk_argbt, &sss, &old, 0);
  *(char **)&sss.stk_Pointer -= cpsize;
  CopyMem(&old, sss.stk_Pointer, cpsize);
  u.u_stk_used->savesp = (char *)callsp + d1; /* store sp */
  *(void **)((char *)sss.stk_Upper - ((char *)argtop - (char *)callsp) + d1)
	= &__stkrst_f; /* set returnaddress */
  return 1;
}
extern long __ixstack;
static int get_stack_size(struct Process *proc, int stack) // 68k use that
{
  struct CommandLineInterface *CLI;
 
  if (stack <= STACKSIZE)
  {
		if (__ixstack)stack = __ixstack;
		else
		stack = 512000;
  }
  if (stack < STACKSIZE)
    stack = STACKSIZE;

  /* NOTE: This is the most appropriate place for future support of
     user-specified, process-specific stacksizes. */

  CLI = BTOCPTR (proc->pr_CLI);

  if (stack <= (CLI ? CLI->cli_DefaultStack * 4 : proc->pr_StackSize))
    return 0;

  return stack;
}




/*
 * Called at startup to set stack to a safe/reasonable value (which is
 * provided in d0). Check if there is a need for stack extension at startup,
 * return 0 if it is not necessary. If it is, perform almost the same actions
 * as stkext_f.
 */
int stkext_startup(struct StackSwapStruct sss, sigset_t old,
 long d0, long d1, long a0, long a1, long a6, long ret1) //68k use that
{
  void *argtop, *callsp = &ret1 + 1;
  int cpsize, stack;

  usetup;
  struct Task *me = SysBase->ThisTask;
  if (!(stack = get_stack_size((struct Process*)me, d0)))
    return 0;

  argtop = (char *)callsp + u.u_stk_argbt;      /* Top of area with arguments */
  if (argtop > STK_UPPER)
    argtop = STK_UPPER;
  cpsize = (char *)argtop - (char *)&old;

  pushframe(stack, &sss, &old, 1);
  *(char **)&sss.stk_Pointer -= cpsize;
  CopyMem(&old,sss.stk_Pointer, cpsize);
 
  u.u_stk_used->savesp = (char *)callsp; /* store sp */
  *(void **)((char *)sss.stk_Upper - ((char *)argtop - (char *)callsp))
	= &__stkrst_f; /* set returnaddress */
  return 1;
}

/*
 * Move all used stackframes upto (and including) sf to the spares list
 * and fill the StackSwapStruct structure.
 */
static void popframes(struct stackframe *sf, struct StackSwapStruct *sss)
{
  struct stackframe *sf2;
  usetup;

  KPRINTF(("popframes\n"));
  
    ix_stack_usage();
  if (sf->next != NULL)
  {
    sss->stk_Lower = sf->next + 1;
    (ULONG)sss->stk_Upper = (ULONG)sf->next->upper;
    *u.u_stk_limit = (char *)(sf->next + 1) + stk_safezone + u.u_stk_argbt;
  }
  else
  {
    sss->stk_Lower = u.u_tc_splower;
    (ULONG)sss->stk_Upper = (ULONG)u.u_tc_spupper;
    *u.u_stk_limit = (char *)u.u_org_lower + stk_safezone + u.u_stk_argbt;
  }
  KPRINTF(("lower = %lx, upper = %lx, limit = %lx\n", sss->stk_Lower, sss->stk_Upper, *u.u_stk_limit));
  sf2 = u.u_stk_spare;
  u.u_stk_spare = u.u_stk_used;
  u.u_stk_used = sf->next;
  sf->next = sf2;

  /* Update stack statistics. */
  for (sf2 = u.u_stk_spare; sf2 != sf->next; sf2 = sf2->next)
    {
      KPRINTF(("poping frame %lx-%lx \n", sf2 + 1, sf2->upper));
      u.u_stk_current -= (char *)sf2->upper - (char *)(sf2 + 1);
    }
}

/*
 * Set stackpointer back to some previous value
 * != NULL: on the same stackframe (returns sp)
 * == NULL: on another stackframe
 */
void *stkrst(struct StackSwapStruct sss, sigset_t old,
 void *d0, long d1, long a0, long a1, long a6, long ret1)
{
  void *callsp = &ret1 + 1;
  int cpsize = (char *)callsp - (char *)&old;
  struct stackframe *sf1, *sf2;
  usetup;

  if (d0 >= STK_LOWER && d0 < STK_UPPER)
    return d0;

  KPRINTF(("stkrst\n"));

  sf1 = u.u_stk_used;
  if (sf1 == NULL)
    return d0;
  for (;;)
  {
    sf2 = sf1->next;
    if (sf2 == NULL)
    {
      if (d0 < u.u_org_lower || d0 >= u.u_org_upper)
	return d0;
      break;
    }
    if (d0 >= (void *)(sf2 + 1) && d0 < sf2->upper) /* This stackframe fits */
      break;
    sf1 = sf2;
  }
  popframes(sf1, &sss);
  sss.stk_Pointer = (char *)d0 - cpsize;
  CopyMem(&old, sss.stk_Pointer,cpsize);
  return NULL;
}

/*
 * return to last stackframe
 */
void stkrst_f(struct StackSwapStruct sss, sigset_t old,
 long d0, long d1, long a0, long a1, long a6)
{
  void *callsp = &a6 + 1; /* This one has no returnaddress - it's a fallback for rts */
  int cpsize = (char *)callsp - (char *)&old;
  usetup;

  KPRINTF(("stkrst_f\n"));

  sss.stk_Pointer = (char *)u.u_stk_used->savesp - cpsize;
  popframes(u.u_stk_used, &sss);
  CopyMem(&old, sss.stk_Pointer, cpsize);
}
#endif
