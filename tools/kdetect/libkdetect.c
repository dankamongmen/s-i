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

#include "internal.h"

static struct devices_s *devices_head = NULL;

int kdetect_init()
{
	devices_head = (devices_t *) malloc(sizeof(devices_t));
	parse_proc_devices(devices_head);

	return 0;
}

char *get_device_list()
{
	char *device_list = NULL;
	struct devices_s *device = NULL;

	device_list = (char *) malloc(1);
	strcpy(device_list, "");
	kdetect_init();
	device = devices_head;
	while (device->next != NULL) {
		if (strncmp(device->major_name, "ide", 3) == 0) {
			ide_t *ide = NULL;
			ide = device->driver;
			if (ide->master != NULL) {
				device_list = realloc(device_list, strlen(device_list) + strlen(ide->master->device_name) + 2);
				strcat(device_list, ide->master->device_name);
				strcat(device_list, " ");
			}
			if (ide->slave != NULL) {
				device_list = realloc(device_list, strlen(device_list) + strlen(ide->slave->device_name) + 2);
				strcat(device_list, ide->slave->device_name);
				strcat(device_list, " ");
			}
		}
/*		else {
			if (strcmp(devices->name, "loop") == 0) {
			}
		}*/
		else {
			if (strcmp(device->major_name, "md") == 0) {
			}
		}
/*		else {
			if (strcmp(devices->name, "lvm") == 0) {
			}
		}
*/
		device = device->next;
	}
	device_list[strlen(device_list) - 1] = '\0';
	if (device_list == NULL) {
		return NULL;
	}
	else {
		return device_list;
	}
}

char *get_default_device()
{
	char *device_list = NULL;

	if (device_list == NULL) {
		return NULL;
	}
	else {
		return device_list;
	}
}
