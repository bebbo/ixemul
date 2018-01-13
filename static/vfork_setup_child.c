extern void ix_get_variables(int);  /* from crt0.c! */
extern void ix_resident();
extern int __datadata_relocs();
void __init_stk_limit(void **limit, unsigned long argbytes);
extern void *__stk_limit;
extern unsigned long __stk_argbytes;

static inline unsigned int get_a4(void)
{
  unsigned int res;

  asm ("movel a4,%0" : "=g" (res));
  return res;
}

static inline int dbsize(void)
{
  int res;

  asm ("movel #___data_size,%0; addl #___bss_size,%0" : "=r" (res));
  return res;
}

void vfork_setup_child(void)
{
  /* this re-relocates the data segment */
  ix_resident(4, get_a4(), dbsize(), __datadata_relocs);

  /* pass the new addresses for internal variables (e.g. errno) to
     ixemul. Pass '1' to tell ix_get_variables that it is called from
     here */
  ix_get_variables(1);

  /* Set the limit variable to finish the initialization of the stackextend
     code.  */
  __init_stk_limit(&__stk_limit, __stk_argbytes);
}
