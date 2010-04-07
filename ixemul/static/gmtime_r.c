/*
 * gmtime_r.c -  ripped from pandora
 */

#include <time.h>
#include <string.h>

struct tm *gmtime_r (t, tp)
     const time_t *t;
     struct tm *tp;
{
  struct tm *tp0;

  tp0 = gmtime(t);
  memcpy(tp, tp0, sizeof(*tp));
  return tp;
}
