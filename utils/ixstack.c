#define UTILITY_TAGITEM_H
#define _SIZE_T
#define __AMIGA_TYPES__

#include <sys/fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <ixemul.h>
#include <ix.h>
#include <proto/exec.h>

char VERSION[] = "\000$VER: ixstack 1.1 (14.06.97)";

void setstack(long size, char *filename)
{
  int fd;
  int len, i;
  caddr_t addr;
  struct stat s;
  static int printed_header = 0;
  
  if (stat(filename, &s) == -1 || !S_ISREG(s.st_mode))
  {
    close(fd);
    return;
  }
  fd = open(filename, O_RDWR);
  if (fd == -1)
  {
    perror(filename);
    return;
  }
  len = lseek(fd, 0, SEEK_END);
  addr = mmap(NULL, len, PROT_READ, MAP_SHARED, fd, 0);
  if (fd == -1)
  {
    perror(filename);
    close(fd);
    return;
  }
  for (i = 0; i < len - 12; i += 2)
    if (*(long *)(addr + i) == 'StCk' && *(long *)(addr + i + 8) == 'sTcK')
    {
      if (size >= 0)
      {
        lseek(fd, i + 4, SEEK_SET);
        write(fd, &size, 4);
      }
      else
      {
        if (!printed_header)
        {
          printed_header = 1;
          printf("stacksize  filename\n---------  --------\n");
        }
        printf("%9d  %s\n", *(long *)(addr + i + 4), filename);
      }
      munmap(addr, len);
      close(fd);
      return;
    }
  close(fd);
  if (size)
    printf("cannot set stack: %s\n", filename);
}

static int ctrlc = 0;

static void sigint()
{
  ctrlc = 1;
}

static void show(void)
{
  struct MsgPort *port;
  struct SUMessage *msg;
  u_long portsig;
  int printed_header = 0;
  
  signal(SIGINT, sigint);
  if ((port = CreatePort("ixstack port", 0)))
  {
    while (!ctrlc)
    {
      portsig = 1 << port->mp_SigBit | SIGBREAKF_CTRL_C;
      ix_wait(&portsig);
      if (portsig & (1 << port->mp_SigBit))
      {
        while ((msg = (struct SUMessage *)GetMsg(port)))
        {
          if (!printed_header)
          {
            printed_header = 1;
            printf("  Usage   Total Program\n\n");
          }
          printf("%7d %7d %s\n", msg->stack_usage, msg->stack_size, msg->name);
          ReplyMsg((struct Message *)msg);
        }
      }
      
      if (portsig & SIGBREAKF_CTRL_C)
      {
        while ((msg = (struct SUMessage *)GetMsg(port)))
          ReplyMsg((struct Message *)msg);
        break;
      }
    }
    DeletePort(port);
  }
  exit(0);
}


main(int argc, char **argv)
{
  long size;

  if (argc == 2 && !strcmp(argv[1], "-s"))
    show();
  if (argc < 3)
  {
    fprintf(stderr, "set stacksize:   ixstack <stacksize> <files ...>
show stacksize:  ixstack -l <files ...>
show stackusage: ixstack -s\n");
    exit(1);
  }

  if (!strcmp(argv[1], "-l"))
    size = -1;
  else
  {
    int i;
    
    for (i = 0; argv[1][i]; i++)
      if (!isdigit(argv[1][i]))
      {
        fprintf(stderr, "stacksize %s is not a number\n", argv[1]);
        exit(1);
      }
    size = atol(argv[1]);
    if (size < 4000 && size)
    {
      fprintf(stderr, "stacksize must be at least 4000 bytes\n");
      exit(1);
    }
    if (size && (size & 3))
    {
      fprintf(stderr, "stacksize must be a multiple of 4\n");
      exit(1);
    }
  }
  argv += 2;
  while (*argv)
    setstack(size, *argv++);
}
