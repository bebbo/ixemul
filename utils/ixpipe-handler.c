#include <exec/types.h>
#include <exec/lists.h>
#include <exec/ports.h>
#include <exec/execbase.h>
#include <dos/dosextens.h>
#include <dos/filehandler.h>
#include <packets.h>
#include <inline/exec.h>

#include <sys/types.h>
#include <sys/file.h>
#include <sys/signal.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <setjmp.h>
#include <stdio.h>

#ifdef DEBUG
#define dprintf(fmt, args...) kprintf(fmt, ##args)
#else
#define dprintf(fmt, args...)
#endif

/* this is our custom packet, which passes along an ixemul-private file
   id, which we then use to clone that file into our file-table space.
   All later operations are then performed on file-descriptors as usual ;-)) */
#define ACTION_IXEMUL_MAGIC	0x4242	/* *very* magic ;-)) */

#define DOS_TRUE		-1
#define DOS_FALSE 		0

/* we require at least ixemul.library v39.41 */
#define NEEDED_IX_VERSION	39	/* or better */
#define NEEDED_IX_REVISION	41	/* or better */

int handler_mainloop (struct DeviceNode *dev_node, struct Process *me,
		      int *errno);

static int __errno_to_ioerr (int err);

int ix_exec_entry (struct DeviceNode *argc, struct Process *argv,
                   int *environ, int *real_errno, 
	           int (*main)(struct DeviceNode *, struct Process *, int *));

/* guarantee that the first location in the code hunk is a jump to where
   we start, and not some shared string that just happend to land at
   location 0... */
asm (".text; jmp _ENTRY;");

static const char version_id[] = "\000$VER: ixpipe-handler 1.1 (30.8.95)";

struct Library *ixemulbase = 0;
struct ExecBase *SysBase;

/* returnpkt() - packet support routine
 * here is the guy who sends the packet back to the sender...
 *
 * (I modeled this just like the BCPL routine [so its a little redundant] )
 */

static void returnpkt(struct DosPacket *packet, struct Process *myproc, 
	   ULONG res1, ULONG res2)
{
  struct Message *mess;
  struct MsgPort *replyport;

  packet->dp_Res1          = res1;
  packet->dp_Res2          = res2;
  replyport                = packet->dp_Port;
  mess                     = packet->dp_Link;
  packet->dp_Port          = &myproc->pr_MsgPort;
  mess->mn_Node.ln_Name    = (char *) packet;
  mess->mn_Node.ln_Succ    =
    mess->mn_Node.ln_Pred  = 0;
  PutMsg (replyport, mess);
}


static void returnpktplain(struct DosPacket *packet, struct Process *myproc)
{
  returnpkt(packet, myproc, packet->dp_Res1, packet->dp_Res2);
}


/*
 * taskwait() ... Waits for a message to arrive at your port and
 *   extracts the packet address which is returned to you.
 */

static struct DosPacket *taskwait(struct Process *myproc)
{
  struct MsgPort *myport;
  struct Message *mymess;

  myport = &myproc->pr_MsgPort;
  WaitPort (myport);
  mymess = GetMsg (myport);
  return ((struct DosPacket *)mymess->mn_Node.ln_Name);
}

int
ENTRY (void)
{
  struct Library *ixbase;
  struct Process *me;
  struct DosPacket *startup_packet;
  struct DeviceNode *dev_node;
  int errno;

  SysBase = *(struct ExecBase **) 4;
  me = (struct Process *) FindTask (0);

  dprintf("ixp-$%lx: waiting for startup-packet\n", me);

  /* wait for the startup packet */
  startup_packet = taskwait (me);

  dprintf("ixp-$%lx: got startup packet\n", me);

  ixbase = OpenLibrary ("ixemul.library", NEEDED_IX_VERSION);
  if (ixbase)
    {
      if (ixbase->lib_Version == NEEDED_IX_VERSION &&
          ixbase->lib_Revision < NEEDED_IX_REVISION)
	CloseLibrary (ixbase);
      else
	{
	  /* make the external library glue work */
	  ixemulbase = ixbase;
	  dev_node = BTOCPTR (startup_packet->dp_Arg3);
	  dev_node->dn_Task = &me->pr_MsgPort;
	  returnpkt (startup_packet, me, DOS_TRUE, 0);
	  
	  dprintf ("ixp-$%lx: init ok, entering handler mainloop\n", me);
	  /* ignore the result _exit() might pass to us.
             pass our device node as `argc' to handler_mainloop() */
	  ix_exec_entry (dev_node, me, &errno, &errno, handler_mainloop);
	  CloseLibrary (ixbase);
	  return 0;
	}
    }
    
  dprintf ("ixp-$%lx: init-error\n", me);
  returnpkt (startup_packet, me, DOS_FALSE, ERROR_BAD_STREAM_NAME);
  return 0;
}


void
dummy_sighandler ()
{
}

jmp_buf jmpbuf;

void
panic_sighandler (int sig)
{
  longjmp (jmpbuf, sig);
}

int 
handler_mainloop (struct DeviceNode *dev_node, struct Process *me, int *errno)
{
  struct DosPacket *volatile dp = NULL;
  struct MsgPort *our_mp = &me->pr_MsgPort;
  int i;

  for (i = 1; i < SIGMSG; i++)
    signal (i, panic_sighandler);
  /* disable ^C propagation as good as we can... */
  signal (SIGMSG, dummy_sighandler);
     
  /* terminated by ACTION_END, Close() that is */
  for (;;)
    {
      if ((i = setjmp (jmpbuf)))
        {
	  dprintf ("ixp-$%lx: SIGNAL %ld\n", me, i);
	  if (dp->dp_Type == ACTION_WRITE && i == SIGPIPE)
	    {
	      Signal (dp->dp_Port->mp_SigTask, SIGBREAKF_CTRL_C);
	      returnpkt (dp, me, -1, 0); /* return EOF */
	      continue;
	    }
	 

	  /* should look like `SIG' plus number ;-) */
	  returnpkt (dp, me, DOS_FALSE, 516000 + i);
	  continue;
	}

      dprintf ("ixp-$%lx: Waiting for packet...\n", me);
      while (!(dp = taskwait (me))) ;

      /* find out what they want us to do.... */
      switch (dp->dp_Type)
        {
        case ACTION_IXEMUL_MAGIC:
	  {
	    /* this is sort of an `Open', that is, we fill out a struct
	       FileHandle. The reason I didn't chose to `abuse' the various
	       ACTION_FIND{INPUT,OUTPUT} packets is simple: I want an
	       ordinary Open() call to fail! The semantics are, that you
               pass a hex string describing the id as name. */
            int fd;
            char name[255];	/* a BSTR can't address more ;-) */
	    u_char *cp;
            unsigned int id;
            struct FileHandle *fh;
            
            fh = BTOCPTR (dp->dp_Arg1);
 	    cp = BTOCPTR (dp->dp_Arg3);
 	    if (cp && fh)
 	      {
 		bcopy (cp + 1, name, *cp);
	  	name[*cp] = 0;
		/* in case the device-qualifier is still contained in the name */
		cp = index (name, ':');
		if (cp)
		  cp++;
		else
		  cp = name;
		if (sscanf (cp, "%x", &id) == 1)
		  {
		    /* this fcntl() command does not require a valid 
		       descriptor. It's quite unique in this behavior... */
		    fd = fcntl (-1, F_INTERNALIZE, id);
		    if (fd >= 0)
		      {
		        fh->fh_Arg1 = fd;
		        fh->fh_Type = our_mp;
		        fh->fh_Port = 0; /* we're not interactive, are we? */

			dprintf ("ixp-$%lx: successful open, fd = %ld\n", me, fd);
			/* Setting the dn_Task field back to 0 makes each 
			   successive opening of IXPIPE: spawn a new handler.
			   This is essential, or opening would block, if the
			   handler is inside a read/write wait */
		        dev_node->dn_Task = 0;
		        returnpkt (dp, me, DOS_TRUE, 0);
		        break;
		      }
		  }
	      }
	    dprintf ("ixp-$%lx: open failed somehow.. \n", me);
	    /* default is to return object-not-found.. */
	    returnpkt (dp, me, DOS_FALSE, ERROR_OBJECT_NOT_FOUND);
	    break;
	  }


	/* all the following packets operate on file descriptors obtained
	   in ACTION_IXEMUL_MAGIC. */
	   
	case ACTION_READ:
	  dprintf ("ixp-$%lx: read (%ld, $%lx, %ld)\n", me, dp->dp_Arg1, (char *) dp->dp_Arg2, dp->dp_Arg3);
	  dp->dp_Res1 = read (dp->dp_Arg1, (char *) dp->dp_Arg2, dp->dp_Arg3);
	  if (dp->dp_Res1 < 0)
	    dp->dp_Res2 = __errno_to_ioerr (*errno);
	  else
	    dp->dp_Res2 = 0;
	  returnpktplain (dp, me);
	  break;

	case ACTION_WRITE:
	  dprintf ("ixp-$%lx: write (%ld, $%lx, %ld)\n", me, dp->dp_Arg1, (char *) dp->dp_Arg2, dp->dp_Arg3);
	  dp->dp_Res1 = write (dp->dp_Arg1, (char *) dp->dp_Arg2, dp->dp_Arg3);
	  if (dp->dp_Res1 < 0)
	    dp->dp_Res2 = __errno_to_ioerr (*errno);
	  else
	    dp->dp_Res2 = 0;
	  returnpktplain (dp, me);
	  break;

	case ACTION_SEEK:
	  dprintf ("ixp-$%lx: lseek (%ld, %ld, %ld)\n", me, dp->dp_Arg1, (char *) dp->dp_Arg2, dp->dp_Arg3);
	  /* we have to return the previous offset, contrary to Unix which
	     returns the offset after seek operation */
	  dp->dp_Res1 = lseek (dp->dp_Arg1, 0, SEEK_CUR);
	  if (dp->dp_Res1 >= 0)
	    {
	      lseek (dp->dp_Arg1, dp->dp_Arg2, dp->dp_Arg3 + 1);
	      dp->dp_Res2 = 0;
	    }
	  else
	    dp->dp_Res2 = __errno_to_ioerr (*errno);
	  returnpktplain (dp, me);
	  break;

	/* a little present for the growing number of >1.3 users out there */
	case ACTION_EXAMINE_FH:
	  {
 	    struct FileInfoBlock *fib = BTOCPTR (dp->dp_Arg2);
	    struct stat stb;
	    long time;
	    
	    dprintf ("ixp-$%lx: fstat (%ld, )\n", me, dp->dp_Arg1);
	    dp->dp_Res1 = fstat (dp->dp_Arg1, &stb) == 0 ? DOS_TRUE : DOS_FALSE;
	    dp->dp_Res2 = (dp->dp_Res1 == DOS_FALSE) ? __errno_to_ioerr (*errno) : 0;
	    if (dp->dp_Res1 == DOS_TRUE)
	      {
		fib->fib_DiskKey = stb.st_ino;
		/* on the packet level, fib's contain the name as a BSTR */
		strcpy (fib->fib_FileName + 1, "you won't be able to reopen me anyway");
		fib->fib_FileName[0] = strlen (fib->fib_FileName + 1);
	        fib->fib_Protection = stb.st_amode; /* nice we kept it ;-)) */
		fib->fib_Size = stb.st_size;
		fib->fib_NumBlocks = stb.st_blocks;
		time = stb.st_mtime - (8*365+2)*24*3600; /* offset to unix-timesystem */
		fib->fib_Date.ds_Tick = (time % 60) * TICKS_PER_SECOND;
		time /= 60;
		/* minutes per day, not minutes per hour! */
		fib->fib_Date.ds_Minute = time % (60 * 24);
		time /= 60 * 24;
		fib->fib_Date.ds_Days = time;
		fib->fib_Comment[0] = 0;
		/* reserved stuff should normally be zero'd, so do right this */
		bzero (fib->fib_Reserved, sizeof (fib->fib_Reserved));

		/* this is a bit tricky ;-)) 
		   Wondering what AmigaOS programs might do when they're faced
		   with a directory type when examining a filehandle.... */
		if (S_ISDIR (stb.st_mode))
		  fib->fib_DirEntryType = ST_USERDIR;
		else if (S_ISCHR (stb.st_mode))
		  fib->fib_DirEntryType = ST_PIPEFILE;
		else if (S_ISLNK (stb.st_mode))
		  fib->fib_DirEntryType = ST_SOFTLINK;
		else
		  fib->fib_DirEntryType = ST_FILE;

		fib->fib_EntryType = fib->fib_DirEntryType;
	      }
	    returnpktplain (dp, me);
	    break;
	  }

	case ACTION_END:
	  dprintf ("ixp-$%lx: close (%ld)\n", me, dp->dp_Arg1);
	  close (dp->dp_Arg1);
	  returnpkt (dp, me, DOS_TRUE, 0);
	  /* terminates the handler */
	  Forbid ();
	  return 0;
	  
	default:
	  dprintf ("ixp-$%lx: returning unknown packet %ld\n", me, dp->dp_Type);
	  returnpkt (dp, me, DOS_FALSE, ERROR_ACTION_NOT_KNOWN);
	  break;
	}
    }
}


static int
__errno_to_ioerr (int err)
{
  switch (err)
    {
    case EAGAIN:
      return ERROR_TASK_TABLE_FULL;
      
    case ENOMEM:
      return ERROR_NO_FREE_STORE;

    case E2BIG:
      return ERROR_LINE_TOO_LONG;
      
    case ENOEXEC:
      return ERROR_FILE_NOT_OBJECT;
      
    case EEXIST:
      return ERROR_OBJECT_EXISTS;
      
    case ENOENT:
      return ERROR_OBJECT_NOT_FOUND;
      
    default:
    case ENODEV:
    case EIO:
      return ERROR_ACTION_NOT_KNOWN;
      
    case EINVAL:
      return ERROR_OBJECT_WRONG_TYPE;
      
    case EROFS:
      return ERROR_DISK_WRITE_PROTECTED;
      
    case EXDEV:
      return ERROR_RENAME_ACROSS_DEVICES;
      
    case ENOTEMPTY:
      return ERROR_DIRECTORY_NOT_EMPTY;
      
    case ELOOP:
      return ERROR_TOO_MANY_LEVELS;
      
    case ENXIO:
      return ERROR_DEVICE_NOT_MOUNTED;
      
    case ESPIPE:
      return ERROR_SEEK_ERROR;
      
    case ENAMETOOLONG:
      return ERROR_COMMENT_TOO_BIG;
      
    case ENOSPC:
      return ERROR_DISK_FULL;
      
    case EACCES:
      return ERROR_READ_PROTECTED;	/* could as well be one of the others... */
    }
}
