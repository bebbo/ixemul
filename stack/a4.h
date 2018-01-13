#ifndef _A4_H_
#define _A4_H_

#ifdef BASEREL
  #define A4(x) "a4@(" #x ":W)"
#else
  #ifdef LBASEREL
    #define A4(x) "a4@(" #x ":L)"
  #else
    #define A4(x) #x
  #endif
#endif

#endif /* _A4_H_ */
