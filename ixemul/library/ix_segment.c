#define _KERNEL
#include <ixemul.h>
#include <ix.h>

static void setup_segment_info(void)
{
  usetup;
  
  u.u_segment_info.type = u.u_segment_no + 1;
  if (u.u_segment_info.type > IX_SEG_TYPE_MAX)
    u.u_segment_info.type = IX_SEG_TYPE_UNKNOWN;
  u.u_segment_info.start = (void *)(u.u_segment_ptr + 4);
  u.u_segment_info.size = *(unsigned long *)(u.u_segment_ptr - 4) - 8;
}

ix_segment *ix_get_first_segment(long seglist)
{
  usetup;

  u.u_segment_no = 0;
  u.u_segment_ptr = seglist << 2;
  setup_segment_info();
  return &u.u_segment_info;
}

ix_segment *ix_get_next_segment(void)
{
  usetup;

  u.u_segment_no++;
  u.u_segment_ptr = (*(long *)u.u_segment_ptr) << 2;
  if (u.u_segment_ptr == NULL)
    return NULL;
  setup_segment_info();
  return &u.u_segment_info;
}

