/* cdrom-checker - verify debian cdrom's
 * Copyright (C) 2003 Thorsten Sauter <tsauter@gmx.net>
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */


#define _GNU_SOURCE 1
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <cdebconf/debconfclient.h>
#include <debian-installer.h>

#define TARGET "/cdrom"

static struct debconfclient *debconf = NULL;
char *cdrom_device = NULL;

void detect_cdrom();
int mount_cdrom();
int md5file_getlines(FILE *md5file);
int valid_cdrom();
int check_cdrom();
int main(int argc, char **argv);

