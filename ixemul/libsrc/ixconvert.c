#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

main(int argc, char **argv)
{
  if (argc == 1)
  {
    fprintf(stderr, "usage: ixconvert <files...>\n");
    exit(2);
  }
  
  while (*++argv)
  {
    int in = open(argv[0], O_RDWR);
    char ch;
    
    if (in == -1)
    {
      fprintf(stderr, "cannot open %s\n", argv[0]);
      continue;
    }
    lseek(in, 399, SEEK_SET);
    read(in, &ch, 1);
    if (ch != 0x18)
    {
      fprintf(stderr, "cannot convert %s\n", argv[0]);
      close(in);
      continue;
    }
    lseek(in, 703, SEEK_SET);
    read(in, &ch, 1);
    if (ch != 0x18)
    {
      fprintf(stderr, "cannot convert %s\n", argv[0]);
      close(in);
      continue;
    }
    ch = 0x2e;
    lseek(in, 399, SEEK_SET);
    write(in, &ch, 1);
    lseek(in, 703, SEEK_SET);
    write(in, &ch, 1);
    close(in);
    fprintf(stderr, "converted %s\n", argv[0]);
  }
}

