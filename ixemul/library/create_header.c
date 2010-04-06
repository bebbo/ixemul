#define _KERNEL
#include "ixemul.h"
#include <stddef.h>
#include <stdio.h>
  
#define LIB struct Library
#define TASK struct Task
 
int main(int argc, char **argv)
{
  int extra = 0;

  printf ("/* This header has been generated by the create_header tool.\n   DO NOT EDIT! */\n\n");
  printf ("/* The size of struct user is %ld bytes. */\n\n", sizeof(struct user));
  printf ("#define P_SIGMASK_OFFSET 0x%x\n\n",
  	  (unsigned)offsetof (struct user, p_sigmask));
  printf ("#define U_ONSTACK_OFFSET 0x%x\n",
  	  (unsigned)offsetof (struct user, u_onstack));
  printf ("#define P_FLAG_OFFSET 0x%x\n",
  	  (unsigned)offsetof (struct user, p_flag));

  printf ("#define IXBASE_FLAGS %ld\n", offsetof(LIB, lib_Flags));
  printf ("#define IXBASE_NEGSIZE %ld\n", offsetof(LIB, lib_NegSize));
  printf ("#define IXBASE_POSSIZE %ld\n", offsetof(LIB, lib_PosSize));
  printf ("#define IXBASE_VERSION %ld\n", offsetof(LIB, lib_Version));
  printf ("#define IXBASE_REVISION %ld\n", offsetof(LIB, lib_Revision));
  printf ("#define IXBASE_IDSTRING %ld\n", offsetof(LIB, lib_IdString));
  printf ("#define IXBASE_SUM %ld\n", offsetof(LIB, lib_Sum));
  printf ("#define IXBASE_OPENCNT %ld\n", offsetof(LIB, lib_OpenCnt));
  printf ("#define IXBASE_LIBRARY %ld\n", sizeof(LIB));

  printf ("#define IXBASE_SIZEOF (IXBASE_C_PRIVATE + %ld)\n",
	  (sizeof (struct ixemul_base) - offsetof (struct ixemul_base, ix_seg_list) - 4) + extra * 6);
  printf ("#define IXFAKEBASE_SIZE %d\n", extra * 6);

#ifdef NOTRAP
  //printf ("#define USERPTR_OFFSET %ld\n", offsetof (TASK, tc_TrapData));
  printf ("#define USERPTR_OFFSET %ld\n", offsetof (TASK, tc_UserData));
#else  
  printf ("#define USERPTR_OFFSET %ld\n", offsetof (TASK, tc_TrapData));
#endif
  printf ("#define IDNESTPTR_OFFSET %ld\n", offsetof (TASK, tc_IDNestCnt));
  printf ("#define SPREGPTR_OFFSET %ld\n", offsetof (TASK, tc_SPReg));
  return 0;
}
