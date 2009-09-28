 
__attribute__((always_inline)) inline long lrintf(float x)
{
	long value;
__asm ("        fmove%.l %1,     %0\n\t"
		 : "=d" (value)
		 : "f" (x));
	  return value;

} 
