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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>

#include "internal.h"

int parse_proc_ide(devices_t *devices)
{
	char *base_name = NULL;
	DIR *dp;
	struct dirent *entry;
	struct disk_s *disk;
	struct ide_s *ide;

	base_name = (char *) malloc(strlen(devices->major_name)+11);
	disk = (struct disk_s *) malloc(sizeof(struct disk_s));
	ide = (struct ide_s *) malloc(sizeof(struct ide_s));
	memset(ide, 0, sizeof(struct ide_s));

	strcpy(base_name, "/proc/ide/");
	strcat(base_name, devices->major_name);

	dp = opendir(base_name);
	while ((entry = readdir(dp)) != NULL) {
		int fd;
		char *dir_name;
		char *file_name;

		if (strncmp(entry->d_name, "hd", 2) == 0) {
			disk->device_name = (char *) malloc(strlen(entry->d_name) + 1);
			strcpy(disk->device_name, entry->d_name);

			dir_name = (char *) malloc(strlen(base_name) + strlen(entry->d_name) + 3);
			strcpy(dir_name, base_name);
			strcat(dir_name, "/");
			strcat(dir_name, entry->d_name);
			strcat(dir_name, "/");

			file_name = (char *) malloc(strlen(dir_name) + 9);
			strcpy(file_name, dir_name);
			strcat(file_name, "model");
			fd = open_proc_file(file_name);
			disk->model = readline(fd);
			close(fd);

			strcpy(file_name, dir_name);
			strcat(file_name, "capacity");
			fd = open_proc_file(file_name);
			disk->capacity = strtoul(readline(fd), NULL, 10);
			close(fd);

			if (((disk->device_name[2] - 'a') % 2) == 0 ){
				ide->master = (struct disk_s *) malloc(sizeof(struct disk_s));
				*ide->master = *disk;
			}
			else {
				ide->slave = (struct disk_s *) malloc(sizeof(struct disk_s));
				*ide->slave = *disk;
			}
		}
	}
	devices->driver = ide;
	closedir(dp);
	return EXIT_SUCCESS;
}
