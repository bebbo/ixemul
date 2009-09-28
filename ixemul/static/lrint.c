__attribute__((always_inline)) inline long lrint(double x)
{
	long value;
__asm ("        fmove%.l %1,     %0\n\t"
		 : "=d" (value)
		 : "f" (x));
	  return value;

} 