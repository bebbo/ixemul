/*
 *  This file is part of the ixemul package for the Amiga.
 *  Copyright (C) 1994 Rafael W. Luebbert
 *  Copyright (C) 1997 Hans Verkuil
 *
 *  This source is placed in the public domain.
 */

#if 0
#include <stdio.h>
#include <inline/exec.h>
#undef FOR_LIBC // No longer supported since pOS is dead...
#if defined(FOR_LIBC)

#define _KERNEL
#include <ixemul.h>

#ifdef FOR_LIBC

typedef char CHAR;
#include <pInline/pExec2.h>
#include "a4.h"

#undef SysBase
extern struct ExecBase *SysBase;
extern void *gb_ExecLib;
#define gb_ExecBase SysBase
static void pos_kprintf(const char *format, ...)

#else

void KPrintF(const char *format, ...)

#endif
{
  pOS_VKPrintf(format, (ULONG *)(&format + 1));
}
#endif
//#define kprintf KPrintF
//int vkprintf(const unsigned char *fmt, _BSD_VA_LIST_ args)
//{
//   unsigned char buf[255];
//   int i, ret;
//
//   ret = vsprintf(buf, fmt, args);
//
//   for(i = 0; buf[i]; i++)
//   {
//      RawPutChar(buf[i]);
//   }
//
//   return ret;
//}
//
//int KPrintF(const unsigned char * fmt, ...)
//{
//   _BSD_VA_LIST_ ap;
//   int result;
//
//   va_start(ap, fmt);
//   result = vkprintf(fmt, ap);
//   va_end(ap);
//
//   return result;
//}


//#if !defined(NATIVE_MORPHOS)
//__asm(".globl  _KPrintF \n\
//\n\
//KPutChar: \n\
//	movel   a6,sp@- \n\
//	movel   4:W,a6 \n\
//	jsr     a6@(-516:W) \n\
//	movel   sp@+,a6 \n\
//	rts \n\
//\n\
//KDoFmt: \n\
//	movel   a6,sp@- \n\
//	movel   4:W,a6 \n\
//	jsr     a6@(-522:W) \n\
//	movel   sp@+,a6 \n\
//	rts \n\
//\n\
//_KPrintF: \n\
//	lea     sp@(4),a1 \n\
//	movel   a1@+,a0 \n\
//	movel   a2,sp@- \n\
//	lea     KPutChar,a2 \n\
//	jbsr    KDoFmt \n\
//	movel   sp@+,a2 \n\
//	rts \n\
//"); 
//
//#endif
#endif
