/* Stubs for crypt functions which may be export restricted by stupid USA
 * ITAR (International Trade in Arms Regulations).
 *
 * To compile a version of the library in the free world (IE outside the USA)
 * simply replace this file with the real crypt.c, which should be widely
 * available on ftp servers.
 *
 */

#define _KERNEL
#include "ixemul.h"
#include <unistd.h>
#include <clib/alib_protos.h>

#if 0 /* __PPC__ */
#define BUFSIZE 12
STRPTR ACrypt(STRPTR buf, CONST_STRPTR key, CONST_STRPTR settings)
{
  int i;
  int k;
  unsigned buf2[BUFSIZE];

  for (i = 0; i < BUFSIZE; ++i)
	buf2[i] = 'A' + (*key? *key++ : i) + (*settings? *settings++ : i);

  for (i = 0; i < BUFSIZE; ++i)
	{
	  for (k = 0; k < BUFSIZE; ++k)
	{
	  buf2[i] += buf2[BUFSIZE - k - 1];
	  buf2[i] %= 53;
	}
	  buf[i] = (char)buf2[i] + 'A' ;
	}
  buf[BUFSIZE - 1] = '\0';
  return buf;
}
#endif

/*
 * Return a pointer to static data consisting of the "setting"
 * followed by an encryption produced by the "key" and "setting".
 */

char *crypt (const char *key, const char *setting)
{
  usetup;
  size_t setlen;

  if (muBase) return 0;

  if (u.u_ixnetbase)
	return (char *)netcall(NET_crypt, key, setting);

  setlen = strlen(setting);
  if (setlen != 9 && setlen != 2) return 0;
  strcpy(u.u_crypt_buf, setting);

  return ACrypt(u.u_crypt_buf + setlen, (char *)key, (char *)setting);
}

/*
 * Encrypt (or decrypt if num_iter < 0) the 8 chars at "in" with abs(num_iter)
 * iterations of DES, using the the given 24-bit salt and the pre-computed key
 * schedule, and store the resulting 8 chars at "out" (in == out is permitted).
 *
 * NOTE: the performance of this routine is critically dependent on your
 * compiler and machine architecture.
 */

int des_cipher (const char *in, char *out, long salt, int num_iter)
{
  ix_warning("This version of ixemul.library does not support encryption/decryption.");
  return 0;
}

/*
 * Set up the key schedule from the key.
 */

int des_setkey (const char *key)
{
  ix_warning("This version of ixemul.library does not support encryption/decryption.");
  return 0;
}

/*
 * "encrypt" routine (for backwards compatibility)
 */

int encrypt (char *block, int flag)
{
  ix_warning("This version of ixemul.library does not support encryption/decryption.");
  return 0;
}

/*
 * "setkey" routine (for backwards compatibility)
 */

int setkey (const char *key)
{
  ix_warning("This version of ixemul.library does not support encryption/decryption.");
  return 0;
}
