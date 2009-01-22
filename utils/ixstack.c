#include <proto/exec.h>
#define UTILITY_TAGITEM_H
#define _SIZE_T
#define __AMIGA_TYPES__

#include <devices/timer.h>
#include <ixemul.h>
#include <sys/fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <ix.h>

char VERSION[] = "\000$VER: ixstack 1.2 (15.03.2006)";


#define STACK_MAGIC_ID0 0x5374436B /* 'StCk' */
#define STACK_MAGIC_ID1 0x7354634B /* 'sTcK' */

void setstack(long size, char *filename)
{
  int fd;
  int len, i;
  caddr_t addr;
  struct stat s;
  static int printed_header = 0;
  
  if (stat(filename, &s) == -1 || !S_ISREG(s.st_mode))
  {
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
    if (*(u_long *)(addr + i) == STACK_MAGIC_ID0 &&
        *(u_long *)(addr + i + 8) == STACK_MAGIC_ID1)
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
	printf("%9d  %s\n", *(int *)(addr + i + 4), filename);
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
  const char portname[] = "ixstack port";
  
  signal(SIGINT, sigint);

  /* Allow only one incarnation */
  Forbid();
  port = FindPort(portname);
  if (port)
  {
    Permit();
    fprintf(stderr, "ixstack show already running elsewhere\n");
    exit(1);
  }
  port = CreateMsgPort();
  if (port)
  {
    port->mp_Node.ln_Name = (STRPTR) portname;
    port->mp_Node.ln_Pri  = 0;
    AddPort(port);
  }
  Permit();

  if (port)
  {
    while (!ctrlc)
    {
      portsig = 1 << port->mp_SigBit | SIGBREAKF_CTRL_C;
      ix_wait(&portsig);
      if (portsig & (1 << port->mp_SigBit))
      {
	while ((msg = (struct SUMessage *) GetMsg(port)))
	{
	  if (!printed_header)
	  {
	    printed_header = 1;
	    printf("  Usage   Total Program\n\n");
	  }
	  printf("%7ld %7ld %s\n", msg->stack_usage, msg->stack_size, msg->name);
	  ReplyMsg(&msg->msg);
	}
      }
      
      if (portsig & SIGBREAKF_CTRL_C)
      {
	break;
      }
    }

    RemPort(port);

    /* Flush any pending messages */
    while ((msg = (struct SUMessage *) GetMsg(port)))
      ReplyMsg(&msg->msg);

    DeleteMsgPort(port);
  }
  exit(0);
}


int main(int argc, char **argv)
{
  long size;

  if (argc == 2 && !strcmp(argv[1], "-s"))
    show();
  if (argc < 3)
  {
    fprintf(stderr, "set stacksize:   ixstack <stacksize> <files ...>\n"
                    "show stacksize:  ixstack -l <files ...>\n"
                    "show stackusage: ixstack -s\n");
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
  return 0;
}
