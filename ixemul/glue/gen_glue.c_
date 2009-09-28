#include <sys/types.h>
#include <sys/syscall.h>

#include <stdio.h>

/**
 * May 2009, Diego Casorran: Ixemul is now able to be loaded dinamically
 * depending which functions are used. That means, if you use V49 SDK and
 * only V48 functions are used, the requested ixemul.library will be V48.
 * Unfortunately, this only seems to work with no-baserel code.. Anyhow,
 * thats the 90% of the software, i guess.
 */
#define HANDLE_DINAMIC_IXEMUL_OPEN	1

#if HANDLE_DINAMIC_IXEMUL_OPEN
# define SYSTEM_CALL_APIV( HEXVER, REV ) {(char *)(0xFFFF##HEXVER),REV},
#endif /* HANDLE_DINAMIC_IXEMUL_OPEN */

struct syscall {
  char *name;
  int   vec;
} syscalls[] = {
#define SYSTEM_CALL(func,vec) { #func, vec},
#include <sys/syscall_68k.def>
#undef SYSTEM_CALL
#undef SYSTEM_CALL_APIV
};

int nsyscall = sizeof(syscalls) / sizeof (syscalls[0]);

#define IXEMULBASE "_ixemulbase"
#define IXEMULBASELEN 11

#define BASEREL_OFFSET1 19
#define BASEREL_OFFSET2 31

/* The following code is a hexdump of this assembly program:

		.globl	_FUNCTION
_FUNCTION:	movel	a4@(_ixemulbase:W),a0
		jmp	a0@(OFFSET:w)
*/

static short baserel_code[] = {
  0x0000, 0x0107, 0x0000, 0x0008, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0018, 0x0000, 0x0000, 0x0000, 0x0008, 0x0000, 0x0000,
  0x206c, 0x0000, 0x4ee8, 0xff46, 0x0000, 0x0002, 0x0000, 0x0138,
  0x0000, 0x0004, 0x0500, 0x0000, 0x0000, 0x0000, 0x0000, 0x0011,
  0x0100, 0x0000, 0x0000, 0x0000
};

#define LARGE_BASEREL_OFFSET1 21
#define LARGE_BASEREL_OFFSET2 33

/* The following code is a hexdump of this assembly program:

		.globl	_FUNCTION
_FUNCTION:	movel	a4@(_ixemulbase:L),a0
		jmp	a0@(OFFSET:w)
*/

static short large_baserel_code[] = {
  0x0000, 0x0107, 0x0000, 0x000c, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0018, 0x0000, 0x0000, 0x0000, 0x0008, 0x0000, 0x0000,
  0x2074, 0x0170, 0x0000, 0x0000, 0x4ee8, 0xffe2, 0x0000, 0x0004,
  0x0000, 0x0158, 0x0000, 0x0004, 0x0500, 0x0000, 0x0000, 0x0000,
  0x0000, 0x000e, 0x0100, 0x0000, 0x0000, 0x0000
};

#if HANDLE_DINAMIC_IXEMUL_OPEN

#define NO_BASEREL_OFFSET1 22
#define NO_BASEREL_OFFSET2 35
#define NO_BASEREL_OFFSET3 16
#define NO_BASEREL_OFFSET4 17
#define NO_BASEREL_OFFSET5 41

/* The following code is a hexdump of this assembly program:

		.globl	_FUNCTION
		.globl  _VAR
_VAR:		.long	(VALUE)
_FUNCTION:	movel	_ixemulbase,a0
		jmp	a0@(OFFSET:w)
*/

static short no_baserel_code[] = {
  0x0000, 0x0107, 0x0000, 0x0010, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0024, 0x0000, 0x0000, 0x0000, 0x0008, 0x0000, 0x0000,
  0x0030, 0x0002, 0x2079, 0x0000, 0x0000, 0x4ee8, 0xffe2, 0x0000,
  0x0000, 0x0006, 0x0000, 0x0250, 0x0000, 0x0004, 0x0500, 0x0000,
  0x0000, 0x0004, 0x0000, 0x000B, 0x0500, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0017, 0x0100, 0x0000, 0x0000, 0x0000
};

#else /* ! HANDLE_DINAMIC_IXEMUL_OPEN */

#define NO_BASEREL_OFFSET1 20
#define NO_BASEREL_OFFSET2 33

/* The following code is a hexdump of this assembly program:

		.globl	_FUNCTION
_FUNCTION:	movel	_ixemulbase,a0
		jmp	a0@(OFFSET:w)
*/

static short no_baserel_code[] = {
  0x0000, 0x0107, 0x0000, 0x000c, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0018, 0x0000, 0x0000, 0x0000, 0x0008, 0x0000, 0x0000,
  0x2079, 0x0000, 0x0000, 0x4ee8, 0xffe2, 0x0000, 0x0000, 0x0002,
  0x0000, 0x0150, 0x0000, 0x0004, 0x0500, 0x0000, 0x0000, 0x0000,
  0x0000, 0x000b, 0x0100, 0x0000, 0x0000, 0x0000
};

#endif /* HANDLE_DINAMIC_IXEMUL_OPEN */

#define PROFILING_OFFSET1 29
#define PROFILING_OFFSET2 51
#define PROFILING_OFFSET3 57
#define PROFILING_OFFSET4 63

/* The following code is a hexdump of this assembly program:

		.globl	_FUNCTION
_FUNCTION:
		.data
PROFFUNCTION:
		.long	0

		.text
		link	a5,#0
		lea	PROFFUNCTION,a0
		jsr	mcount
		unlk	a5
		movel	_ixemulbase,a0
		jmp	a0@(OFFSET:w)
*/

static short profiling_code[] = {
  0x0000, 0x0107, 0x0000, 0x001c, 0x0000, 0x0004, 0x0000, 0x0000,
  0x0000, 0x0030, 0x0000, 0x0000, 0x0000, 0x0018, 0x0000, 0x0000,
  0x4e55, 0x0000, 0x41f9, 0x0000, 0x001c, 0x4eb9, 0x0000, 0x0000,
  0x4e5d, 0x2079, 0x0000, 0x0000, 0x4ee8, 0xffe2, 0x0000, 0x0000,
  0x0000, 0x0006, 0x0000, 0x0640, 0x0000, 0x000c, 0x0000, 0x0250,
  0x0000, 0x0014, 0x0000, 0x0350, 0x0000, 0x0004, 0x0500, 0x0000,
  0x0000, 0x0000, 0x0000, 0x000b, 0x0600, 0x0000, 0x0000, 0x001c,
  0x0000, 0x0015, 0x0100, 0x0000, 0x0000, 0x0000, 0x0000, 0x001c,
  0x0100, 0x0000, 0x0000, 0x0000
};

#if HANDLE_DINAMIC_IXEMUL_OPEN

#define IXVC_OFFSET1 25

/* The following code is a hexdump of this assembly program:

		.globl	_VAR
_VAR:		.long (VALUE)
*/

static short ixv_code[] = {
  0x0002, 0x0107, 0x0000, 0x0004, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x000C, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0000, 0x0000, 0x0004, 0x0500, 0x0000, 0x0000, 0x0000,
  0x0000, 0x0010,
};

#endif /* HANDLE_DINAMIC_IXEMUL_OPEN */

void usage(void)
{
  fprintf(stderr, "Usage: gen_glue baserel | large-baserel | no-baserel | profiling\n");
  exit(1);
}

void write_code(short *code, int len, FILE *f)
{
  const short endian = 0x0100;

  /* test if gen_glue is compiled on a big or little endian machine */
  if (((char *)&endian)[0])
  {
    fwrite(code, len, 1, f);		/* big endian */
  }
  else
  {
    int i;

    for (i = 0; i < len / 2; i++)
    {
      short x = code[i];		/* little endian */
      
      fputc(((char *)&x)[1], f);
      fputc(((char *)&x)[0], f);
    }
  }
}

int main(int argc, char **argv)
{
  FILE *fp;
  struct syscall *sc;
  int i, v, baserel = 0, profiling = 0, lbaserel = 0, nobaserel = 0;
  short *code;
  int offset1, offset2, size, gblfl = 0;
  char *gblfs;
  int zero = 0;
  short ixv = 48, ixr = 2;

  if (argc != 2)
    usage();
  if (!strcmp(argv[1], "baserel"))
    baserel = 1;
  else if (!strcmp(argv[1], "baserel32"))
    lbaserel = 1;
  else if (!strcmp(argv[1], "profiling"))
    profiling = 1;
  else if (!strcmp(argv[1], "no-baserel"))
    nobaserel = 1;
  else
    usage();
  
  for (i = 0, sc = syscalls; i < nsyscall; i++, sc++)
    {
      int namelen;
      char name[128];
      
      #if HANDLE_DINAMIC_IXEMUL_OPEN
      if((((unsigned long)sc->name) >> 16) == 0xffff)
      {
         ixv = ((unsigned long)sc->name) & 0xffff;
         ixr = sc->vec;
         continue;
      }
      #endif /* HANDLE_DINAMIC_IXEMUL_OPEN */
      
      namelen = strlen(sc->name);
      if (!memcmp(sc->name, "__obsolete", 10))
        continue;
      if (!memcmp(sc->name, "__must_recompile", 16))
        continue;
      if (!memcmp(sc->name, "__stk", 5))
        continue;
      v = -(sc->vec + 4) * 6;
      sprintf (name, "%s.o", sc->name);

      fp = fopen (name, "w");
      
      if (!fp)
        {
          perror (sc->name);
          exit (20);
        }

      gblfl += namelen + 1 + 4;
      code = no_baserel_code;
      size = sizeof(no_baserel_code);
      offset1 = NO_BASEREL_OFFSET1;
      offset2 = NO_BASEREL_OFFSET2;
      if (profiling)
        { 
          profiling_code[PROFILING_OFFSET1] = v;
          profiling_code[PROFILING_OFFSET2] = 6 + namelen;
          profiling_code[PROFILING_OFFSET3] = 6 + namelen * 2 + 5;
          profiling_code[PROFILING_OFFSET4] = 6 + namelen * 2 + 5 + 7;
          write_code(profiling_code, sizeof(profiling_code), fp);
          fwrite(&zero, 3, 1, fp);
          fputc((char)(namelen * 2 + 7 + IXEMULBASELEN + 1 + 7 + 4), fp);
          fputc('_', fp);
          fwrite(sc->name, namelen + 1, 1, fp);
          fprintf(fp, "PROF");
          fwrite(sc->name, namelen + 1, 1, fp);
          fwrite("mcount", 7, 1, fp);
        }
      #if HANDLE_DINAMIC_IXEMUL_OPEN
      else if ( nobaserel )
        {
          int len;
          
          code[offset1] = v;
          code[offset2] = namelen + 4 + 2;
          code[NO_BASEREL_OFFSET3] = ixv;
          code[NO_BASEREL_OFFSET4] = ixr;
          len = ((namelen*2) + 4 + IXEMULBASELEN + 1 + 4 + sizeof("_ixv"));
          code[NO_BASEREL_OFFSET5] = len - 2 - 4 - 6;
          write_code(code, size, fp);
          fwrite(&zero, 3, 1, fp);
          fputc((char)len, fp);
          fputc('_', fp);
          fwrite(sc->name, namelen + 1, 1, fp);
          fwrite("__ixv_", sizeof("_ixv_"),1,fp);
          fwrite(sc->name, namelen + 1, 1, fp);
        }
      #endif /* HANDLE_DINAMIC_IXEMUL_OPEN */
      else
	{
	  if (baserel && sc->vec != SYS_ix_geta4)
	    {
              code = baserel_code;
              size = sizeof(baserel_code);
              offset1 = BASEREL_OFFSET1;
              offset2 = BASEREL_OFFSET2;
            }
	  else if (lbaserel && sc->vec != SYS_ix_geta4)
	    {
              code = large_baserel_code;
              size = sizeof(large_baserel_code);
              offset1 = LARGE_BASEREL_OFFSET1;
              offset2 = LARGE_BASEREL_OFFSET2;
            }
          code[offset1] = v;
          code[offset2] = namelen + 4 + 2;
          write_code(code, size, fp);
          fwrite(&zero, 3, 1, fp);
          fputc((char)(namelen + 2 + IXEMULBASELEN + 1 + 4), fp);
          fputc('_', fp);
          fwrite(sc->name, namelen + 1, 1, fp);
        }
      fwrite(IXEMULBASE, IXEMULBASELEN + 1, 1, fp);
      fclose (fp);
      
      #if HANDLE_DINAMIC_IXEMUL_OPEN
      if( nobaserel )
      {
        sprintf(name, "%s_ixv.o", sc->name);
        
        if(!(fp = fopen (name, "w")))
        {
          perror (name);
          exit (20);
        }
        
        code = ixv_code;
        size = sizeof(ixv_code);
        code[IXVC_OFFSET1] = strlen(name) + 1 + 4;
        write_code(code, size, fp);
        fwrite("__ixv_", sizeof("_ixv_"),1,fp);
        fwrite(sc->name, namelen + 1, 1, fp);
        fclose( fp );
      }
      #endif /* HANDLE_DINAMIC_IXEMUL_OPEN */
    }
    
    #if HANDLE_DINAMIC_IXEMUL_OPEN
    /**
     * Generate the global-reference version-type variables..
     */
    
    if(!(gblfs = (char *)calloc(gblfl+i,sizeof("_ixv_"))))
    {
    	perror("gblfs");
    	exit(20);
    }
    
    ixv = 0;
    for (i = 0, sc = syscalls; i < nsyscall; i++, sc++)
    {
      if((((unsigned long)sc->name) >> 16) == 0xffff)
      {
         if( ixv != 0 )
         {
           char fn[40];
           
           sprintf( fn, "../../libsrc/varjumbo_%d.%d.h", ixv, ixr );
           
           if(!(fp = fopen ( fn, "w")))
           {
             perror(fn);
             exit (20);
           }
           
           sprintf( &gblfs[strlen(gblfs)-3], "END\n");
           
           fwrite( gblfs, strlen(gblfs), 1, fp );
           fclose(fp);
         }
         
         ixv = ((unsigned long)sc->name) & 0xffff;
         ixr = sc->vec;
         
         strcpy( gblfs, "EXTLONG ");
         
         continue;
      }
      
      sprintf( &gblfs[strlen(gblfs)], "_ixv_%s OR ", sc->name );
    }
    
    free(gblfs);
    
    #endif /* HANDLE_DINAMIC_IXEMUL_OPEN */
    
  exit(0);
}
