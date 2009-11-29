#define _KERNEL
#include <string.h>
#include "ixemul.h"
#include "kprintf.h"

#include <utility/tagitem.h>
#include <dos/dostags.h>
#include <exec/tasks.h>
#include <user.h>
 
int ix_UseAmigaPaths(int val)
{ 
	usetup;
    if (u.u_parent_userdata)u_ptr=u.u_parent_userdata;
	
	
	if (val == -1)
	{
       return u.u_use_amiga_paths;
	}
	else
	{
	u.u_use_amiga_paths = val;
	return val;
	}
}
