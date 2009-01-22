#define _KERNEL
#include <exec/resident.h>
#include <exec/libraries.h>
#include "ixemul.h"
#include "ixnet.h"
#include "version.h"
#include "kprintf.h"

#define STR(x)  #x

struct ixnet_base *ixnet_init(struct ixnet_base *ixbase);
struct ixnet_base *ixnet_open(struct ixnet_base *ixbase);
void ixnet_close(struct ixnet_base *ixbase);
void ixnet_expunge(struct ixnet_base *ixbase);

int (*funcTable[])();
extern int (*ppcFuncTable[])();

struct ixnet_base* _initRoutine(struct ixnet_base *base,
				BPTR seglist,
				struct ExecBase *sysbase);

struct LibInitStruct {
  ULONG LibSize;
  void  *FuncTable;
  void  *DataTable;
  void  (*InitFunc)(void);
};

struct LibInitStruct Init={
  sizeof(struct ixnet_base),
  funcTable,
  NULL,
  (void (*)(void)) &_initRoutine
};


struct Resident initDDescrip={
  RTC_MATCHWORD,
  &initDDescrip,
  &initDDescrip + 1,
  RTF_PPC | RTF_AUTOINIT | RTF_EXTENDED,
  IXNET_VERSION,
  NT_LIBRARY,
  IXNET_PRIORITY,
  IXNET_NAME,
  IXNET_IDSTRING "\r\n",
  &Init,
  IXNET_REVISION,
  NULL
};

/*
 * To tell the loader that this is a new emulppc elf and not
 * one for the ppc.library.
 * ** IMPORTANT **
 */
ULONG   __abox__=1;


int libReserved(void) {
  return 0;
}

/* RTF_PPC flag set, so the initialization routine is called
 * as a normal PPC SysV4 ABI */
struct ixnet_base *_initRoutine(struct ixnet_base *base,
				BPTR seglist,
				struct ExecBase *sysbase) {
  KPRINTF(("init(%lx,%lx,%lx)\n",base,seglist,sysbase));
  base->ix_seg_list = seglist;
  base->basearray = ppcFuncTable;
  return ixnet_init(base);
}


struct ixnet_base *libOpen(void) {
  struct ixnet_base *base = (struct ixnet_base *) REG_A6;
  KPRINTF(("Open(%lx)\n",base));

  ++base->ixnet_lib.lib_OpenCnt;
  base->ixnet_lib.lib_Flags &= ~LIBF_DELEXP;
  if(!ixnet_open(base))
    {
      --base->ixnet_lib.lib_OpenCnt;
      base = NULL;
    }
  return base;
}

ULONG libExpungeFunc(struct ixnet_base *base) {
  ULONG seg;

  /* assume we can't expunge */
  seg = 0;
  base->ixnet_lib.lib_Flags |= LIBF_DELEXP;

  if (base->ixnet_lib.lib_OpenCnt == 0)
    {
      Remove(&base->ixnet_lib.lib_Node);

      ixnet_expunge(base);

      seg = base->ix_seg_list;

      FreeMem((char *)base - base->ixnet_lib.lib_NegSize,
	      base->ixnet_lib.lib_NegSize + base->ixnet_lib.lib_PosSize);
    }

  return seg;
}

ULONG libExpunge(void) {
  struct ixnet_base *base = (struct ixnet_base *) REG_A6;
  return libExpungeFunc(base);
}

ULONG libClose(void) {
  struct ixnet_base *base = (struct ixnet_base *) REG_A6;

  ixnet_close(base);

  if (--base->ixnet_lib.lib_OpenCnt == 0 &&
      base->ixnet_lib.lib_Flags & LIBF_DELEXP)
    return libExpungeFunc(base);

  return 0;
}


static const struct EmulLibEntry _gate_libOpen={
  TRAP_LIB, 0, (void(*)(void))libOpen
};

static const struct EmulLibEntry _gate_libClose={
  TRAP_LIB, 0, (void(*)(void))libClose
};

static const struct EmulLibEntry _gate_libExpunge={
  TRAP_LIB, 0, (void(*)(void))libExpunge
};

static const struct EmulLibEntry _gate_libReserved={
  TRAP_LIB, 0, (void(*)(void))libReserved
};

void obsolete(void) {}

static const struct EmulLibEntry _gate_obsolete={
  TRAP_LIBNR, 0, obsolete
};

/* declare the gates */

#define _gate_obsolete29 _gate_obsolete
#define _gate_obsolete30 _gate_obsolete
#define _gate_obsolete31 _gate_obsolete
#define _gate_obsolete32 _gate_obsolete
#define _gate_obsolete33 _gate_obsolete
#define _gate_obsolete34 _gate_obsolete
#define _gate_obsolete35 _gate_obsolete
#define _gate_obsolete36 _gate_obsolete
#define _gate_obsolete37 _gate_obsolete
#define _gate_obsolete40 _gate_obsolete
#define _gate_obsolete41 _gate_obsolete
#define _gate_obsolete47 _gate_obsolete
#define _gate_obsolete56 _gate_obsolete
#define _gate_obsolete57 _gate_obsolete
#define _gate_obsolete58 _gate_obsolete
#define _gate_obsolete59 _gate_obsolete
#define _gate_obsolete74 _gate_obsolete
#define _gate_obsolete75 _gate_obsolete
#define _gate_obsolete76 _gate_obsolete
#define _gate_obsolete77 _gate_obsolete
#define _gate_obsolete78 _gate_obsolete
#define _gate_obsolete79 _gate_obsolete
#define _gate_obsolete80 _gate_obsolete
#define _gate_obsolete81 _gate_obsolete
#define _gate_obsolete82 _gate_obsolete
#define _gate_obsolete83 _gate_obsolete
#define _gate_obsolete84 _gate_obsolete
#define _gate_obsolete95 _gate_obsolete
#define _gate_obsolete96 _gate_obsolete
#define _gate_obsolete97 _gate_obsolete
#define _gate_obsolete98 _gate_obsolete
#define _gate_obsolete99 _gate_obsolete
#define _gate_obsolete100 _gate_obsolete
#define _gate_obsolete102 _gate_obsolete

#ifdef TRACE_LIBRARY
#define SYSTEM_CALL(func, vec, nargs) extern const struct EmulLibEntry _gate_trace_ ## func;
#else
#define SYSTEM_CALL(func, vec, nargs) extern const struct EmulLibEntry _gate_ ## func;
#endif
#include <sys/ixnet_syscall.def>
#undef SYSTEM_CALL

/* The library 68k gates table. We use gates that read the function
 * arguments from the stack and put them in registers. */

int (*funcTable[])() = {

  /* standard system routines */
  (int(*)())&_gate_libOpen,
  (int(*)())&_gate_libClose,
  (int(*)())&_gate_libExpunge,
  (int(*)())&_gate_libReserved,

  /* my libraries definitions */

#ifdef TRACE_LIBRARY
#define SYSTEM_CALL(func, vec, nargs) (int(*)())&_gate_trace_ ## func,
#else
#define SYSTEM_CALL(func, vec, nargs) (int(*)())&_gate_ ## func,
#endif
#include <sys/ixnet_syscall.def>
#undef SYSTEM_CALL

  (int(*)())-1
};

/* The library ppc functions table. */
/* Hack: build that table in asm to avoid getting errors about
 * bad prototypes... */

asm("
	.globl      ppcFuncTable
	.section    \".rodata\"
	.align      2
	.type       ppcFuncTable,@object
ppcFuncTable:"

#define obsolete29 obsolete
#define obsolete30 obsolete
#define obsolete31 obsolete
#define obsolete32 obsolete
#define obsolete33 obsolete
#define obsolete34 obsolete
#define obsolete35 obsolete
#define obsolete36 obsolete
#define obsolete37 obsolete
#define obsolete40 obsolete
#define obsolete41 obsolete
#define obsolete47 obsolete
#define obsolete56 obsolete
#define obsolete57 obsolete
#define obsolete58 obsolete
#define obsolete59 obsolete
#define obsolete74 obsolete
#define obsolete75 obsolete
#define obsolete76 obsolete
#define obsolete77 obsolete
#define obsolete78 obsolete
#define obsolete79 obsolete
#define obsolete80 obsolete
#define obsolete81 obsolete
#define obsolete82 obsolete
#define obsolete83 obsolete
#define obsolete84 obsolete
#define obsolete95 obsolete
#define obsolete96 obsolete
#define obsolete97 obsolete
#define obsolete98 obsolete
#define obsolete99 obsolete
#define obsolete100 obsolete
#define obsolete102 obsolete
#ifdef TRACE_LIBRARY
#define SYSTEM_CALL(func, vec, nargs) ".long " STR(_trace_ ## func) "\n"
#else
#define SYSTEM_CALL(func, vec, nargs) ".long " STR(func) "\n"
#endif
#include <sys/ixnet_syscall.def>
#undef SYSTEM_CALL

"endppcFuncTable:
	.size    ppcFuncTable,endppcFuncTable-ppcFuncTable
");


