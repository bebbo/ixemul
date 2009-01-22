#ifdef NATIVE_MORPHOS
#include <exec/types.h>
#include <emul/emulinterface.h>
#include <emul/emulregs.h>
#endif

extern void ix_get_variables(int);  /* from crt0.c! */
extern void ix_resident();
extern int __datadata_relocs();
void __init_stk_limit(void **limit, unsigned long argbytes);
extern void *__stk_limit;
extern unsigned long __stk_argbytes;

static inline unsigned int get_a4(void)
{
  unsigned int res;
#ifdef NATIVE_MORPHOS
  asm ("mr %0,13" : "=g" (res));
#else
  asm ("movel a4,%0" : "=g" (res));
#endif
  return res;
}

extern int __dbsize(void);

void vfork_setup_child(void)
{
  /* this re-relocates the data segment */
  ix_resident(4, get_a4(), __dbsize(), __datadata_relocs);

  /* pass the new addresses for internal variables (e.g. errno) to
     ixemul. Pass '1' to tell ix_get_variables that it is called from
     here */
  ix_get_variables(1);

  /* Set the limit variable to finish the initialization of the stackextend
     code.  */
  __init_stk_limit(&__stk_limit, __stk_argbytes);
}
