#ifndef MY_VARARGS_H
#define MY_VARARGS_H

#include <stdarg.h>

#ifdef NATIVE_MORPHOS

struct my_va_list {
    union {
	va_list ppc;
	char*   m68k;
    } args;
    int is_68k;
};
typedef struct my_va_list my_va_list;

#define my_va_start(ap,x)       (va_start((ap).args.ppc,x),(ap).is_68k=0)
#define my_va_end(ap)           va_end((ap).args.ppc)

#define my_va_start_68k(ap,x)   ((ap).args.m68k=((char*)&(x)+sizeof(x)),(ap).is_68k=1)
#define my_va_end_68k(ap)       ((void)0)

#define my_va_init_ppc(ap,ap1) (__va_copy((ap).args.ppc,(ap1)),(ap).is_68k=0)
#define my_va_init_68k(ap,ap1) ((ap).args.m68k=(ap1),(ap).is_68k=1)
#define my_va_init(ap,ap1) (__va_copy((ap).args.ppc,(ap1)),(ap).is_68k=0)

#define my_va_arg(ap,T) \
    ((ap).is_68k ? \
	((T*)(ap.args.m68k += sizeof(T)))[-1] : \
	(va_arg(ap.args.ppc,T)))

#else

typedef va_list my_va_list;
#define my_va_start(ap,x)       va_start(ap,x)
#define my_va_end(ap)           va_end(ap)

#define my_va_init(ap,ap1)      ((ap)=(ap1))

#define my_va_arg(ap,T)         va_arg(ap,T)

#endif

#endif
