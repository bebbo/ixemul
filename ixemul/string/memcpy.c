#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)memcpy.c    1.0 (mw)";
#endif /* LIBC_SCCS and not lint */

#define _KERNEL
#include <ixemul.h>
#include <inline/exec.h>
//#include <string.h>
#define COPY1LONG *(long *)s1=* (long *)s2; s1+=4;s2+=4;
#define COPY1WORD *(short *)s1=*(short *)s2; s1+=2;s2+=2;
#define COPY1BYTE *(char *)s1=*(char *)s2;
void *memcpy(void *s1,const void *s2,size_t n)
{ 
  
  if (s1 == s2)return 0; 
  switch (n) 
  {
	case 0: return;
	case 1: COPY1BYTE;return;
		
	case 2: COPY1WORD;return;
		
	case 3: COPY1WORD;COPY1BYTE;return;
		
    case 4: COPY1LONG;return;
		 
    case 5: COPY1LONG;COPY1BYTE;return;
		
    case 6: COPY1LONG;COPY1WORD;;return;
	    
    case 7: COPY1LONG;COPY1WORD;COPY1BYTE;return;
	    
    case 8: COPY1LONG;COPY1LONG; return ;
	    
    case 9: COPY1LONG;COPY1LONG;COPY1BYTE; return;
	    
    case 10: COPY1LONG;COPY1LONG;COPY1WORD;return;
	    
    case 11: COPY1LONG;COPY1LONG;COPY1WORD;COPY1BYTE;return;
	    
    case 12: COPY1LONG;COPY1LONG;COPY1LONG;return;
	    
    case 13: COPY1LONG;COPY1LONG;COPY1LONG;COPY1BYTE;return;
	
	case 14: COPY1LONG;COPY1LONG;COPY1LONG;COPY1WORD;return;
    
	case 15: COPY1LONG;COPY1LONG;COPY1LONG;COPY1WORD;COPY1BYTE;return;
    
	case 16: COPY1LONG;COPY1LONG;COPY1LONG;COPY1LONG;return;
	default:
		CopyMem((APTR)s2,s1,n); return;
  }
}

//void *
//memcpy(dst0, src0, length)
//	void *dst0;
//	const void *src0;
//	size_t length;
//{
//	CopyMem ((char *)src0, dst0, length);
//	return(dst0);
//}
