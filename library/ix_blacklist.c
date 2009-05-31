/**
 * Ixemul.library tasks black-listing management routines.
 * Copyright (c) 2009 Diego Casorran <diegocr at users dot sf dot net>
 * 
 * Redistribution  and  use  in  source  and  binary  forms,  with  or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * -  Redistributions  of  source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 * 
 * -  Neither  the name of the author(s) nor the names of its contributors may
 * be  used  to endorse or promote products derived from this software without
 * specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND  ANY  EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE  DISCLAIMED.   IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE  FOR  ANY  DIRECT,  INDIRECT,  INCIDENTAL,  SPECIAL,  EXEMPLARY,  OR
 * CONSEQUENTIAL  DAMAGES  (INCLUDING,  BUT  NOT  LIMITED  TO,  PROCUREMENT OF
 * SUBSTITUTE  GOODS  OR  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION)  HOWEVER  CAUSED  AND  ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT,  STRICT  LIABILITY,  OR  TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING  IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 * 
 * $Id: ix_blacklist.c,v 0.2 2009/05/31 14:00:36 diegocr Exp $
 * 
 */

#define _KERNEL
#include "ixemul.h"
#include <sys/stat.h>
#include <string.h>

#define BLM_PARANOID
#define BLM_FILENAME	"ENVARC:ixemul-blacklist.db"
#define BLM_VERSION	1
#define BLM_IDENTIFIER	MAKE_ID('B','L','M',BLM_VERSION)

typedef struct BlackListManager
{
	#ifdef BLM_PARANOID
	unsigned long MagicID;
	# define BLMMID	0x9ff47356
	#endif
	
	void *mempool;
	long mtime;
	struct ix_mutex msem;
	struct ixlist bltasks;
	
} BlackListManager;

typedef struct BlackListNode
{
	struct ixnode node;
	
	unsigned long flags;
	char tname[2];
	
} BlackListTask;

STATIC BlackListManager * gblblm = NULL;

#ifndef MAKE_ID
# define MAKE_ID(a,b,c,d)	\
	((ULONG) (a)<<24 | (ULONG) (b)<<16 | (ULONG) (c)<<8 | (ULONG) (d))
#endif

static __inline void lEntryAdd(unsigned long flags,char *name,unsigned char len)
{
	BlackListTask *blt;
	
	if((blt = AllocPooled(gblblm->mempool,sizeof(*blt) + len + 2)))
	{
		blt->flags = flags;
		CopyMem( name, &blt->tname[0], len );
		
		ixaddtail( &gblblm->bltasks,(struct ixnode *)blt);
	}
}

static __inline int LoadDatabase_V1(BPTR fd)
{
	int rc = 0;
	ULONG ul;
	UBYTE name[256], len;
	
	while(TRUE)
	{
		if(Read(fd, &ul, sizeof(ULONG)) != sizeof(ULONG))
			return -3;
		
		if( ul == MAKE_ID('.','E','O','F'))
		{
			rc = 1; /* everything OK. */
			break;
		}
		
		if(Read(fd, &len, sizeof(UBYTE)) != sizeof(UBYTE))
			return -4;
		
		if(Read(fd, name, len ) != len )
			return -5;
		
		lEntryAdd(ul,name,len);
	}
	
	return(rc);
}

static __inline int LoadDatabase( void )
{
	BPTR fd;
	int rc = 0;
	
	if((fd = Open( BLM_FILENAME, MODE_OLDFILE )))
	{
		ULONG id;
		
		if(Read(fd, &id, sizeof(ULONG))==sizeof(ULONG))
		{
			switch( id )
			{
				case BLM_IDENTIFIER:
					rc = LoadDatabase_V1(fd);
					break;
				
				default:
					rc = -2;
					break;
			}
		}
		else rc = -1;
		
		Close(fd);
	}
	
	return(rc);
}

static __inline int CreateBlackListManager( void )
{
	APTR mempool;
	
	mempool = CreatePool( MEMF_PUBLIC|MEMF_CLEAR, 512, 512 );
	
	if( mempool == NULL )
		return -1;
	
	gblblm = AllocPooled(mempool,sizeof(BlackListManager));
	
	if( gblblm == NULL )
	{ /* !?!? */
		DeletePool(mempool);
		return -3;
	}
	
	ixnewlist(&gblblm->bltasks);
	gblblm->mempool = mempool;
	
	return(0);
}

static __inline int blm_init( void )
{
	int rc = 0;
	
	Forbid();
	if( gblblm == NULL )
		rc = CreateBlackListManager ( ) ;
	Permit();
	
	if( rc != 0 )
		return -2;
	
	#ifdef BLM_PARANOID
	if( gblblm->MagicID != BLMMID )
		return -3;
	#endif
	
	return(rc);
}

static __inline char *taskname(struct Task *task)
{
	struct Process *pr;
	char * result = NULL;
	
	pr = (struct Process *)task;
	if( task->tc_Node.ln_Type == NT_PROCESS && pr->pr_CLI )
	{
		struct CommandLineInterface *cli =
			(struct CommandLineInterface *)BADDR(pr->pr_CLI);
		
		if( cli && cli->cli_Module && cli->cli_CommandName )
		{
			char *n = (char *)(cli->cli_CommandName<<2);
			
			if(*n++ > 0)
				result = FilePart(n);
		}
	}
	
	if( result == NULL )
		result = task->tc_Node.ln_Name;
	
	return(result);
}


unsigned long BlacklistedTaskFlags(void *task)
{
	unsigned long rc = (~0); /* task not found */
	
	if( blm_init ( ) == 0 )
	{
		struct stat st;
		
		ix_mutex_lock(&gblblm->msem);
		
		// Check if there were changes to the database..
		if(!stat(BLM_FILENAME,&st) && gblblm->mtime != st.st_mtime)
		{
			LoadDatabase ( ) ;
				gblblm->mtime = st.st_mtime;
		}
		
		if( ! IsIxListEmpty ( &gblblm->bltasks ))
		{
			char *tname;
			BlackListTask *blt;
			
			if(task == NULL)
				task = (void *)FindTask(NULL);
			
			tname = taskname(task);
			
			ITERATE_IXLIST( &gblblm->bltasks, blt )
			{
				// TODO: allow amiga patterns (?)
				if(strstr( tname, &blt->tname[0] ))
				{
					rc = blt->flags;
					break;
				}
			}
		}
		
		ix_mutex_unlock(&gblblm->msem);
	}
	
	return(rc);
}
