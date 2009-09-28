//used from libnix and modify

//#include <math.h>
      
double fmod(double x,double y)
{
  double a= x / y;
  if(a >= 0)
    return x - y*floor(a);
  else
    return x - y*ceil(a);
}
