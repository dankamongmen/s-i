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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "internal.h"

int parse_proc_devices(devices_t *devices)
{
	int fd = 0;
	char *line = NULL;

	fd = open_proc_file("/proc/devices");

	while (readline(fd) != NULL);
	readline(fd);

	while ((line = readline(fd)) != NULL) {
		char *name = NULL;

		name = (char *) malloc(strlen(line));
		sscanf(line, "%d %s\n", &devices->major, name);
		devices->major_name = (char *) malloc(strlen(name)+1);
		strcpy(devices->major_name, name);

		if (strncmp(devices->major_name, "ide", 3) == 0) {
			parse_proc_ide(devices);
		}
/*		else {
			if (strcmp(devices->name, "loop") == 0) {
			}
		}*/
		else {
			if (strcmp(devices->major_name, "md") == 0) {
				parse_proc_mdstat(devices);
			}
		}
/*		else {
			if (strcmp(devices->name, "lvm") == 0) {
			}
		}
*/
		devices->next = (devices_t *) malloc(sizeof(devices_t));
		devices = devices->next;
		devices->next = NULL;
		free(name);
	}
	free(line);
	close(fd);
	return EXIT_SUCCESS;
}
