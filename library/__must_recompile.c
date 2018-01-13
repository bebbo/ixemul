#define _KERNEL
#include <ixemul.h>
#include <stdlib.h>

void __must_recompile (void)
{
  ix_panic ("Obsolete ixemul.library syscall number used.
Relink program with current ixemul crt0.o and libc.a.");
  exit (EXIT_FAILURE);
}
