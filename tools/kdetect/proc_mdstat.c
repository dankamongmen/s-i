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

int parse_proc_mdstat(devices_t *devices)
{
	char *line = NULL;
	int fd;
	const int md_linear = 1;
	const int md_raid0 = 2;
	const int md_raid1 = 4;
	const int md_raid5 = 8;
	struct raid_s *md;

	md = (raid_t *) malloc(sizeof(raid_t));
	fd = open_proc_file("/proc/mdstat");

	md->supported_modes = 0;
	line = readline(fd);
	if (strstr(line, "linear") != NULL) {
		md->supported_modes &= md_linear;
	}	
	if (strstr(line, "raid0") != NULL) {
		md->supported_modes &= md_raid0;
	}	
	if (strstr(line, "raid1") != NULL) {
		md->supported_modes &= md_raid1;
	}	
	if (strstr(line, "raid5") != NULL) {
		md->supported_modes &= md_raid5;
	}	

	line = readline(fd);
	while ((line = readline(fd)) != NULL) {
		char *device, *type;

		device = (char *) malloc(strlen(line));
		type = (char *) malloc(strlen(line));

		if (sscanf(line, "%s : active %s", device, type) == 2) {
			char *names;

			names = (char *) malloc(strlen(line));
			names = strstr(line, type);
			names+=strlen(type)+1;

			readline(fd);
			readline(fd);
		}
		
	}
	return EXIT_FAILURE;
}

