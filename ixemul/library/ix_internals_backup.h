/* contain the fixed data offsets IXBASE_C_PRIVATE contain 100 bytes more than need*/

/* The size of struct user is 10648 bytes. */
#define IX_NAME         "ixemul.library"
#ifdef TRACE_LIBRARY
#define IX_IDSTRING     "ixemul 63.1 [notrap 68060, amigaos] (23.11.2009)"
#else
#define IX_IDSTRING     "ixemul 63.1 [notrap, 68060, amigaos] (23.11.2009)"
#endif
#define IX_VERSION      63
#define IX_REVISION     1
#define IX_PRIORITY     0

#define P_SIGMASK_OFFSET 0x140

#define U_ONSTACK_OFFSET 0x124
#define P_FLAG_OFFSET 0x130
#define IXBASE_FLAGS 14
#define IXBASE_NEGSIZE 16
#define IXBASE_POSSIZE 18
#define IXBASE_VERSION 20
#define IXBASE_REVISION 22
#define IXBASE_IDSTRING 24
#define IXBASE_SUM 28
#define IXBASE_OPENCNT 32
#define IXBASE_LIBRARY 34

#define IXBASE_SIZEOF (IXBASE_C_PRIVATE + 700) 
#define IXFAKEBASE_SIZE 0
#define USERPTR_OFFSET 88
#define IDNESTPTR_OFFSET 16
#define SPREGPTR_OFFSET 54
