/*
 * Copyright (c) 1987 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)getenv.c    5.8 (Berkeley) 2/23/91";
#endif /* LIBC_SCCS and not lint */

#define _KERNEL
#include "ixemul.h"

#include <stdlib.h>
#include <stddef.h>
#include <string.h>

/*
 * _findenv --
 *      Returns pointer to value associated with name, if any, else NULL.
 *      Sets offset to be the offset of the name/value combination in the
 *      environmental array, for use by setenv(3) and unsetenv(3).
 *      Explicitly removes '=' in argument name.
 *
 *      This routine *should* be a static; don't use it.
 */
char *
_findenv(char **env, const char *name, int *offset)
{
  int len;
  char **P;
  const char *C;

  for (C = name, len = 0; *C && *C != '='; ++C, ++len);
  for (P = env; *P; ++P)
    if (!strncmp(*P, name, len))
      if (*(C = *P + len) == '=') {
	*offset = P - env;
	return (char *)++C;
      }
  return NULL;
}

/*
 * getenv --
 *      Returns ptr to value associated with name, if any, else NULL.
 */
char *
getenv(const char *name)
{
  usetup;
  int offset;
  int ret;
  if (u.u_parent_userdata)u_ptr = u.u_parent_userdata;
  char name2[256];
  
  char * r = _findenv(*u.u_environ, name, &offset);
  
  strcpy (&name2[0],"env:"); // code so easy can see in snoopdos what env vars a ixemul program use.
  strncat (&name2[0],name,230);
  ret = Open(name2,MODE_OLDFILE);
  if (ret)Close(ret);
  return r;
  }

