/*
 *  This file is part of ixemul.library for the Amiga.
 *  Copyright (C) 1991, 1992  Markus M. Wild
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
 *  ixemul.h,v 1.1.1.1 1994/04/04 04:29:38 amiga Exp
 *
 *  ixemul.h,v
 * Revision 1.1.1.1  1994/04/04  04:29:38  amiga
 * Initial CVS check in.
 *
 *  Revision 1.4  1993/11/05  22:14:40  mwild
 *  changes there, here...
 *
 *  Revision 1.3  1992/10/20  16:32:33  mwild
 *  *** empty log message ***
 *
 *  Revision 1.2  1992/07/04  19:25:26  mwild
 *  change __rwport to reflect the current state of the (now global) async port
 *
 * Revision 1.1  1992/05/14  20:36:14  mwild
 * Initial revision
 *
 */

#ifndef _IXEMUL_H_
#define _IXEMUL_H_

#ifdef START

/* definitions for the assembler startup file */

/* when I've REALLY lots of free time, I'll rewrite header files, but now... */

#define _LVOOpenLibrary		-0x228
#define _LVOCloseLibrary 	-0x19e
#define _LVOAlert		-0x6c
#define _LVOFreeMem		-0xd2
#define _LVORemove		-0xfc

#define RTC_MATCHWORD	0x4afc
#define RTF_AUTOINIT	(1<<7)

#define LIBF_CHANGED	(1<<1)
#define LIBF_SUMUSED	(1<<2)
/* seems there is an assembler bug in expression evaluation here.. */
#define LIBF_CHANGED_SUMUSED 0x6
#define LIBF_DELEXP	(1<<3)
#define LIBB_DELEXP	3

#define LN_TYPE		8
#define LN_NAME		10
#define NT_LIBRARY	9
#define MP_FLAGS	14
#define PA_IGNORE	2

#define LIST_SIZEOF	14

#define THISTASK	276

#define INITBYTE(field,val)	.word 0xe000; .word (field); .byte (val); .byte 0
#define INITWORD(field,val)	.word 0xd000; .word (field); .word (val)
#define INITLONG(field,val)	.word 0xc000; .word (field); .long (val)

/*
 * our library base.. 
 */

/* custom part */
#define IXBASE_MYFLAGS		(IXBASE_LIBRARY + 0)
#define IXBASE_SEGLIST		(IXBASE_MYFLAGS + 2)
#define IXBASE_C_PRIVATE	(IXBASE_SEGLIST + 4)

#else  /* C-part */

#include <inline/stubs.h>
#include <sys/types.h>

#define _SIZE_T

#include <exec/types.h>
#include <exec/libraries.h>
#include <exec/execbase.h>
#include <exec/ports.h>
#include <libraries/dosextens.h>
#include <dos/notify.h>

#ifdef _KERNEL
#define _INTERNAL_FILE
#endif

#include <sys/file.h>
#include <sys/param.h>
#include <sys/ipc.h>
#include <packets.h>
#include <sys/syscall.h>
#include <sys/ixnet_syscall.h>
#include <sys/unix_socket.h>
#include <signal.h>

#include <user.h>
#include <errno.h>

extern struct ixlist timer_wait_queue;
extern struct ixlist timer_ready_queue;
extern struct ixlist timer_task_list;
extern ULONG timer_resolution;

/* ix_flags defines */

   #define ix_support_mufs				0x00020000
   #define ix_show_stack_usage				0x00010000
   #define ix_profile_method_mask 			0x0000C000
   #define ix_create_enforcer_hit 			0x00002000
   #define ix_do_not_flush_library 			0x00001000
   #define ix_allow_amiga_wildcard 			0x00000800
   #define ix_no_insert_disk_requester 			0x00000400
   #define ix_unix_pattern_matching_case_sensitive	0x00000200
/* #define ix_unix_pattern_matching 			0x00000100 obsolete */
/* #define ix_no_ces_then_open_console 			0x00000080 obsolete */
   #define ix_ignore_global_env 			0x00000040
/* #define ix_disable_fibcache 				0x00000020 obsolete */
/* #define ix_translate_dots 				0x00000010 obsolete */
/* #define ix_watch_stack 				0x00000008 obsolete */
/* #define ix_force_translation 			0x00000004 obsolete */
/* #define ix_translate_symlinks 			0x00000002 obsolete */
   #define ix_translate_slash				0x00000001

/* ix_profile_method defines */

/* only if the pc is in the program is a hit recorded */
#define IX_PROFILE_PROGRAM 	0x00000000

/* while the task is running, the last function in your program that was
   entered records a hit */
#define IX_PROFILE_TASK 	0x00004000

/* always record a hit (again the last function that was entered), even
   if another task is running */
#define IX_PROFILE_ALWAYS	0x00008000

/* not used */
#define IX_PROFILE_UNUSED	0x0000C000


/* ix_network_type enum */

enum
{
  IX_NETWORK_AUTO,
  IX_NETWORK_NONE,
  IX_NETWORK_AS225,
  IX_NETWORK_AMITCP,
  IX_NETWORK_END_OF_ENUM
};

/* configure this to the number of hash queues you like, 
 * use a prime number !!
 */
#define IX_NUM_SLEEP_QUEUES	31

/* the number of available ptys ( '/dev/pty[p-u][0-9a-f]' ) */
#define IX_NUM_PTYS		(6 * 16)

#define IX_PTY_SLAVE		0x03  /* 0011 */
#define IX_PTY_MASTER		0x0c  /* 1100 */

#define IX_PTY_OPEN		0x05  /* 0101 */
#define IX_PTY_CLOSE		0x0a  /* 1010 */

struct ixemul_base {
  struct Library		ix_lib;

/* don't use the remainder of the library base. The contents can change
   without warning in the future. */
#ifdef _KERNEL
  unsigned char			ix_myflags;	/* used by start.s */
  unsigned char			ix_pad;
  BPTR				ix_seg_list;	/* used by start.s */

  /* the global file table with current size */
  struct file			*ix_file_tab;
  struct file			*ix_fileNFILE;
  struct file			*ix_lastf;

  int				ix_membuf_limit;
  
  /* multiplier for id_BytesPerBlock to get to st_blksize, default 64 */
  int				ix_fs_buf_factor;

  unsigned long			ix_flags;

  struct ixlist		        ix_sleep_queues [IX_NUM_SLEEP_QUEUES];

  long                  	ix_gmt_offset;
  int				ix_network_type;

  int				ix_next_free_port;
  struct Task                  *ix_task_switcher;
  char		              **ix_global_environment;
  struct NotifyRequest          ix_notify_request;
  int                           ix_env_has_changed;
  char				ix_ptys[IX_NUM_PTYS];
  struct ix_unix_name          *ix_unix_names;
  struct Interrupt	        ix_itimerint;
  char                          ix_func_table[0];
#endif /* _KERNEL */
  struct ixlist 		ix_detached_processes;
};

#define syscall(vec, args...) \
  ({register int (*_sc)()=(void *)(&((char *)ixemulbase)[-((vec)+4)*6]); _sc(args);})

#define netcall(vec, args...) \
  ({register int (*_sc)()=(void *)(&((char *)u.u_ixnetbase)[-((vec)+4)*6]); _sc(args);})

/* Structure to pass ixemul.library settings to/from the library base */
struct ix_settings {
  int version;
  int revision;
  unsigned long flags;
  int membuf_limit;
  int red_zone_size;    /* obsolete */
  int fs_buf_factor;
  int network_type;
};

/* structure to keep track of the current SegList */
struct my_seg {
  BPTR	segment;	/* the thing our clients can use */
  enum { LOADSEG, RESSEG } type;
  char  *name;		/* name of the executable */
  BPTR  programdir;	/* lock of the program's directory */
  u_int	priv;		/* information depending on type */
};

/* message for the stack usage */
struct SUMessage {
  struct Message msg;
  u_long stack_usage;
  u_long stack_size;
  char name[1024];
};

/* Environment name of ixemul settings */
#define IX_ENV_SETTINGS "ixemul.prefs"

void ix_set_gmt_offset(long offset);
long ix_get_gmt_offset(void);
struct ix_settings *ix_get_default_settings(void);
struct ix_settings *ix_get_settings(void);
void ix_set_settings(struct ix_settings *settings);

/* Minimum stack size to use, used in vfork() and __stack */
#define STACKSIZE (16384)

#ifdef _KERNEL

#ifndef TF_LAUNCH
#define TF_SWITCH	(1L<<6)
#define TF_LAUNCH	(1L<<7)
#endif

#include <ixprotos.h>

extern struct ixemul_base *ixemulbase;
extern char *ixfakebase;

#ifdef NOTRAP
#define getuser(p)	  ((struct user *)(((struct Process *)(p))->pr_Task.tc_UserData))
#else
#define getuser(p)        ((struct user *)(((struct Process *)(p))->pr_Task.tc_TrapData))
#endif

struct user *safe_getuser(struct Process *);

extern int has_fpu;
extern int has_68010_or_up;
extern int has_68020_or_up;
extern int has_68030_or_up;
extern int has_68040_or_up;
extern int has_68060_or_up;

#ifndef AFB_68060
#define AFB_68060         (7)
#define AFF_68060         (1L<<7)
#endif

#define usetup            struct user *u_ptr=getuser(SysBase->ThisTask)

#define u		  (*u_ptr)
#define ix		  (*ixemulbase)

/* Used by __plock and all the functions called through __plock. */
struct lockinfo
{
  char buf[1024];
  char *name;

  struct StandardPacket sp;
  char str[257];
  BPTR bstr;    /* BCPL pointer to str[] */

  struct MsgPort *handler;

  BPTR parent_lock;
  int unlock_parent;
  int is_root;
  int is_fs;
  int link_levels;
  int result;
};

static inline u_int get_usp (void) 
{ 
  u_int res;

  asm volatile ("movel	usp,%0" : "=a" (res));
  return res;
}

static inline void set_usp (u_int new_usp)
{
  asm volatile ("movel  %0,usp" : /* no output */ : "a" (new_usp));
}

static inline u_int get_sp (void) 
{ 
  u_int res;

  asm volatile ("movel	sp,%0" : "=a" (res));
  return res;
}

static inline void set_sp (u_int new_sp)
{
  asm volatile ("movel  %0,sp" : /* no output */ : "a" (new_sp));
}

static inline u_short get_sr (void) 
{ 
  u_short res;

  asm volatile ("movew	sr,%0" : "=g" (res));
  return res;
}

static inline u_int get_fp (void) 
{ 
  u_int res;

  asm volatile ("movel	a5,%0" : "=g" (res));
  return res;
}

#define PRIVATE
#define OFFSET_FROM_1970 (8*365+2)

extern struct ExecBase *SysBase;
#include <inline/exec.h>

extern struct Library *muBase;
/* Multiuser inlines */
#include "multiuser_inlines.h"

extern struct DosLibrary *DOSBase;
#include <inline/dos.h>

#include <inline/alib.h>

#undef PRIVATE

#define errno (* u.u_errno)

/* useful define */
#define errno_return(e, r) do { errno = (e); return (r); } while (0)

#define __srwport (u.u_sync_mp)
#define __selport (u.u_select_mp)

#endif /* _KERNEL */

#endif /* START */

#endif /* _IXEMUL_H */
