/*
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */


#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "internal.h"

char *readline(int fd)
{
	char ch[1];
	char *line = NULL;
	int length = 1;

	line = (char *) malloc(length);
	strcpy(line,"");	
	while (read(fd, ch, 1) > 0) {
		if ((ch[0] == '\n') || (ch[0] == '\0')) {
			break;
		}
		line = (char *) realloc(line, ++length);
		strncat(line, &ch[0], 1);
	}
	if (length==1) {
		return NULL;
	}
	else {
		return line;
	}
}

int open_proc_file(const char *filename)
{
	int fd;
	if ((fd = open(filename, O_RDONLY)) == -1) {
		perror("FIXME: need to mount /proc first\n");
		exit errno;
	} 
	return fd;
}

