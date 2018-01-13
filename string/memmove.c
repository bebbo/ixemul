#include <stdlib.h>
#include <string.h>

void *
memmove (void *dst0, const void *src0, size_t length)
{
  bcopy (src0, dst0, length);
  return dst0;
}

