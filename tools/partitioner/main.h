
#ifndef __MAIN_H__
#define __MAIN_H__

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <string.h>
#include <sys/utsname.h>
#include <debian-installer.h>
#include <cdebconf/debconfclient.h>
#include <parted/parted.h>

#define MEGABYTE (1024 * 1024)
#define MEGABYTE_SECTORS (MEGABYTE / 512)
#define MAX_DISKS 128

#define FDISK_PATH "/usr/share/partitioner"

static struct debconfclient *debconf;

int get_all_disks();
int main(int argc, char *argv[]);

#endif /* __MAIN_H__ */

