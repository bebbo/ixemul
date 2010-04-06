#include <inline/exec.h>
#include <machine/ansi.h>
//#include <string.h>
extern struct ExecBase  *SysBase;
#define COPY1LONG *(long *)s1=* (long *)s2; s1+=4;s2+=4;
#define COPY1WORD *(short *)s1=*(short *)s2; s1+=2;s2+=2;
#define COPY1BYTE *(char *)s1=*(char *)s2;

void *memcpy(void *s1,const void *s2,size_t n)
{ 
  
  if (s1 == s2)return s1; 
  switch (n) 
  {
	case 0: return s1;
	case 1: COPY1BYTE;return s1;
		
	case 2: COPY1WORD;return s1;
		
	case 3: COPY1WORD;COPY1BYTE;return s1;
		
    case 4: COPY1LONG;return s1;
		 
    case 5: COPY1LONG;COPY1BYTE;return s1;
		
    case 6: COPY1LONG;COPY1WORD;;return s1;
	    
    case 7: COPY1LONG;COPY1WORD;COPY1BYTE;return s1;
	    
    case 8: COPY1LONG;COPY1LONG; return s1;
	    
    case 9: COPY1LONG;COPY1LONG;COPY1BYTE; return s1;
	    
    case 10: COPY1LONG;COPY1LONG;COPY1WORD;return s1;
	    
    case 11: COPY1LONG;COPY1LONG;COPY1WORD;COPY1BYTE;return s1;
	    
    case 12: COPY1LONG;COPY1LONG;COPY1LONG;return s1;
	    
    case 13: COPY1LONG;COPY1LONG;COPY1LONG;COPY1BYTE;return s1;
	
	case 14: COPY1LONG;COPY1LONG;COPY1LONG;COPY1WORD;return s1;
    
	case 15: COPY1LONG;COPY1LONG;COPY1LONG;COPY1WORD;COPY1BYTE;return s1;
    
	case 16: COPY1LONG;COPY1LONG;COPY1LONG;COPY1LONG;return s1;
	default:
		CopyMem((APTR)s2,s1,n); return s1;
  }
}