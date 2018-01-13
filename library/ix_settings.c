#define _KERNEL
#include <ixemul.h>
#include "version.h"

long ix_get_gmt_offset(void)
{
  return ix.ix_gmt_offset;
}

void ix_set_gmt_offset(long offset)
{
  ix.ix_gmt_offset = offset;
}

struct ix_settings *ix_get_settings(void)
{
  static struct ix_settings settings;

  settings.version = IX_VERSION;
  settings.revision = IX_REVISION;
  settings.flags = ix.ix_flags;
  settings.membuf_limit = ix.ix_membuf_limit;
  settings.red_zone_size = 0; /* obsolete */
  settings.fs_buf_factor = ix.ix_fs_buf_factor;
  settings.network_type = ix.ix_network_type;
  return &settings;
}

struct ix_settings *ix_get_default_settings(void)
{
  static struct ix_settings default_settings =
  {
    IX_VERSION,
    IX_REVISION,
    ix_translate_slash | ix_no_insert_disk_requester | ix_allow_amiga_wildcard,
    0,			/* membuf_limit  */
    0,			/* red_zone_size */
    64,			/* fs_buf_factor */
    IX_NETWORK_AUTO	/* network_type */
  };

  return &default_settings;
}

void ix_set_settings(struct ix_settings *settings)
{
  if (ix.ix_flags & ix_do_not_flush_library)
    if (!(settings->flags & ix_do_not_flush_library))
      ix.ix_lib.lib_OpenCnt--;
  if (!(ix.ix_flags & ix_do_not_flush_library))
    if (settings->flags & ix_do_not_flush_library)
      ix.ix_lib.lib_OpenCnt++;
  ix.ix_flags = settings->flags;
  if (settings->membuf_limit >= 0)
    ix.ix_membuf_limit = settings->membuf_limit;
  if (settings->fs_buf_factor > 0)
    ix.ix_fs_buf_factor = settings->fs_buf_factor;
  if (settings->network_type >= 0 && settings->network_type < IX_NETWORK_END_OF_ENUM)
    ix.ix_network_type = settings->network_type;
}

long ix_get_long(unsigned long id, unsigned long extra)
{
  usetup;

  switch (id)
  {
    case IXID_VERSION:
      return ix.ix_lib.lib_Version;

    case IXID_REVISION:
      return ix.ix_lib.lib_Revision;

    case IXID_USERDATA:
      return (long)u.u_user;

    case IXID_USER:
      return (long)&u;

    case IXID_A4_PTRS:
      return u.u_a4_pointers_size;

    case IXID_HAVE_FPU:
      return has_fpu;

    case IXID_CPU:
      if (has_68060_or_up)
        return IX_CPU_68060;
      if (has_68040_or_up)
        return IX_CPU_68040;
      if (has_68030_or_up)
        return IX_CPU_68030;
      if (has_68020_or_up)
        return IX_CPU_68020;
      if (has_68010_or_up)
        return IX_CPU_68010;
      return IX_CPU_68000;

    case IXID_OS:
      return OS_IS_AMIGAOS;

    case IXID_OFILE:
      return u.u_ofile;

    case IXID_ENVIRON:
      return u.u_environ;
      
    case IXID_EXPAND_CMD_LINE:
      return u.u_expand_cmd_line;
  }
  return -1;
}

long ix_set_long(unsigned long id, long value)
{
  usetup;
  long result = -1;

  switch (id)
  {
    case IXID_USERDATA:
      result = (long)u.u_user;
      u.u_user = (void *)value;
      break;
   
    case IXID_ENVIRON:
      result = (long)u.u_environ;
      u.u_environ = (void *)value;
      break;
      
    case IXID_EXPAND_CMD_LINE:
      result = (long)u.u_expand_cmd_line;
      u.u_expand_cmd_line = value;
      break;
  }
  return result;
}
