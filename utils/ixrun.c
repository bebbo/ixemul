#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <proto/dos.h>
#include <utility/tagitem.h>
#include <ix.h>

char VERSION[] = "\000$VER: ixrun 1.1 (14.06.97)";

static void usage(void)
{
  fprintf(stderr, "Usage: ixrun [-n | -q] filename [arguments...]
-n\tdon't add quotes (\") around the arguments
-q\tadd quotes (\") around the arguments (default)
-nv\tas -n, but print the command line to standard error, don't execute it
-qv\tas -q, but print the command line to standard error, don't execute it\n");
  exit(1);
}

main(int argc, char **argv)
{
  char *p;
  long size, i, first_opt = 1, add_quotes = 2, debug = 0;

  if (argc == 1)
    usage();
  if (!strcmp(argv[1], "-q") || !strcmp(argv[1], "-qv"))
  {
    if (argc == 2)
      usage();
    first_opt++;
    debug = !strcmp(argv[1], "-qv");
  }
  if (!strcmp(argv[1], "-n") || !strcmp(argv[1], "-nv"))
  {
    if (argc == 2)
      usage();
    add_quotes = 0;
    first_opt++;
    debug = !strcmp(argv[1], "-nv");
  }
  if (argv[first_opt][0] == '/' && (p = strchr(argv[first_opt] + 1, '/')))
  {
    *p = ':';
    (argv[first_opt])++;
  }
  for (size = strlen(argv[first_opt]) + 1, i = first_opt + 1; argv[i]; i++)
    size += strlen(argv[i]) + 1 + add_quotes;
  p = malloc(size);
  if (p == NULL)
  {
    fprintf(stderr, "couldn't allocate %d bytes\n", size);
    exit(1);
  }
  strcpy(p, argv[first_opt]);
  for (i = first_opt + 1; argv[i]; i++)
  {
    if (add_quotes)
      strcat(p, " \"");
    else
      strcat(p, " ");
    strcat(p, argv[i]);
    if (add_quotes)
      strcat(p, "\"");
  }
  if (debug)
    fprintf(stderr, "command line = '%s'\n", p);
  else
  {
    int result, omask;

    omask = sigsetmask(~0);
    result = !Execute(p, NULL, NULL);
    sigsetmask(omask);
    exit(result);
  }
}
