__attribute__((always_inline)) inline double round(double x)
{ 
 if( x > 0.0 )return floor(x + 0.5);
 return ceil(x - 0.5);
}


