/* <!-- copyright */
/*
 * Copyright (C) 2006 Tatsuhiro Tsujikawa
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations
 * including the two.
 * You must obey the GNU General Public License in all respects
 * for all of the code used other than OpenSSL.  If you modify
 * file(s) with this exception, you may extend this exception to your
 * version of the file(s), but you are not obligated to do so.  If you
 * do not wish to do so, delete this exception statement from your
 * version.  If you delete this exception statement from all source
 * files in the program, then also delete it here.
 */
/* copyright --> */

#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>

#define IS_PATH_SEPARATOR(c) ((c) == '/')

/* per http://www.scit.wlv.ac.uk/cgi-bin/mansec?3C+basename */
char* basename(char* s) {
	char* rv;

	if (!s || !*s)
		return ".";

	rv = s + strlen(s) - 1;

	do {
		if (IS_PATH_SEPARATOR(*rv))
			return rv + 1;
		--rv;
	} while (rv >= s);

	return s;
}

/* Amiga - ripped from: http://mpa.itc.it/markus/grass63progman/basename_8c-source.html */
char * basename_b(char *filename, const char *desired_ext)
{
    /* Find the last . in the filename */
    char *dot = strrchr(filename, '.');

    /* Check there is a . and it's not the last character
     * in the string, i.e. there is an extension */
    if(dot && ((dot - filename) < strlen(filename)) )
    {
        char *ext = dot + 1;
        int i, match = 1;

        /* if the extension matches (case insensitive)
         * then truncate the filename to the basename */
        for( i = 0; i < strlen(desired_ext); i++ )
        {
            if( (ext[i] == '\0') || (tolower(ext[i]) != tolower(desired_ext[i])) )
            {
                match = 0;
                break;
            }
        }
        
        if( match )
            *dot = '\0';

    }

    return filename;
} 

/* per http://www.scit.wlv.ac.uk/cgi-bin/mansec?3C+dirname */
char* dirname(char* path) {
	char *p;

	if (path == NULL || *path == '\0')
		return ".";
	p = path + strlen(path) - 1;
	while (IS_PATH_SEPARATOR(*p)) {
		if (p == path)
			return path;
		*p-- = '\0';
	}

	while (p >= path && !IS_PATH_SEPARATOR(*p))
		p--;

	if (p < path)
		return ".";

	if (p == path)
		return "/";

	*p = '\0';

	return path;
}
