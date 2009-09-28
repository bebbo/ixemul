#define	M_LN2		0.69314718055994530942	/* log e2 */
__attribute__((always_inline)) inline double log2(double x)
{
 return (log(x) / M_LN2);
}

