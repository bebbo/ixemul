#define _KERNEL
#include "ixemul.h"

void abort (void)
{
  syscall (SYS_kill, 0, SIGABRT);
  /* if this should be caught, do exit directly... */
  ix_panic ("ABORT!");
  exit(20);
}
