/*
 *  Written by Hans Verkuil (hans@wyst.hobby.nl)
 *
 *  battclock.resource patch idea was shamelessly stolen from the unixclock
 *  utility written by Geert Uytterhoeven. unixclock is available on Aminet.
 */

#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <ixemul.h>
#include <exec/memory.h>
#include <clib/exec_protos.h>
#include <dos/var.h>

#include <sys/time.h>
#include <proto/exec.h>
#include <proto/dos.h>
#include <proto/battclock.h>

#define LVOReadBattClock        (-12)
#define LVOWriteBattClock       (-18)

#ifdef __MORPHOS__
#define NewReadBattClock 	_NewReadBattClock
#define NewWriteBattClock 	_NewWriteBattClock
#define OldReadBattClock 	_OldReadBattClock
#define OldWriteBattClock 	_OldWriteBattClock
#define EndOfPatch 		_EndOfPatch
#define GMTOffset 		_GMTOffset
#endif

/* Flags. Currently only bit 0 is used to tell whether Daylight Saving Time
   is in effect or not. However, ixtimezone only sets this flag, but it doesn't
   test it. And neither does ixemul.library. In fact, the library completely
   ignores the flag field. */

#define DST_ON  0x01

typedef struct
{
  long          offset;
  unsigned char flags;
} ixtime;

struct Node *BattClockBase;

char VERSION[] = "\000$VER: ixtimezone 1.0 (10.11.95)";

/* battclock.resource patch code */
asm("
	.text
	.globl _NewReadBattClock
	.globl _NewWriteBattClock
	.globl _OldReadBattClock
	.globl _OldWriteBattClock
	.globl _GMTOffset
	.globl _EndOfPatch

_NewReadBattClock:
	.int    0x207a0014, 0x4e9090ba, 0x00164e75

/*      Since the GNU assembler is currently unable to properly compile
	PC-relative code, I'm using the hex-code directly. As soon as the
	assembler can handle PC-relative code, the line above should be
	replaced by:

	move.l          _OldReadBattClock(pc),a0
	jsr             (a0)
	sub.l           _GMTOffset(pc),d0
	rts
*/

_NewWriteBattClock:
	.int    0xd0ba0010, 0x207a0008
	.short  0x4ed0

/*      Since the GNU assembler is currently unable to properly compile
	PC-relative code, I'm using the hex-code directly. As soon as the
	assembler can handle PC-relative code, the line above should be
	replaced by:

	add.l           _GMTOffset(pc),d0
	move.l          _OldWriteBattClock(pc),a0
	jmp             (a0)
*/

_OldReadBattClock:
	.int            0
_OldWriteBattClock:
	.int            0
_GMTOffset:
	.int            0
_EndOfPatch:
");

extern char NewReadBattClock;
extern char NewWriteBattClock;
extern char OldReadBattClock;
extern char OldWriteBattClock;
extern long GMTOffset; 
extern long EndOfPatch;

static ixtime *read_ixtime(void)
{
  static ixtime t;
  char buf[10];
  
  /*
   * Read the GMT offset. This environment variable is 5 bytes long. The
   * first 4 form a long that contains the offset in seconds and the fifth
   * byte contains flags.
   */
  if (GetVar("IXGMTOFFSET", buf, 6, GVF_BINARY_VAR) == 5 && IoErr() == 5)
  {
    memcpy(&t.offset, buf, 4);
    t.flags = buf[4];
    return &t;
  }
  return NULL;
}

static void create_ixtime(ixtime *t, char *pathname)
{
  FILE *f = fopen(pathname, "w");
  
  if (f)
  {
    fwrite(t, 5, 1, f);
    fclose(f);
  }
}

static void write_ixtime(ixtime *t, int write_also_to_envarc)
{
  ix_set_gmt_offset(t->offset);
  create_ixtime(t, "/ENV/IXGMTOFFSET");
  if (write_also_to_envarc)
    create_ixtime(t, "/ENVARC/IXGMTOFFSET");
}

static void set_clock(long offset)
{
  struct timeval tv;
  
  gettimeofday(&tv, NULL);
  tv.tv_sec += offset;
  settimeofday(&tv, NULL);
}

static void reset_clock(void)
{
  struct timeval tv;
  
  tv.tv_usec = 0;
  tv.tv_sec = ReadBattClock();
  tv.tv_sec += (8*365+2) * 24 * 3600 + ix_get_gmt_offset();
  settimeofday(&tv, NULL);
}

static long *get_function_addr(void)
{
  return *((long **)((char *)BattClockBase + LVOReadBattClock + 2));
}

static void patch_batt_resource(long offset)
{
  char *mem;
  long size = (long)&EndOfPatch - (long)&NewReadBattClock;
  long mem_offset;
  long *oldread, *oldwrite, *gmt;

  BattClockBase = (struct Node *)OpenResource("battclock.resource");
  if (*(gmt = get_function_addr()) == 0x207a0014)
  {
    printf("battclock.resource was already patched.\n");
    gmt = (long *)(((char *)&GMTOffset) + (long)((char *)gmt - &NewReadBattClock));
    if (*gmt != offset)
    {
      *gmt = offset;
      reset_clock();
    }
    return;  /* already patched */
  }
  GMTOffset = offset;
  mem = AllocMem(size, MEMF_PUBLIC);
  mem_offset = mem - &NewReadBattClock;
  memcpy(mem, &NewReadBattClock, size);
  CacheClearE(mem, size, CACRF_ClearI);  /* clear instruction cache */
  oldread = (long *)(&OldReadBattClock + mem_offset);
  oldwrite = (long *)(&OldWriteBattClock + mem_offset);
  Disable();
  *oldread = (long)SetFunction((struct Library *)BattClockBase, LVOReadBattClock, (void *)(&NewReadBattClock + mem_offset));
  *oldwrite = (long)SetFunction((struct Library *)BattClockBase, LVOWriteBattClock, (void *)(&NewWriteBattClock + mem_offset));
  Enable();
  reset_clock();
  printf("patched battclock.resource.\n");
}

static void remove_patch(void)
{
  long *p, mem_offset, *oldread, *oldwrite;
  long size = (long)&EndOfPatch - (long)&NewReadBattClock;

  BattClockBase = (struct Node *)OpenResource("battclock.resource");
  if (*(p = get_function_addr()) != 0x207a0014)
  {
    printf("battclock.resource wasn't patched.\n");
    exit(0);  /* not patched */
  }
  mem_offset = (char *)p - &NewReadBattClock;
  oldread = (long *)(&OldReadBattClock + mem_offset);
  oldwrite = (long *)(&OldWriteBattClock + mem_offset);
  Disable();
  SetFunction((struct Library *)BattClockBase, LVOReadBattClock, (void *)*oldread);
  SetFunction((struct Library *)BattClockBase, LVOWriteBattClock, (void *)*oldwrite);
  Enable();
  FreeMem(p, size);
  reset_clock();
  printf("removed battclock.resource patch.\n");
  exit(0);
}

static void test(void)
{
  time_t t;
  
  time(&t);
  printf("GMT:   %s", asctime(gmtime(&t)));
  printf("Local: %s", asctime(localtime(&t)));
  exit(0);
}

static void usage(void)
{
  fprintf(stderr, "Usage: ixtimezone <option>
Where <option> is one of:

-test           Show GMT and localtime
-get-offset     Get GMT offset and patch ixemul.library
-check-dst      As -get-offset, but also automatically adjust the Amiga
		time if Daylight Saving Time has gone in effect (or vice
		versa)
-patch-resource As -get-offset, but also patch the battclock.resource
-remove-patch   Remove the battclock.resource patch\n");
  exit(1);
}

int main(int argc, char **argv)
{
  struct tm *local_tm, *gmt_tm;
  int local_hms, gmt_hms;
  time_t t, local_t, gmt_t;
  ixtime *old, new;
  int set_the_clock = 0, patch_resource = 0;
  int write_to_envarc = 0;

  if (argc != 2)
    usage();

  if (!strcmp(argv[1], "-test"))
    test();
  else if (!strcmp(argv[1], "-check-dst"))
    set_the_clock = 1;
  else if (!strcmp(argv[1], "-patch-resource"))
    patch_resource = 1;
  else if (!strcmp(argv[1], "-remove-patch"))
    remove_patch();
  else if (strcmp(argv[1], "-get-offset"))
    usage();

  /*
   * Get current time, both GMT and local.  
   * We don't care if these values are correct or not, we are only interested
   * in the difference between the two.
   */

  time(&t);
  gmt_tm = gmtime(&t);
  gmt_hms = gmt_tm->tm_hour * 3600 + gmt_tm->tm_min * 60 + gmt_tm->tm_sec;
  local_tm = localtime(&t);
  local_hms = local_tm->tm_hour * 3600 + local_tm->tm_min * 60 + local_tm->tm_sec;
  new.flags = (local_tm->tm_isdst ? DST_ON : 0);
  new.offset = 0;
  if (gmt_hms != local_hms)
  {
    /* They are not the same. So compute the difference between them */

    local_tm->tm_isdst = 0;     /* don't let these values influence the result! */
    local_tm->tm_zone = NULL;
    local_tm->tm_gmtoff = 0;
    local_t = mktime(local_tm);
    gmt_tm = gmtime(&t);
    gmt_tm->tm_isdst = 0;
    gmt_tm->tm_zone = NULL;
    gmt_tm->tm_gmtoff = 0;
    gmt_t = mktime(gmt_tm);
    new.offset = gmt_t - local_t;     /* obtain the difference */
  }

  old = read_ixtime();
  if (old == NULL || old->offset != new.offset)
  {
    write_to_envarc = 1;
    if (set_the_clock && old)
      set_clock(old->offset - new.offset);
  }
  write_ixtime(&new, write_to_envarc);
  if (patch_resource)
    patch_batt_resource(new.offset);
  return 0;
}
