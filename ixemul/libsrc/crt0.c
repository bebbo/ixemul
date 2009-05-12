#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/execbase.h>

#include <libraries/dosextens.h>
#include <proto/intuition.h>
#include <limits.h>
#include "kprintf.h"
#include "ixemul.h"
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <proto/dos.h>
#include <proto/exec.h>
//#include <inline/exec.h>
//#include <inline/dos.h>

#include <sys/syscall.h>
#include <sys/exec.h>

#ifdef SHAREDL
#include <setjmp.h>
#include <stdio.h>
#include <powerup/gcclib/powerup_protos.h>
#include <powerup/ppclib/interface.h>
#include <powerup/ppclib/message.h>
#include <powerup/ppclib/object.h>
#include <powerup/ppclib/tasks.h>

#include <powerup/ppcproto/intuition.h>
#include <powerup/ppcproto/exec.h>
#include <powerup/ppcproto/dos.h>
#endif

#ifdef NATIVE_MORPHOS
#include <emul/emulinterface.h>
#include <emul/emulregs.h>
#endif

#define FROM_CRT0

#include <ix.h>

/* get the current revision number. Version control is automatically done by
 * OpenLibrary(), I just have to check the revision number
 */
#undef IX_VERSION
#include "ix_internals_backup.h"

#define MSTRING(x) STRING(x)
#define STRING(x) #x

struct ixemul_base *ixemulbase; // Used by the glue functions
#ifdef NATIVE_MORPHOS
int (**_ixbasearray)();
int __amigappc__ = 1;
int __abox__ = 1;
#ifdef SUPPORT_CTORS
extern void (*__ctrslist[])() __attribute__((section(".ctors")));
extern void (*__dtrslist[])() __attribute__((section(".dtors")));
#endif /* SUPPORT_CTORS */
#ifdef SUPPORT_INIT
static void call_init();
static void call_fini();
#endif /* SUPPORT_INIT */
#else
struct Library *ixrealbase;     // The real ixemul base (may be identical to ixemulbase, though)
#endif /* NATIVE_MORPHOS */

/*static*/ int start_stdio(int, char **, char **);
static int exec_entry(struct ixemul_base *ixembase, int argc, char *argv[], char *env[], long os);
static int ENTRY();

int extend_stack_ix_exec_entry(int argc, char **argv, char **environ, int *real_errno,
			       int (*main)(int, char **, char **));
int extend_stack_ix_startup(char *aline, int alen, int expand,
			    char *wb_default_window, unsigned main, int *real_errno);
void monstartup(char *lowpc, char *highpc);
void __init_stk_limit(void **limit, unsigned long argbytes);

/*
 * Have to take care.. I may not use any library functions in this file,
 * since they are exactly then used, when the library itself couldn't be
 * opened...
 */

extern int main();

/*
 * This is the first code executed.  Note that a_magic has to be at
 * a known offset from the start of the code section in order for
 * execve() to know that the program that it is starting uses the
 * ixemul library.
 *
 * The first instruction used to be a "jmp pc@(_ENTRY)", which when
 * assembled by gas 2.5.2 produces an 8 byte 68020 PC relative jump
 * rather than the desired 68000 short PC relative jump.  Changing
 * it to "jmp pc@(_ENTRY:W)" generated the right instruction but
 * a bad jmp offset.  So we use "jra _ENTRY" which seems to work
 * fine on all m68k.  However, since programs are now in use
 * linked with crt0 files with the 8 byte instruction, we need to
 * maintain the hack in execve() that allows either jmp, and now
 * the jra, and we need to ensure that a 16 bit displacement is
 * required to reach ENTRY() or else we will have to add yet
 * another pattern to match in execve().
 *
 */


#ifdef SHAREDL
asm("

");

// Not used by ELF loader
#define STACKSIZE 100000
#define str(s) #s
#define sstr(s) str(s)
asm("
      .text

extend_stack_ix_startup:
/*    stwu    1,-16(1)
      stw     5,12(1)
      lis     5,___stack@ha
      lwz     5,___stack@l(5)
      lwz     5,0(5)
      bl      __stkext_startup
      lwz     5,12(1)
      addi    1,1,16*/
no_stkext1:
      b       ix_startup

      .type   extend_stack_ix_startup,@function
      .size   extend_stack_ix_startup,$-extend_stack_ix_startup

      .data
      .globl  ___stack
      .ascii  \"StCk\"
___stack:
      .long   " sstr(STACKSIZE) "
      .ascii  \"sTcK\"
");
#elif defined(NATIVE_MORPHOS)
asm("
	.section    \".text\"

	b       ENTRY           /* by default jump to normal AmigaOS startup */

	/* this is a struct exec, for now only OMAGIC is supported */
	.globl  exec
exec:
	.short  1 /*__machtype      /* a_mid */
	.short  0407            /* a_magic = OMAGIC */
	.long   __text_size     /* a_text */
data_size:
	.long   __sdata_size    /* a_data */
bss_size:
	.long   __sbss_size     /* a_bss */
	.long   0               /* a_syms */
	.long   exec_entry      /* a_entry */
	.long   0               /* a_trsize */
	.long   0               /* a_drsize */

	/* word alignment is guaranteed */
	.section    \".sdata\"

");
#else
asm(" \n\
	.text \n\
 \n\
	jra     _ENTRY          | by default jump to normal AmigaOS startup \n\
	.align  2               | ensure exec starts at byte offset 4 \n\
 \n\
	| this is a struct exec, for now only OMAGIC is supported \n\
	.globl  exec \n\
exec: \n\
	.word   ___machtype     | a_mid \n\
	.word   0407            | a_magic = OMAGIC \n\
	.long   ___text_size    | a_text \n\
	.long   ___data_size    | a_data \n\
	.long   ___bss_size     | a_bss \n\
	.long   0               | a_syms \n\
	.long   _exec_entry     | a_entry \n\
	.long   0               | a_trsize \n\
	.long   0               | a_drsize \n\
 \n\
	| word alignment is guaranteed \n\
");
#endif

extern  int ix_expand_cmd_line; /* expand wildcards ? */
int     h_errno = 0;            /* networking error code */
struct  __res_state _res = {    /* Resolver state default settings */
	RES_TIMEOUT,            /* retransmition time interval */
	4,                      /* number of times to retransmit */
	RES_DEFAULT,            /* options flags */
	1                       /* number of name servers */
};
int     _res_socket = -1;       /* resolv socket used for communications */
char    *ix_default_wb_window = 0; /* Default Workbench output window name. */
int     errno = 0;              /* error results from the library come in here.. */
char    *_ctype_;               /* we use a pointer into the library, this is static anyway */
int     sys_nerr;               /* number of system error codes */
struct  ExecBase *SysBase;      /* ExecBase or pOS_ExecBase pointer */
struct  DosLibrary *DOSBase;    /* DOSBase or pOS_DosBase pointer */

struct  __sFILE **__sF;         /* pointer to stdin/out/err */
static  char *dummy_environ = 0;        /* dummy environment */
char    **environ = { &dummy_environ }; /* this is a default for programs not started via exec_entry */
char    *__progname = "";       /* program name */
int     ix_os = 0;              /* the OS that the program is running under */
				/* AmigaOS: 0 */
				/* pOS: 'pOS\0' */

extern  void *__stk_limit;
extern  unsigned long __stk_argbytes;

#ifdef __MORPHOS__
#define GET_VA_ARRAY(x) __va_overflow(x)
#else
#define GET_VA_ARRAY(x) x
#endif

static void ix_panic(void *SysBase, const char *msg, ...)
{
  struct IntuitionBase *IntuitionBase;
  va_list ap;

/* Use address 4 instead of the SysBase global as globals may not yet be
   available (a4-handling) */
#undef EXEC_BASE_NAME
#define EXEC_BASE_NAME *(void **)4

  va_start(ap, msg);

  IntuitionBase = (struct IntuitionBase *)OpenLibrary("intuition.library", 0);

  if (IntuitionBase)
    {
      struct EasyStruct panic = {
	sizeof(struct EasyStruct),
	0,
	"startup message",
	(char *)msg,
	"Abort"
      };

      EasyRequestArgs(NULL, &panic, NULL, GET_VA_ARRAY(ap));

      CloseLibrary((struct Library *) IntuitionBase);
    }
  va_end(ap);

#undef EXEC_BASE_NAME
#define EXEC_BASE_NAME SysBase
}


#ifdef BASECRT0
extern int __datadata_relocs();
/*#ifdef NATIVE_MORPHOS
extern int __sdata_size, __sbss_size;
#else
extern int __data_size, __bss_size;
#endif*/

#ifdef RCRT0
/* have to do this this way, or it is done base-relative.. */
#ifdef __PPC__
int __dbsize(void);
asm("
      .section  \".text\"
      .type	__dbsize,@function
      .globl    __dbsize
__dbsize:
      lis 3,data_size@ha
      lis 4,bss_size@ha
      lwz 3,data_size@l(3)
      lwz 4,bss_size@l(4)
      add 3,3,4
      blr
__end__dbsize:
      .size	__dbsize,__end__dbsize-__dbsize
");
#else
int __dbsize(void)
{
  int res;
  asm ("movel #___data_size,%0; addl #___bss_size,%0" : "=r" (res));
  return res;
}
#endif

static void inline
ix_resident(struct ixemul_base *base, int num, int a4, int size, void *relocs)
{
#ifdef NATIVE_MORPHOS
  base->basearray[SYS_ix_resident-1](num, a4, size, relocs);
#else
  typedef void (*func)(int, int, int, void *);

  ((func)((int)base - 6*(SYS_ix_resident + 4))) (num, a4, size, relocs);
#endif
}

#else

static void inline
ix_resident(struct ixemul_base *base, int num, int a4)
{
#ifdef NATIVE_MORPHOS
  base->basearray[SYS_ix_resident-1](num, a4);
#else
  typedef void (*func)(int, int);

  ((func)((int)base - 6*(SYS_ix_resident + 4))) (num, a4);
#endif
}
#endif
#endif /* BASECRT0 */

static int
exec_entry(struct ixemul_base *ixembase, int argc, char *argv[], char *env[], long os)
{
  struct ixemul_base *base = ixembase;

#ifdef BASECRT0
  register int a4 = 0;
  /* needed, so that data can be accessed. ix_resident might change this
     again afterwards */
#ifdef NATIVE_MORPHOS
  asm volatile ("lis 13,__r13_init@ha; addi 13,13,__r13_init@l" : "=r" (a4) : "0" (a4));
  asm volatile ("mr  %0,13" : "=r" (a4) : "0" (a4));
#else
  asm volatile ("lea    ___a4_init,a4" : "=r" (a4) : "0" (a4));
  asm volatile ("movel  a4,%0" : "=r" (a4) : "0" (a4));
#endif

#ifdef RCRT0
  ix_resident(base, 4, a4, __dbsize(), __datadata_relocs);
#else
  ix_resident(base, 2, a4);
#endif
#endif /* BASECRT0 */
  ixemulbase = base;
#ifndef NATIVE_MORPHOS
  ixrealbase = ixembase;
#else
  _ixbasearray = base->basearray;
#endif

  SysBase = *(void **)4;

  if (ixembase->ix_lib.lib_Version < IX_VERSION)
  {
    ix_panic(SysBase, "Need at least version " MSTRING (IX_VERSION) " of " IX_NAME ".");
    return W_EXITCODE(20, 0);
  }

  ix_os = os;

/*
 * Set the limit variable to finish the initialization of the stackextend code.
 */
  __init_stk_limit(&__stk_limit,__stk_argbytes);

  return extend_stack_ix_exec_entry(argc, argv, env, &errno, start_stdio);
}

/* this thing is best done with sprintf else, but it has to work without the
 * library as well ;-(
 */
__inline static char *
itoa(int num)
{
  short snum = num;

  /* how large can a long get...?? */
  /* Answer (by ljr): best method to use (in terms of portability)
     involves number theory.  The exact number of decimal digits
     needed to store a given number of binary digits is

	ceiling ( number_of_binary_digits * log(2) / log(10) )
     or
	ceiling ( number_of_binary_digits * 0.301029996 )

     Since sizeof evaluates to the number of bytes a given type takes
     instead of the number of bits, we need to multiply sizeof (type) by
     CHAR_BIT to obtain the number of bits.  Since an array size specifier
     needs to be integer type, we multiply by 302 and divide by 1000 instead
     of multiplying by 0.301029996.  Finally, we add 1 for the null terminator
     and 1 because we want the ceiling of the function instead of the floor.
     Funny thing about this whole affair is that you really wanted to know
     the size a short could expand to be and not a long...  :-) I know
     comments get out of date, etc.  The nice thing about this method is
     that the size of the array is picked at compile time based upon the
     number of bytes really needed by the local C implementation. */

  static char buf[sizeof snum * CHAR_BIT * 302 / 1000 + 1 + 1];
  char *cp;

  buf[sizeof buf - 1] = 0;
  cp = &buf[sizeof buf - 1];
  do
  {
    *--cp = (snum % 10) + '0';
    snum /= 10;
  } while (snum);

  return cp;
}

__inline static char *
pstrcpy(char *start, char *arg)
{
  while ((*start++ = *arg++)) ;
  return start - 1;
}

/* Note: This routine must be far enough away from the start of code
   so that the PC relative offset won't fit in a byte and the assembler
   will generate the right instruction pattern that execve() is looking
   for to know that this is a program that uses the ixemul.library. */

#ifdef NATIVE_MORPHOS
static int
ENTRY(UBYTE *aline, ULONG alen)
{
#else
static int
ENTRY(void)
{
  register unsigned char *rega0  asm("a0");
  register unsigned long  regd0  asm("d0");
  UBYTE *aline = rega0;
  ULONG alen = regd0;
#endif

#ifdef BASECRT0
  register int a4 = 0;
#endif /* BASECRT0 */
  struct ixemul_base *ibase;
  int res;

#ifdef BASECRT0
  /* needed, so that data can be accessed. ix_resident() might change this
     again afterwards */
#ifdef NATIVE_MORPHOS
  asm volatile ("lis 13,__r13_init@ha; addi 13,13,__r13_init@l" : "=r" (a4) : "0" (a4));
  asm volatile ("mr  %0,13" : "=r" (a4) : "0" (a4));
#else
  asm volatile("lea     ___a4_init,a4" : "=r" (a4) : "0" (a4));
  asm volatile("movel a4,%0" : "=r" (a4) : "0" (a4));
#endif
#endif /* BASECRT0 */

/* Use address 4 instead of the SysBase global as globals may not yet be
   available (a4-handling) */
#undef EXEC_BASE_NAME
#define EXEC_BASE_NAME *(void **)4

  ibase = (struct ixemul_base *)OpenLibrary(IX_NAME, IX_VERSION);

  if (ibase)
    {
      struct ixemul_base *base = ibase;

#ifdef BASECRT0
#ifdef RCRT0
      ix_resident(base, 4, a4, __dbsize(), __datadata_relocs);
#else
      ix_resident(base, 2, a4);
#endif
#endif /* BASECRT0 */

#ifdef NATIVE_MORPHOS
      _ixbasearray = base->basearray;
#endif

#ifndef NATIVE_MORPHOS
      ixrealbase = ibase;
#endif
      ixemulbase = base;

      SysBase = *(void **)4;

      ix_os = 0;

/*
 * Set the limit variable to finish the initialization of the stackextend code.
 */
	 
      __init_stk_limit(&__stk_limit,__stk_argbytes);

      res = extend_stack_ix_startup(aline, alen, ix_expand_cmd_line,
				    ix_default_wb_window, (int)start_stdio, &errno);

      CloseLibrary((struct Library *)base);
	 
    }
  else
    {
      struct Process *me = (struct Process *)((*(struct ExecBase **)4)->ThisTask);

      ix_panic(SysBase, "Need at least version " MSTRING (IX_VERSION) " of " IX_NAME ".");

      /* quickly deal with the WB startup message, as the library couldn't do
       * this for us. Nothing at all is done that isn't necessary to just shutup
       * workbench..*/
      if (!me->pr_CLI)
	{
	  Forbid();
	  ReplyMsg((WaitPort(&me->pr_MsgPort), GetMsg(&me->pr_MsgPort)));
	}

      res = 20;
    }

  return res;

#undef EXEC_BASE_NAME
#define EXEC_BASE_NAME SysBase
}

void ix_get_variables(int from_vfork_setup_child)
{
  /* more to follow ;-) */
  ix_get_vars(12, &_ctype_, &sys_nerr, (struct Library **)&SysBase, &DOSBase, &__sF,
	      &environ, (from_vfork_setup_child ? &environ : NULL),
	      (from_vfork_setup_child ? &errno : NULL), &h_errno,
	      &_res, &_res_socket, NULL);
}

#ifdef NATIVE_MORPHOS
#ifdef SUPPORT_CTORS
void call_dtors(void)
{
  int i, size;

  size = (int)__dtrslist[-2] / sizeof(__dtrslist[0]) - 2;

  for (i = 0; i < size; ++i) {
    if (__dtrslist[i])
      __dtrslist[i]();
  }
}
#endif
#endif


int
start_stdio(int argc, char **argv, char **env)
{
#ifndef BASECRT0
  extern void _etext ();
  extern void _mcleanup (void);
#endif /* BASECRT0 */

  ix_get_variables(0);
  environ = env;

#ifndef BASECRT0
#ifdef MCRT0
  atexit(_mcleanup);
  monstartup((char *)start_stdio, (char *)_etext);
#endif
#endif /* not BASECRT0 */

  if (argv[0])
    if ((__progname = "" /*strrchr(argv[0], '/')*/))
      __progname++;
    else
      __progname = argv[0];
  else
  {
    __progname = alloca(40);
    if (!GetProgramName(__progname, 40))
      __progname = "";
  }

#ifdef NATIVE_MORPHOS
#ifdef SUPPORT_CTORS
  atexit(call_dtors);
  {
    int i, size;

    size = (int)__ctrslist[-2] / sizeof(__ctrslist[0]) - 2;

    for (i = 0; i < size; ++i) {
      if (__ctrslist[i])
	__ctrslist[i]();
    }
  }
#endif
#endif

#ifdef SUPPORT_INIT
  atexit(call_fini);
  call_init();
#endif

  return main(argc, argv, env);
}

#ifdef NATIVE_MORPHOS
asm("
	.section \".text\"
	.type	extend_stack_ix_startup,@function
extend_stack_ix_startup:
"
#ifdef BASECRT0
  #ifdef LBASE
"       addis   11,13,__stack@drelha
	lwz     11,__stack@drell(11)"
  #else
"       lwz     11,__stack@sdarel(13)"
  #endif
#else
"       lis     11,__stack@ha
	lwz     11,__stack@l(11)"
#endif
"
	lis     10,ix_startup@ha
	addi    10,10,ix_startup@l
	b       __stkext_startup
__end_extend_stack_ix_startup:
	.size	extend_stack_ix_startup,__end_extend_stack_ix_startup-extend_stack_ix_startup

	.type	extend_stack_ix_exec_entry,@function
extend_stack_ix_exec_entry:
"
#ifdef BASECRT0
  #ifdef LBASE
"       addis   11,13,__stack@drelha
	lwz     11,__stack@drell(11)"
  #else
"       lwz     11,__stack@sdarel(13)"
  #endif
#else
"       lis     11,__stack@ha
	lwz     11,__stack@l(11)"
#endif
"
	lis     10,ix_exec_entry@ha
	addi    10,10,ix_exec_entry@l
	b       __stkext_startup
__end_extend_stack_ix_exec_entry:
	.size	extend_stack_ix_exec_entry,__end_extend_stack_ix_exec_entry-extend_stack_ix_exec_entry
");
#else
asm(" \n\
	.text \n\
 \n\
_extend_stack_ix_startup: \n\
"); 
#ifdef BASECRT0 
  #ifdef LBASE 
asm(" \n\
	movel   a4@(___stack:L),d0 \n");
  #else 
asm(" \n\
	movel   a4@(___stack:W),d0 \n");
  #endif 
#else 
asm("       movel   ___stack,d0 \n");
#endif 
asm (" \n\
	beq     no_stkext1 \n\
	jbsr    ___stkext_startup \n\
no_stkext1: \n\
	jmp     _ix_startup \n\
\n\
_extend_stack_ix_exec_entry: \n\
");
#ifdef BASECRT0
  #ifdef LBASE
asm("       movel   a4@(___stack:L),d0 \n");
  #else
asm("       movel   a4@(___stack:W),d0 \n");
  #endif
#else
asm("       movel   ___stack,d0 \n");
#endif
asm (" \n\
	beq     no_stkext2 \n\
	jbsr    ___stkext_startup \n\
no_stkext2: \n\
	jmp     _ix_exec_entry \n\
");
#endif

#ifdef NATIVE_MORPHOS
#ifndef BASECRT0
#ifdef CRT0
/*
 * null mcount and moncontrol,
 * just in case some routine is compiled for profiling
 */
asm(".globl mcount");
asm(".globl _moncontrol");
asm("_moncontrol:");
asm("mcount: blr");
#endif /* CRT0 */
#endif /* not BASECRT0 */
#else /* NATIVE_MORPHOS */
#ifndef BASECRT0
#ifdef CRT0
/*
 * null mcount and moncontrol,
 * just in case some routine is compiled for profiling
 */
asm(".globl mcount \n");
asm(".globl _moncontrol \n");
	asm("_moncontrol: \n");
asm("mcount: rts \n");
#endif /* CRT0 */
#endif /* not BASECRT0 */
#endif /* NATIVE_MORPHOS */

#ifndef BASECRT0
#ifdef MCRT0
#include "gmon.c"
#endif
#endif /* not BASECRT0 */

#ifdef NATIVE_MORPHOS
#ifdef SUPPORT_CTORS
asm("
	.section \".ctors\",\"a\",@progbits
__ctrslist:
	.long 0
");
asm("
	.section \".dtors\",\"a\",@progbits
__dtrslist:
	.long 0
");
#endif

#ifdef SUPPORT_INIT
asm("\n\
	.section \".text\",\"ax\",@progbits\n\
	.globl	__init
	.globl	__fini

call_init:
	lis	9,__init@ha
	addi	9,9,__init@l
	mtctr	9
	bctr

call_fini:
	lis	9,__fini@ha
	addi	9,9,__fini@l
	mtctr	9
	bctr
	");
#endif
#endif
