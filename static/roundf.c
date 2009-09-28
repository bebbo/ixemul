__attribute__((always_inline)) inline float roundf(float x)
{ 
 if( x > 0.0 )return floor(x + 0.5);
 return ceil(x - 0.5);
}

