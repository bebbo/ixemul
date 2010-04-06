
__attribute__((always_inline)) inline float exp2f(float x)
{
 return (exp((double)x) * 0.693147180559945);
}

