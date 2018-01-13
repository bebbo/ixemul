#define	_KERNEL
#include "ixemul.h"

#include <exec/memory.h>
#include <signal.h>
#include <unistd.h>

/*
 * Glue asm to C.
 */
asm("
	.globl	___stkext
___stkext:
	moveml	d0/d1/a0/a1/a6,sp@-
	subqw	#4,sp		| sigset_t
	jbsr	_atomic_on
	subw	#12,sp		| struct StackSwapStruct
	jbsr	_stkext
	tstl	d0
	jeq	s_noext
	movel	4:W,a6
	movel	sp,a0
	jsr	a6@(-0x2dc)	| StackSwap(a0)
s_ret:
	jbsr	_atomic_off
	addqw	#4,sp		| StackSwapStruct is not copied
	moveml	sp@+,d0/d1/a0/a1/a6
	rts
s_noext:
	addw	#12,sp
	jra	s_ret

	.globl	___stkext_f    | see above
___stkext_f:
	moveml	d0/d1/a0/a1/a6,sp@-
	subqw	#4,sp
	jbsr	_atomic_on
	subw	#12,sp
	jbsr	_stkext_f
	tstl	d0
	jeq	sf_noext
	movel	4:W,a6
	movel	sp,a0
	jsr	a6@(-0x2dc)
sf_ret:
	jbsr	_atomic_off
	addqw	#4,sp
	moveml	sp@+,d0/d1/a0/a1/a6
  	rts
sf_noext:
	addw	#12,sp
	jra	sf_ret
  
	.globl	___stkext_startup
___stkext_startup:
	moveml	d0/d1/a0/a1/a6,sp@-
	subqw	#4,sp
	jbsr	_atomic_on	| FIXME: Is this necessary/allowed that early?
	subw	#12,sp
	jbsr	_stkext_startup
	tstl	d0
	jeq	ss_noext
	movel	4:W,a6
	movel	sp,a0
	jsr	a6@(-0x2dc)
ss_ret:
	jbsr	_atomic_off	| FIXME: Is this necessary/allowed that early?
	addqw	#4,sp
	moveml	sp@+,d0/d1/a0/a1/a6
  	rts
ss_noext:
	addw	#12,sp
	jra	ss_ret

	.globl	___stkrst_f    | see above
___stkrst_f:
	moveml	d0/d1/a0/a1/a6,sp@-
	subqw	#4,sp
	jbsr	_atomic_on
	subw	#12,sp
	jbsr	_stkrst_f
	movel	4:W,a6
	movel	sp,a0
	jsr	a6@(-0x2dc)
	jbsr	_atomic_off
	addqw	#4,sp
	moveml	sp@+,d0/d1/a0/a1/a6
  	rts
  
	.globl	___stkrst
___stkrst:
	moveml	d0/d1/a0/a1/a6,sp@-	    | preserve registers
	subqw	#4,sp		| make room for the signal mask
	jbsr	_atomic_on	| disable all signals
	subw	#12,sp		| make room for a StackSwapStruct
	jbsr	_stkrst		| calculate either target sp or StackSwapStruct
	tstl	d0		| set target sp?
	jeq	swpfrm		| jump if not
	movel	d0,a0		| I have a lot of preserved registers and
				| returnadresses on the stack. It's necessary
				| to copy them to the new location
	moveq	#6,d0		| 1 rts, 5 regs and 1 signal mask to copy (1+5+1)-1=6
	lea	sp@(40:W),a1	| get address of uppermost byte+1 (1+5+1)*4+12=40
	cmpl	a0,a1		| compare with target location
	jls	lp1		| jump if source<=target
	lea	a0@(-28:W),a0	| else start at lower bound (1+5+1)*4=28
	lea	a1@(-28:W),a1
	movel	a0,sp		| set sp to reserve the room
lp0:	movel	a1@+,a0@+	| copy with raising addresses
	dbra	d0,lp0		| as long as d0>=0.
	jra	endlp		| ready
lp1:	movel	a1@-,a0@-	| copy with falling addresses
	dbra	d0,lp1		| as long as d0>=0
	movel	a0,sp		| finally set sp
	jra	endlp		| ready
swpfrm:	movel	4:W,a6		| If sp wasn't set call StackSwap()
	movel	sp,a0
	jsr	a6@(-0x2dc)
endlp:	jbsr	_atomic_off	| reenable signals
	addqw	#4,sp		| adjust sp
	moveml	sp@+,d0/d1/a0/a1/a6	    | restore registers
	rts			| and return
");

void __stkrst_f(void);

#define	STK_UPPER	\
(u.u_stk_used != NULL ? u.u_stk_used->upper : u.u_org_upper)

#define	STK_LOWER	\
(u.u_stk_used != NULL ? (void *)(u.u_stk_used + 1) : u.u_org_lower)

#define	stk_safezone	6144	/* into ixprefs? */
#define	stk_minframe	32768

void initstack(void)
{
  APTR lower, upper;
  struct Task *me = FindTask(0);
  struct user *u_ptr = getuser(me);
  struct Process *proc = (struct Process *)me;

  u.u_tc_splower = me->tc_SPLower;
  u.u_tc_spupper = me->tc_SPUpper;

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
}

/*
 * Free all spare stackframes
 */
void freestack(void)
{
  struct stackframe *sf, *s2;
  usetup;

  sf = u.u_stk_spare;
  u.u_stk_spare = NULL;
  while (sf != NULL)
  {
    s2 = sf->next;
    FreeMem(sf, (char *)sf->upper - (char *)sf);
    sf = s2;
  }
}

void __stkovf(void)
{
  usetup;

  u.u_stk_limit = NULL; /* disable stackextend from now on */
  for (;;)
    kill(getpid(), SIGSEGV); /* Ciao */
}

/*
 * Signal routines may want to benefit from stackextension too -
 * so make all the stack handling functions atomic.
 * FIXME: This seems to have the potential to break a Forbid() ?!?
 */
//void atomic_on(sigset_t old)
void atomic_on(int old)
{
  usetup;

  old = u.p_sigmask;
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

/*
 * Move a stackframe with a minimum of requiredstack bytes to the used list
 * and fill the StackSwapStruct structure.
 */
static void pushframe(ULONG requiredstack, struct StackSwapStruct *sss, sigset_t *old, int startup)
{
  struct stackframe *sf;
  ULONG recommendedstack;
  usetup;

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
	if (sf != NULL)
	  break;
      }
      if (sf == NULL)
      { /* and we have no way to extend it :-| */
        sigprocmask(SIG_SETMASK, old, NULL);
        __stkovf();
      }
      sf->upper = (char *)(sf + 1) + recommendedstack;
      break;
    }
    u.u_stk_spare = sf->next;
    if ((char *)sf->upper - (char *)(sf + 1) >= recommendedstack)
      break;
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

  argtop = (char *)callsp + u.u_stk_argbt;	/* Top of area with arguments */
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

static int get_stack_size(struct Process *proc, int stack)
{
  struct CommandLineInterface *CLI;

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
 long d0, long d1, long a0, long a1, long a6, long ret1)
{
  void *argtop, *callsp = &ret1 + 1;
  int cpsize, stack;
  usetup;

  if (!(stack = get_stack_size((struct Process*)FindTask(0), d0)))
    return 0;

  argtop = (char *)callsp + u.u_stk_argbt;	/* Top of area with arguments */
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
  sf2 = u.u_stk_spare;
  u.u_stk_spare = u.u_stk_used;
  u.u_stk_used = sf->next;
  sf->next = sf2;

  /* Update stack statistics. */
  for (sf2 = u.u_stk_spare; sf2 != sf->next; sf2 = sf2->next)
    u.u_stk_current -= (char *)sf2->upper - (char *)(sf2 + 1);
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

  sss.stk_Pointer = (char *)u.u_stk_used->savesp - cpsize;
  popframes(u.u_stk_used, &sss);
  CopyMem(&old, sss.stk_Pointer, cpsize);
}
