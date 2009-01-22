#include <exec/types.h>
#include <emul/emulinterface.h>
#include <emul/emulregs.h>
/* Don't include any std include, or gcc will report that prototypes
   don't match. */

#define FUNC_I_(func) \
	int func(void); \
	int _trampoline_ ## func(void) { \
	  return func(); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIB, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_I_I(func) \
	int func(int); \
	int _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  return func(p[1]); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIB, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_I_II(func) \
	int func(int, int); \
	int _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  return func(p[1], p[2]); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIB, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_I_III(func) \
	int func(int, int, int); \
	int _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  return func(p[1], p[2], p[3]); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIB, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_I_IIII(func) \
	int func(int, int, int, int); \
	int _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  return func(p[1], p[2], p[3], p[4]); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIB, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_I_IIIII(func) \
	int func(int, int, int, int, int); \
	int _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  return func(p[1], p[2], p[3], p[4], p[5]); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIB, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_I_IIIIII(func) \
	int func(int, int, int, int, int, int); \
	int _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  return func(p[1], p[2], p[3], p[4], p[5], p[6]); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIB, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_V_(func) \
	void func(void); \
	void _trampoline_ ## func(void) { \
	  func(); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIBNR, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_V_I(func) \
	void func(int); \
	void _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  func(p[1]); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIBNR, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_V_II(func) \
	void func(int, int); \
	void _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  func(p[1], p[2]); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIBNR, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_V_IIA(func) \
	void _varargs68k_ ## func(int, int, void *); \
	void _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  _varargs68k_ ## func(p[1], p[2], (void *)p[3]); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIBNR, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_V_III(func) \
	void func(int, int, int); \
	void _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  func(p[1], p[2], p[3]); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIBNR, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_V_IIII(func) \
	void func(int, int, int, int); \
	void _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  func(p[1], p[2], p[3], p[4]); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIBNR, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_V_IIIII(func) \
	void func(int, int, int, int, int); \
	void _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  func(p[1], p[2], p[3], p[4], p[5]); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIBNR, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_V_IIV(func) \
	void _varargs68k_ ## func(int, int, void *); \
	void _trampoline_ ## func(void) { \
	  GETEMULHANDLE \
	  int *p = (int *)REG_A7; \
	  _varargs68k_ ## func(p[1], p[2], p + 3); \
	} \
	const struct EmulLibEntry _gate_ ## func = { \
	  TRAP_LIBNR, 0, (void(*)())_trampoline_ ## func \
	};

#define FUNC_X(func) \
	extern struct EmulLibEntry _gate_ ## func;

#define SYSTEM_CALL(func, vec, args)  FUNC_##args(func)
#include <sys/ixnet_syscall.def>
#undef SYSTEM_CALL


