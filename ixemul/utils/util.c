#include <stdio.h>
#include "ixemul.h"
#include "version.h"
#include "ixprefs.h"

static struct ix_settings settings;

void
About(void)
{
  /* routine when (sub)item "About" is selected. */
  ShowRequester(
    "Ixprefs " IXPREFS_VERSION " - Ixemul.library configuration program\n"
    "Copyright \251 1995-1997 Kriton Kyrimis\n"
    "Copyright \251 2009 Diego Casorran\n\n"
    "This program is free software; you can redistribute it\n"
    "and/or modify it under the terms of the GNU General\n"
    "Public License as published by the Free Software Foundation;\n"
    "either version 2 of the License, or (at your option)\n"
    "any later version.\n\n"
    "GUI designed using GadToolsBox 2.0c by Jan van den Baard",
    0, "OK");
}

void
ReadFromSettings(struct ix_settings *settings)
{
  translateslash = (settings->flags & ix_translate_slash) != 0;
  cases = (settings->flags & ix_unix_pattern_matching_case_sensitive) != 0;
  suppress = (settings->flags & ix_no_insert_disk_requester) != 0;
  amigawildcard = (settings->flags & ix_allow_amiga_wildcard) != 0;
  noflush = (settings->flags & ix_do_not_flush_library) != 0;
  ignoreenv = (settings->flags & ix_ignore_global_env) != 0;
  stackusage = (settings->flags & ix_show_stack_usage) != 0;
  enforcerhit = (settings->flags & ix_create_enforcer_hit) != 0;
  mufs = (settings->flags & ix_support_mufs) != 0;
  membuf = settings->membuf_limit;
  blocks = settings->fs_buf_factor;
  networking = settings->network_type;
  profilemethod = (settings->flags >> 14) & 3;
  watchAvailMem = (settings->flags & ix_watch_availmem) != 0;
  catchfailedallocs = (settings->flags & ix_catch_failed_malloc) != 0;
  killappallocerr = (settings->flags & ix_kill_app_on_failed_malloc) != 0;
}

void Defaults(void)
{
  ReadFromSettings(ix_get_default_settings());
}

int
LastSaved(void)
{
  FILE *f;
  int status;

  f = fopen(CONFIGFILE, "r");
  if (f) {
    fread(&settings, sizeof(settings), 1, f);
    fclose(f);
    ReadFromSettings(&settings);
    status = 0;
  }else{
    ShowRequester("Can't open " CONFIGFILE, 0, "OK");
    status = 1;
  }
  return status;
}

int
Use(void)
{
  FILE *f;
  unsigned long new_flags;
  int status;

#if 0
  membufClicked();
  blocksClicked();
#endif
  new_flags =
    (translateslash ? ix_translate_slash : 0)
  | (cases ? ix_unix_pattern_matching_case_sensitive : 0)
  | (suppress ? ix_no_insert_disk_requester : 0)
  | (amigawildcard ? ix_allow_amiga_wildcard : 0)
  | (noflush ? ix_do_not_flush_library : 0)
  | (ignoreenv ? ix_ignore_global_env : 0)
  | (stackusage ? ix_show_stack_usage : 0)
  | (enforcerhit ? ix_create_enforcer_hit : 0)
  | (mufs ? ix_support_mufs : 0)
  | (watchAvailMem ? ix_watch_availmem : 0)
  | (catchfailedallocs ? ix_catch_failed_malloc : 0)
  | (killappallocerr ? ix_kill_app_on_failed_malloc : 0)
  | (profilemethod << 14);
  settings.version = IX_VERSION;
  settings.revision = IX_REVISION;
  settings.flags = new_flags;
  settings.membuf_limit = membuf;
  settings.fs_buf_factor = blocks;
  settings.network_type = networking;
  ix_set_settings(&settings);
  f = fopen(ENVFILE, "w");
  if (f) {
    fwrite(&settings, sizeof(settings), 1, f);
    fclose(f);
    status = 0;
  } else {
    ShowRequester("Can't open " ENVFILE, 0, "OK");
    status = 1;
  }
  return status;
}

int
Save(void)
{
  FILE *f;
  int status;

  Use();
  f = fopen(CONFIGFILE, "w");
  if (f) {
    fwrite(&settings, sizeof(settings), 1, f);
    fclose(f);
    status = 0;
  }else{
    ShowRequester("Can't open " CONFIGFILE, 0, "OK");
    status = 1;
  }
  return status;
}

void LoadSettings(void)
{
  struct ix_settings *settings;

  settings = ix_get_settings();
  ReadFromSettings(settings);
}
