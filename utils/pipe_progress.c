/*
 * Monitor a pipe with a simple progress display.
 *
 * Copyright (C) 2003 by Rob Landley <rob@landley.net>, Joey Hess
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <time.h>

/* Pipes work in system page size chunks. */
#define BUFFER_SIZE 4096

int main () {
	char buf[BUFFER_SIZE];
	time_t t = time(NULL);

	for(;;) {
		int len = read(0, buf, BUFSIZ);
		if (len > 0) {
			time_t new_time = time(NULL);
			if (new_time != t) {
				t = new_time;
				fprintf(stderr, ".");
			}
			write(1, buf, len);
		}
		else {
			break;
		}
	}

	fprintf(stderr, "\n");

	return 0;
}
